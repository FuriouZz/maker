#include "maker_internal.h"

void maker_video_decoder_uninit(MakerVideoDecoder* video)
{
    if (video == NULL) return;

    MAKER_LOG_DEBUG("Will abort video decoder");

    bool expected = FALSE;
    if (MAKER_ATOMIC_COMPARE_EXCHANGE(&video->is_aborted, &expected, TRUE)) {
        MAKER_LOG_DEBUG("Wait video decoder");
        maker_cond_broadcast(&video->frame_queue.new_item_signal);
        maker_mutex_lock(&video->aborted_lock);
        maker_cond_wait(&video->aborted_signal, &video->aborted_lock);
        maker_mutex_unlock(&video->aborted_lock);
    }

    MAKER_LOG_DEBUG("Release video decoder");
    maker_mutex_uninit(&video->aborted_lock);
    maker_cond_uninit(&video->aborted_signal);
    maker_frame_queue_uninit(&video->frame_queue);
    av_frame_free(&video->frame);
    av_packet_free(&video->packet);
    avcodec_free_context(&video->codec);
    video->is_aborted    = TRUE;
    video->packet_serial = -1;

    maker_video_converter_uninit(video->converter);
    maker_free(video->converter);
}

MakerStatus maker_video_decoder_init(MakerVideoDecoder* video, MakerMedia* media, MakerPacketQueue* packet_queue)
{
    MAKER_CHECK(video);
    MAKER_CHECK(media);

    maker_clear(video, sizeof(*video));

    video->packet_queue = packet_queue;
    video->is_aborted   = TRUE;

    u32                stream_index = media->info.streams[MAKER_TRACK_TYPE_VIDEO];
    AVFormatContext*   format       = media->format;
    AVStream*          stream       = format->streams[stream_index];
    AVCodecParameters* params       = stream->codecpar;

    const AVCodec* codec = avcodec_find_decoder(params->codec_id);
    if (codec == NULL) {
        MAKER_LOG_WARN("Cannot find decoder");
        goto cleanup;
    }

    video->codec = avcodec_alloc_context3(codec);
    if (video->codec == NULL) {
        MAKER_OUT_OF_MEMORY;
        goto cleanup;
    }

    if (avcodec_parameters_to_context(video->codec, params) != 0) {
        MAKER_LOG_WARN("Failed to transfer stream parameters to code context");
        goto cleanup;
    }

    if (avcodec_open2(video->codec, codec, NULL) != 0) {
        MAKER_LOG_WARN("Failed to open codec context");
        goto cleanup;
    }

    video->packet = av_packet_alloc();
    if (video->packet == NULL) {
        MAKER_OUT_OF_MEMORY;
        goto cleanup;
    }

    video->frame = av_frame_alloc();
    if (video->frame == NULL) {
        MAKER_OUT_OF_MEMORY;
        goto cleanup;
    }

    if (maker_frame_queue_init(&video->frame_queue, packet_queue, 16) != MAKER_STATUS_OK) {
        goto cleanup;
    }

    if (maker_cond_init(&video->aborted_signal) != MAKER_STATUS_OK) {
        goto cleanup;
    }

    if (maker_mutex_init(&video->aborted_lock) != MAKER_STATUS_OK) {
        goto cleanup;
    }

    return MAKER_STATUS_OK;

cleanup:
    maker_video_decoder_uninit(video);

    return MAKER_STATUS_ERROR;
}

