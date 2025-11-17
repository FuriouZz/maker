#include "libavcodec/packet.h"
#include "libavutil/frame.h"
#include "maker/maker.h"
#include "maker_internal.h"

#define MAX_QUEUE_ITEM 15

MK_PRIVATE MKDecoder*
mk__lookup_decoder(MKDecoderPool* pool, uint32_t slot_id)
{
    mk_mutex_lock(&pool->mutex);
    uint32_t   index   = mk_pool_get_index(slot_id);
    MKDecoder* decoder = &pool->items[index];
    if (decoder->slot.id == slot_id) {
        mk_mutex_unlock(&pool->mutex);
        return decoder;
    }
    mk_mutex_unlock(&pool->mutex);
    return NULL;
}

/**
 * @return 0 when no video stream, 1 when initialized and -1 on error
 */
MK_PRIVATE int
mk__create_video_decoder(MKVideoDecoder* video, MKMedia2* media)
{
    i32 status;

    video->codec  = NULL;
    video->format = NULL;

    i32 stream_index = media->streams[MK_TRACK_TYPE_VIDEO];
    if (stream_index < 0) {
        return 0;
    }

    AVFormatContext*   format = media->format;
    AVStream*          stream = format->streams[stream_index];
    AVCodecParameters* params = stream->codecpar;

    status = mk_packet_queue_init(&video->packet_q);
    if (status != 0) {
        return -1;
    }

    status = mk_frame_queue_init(&video->frame_q, &video->packet_q, 16, 1);
    if (status != 0) {
        goto cleanup_packet_q;
    }

    const AVCodec* codec = avcodec_find_decoder(params->codec_id);
    if (codec == NULL) {
        goto cleanup_frame_q;
    }

    AVCodecContext* codec_context = avcodec_alloc_context3(codec);
    if (codec_context == NULL) {
        goto cleanup_frame_q;
    }
    video->codec = codec_context;

    status = avcodec_parameters_to_context(codec_context, params);
    if (status != 0) {
        goto cleanup_codec_context;
    }

    status = avcodec_open2(codec_context, codec, NULL);
    if (status != 0) {
        goto cleanup_codec_context;
    }

    AVPacket* packet = av_packet_alloc();
    if (packet == NULL) {
        goto cleanup_codec_context;
    }

    video->packet       = packet;
    video->format       = format;
    video->stream_index = stream_index;

    return 1;

cleanup_codec_context:
    avcodec_free_context(&video->codec);
    video->codec = NULL;

cleanup_frame_q:
    mk_frame_queue_uninit(&video->frame_q);

cleanup_packet_q:
    mk_packet_queue_uninit(&video->packet_q);

    return -1;
}

MK_PRIVATE void mk__destroy_video_decoder(MKVideoDecoder* video)
{
    MK_ASSERT(video);

    avcodec_free_context(&video->codec);
    video->codec  = NULL;
    video->format = NULL;

    mk_frame_queue_uninit(&video->frame_q);
    mk_packet_queue_uninit(&video->packet_q);
}

i32 mk_decoder_pool_init(MKDecoderPool* pool)
{
    MK_ASSERT(pool);

    mk_pool_init(&pool->pool, MK_MAX_MEDIA_POOL_SIZE);
    pool->items = mk_malloc(sizeof(MKDecoder) * pool->pool.size);

    if (pool->items == NULL) {
        goto cleanup_pool;
    }

    if (mk_mutex_init(&pool->mutex) != 0) {
        goto cleanup_items;
    }

    return 0;

cleanup_items:
    mk_free(pool->items);

cleanup_pool:
    mk_pool_uninit(&pool->pool);

    return -1;
}

void mk_decoder_pool_uninit(MKDecoderPool* pool)
{
    MK_ASSERT(pool);

    if (pool->items != NULL) {
        mk_free(pool->items);
        pool->items = NULL;
    }

    mk_pool_uninit(&pool->pool);
    mk_mutex_destroy(&pool->mutex);
}

MKDecoderHandle mk_decoder_pool_alloc_decoder(MKDecoderPool* pool)
{
    MK_ASSERT(pool);

    mk_mutex_lock(&pool->mutex);

    MKDecoderHandle handle = { 0 };

    u32 index = mk_pool_alloc_index(&pool->pool);
    if (index == 0) {
        MK_LOG_ERROR("Decoder pool exhausted");
        goto the_end;
    }

    MKDecoder* decoder = &pool->items[index];
    mk_pool_alloc_slot(&pool->pool, &decoder->slot, index);
    handle.slot_id = decoder->slot.id;

the_end:
    mk_mutex_unlock(&pool->mutex);

    return handle;
}

void mk_decoder_pool_dealloc_decoder(MKDecoderPool* pool, MKDecoderHandle* handle)
{
    MK_ASSERT(pool);

    mk_mutex_lock(&pool->mutex);

    MKDecoder* decoder = mk__lookup_decoder(pool, handle->slot_id);
    if (decoder == NULL) {
        MK_LOG_WARN("MKDecoderHandle is invalid.");
    } else {
        mk_pool_dealloc_slot(&pool->pool, &decoder->slot);
        mk_clear(&decoder->slot, sizeof(decoder->slot));
    }

    mk_mutex_unlock(&pool->mutex);
}

