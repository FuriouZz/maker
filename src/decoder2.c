#include "decoder2.h"
#include "frame_queue.h"
#include "libavcodec/avcodec.h"
#include "libavcodec/packet.h"
#include "libavformat/avformat.h"
#include "maker/maker.h"
#include "media.h"
#include "mutex.h"
#include "packet_queue.h"
#include "util.h"

#define MAX_QUEUE_ITEM (15 * 1024 * 1024)

int mk_decoder2_init(MKDecoder2* decoder, MKDecoder2Desc* desc)
{
    if (decoder == NULL) {
        return -1;
    }
    if (desc->media == NULL) {
        return -1;
    }
    if (desc->is_aborted == NULL) {
        return -1;
    }
    if (desc->is_eof == NULL) {
        return -1;
    }

    decoder->media = desc->media;
    decoder->is_aborted = desc->is_aborted;
    decoder->is_eof = desc->is_eof;

    int status;

    status = mk_cond_init(&decoder->demuxer.continue_signal);
    if (status != 0) {
        goto fail;
    }

    int video_stream_index = decoder->media->streams[MK_TRACK_TYPE_VIDEO];

    if (video_stream_index > -1) {
        MKVideoDecoder2* video = &decoder->video;

        status = mk_packet_queue_init(&video->packet_q);
        if (status != 0) {
            goto fail;
        }

        status = mk_frame_queue_init(&video->frame_q, &video->packet_q, 16, 1);
        if (status != 0) {
            goto fail;
        }

        AVFormatContext* format = decoder->media->context->format;
        AVStream* stream = format->streams[video_stream_index];
        AVCodecParameters* params = stream->codecpar;

        const AVCodec* codec = avcodec_find_decoder(params->codec_id);
        if (codec == NULL) {
            goto fail;
        }

        AVCodecContext* codec_context = avcodec_alloc_context3(codec);
        if (codec_context == NULL) {
            goto fail;
        }
        video->codec_context = codec_context;

        status = avcodec_parameters_to_context(codec_context, params);
        if (status != 0) {
            goto fail;
        }

        status = avcodec_open2(codec_context, codec, NULL);
        if (status != 0) {
            goto fail;
        }

        AVPacket* packet = av_packet_alloc();
        if (codec_context == NULL) {
            goto fail;
        }
        video->packet = packet;
    }

    return 0;

fail:
    mk_frame_queue_destroy(&decoder->video.frame_q);
    mk_packet_queue_destroy(&decoder->video.packet_q);
    avcodec_free_context(&decoder->video.codec_context);
    decoder->video.codec_context = NULL;
    av_packet_free(&decoder->video.packet);
    mk_cond_destroy(&decoder->demuxer.continue_signal);
    decoder->video.packet = NULL;
    decoder->media = NULL;
    decoder->is_aborted = NULL;
    decoder->is_eof = NULL;

    return -1;
}

int mk_decoder2_destroy(MKDecoder2* decoder)
{
    if (decoder == NULL) {
        return -1;
    }

    mk_frame_queue_destroy(&decoder->video.frame_q);
    mk_packet_queue_destroy(&decoder->video.packet_q);
    avcodec_free_context(&decoder->video.codec_context);
    decoder->video.codec_context = NULL;
    av_packet_free(&decoder->video.packet);
    mk_cond_destroy(&decoder->demuxer.continue_signal);
    decoder->video.packet = NULL;
    decoder->media = NULL;
    decoder->is_aborted = NULL;
    decoder->is_eof = NULL;

    return 0;
}

_MK_PRIVATE int mk_decoder2_demuxer_thread(void* data)
{
    int status;
    MKDecoder2* decoder = data;
    MKMedia* media = decoder->media;
    MKPacketQueue* video_q = &decoder->video.packet_q;
    AVFormatContext* format = media->context->format;

    AVPacket* packet = av_packet_alloc();
    if (packet == NULL) {
        goto the_end;
    }

    MKMutex wait_mutex;
    status = mk_mutex_init(&wait_mutex);
    if (status != 0) {
        goto the_end;
    }

    for (;;) {
        if (*decoder->is_aborted == 1) {
            status = 0;
            break;
        }

        if (video_q->packet_count >= MAX_QUEUE_ITEM) {
            printf("Queue is full.\n");
            mk_mutex_lock(&wait_mutex);
            mk_cond_timedwait(
                &decoder->demuxer.continue_signal, &wait_mutex, 10
            );
            mk_mutex_unlock(&wait_mutex);
            continue;
        }

        status = av_read_frame(format, packet);
        if (status < 0) {
            if (status == AVERROR_EOF && *decoder->is_eof == 0) {
                *decoder->is_eof = 1;
            }

            mk_mutex_lock(&wait_mutex);
            mk_cond_timedwait(
                &decoder->demuxer.continue_signal, &wait_mutex, 10
            );
            mk_mutex_unlock(&wait_mutex);
            continue;
        } else {
            *decoder->is_eof = 0;
        }

        if (packet->stream_index == media->streams[MK_TRACK_TYPE_VIDEO]) {
            printf(
                "packet - pts=%lld dts=%lld pos=%lld\n", packet->pts,
                packet->dts, packet->pos
            );
            mk_packet_queue_put(video_q, packet);
        } else {
            av_packet_unref(packet);
        }
    }

the_end:
    mk_mutex_destroy(&wait_mutex);
    av_packet_free(&packet);

    return 0;
}

