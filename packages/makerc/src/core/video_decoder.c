#include "maker_internal.h"

MakerStatus maker_video_decoder_init(MakerVideoDecoder* video, MakerMedia* media)
{
    MAKER_CHECK(video);
    MAKER_CHECK(media);

    MakerMediaInternal* internal_media = (MakerMediaInternal*)media;

    maker_clear(video, sizeof(*video));

    video->codec     = NULL;
    video->converter = NULL;
    video->frame     = NULL;
    video->packet    = NULL;

    u32                stream_index = internal_media->streams[MAKER_TRACK_TYPE_VIDEO];
    AVFormatContext*   format       = internal_media->format;
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

    video->is_aborted   = FALSE;
    video->codec        = codec_context;
    video->packet       = packet;
    video->frame        = frame;
    video->stream_index = stream_index;
    video->is_aborted   = TRUE;

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
        do {
            if (__atomic_load_n(is_aborted, __ATOMIC_SEQ_CST) == TRUE) {
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

        for (;;) {
            if (__atomic_load_n(is_aborted, __ATOMIC_SEQ_CST) == TRUE) {
                return -1;
            }

            status = maker_packet_queue_get(&video->packet_queue, video->packet, FALSE, is_aborted);

            if (status < 0) {
                return -1;
            } else if (status == 1) {
                break;
            }
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

MakerStatus maker_video_decoder_start(MakerVideoDecoder* video, MakerVideoDecoderOptions* options)
{
    MAKER_CHECK(video);

    if (__atomic_load_n(&video->is_aborted, __ATOMIC_SEQ_CST) == FALSE) {
        return MAKER_STATUS_BUSY;
    }
    __atomic_store_n(&video->is_aborted, FALSE, __ATOMIC_SEQ_CST);

    MakerStatus status = MAKER_STATUS_ERROR;

    u32 remaining_frame_count = 0;
    if (options != NULL) {
        remaining_frame_count = options->max_count;
    }

    for (;;) {
        i32 result = maker__video_decoder_decode_frame(video, &video->is_aborted);
        if (result < 0) break;
        if (result == 0) continue;

        MakerFrameQueueItem* item = maker_frame_queue_peek_writable(&video->frame_queue, &video->is_aborted);
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

    bool expected = FALSE;
    bool desired  = TRUE;
    __atomic_compare_exchange(&video->is_aborted, &expected, &desired, FALSE, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);

    status = MAKER_STATUS_OK;

    return status;
}

MakerStatus maker_video_decoder_stop(MakerVideoDecoder* video)
{
    MAKER_CHECK(video);

    if (__atomic_load_n(&video->is_aborted, __ATOMIC_SEQ_CST) == TRUE) {
        return MAKER_STATUS_OK;
    }

    __atomic_store_n(&video->is_aborted, TRUE, __ATOMIC_SEQ_CST);

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