void mk_decoder_pool_init_decoder(
    MKDecoderPool* pool, MKDecoderHandle* handle, MKMedia2* media
)
{
    MK_ASSERT(pool);
    MK_ASSERT(handle);
    MK_ASSERT(media && media->slot.state == MK_RESOURCESTATE_VALID);

    i32 status;

    MKDecoder* decoder = mk__lookup_decoder(pool, handle->slot_id);
    if (decoder == NULL || decoder->slot.state != MK_RESOURCESTATE_ALLOC) {
        MK_LOG_WARN("MKDecoderHandle is invalid");
        return;
    }

    status = mk__create_video_decoder(&decoder->video, media);
    if (status < 0) {
        decoder->slot.state = MK_RESOURCESTATE_FAILED;
        MK_LOG_WARN("Failed to create video decoder.\n");
        return;
    }

    decoder->slot.state = MK_RESOURCESTATE_VALID;
}

void mk_decoder_pool_uninit_decoder(MKDecoderPool* pool, MKDecoderHandle* handle)
{
    MK_ASSERT(pool);
    MK_ASSERT(handle);

    MKDecoder* decoder = mk__lookup_decoder(pool, handle->slot_id);
    if (decoder != NULL) {
        mk__destroy_video_decoder(&decoder->video);
        decoder->slot.state = MK_RESOURCESTATE_ALLOC;
    } else {
        MK_LOG_WARN("MKDecoderHandle is invalid");
    }
}

i32 mk_decoder_has_video_packets(MKDecoderPool* pool, MKDecoderHandle* handle)
{
    MK_CHECK_VALID(pool);
    MK_CHECK_VALID(handle);

    MKDecoder* decoder;

    decoder = mk__lookup_decoder(pool, handle->slot_id);
    if (decoder == NULL || decoder->slot.state != MK_RESOURCESTATE_VALID) {
        MK_LOG_WARN("Invalid decoder");
        return -1;
    }

    return decoder->video.packet_q.packet_count;
}

i32 mk_decoder_has_video_frames(MKDecoderPool* pool, MKDecoderHandle* handle)
{
    MK_CHECK_VALID(pool);
    MK_CHECK_VALID(handle);

    MKDecoder* decoder;

    decoder = mk__lookup_decoder(pool, handle->slot_id);
    if (decoder == NULL || decoder->slot.state != MK_RESOURCESTATE_VALID) {
        MK_LOG_WARN("Invalid decoder");
        return -1;
    }

    return decoder->video.frame_q.frame_count;
}

i32 mk_decoder_pool_demux(MKDecoderPool* pool, MKDecoderHandle* handle, boolean* aborted)
{
    MK_CHECK_VALID(pool);
    MK_CHECK_VALID(handle);

    i32             status = -1;
    MKDecoder*      decoder;
    MKVideoDecoder* video;
    AVPacket*       packet;

    decoder = mk__lookup_decoder(pool, handle->slot_id);
    if (decoder == NULL || decoder->slot.state != MK_RESOURCESTATE_VALID) {
        MK_LOG_WARN("Invalid decoder");
        return -1;
    }

    packet = av_packet_alloc();
    if (packet == NULL) {
        goto the_end;
    }

    video = &decoder->video;
    mk_packet_queue_start(&video->packet_q);

    while (1) {
        if (aborted != NULL && *aborted) {
            break;
        }

        if (decoder->slot.state != MK_RESOURCESTATE_VALID) {
            break;
        }

        if (video->packet_q.packet_count >= MAX_QUEUE_ITEM) {
            MK_LOG_DEBUG("Queue is full.");
            break;
        }

        status = av_read_frame(video->format, packet);

        if (status < 0) {
            if (status == AVERROR_EOF && !video->is_eof) {
                video->is_eof = TRUE;
            }

            MK_LOG_DEBUG("End of file.");
            break;
        } else {
            video->is_eof = FALSE;
        }

        if (packet->stream_index == video->stream_index) {
            status = mk_packet_queue_put(&video->packet_q, packet);
            if (status < 0) {
                MK_LOG_WARN("Failed to put packet");
                status = -1;
                break;
            }
            MK_LOG_DEBUG("received video packet");
        } else {
            av_packet_unref(packet);
        }
    }

    mk_packet_queue_abort(&video->packet_q);

the_end:
    av_packet_free(&packet);

    return status;
}

/**
 * Decoding API
 * https://ffmpeg.org/doxygen/4.0/group__lavc__encdec.html
 */
