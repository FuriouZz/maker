#include "maker_internal.h"

MakerStatus maker_video_decoder_init(MakerVideoDecoder* video, MakerMedia* media)
{
    MAKER_CHECK(video);
    MAKER_CHECK(media);

    maker_clear(video, sizeof(*video));

    video->codec     = NULL;
    video->converter = NULL;
    video->frame     = NULL;
    video->packet    = NULL;

    u32                stream_index = media->info.streams[MAKER_TRACK_TYPE_VIDEO];
    AVFormatContext*   format       = media->format;
    AVStream*          stream       = format->streams[stream_index];
    AVCodecParameters* params       = stream->codecpar;

    const AVCodec* codec = avcodec_find_decoder(params->codec_id);
    if (codec == NULL) {
        MAKER_LOG_WARN("Cannot find decoder");
        goto fail;
    }

    AVCodecContext* codec_context = avcodec_alloc_context3(codec);
    if (codec_context == NULL) {
        MAKER_OUT_OF_MEMORY;
        goto fail;
    }

    if (avcodec_parameters_to_context(codec_context, params) != 0) {
        MAKER_LOG_WARN("Failed to transfer stream parameters to code context");
        goto cleanup_codec_context;
    }

    if (avcodec_open2(codec_context, codec, NULL) != 0) {
        MAKER_LOG_WARN("Failed to open codec context");
        goto cleanup_codec_context;
    }

    AVPacket* packet = av_packet_alloc();
    if (packet == NULL) {
        MAKER_OUT_OF_MEMORY;
        goto cleanup_codec_context;
    }

    AVFrame* frame = av_frame_alloc();
    if (frame == NULL) {
        MAKER_OUT_OF_MEMORY;
        goto cleanup_packet;
    }

    if (maker_packet_queue_init(&video->packet_queue) != MAKER_STATUS_OK) {
        goto cleanup_frame;
    }

    if (maker_frame_queue_init(&video->frame_queue, 16) != MAKER_STATUS_OK) {
        goto cleanup_packet_queue;
    }

    video->codec         = codec_context;
    video->packet        = packet;
    video->frame         = frame;
    video->stream_index  = stream_index;
    video->packet_serial = -1;
    video->is_aborted    = TRUE;

    return MAKER_STATUS_OK;

cleanup_packet_queue:
    maker_packet_queue_uninit(&video->packet_queue);

cleanup_frame:
    av_frame_free(&frame);

cleanup_packet:
    av_packet_free(&packet);

cleanup_codec_context:
    avcodec_free_context(&codec_context);

fail:
    return MAKER_STATUS_ERROR;
}

void maker_video_decoder_uninit(MakerVideoDecoder* video)
{
    if (video == NULL) return;

    avcodec_free_context(&video->codec);
    av_packet_free(&video->packet);
    av_frame_free(&video->frame);
    maker_packet_queue_uninit(&video->packet_queue);
    maker_video_converter_free(video->converter);
    video->stream_index = -1;
}

static i32 maker__video_decoder_decode_frame(MakerVideoDecoder* video, bool* is_aborted)
{
    i32      status = AVERROR(EAGAIN);
    AVFrame* frame  = video->frame;

    for (;;) {
        if (video->packet_queue.serial == video->packet_serial) {
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
                &video->packet_queue,
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

            if (video->packet_queue.serial == video->packet_serial) {
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

    u32 remaining_frame_count = 0;
    if (options != NULL) {
        remaining_frame_count = options->max_count;
    }

    for (;;) {
        i32 result = maker__video_decoder_decode_frame(video, is_aborted);

        if (result < 0) break;
        if (result == 0) continue;

        MakerFrameQueueItem* item = maker_frame_queue_peek_writable(&video->frame_queue, is_aborted);
        if (item == NULL) {
            MAKER_LOG_WARN("Failed to peek writable frame");
            break;
        }

        av_frame_move_ref(item->frame, video->frame);
        av_frame_unref(video->frame);
        maker_frame_queue_push_writable(&video->frame_queue);

        if (remaining_frame_count > 0) {
            remaining_frame_count--;
            if (remaining_frame_count == 0) break;
        }
    }

    status = MAKER_STATUS_OK;
    MAKER_ATOMIC_STORE(&video->is_aborted, TRUE);

    return status;
}

MakerStatus maker_video_decoder_start(MakerVideoDecoder* video, MakerVideoDecoderOptions* options)
{
    MAKER_CHECK(video);

    bool expected = TRUE;
    if (MAKER_ATOMIC_COMPARE_EXCHANGE(&video->is_aborted, &expected, FALSE) == FALSE) {
        return MAKER_STATUS_BUSY;
    }

    return maker__video_decoder_decode(video, options);
}

MakerStatus maker_video_decoder_stop(MakerVideoDecoder* video)
{
    MAKER_CHECK(video);

    bool expected = FALSE;
    if (MAKER_ATOMIC_COMPARE_EXCHANGE(&video->is_aborted, &expected, TRUE) == FALSE) {
        return MAKER_STATUS_OK;
    }

    maker_cond_broadcast(&video->frame_queue.new_item_signal);

    return MAKER_STATUS_OK;
}

MakerStatus maker_video_decoder_yuv2rgb(MakerVideoDecoder* decoder, MakerVideoFrame* target, AVFrame* src_frame)
{
    MAKER_CHECK(decoder);
    MAKER_CHECK(target);
    MAKER_CHECK(src_frame);

    if (decoder->converter == NULL) {
        decoder->converter = maker_video_converter_alloc();
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