static i32 maker__video_decoder_decode_frame(MakerVideoDecoder* video, bool* is_aborted)
{
    i32      status = AVERROR(EAGAIN);
    AVFrame* frame  = video->frame;

    for (;;) {
        if (video->packet_queue->serial == video->packet_serial) {
            do {
                if (MAKER_ATOMIC_LOAD(is_aborted) == TRUE) {
                    return -1;
                }

                status = avcodec_receive_frame(video->codec, frame);
                if (status == AVERROR_EOF) {
                    avcodec_flush_buffers(video->codec);
                    return 0;
                }

                if (status == 0) {
                    MAKER_LOG_DEBUG("Received frame");
                    return 1;
                }
            } while (status != AVERROR(EAGAIN));
        }

        for (;;) {
            if (MAKER_ATOMIC_LOAD(is_aborted) == TRUE) {
                return -1;
            }

            i32 old_serial = video->packet_serial;

            status = maker_packet_queue_get(
                video->packet_queue,
                video->packet,
                FALSE,
                &video->packet_serial
            );

            if (status < 0) {
                return -1;
            }

            if (old_serial != video->packet_serial) {
                avcodec_flush_buffers(video->codec);
            }

            if (video->packet_queue->serial == video->packet_serial) {
                break;
            }

            av_packet_unref(video->packet);
        }

        if (avcodec_send_packet(video->codec, video->packet) == AVERROR(EAGAIN)) {
            MAKER_LOG_WARN(
                "receive_frame and send_packet both returned EAGAIN, which is "
                "an API violation.\n "
            );
            return -1;
        }

        av_packet_unref(video->packet);
    }

    return 0;
}

static MakerStatus maker__video_decoder_decode(MakerVideoDecoder* video, MakerVideoDecoderOptions* options)
{
    bool* is_aborted = &video->is_aborted;

    MakerStatus status = MAKER_STATUS_ERROR;

    bool should_wait = TRUE;
    if (options != NULL) {
        should_wait = options->should_wait;
    }

    for (;;) {
        if (maker_frame_queue_is_full(&video->frame_queue) && !should_wait) {
            break;
        }

        i32 result = maker__video_decoder_decode_frame(video, is_aborted);

        if (result < 0) break;
        if (result == 0) continue;

        MakerFrameQueueItem* item = maker_frame_queue_peek_writable(&video->frame_queue);
        if (item == NULL) {
            MAKER_LOG_WARN("Failed to peek writable frame");
            break;
        }

        item->pts      = video->frame->pts;
        item->duration = video->frame->duration;

        av_frame_move_ref(item->frame, video->frame);
        av_frame_unref(video->frame);
        maker_frame_queue_push_writable(&video->frame_queue);
    }

    status = MAKER_STATUS_OK;
    MAKER_ATOMIC_STORE(&video->is_aborted, TRUE);
    maker_cond_signal(&video->aborted_signal);
    maker_cond_broadcast(&video->frame_queue.new_item_signal);

    return status;
}

MakerStatus maker_video_decoder_start(MakerVideoDecoder* video, MakerVideoDecoderOptions* options)
{
    MAKER_CHECK(video);

    bool expected = TRUE;
    if (MAKER_ATOMIC_COMPARE_EXCHANGE(&video->is_aborted, &expected, FALSE)) {
        return maker__video_decoder_decode(video, options);
    }

    MAKER_LOG_DEBUG("Video decoder is busy.");
    return MAKER_STATUS_OK;
}

MakerStatus maker_video_decoder_stop(MakerVideoDecoder* video)
{
    MAKER_CHECK(video);

    bool expected = FALSE;
    if (MAKER_ATOMIC_COMPARE_EXCHANGE(&video->is_aborted, &expected, TRUE)) {
        maker_cond_broadcast(&video->frame_queue.new_item_signal);
    }

    return MAKER_STATUS_OK;
}

MakerStatus maker_video_decoder_yuv2rgb(MakerVideoDecoder* decoder, MakerVideoFrame* target, AVFrame* src_frame)
{
    MAKER_CHECK(decoder);
    MAKER_CHECK(target);
    MAKER_CHECK(src_frame);

    if (decoder->converter == NULL) {
        decoder->converter = maker_malloc_clear(sizeof(*decoder->converter));
        if (decoder->converter == NULL) {
            MAKER_OUT_OF_MEMORY;
            return MAKER_STATUS_ERROR;
        }

        if (maker_video_converter_init(
                decoder->converter,
                decoder->codec->width,
                decoder->codec->height,
                maker_format_from_av_pixel_format(decoder->codec->pix_fmt),
                target->format
            )
            != MAKER_STATUS_OK) {
            return MAKER_STATUS_ERROR;
        }
    }

    return maker_video_converter_yuv2rgba(decoder->converter, target, src_frame);
}

bool maker_video_decoder_can_run(MakerVideoDecoder* video)
{
    MAKER_ASSERT(video);
    return !maker_frame_queue_is_full(&video->frame_queue);
}