MK_PRIVATE int mk__decoder_pool_get_video_frame(
    MKVideoDecoder* decoder, AVFrame* frame
)
{
    i32             status       = AVERROR(EAGAIN);
    AVCodecContext* context      = decoder->codec;
    MKPacketQueue*  packet_queue = &decoder->packet_q;

    for (;;) {
        do {
            if (packet_queue->is_aborted) {
                return -1;
            }

            status = avcodec_receive_frame(context, frame);
            if (status == AVERROR_EOF) {
                avcodec_flush_buffers(context);
                return 0;
            }

            if (status >= 0) {
                MK_LOG_DEBUG("received frame");
                return 1;
            }
        } while (status != AVERROR(EAGAIN));

        for (;;) {
            status = mk_packet_queue_get(
                packet_queue,
                decoder->packet,
                1,
                NULL
            );

            if (status < 0) {
                return -1;
            } else if (status == 1) {
                break;
            }

            av_packet_unref(decoder->packet);
        }

        if (avcodec_send_packet(context, decoder->packet) == AVERROR(EAGAIN)) {
            MK_LOG_WARN(
                "receive_frame and send_packet both returned EAGAIN, which is "
                "an API violation.\n "
            );
            return -1;
        }

        av_packet_unref(decoder->packet);
    }

    return 0;
}

i32 mk_decoder_pool_decode_video(MKDecoderPool* pool, MKDecoderHandle* handle, boolean* aborted)
{
    MK_CHECK_VALID(pool);
    MK_CHECK_VALID(handle);

    i32               status = -1;
    MKDecoder*        decoder;
    MKVideoDecoder*   video;
    MKFrameQueue*     frame_q;
    MKFrameQueueItem* item;
    AVFrame*          frame;

    decoder = mk__lookup_decoder(pool, handle->slot_id);
    if (decoder == NULL || decoder->slot.state != MK_RESOURCESTATE_VALID) {
        MK_LOG_WARN("Invalid decoder");
        return -1;
    }

    video   = &decoder->video;
    frame_q = &video->frame_q;

    frame = av_frame_alloc();
    if (frame == NULL) {
        goto the_end;
    }

    while (frame_q->frame_count < frame_q->max_frame_count) {
        if (aborted != NULL && *aborted) break;
        if (decoder->slot.state != MK_RESOURCESTATE_VALID) break;

        status = mk__decoder_pool_get_video_frame(video, frame);

        if (status < 0) break;
        if (status == 0) continue;

        item = mk_frame_queue_peek_writable(frame_q);
        if (item == NULL) {
            break;
        }

        item->width    = frame->width;
        item->height   = frame->height;
        item->format   = frame->format;
        item->pts      = frame->best_effort_timestamp;
        item->duration = frame->duration;

        av_frame_move_ref(item->frame, frame);
        av_frame_unref(frame);
        mk_frame_queue_push_writable(frame_q);
        break;
    }

the_end:
    av_frame_free(&frame);

    return status;
}

i32 mk_context_get_current_video_frame(MKDecoderPool* pool, MKDecoderHandle handle, MKImageData* target)
{
    MK_CHECK_VALID(pool);

    MKDecoder* decoder = mk__lookup_decoder(pool, handle.slot_id);
    if (decoder == NULL || decoder->slot.state != MK_RESOURCESTATE_VALID) {
        MK_LOG_WARN("Invalid decoder");
        return -1;
    }

    MKVideoDecoder* video_decoder = &decoder->video;
    MKFrameQueue*   frame_q       = &video_decoder->frame_q;
    // MKFrameQueueItem* item;
    // AVFrame*          frame;

    // if (ctx == NULL) {
    //     return -1;
    // }

    // MKInternalContext* context = ctx->context;

    // int32 status;
    // status = mk_context_video_refresh(ctx);
    // if (status != 0) {
    //     return -1;
    // }

    // int32          ret;
    // MKFrameQueue*  picture_queue = &context->decoder.video.frame_q;
    // MKVideoOutput* output        = &context->video_output;

    // MKFrameQueueItem* item = mk_frame_queue_peek(picture_queue);
    // MKFrameQueueItem* next = mk_frame_queue_peek_next(picture_queue);
    // if (next->pts == context->next_pts) {
    //     return item->pts;
    // }

    // ret = mk_image_data_init(
    //     target,
    //     &(MKImageDataDesc) {
    //         .width  = item->width,
    //         .height = item->height,
    //         .format = output->pixel_format,
    //     }
    // );

    // if (ret != 0) {
    //     printf("Failed to initialize image data\n");
    //     return -1;
    // }

    // if (mk_media_async_decoder_yuv2rgb(ctx, item->frame)) {
    //     printf("Failed to convert\n");
    //     return -1;
    // }

    // ret = av_image_copy_to_buffer(
    //     (target)->buffer, (target)->buffer_size,
    //     (const uint8_t* const*)output->frame->data, output->frame->linesize,
    //     mk_format_to_av_pixel_format(output->pixel_format), (target)->width,
    //     (target)->height, 1
    // );

    // if (ret < 0) {
    //     printf("Failed to copy image data\n");
    //     return -1;
    // }

    // context->next_pts = next->pts;
    // return item->pts;
}