/**
 * Decoding API
 * https://ffmpeg.org/doxygen/4.0/group__lavc__encdec.html
 */
_MK_PRIVATE int mk_decoder2_get_video_frame(
    MKVideoDecoder2* decoder, AVFrame* frame, MKCond* is_empty_signal
)
{
    int status = AVERROR(EAGAIN);
    AVCodecContext* context = decoder->codec_context;
    MKPacketQueue* packet_queue = &decoder->packet_q;

    for (;;) {
        do {
            if (packet_queue->is_aborted) {
                return -1;
            }

            status = avcodec_receive_frame(context, frame);
            if (status == AVERROR_EOF) {
                decoder->is_finished = 1;
                avcodec_flush_buffers(context);
                return 0;
            }

            if (status >= 0) {
                return 1;
            }
        } while (status != AVERROR(EAGAIN));

        for (;;) {
            if (packet_queue->packet_count == 0) {
                mk_cond_signal(is_empty_signal);
            }

            status
                = mk_packet_queue_get(packet_queue, decoder->packet, 1, NULL);

            if (status < 0) {
                return -1;
            } else if (status == 1) {
                break;
            }

            av_packet_unref(decoder->packet);
        }

        if (avcodec_send_packet(context, decoder->packet) == AVERROR(EAGAIN)) {
            printf(
                "receive_frame and send_packet both returned EAGAIN, which is "
                "an API violation.\n "
            );
            return -1;
        }

        av_packet_unref(decoder->packet);
    }

    return 0;
}

_MK_PRIVATE int mk_decoder2_video_thread(void* data)
{
    int status;
    MKDecoder2* decoder = data;
    MKFrameQueue* picture_queue = &decoder->video.frame_q;
    MKVideoDecoder2* video_decoder = &decoder->video;

    AVFrame* frame = av_frame_alloc();
    if (frame == NULL) {
        goto the_end;
    }

    for (;;) {
        status = mk_decoder2_get_video_frame(
            video_decoder, frame, &decoder->demuxer.continue_signal
        );

        if (status < 0) {
            goto the_end;
        }

        if (status == 0) {
            continue;
        }

        MKFrameQueueItem* item = mk_frame_queue_peek_writable(picture_queue);
        if (item == NULL) {
            break;
        }

        printf(
            "frame - pts=%lld best_effort_timestamp=%lld dts=%lld\n",
            frame->pts, frame->best_effort_timestamp, frame->pkt_dts
        );

        item->width = frame->width;
        item->height = frame->height;
        item->format = frame->format;
        item->pts = frame->best_effort_timestamp;
        item->duration = frame->duration;
        av_frame_move_ref(item->frame, frame);
        av_frame_unref(frame);
        mk_frame_queue_push(picture_queue);
    }

the_end:
    av_frame_free(&frame);

    return 0;
}

int mk_decoder2_start(MKDecoder2* decoder)
{
    if (decoder == NULL) {
        return -1;
    }
    int status;

    MKMedia* media = decoder->media;
    MKThread* demuxer_thread = &decoder->demuxer.thread;

    demuxer_thread->name = "demuxer_thread";
    demuxer_thread->fn = mk_decoder2_demuxer_thread;
    demuxer_thread->userdata = decoder;
    status = mk_thread_init(demuxer_thread);
    if (status != 0) {
        return -1;
    }

    if (media->streams[MK_TRACK_TYPE_VIDEO] > -1) {
        mk_packet_queue_start(&decoder->video.packet_q);
        MKThread* video_thread = &decoder->video.thread;
        video_thread->name = "video_thread";
        video_thread->fn = mk_decoder2_video_thread;
        video_thread->userdata = decoder;
        status = mk_thread_init(video_thread);
        if (status != 0) {
            return -1;
        }
    }

    return 0;
}

int mk_decoder2_stop(MKDecoder2* decoder)
{
    if (decoder == NULL) {
        return -1;
    }

    MKMedia* media = decoder->media;
    MKThread* demuxer_thread = &decoder->demuxer.thread;

    *decoder->is_aborted = 1;
    mk_cond_signal(
        &decoder->demuxer.continue_signal
    ); // force demuxer to cancel sooner
    mk_thread_wait(demuxer_thread, NULL);

    if (media->streams[MK_TRACK_TYPE_VIDEO] > -1) {
        MKVideoDecoder2* video_decoder = &decoder->video;
        mk_packet_queue_abort(&video_decoder->packet_q);
        mk_frame_queue_trigger_changes(&video_decoder->frame_q);
        mk_thread_wait(&video_decoder->thread, NULL);
        mk_packet_queue_flush(&video_decoder->packet_q);
    }

    return 0;
}
