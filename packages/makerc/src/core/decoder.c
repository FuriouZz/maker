#include "maker_internal.h"

MakerStatus maker_decoder_demux(void* data)
{
    MakerJobData*         worker       = (MakerJobData*)data;
    MakerDecoder*         user_decoder = (MakerDecoder*)worker->data;
    MakerDecoderInternal* decoder      = (MakerDecoderInternal*)user_decoder->internal_state;

    MakerStatus status = MAKER_STATUS_OK;
    if (MAKER_ATOMIC_LOAD(&worker->status) == MAKER_WORKER_STATUS_PENDING) {
        MAKER_ATOMIC_STORE(&worker->status, MAKER_WORKER_STATUS_BUSY);
        MAKER_LOG_INFO("demux start");
        status = maker_demuxer_start(&decoder->demuxer);
        MAKER_ATOMIC_STORE(&worker->status, MAKER_WORKER_STATUS_IDLE);
        MAKER_LOG_INFO("demux stopped");
    }

    return status;
}

MakerStatus maker_decoder_decode_video(void* data)
{
    MakerJobData*         worker       = (MakerJobData*)data;
    MakerDecoder*         user_decoder = (MakerDecoder*)worker->data;
    MakerDecoderInternal* decoder      = (MakerDecoderInternal*)user_decoder->internal_state;

    MakerStatus status = MAKER_STATUS_OK;
    if (MAKER_ATOMIC_LOAD(&worker->status) == MAKER_WORKER_STATUS_PENDING) {
        MAKER_ATOMIC_STORE(&worker->status, MAKER_WORKER_STATUS_BUSY);
        MAKER_LOG_INFO("decode_video start");
        status = maker_video_decoder_start(
            &decoder->video,
            &(MakerVideoDecoderOptions) {
                .should_wait = FALSE,
            }
        );
        MAKER_ATOMIC_STORE(&worker->status, MAKER_WORKER_STATUS_IDLE);
        MAKER_LOG_INFO("decode_video stopped");
    }

    return status;
}

static MakerStatus maker__decoder_start(MakerDecoder* user_decoder)
{
    MAKER_CHECK(user_decoder);

    MakerDecoderInternal* decoder = (MakerDecoderInternal*)user_decoder->internal_state;
    maker_demuxer_setup(&decoder->demuxer);

    return MAKER_STATUS_OK;
}

static void maker__decoder_stop(MakerDecoder* user_decoder)
{
    if (user_decoder == NULL) return;

    MakerDecoderInternal* decoder = (MakerDecoderInternal*)user_decoder->internal_state;
    maker_demuxer_stop(&decoder->demuxer);
    maker_video_decoder_stop(&decoder->video);
}

/**
 * Initialize decoder
 * @param char* url of the video to decode
 * @param MakerDecoderDesc description
 * @return MakerDecoder
 */
MakerStatus maker_decoder_init(MakerDecoder* user_decoder, MakerDecoderDesc* desc)
{
    MAKER_CHECK(user_decoder);
    MAKER_CHECK(desc);

    MakerDecoderInternal* decoder = maker_malloc_clear(sizeof(*decoder));
    if (decoder == NULL) {
        MAKER_OUT_OF_MEMORY;
        goto cleanup;
    }

    memcpy(&decoder->desc, desc, sizeof(*desc));

    decoder->use_local_context = decoder->desc.context == NULL;
    if (decoder->use_local_context) {
        MakerContextDesc context_desc = { 0 };
        if (decoder->desc.context_desc != NULL) {
            memcpy(&context_desc, decoder->desc.context_desc, sizeof(context_desc));
        }

        MakerContext* context = maker_malloc_clear(sizeof(MakerContext));
        decoder->desc.context = context;

        if (maker_context_init(context, &context_desc) != MAKER_STATUS_OK) {
            goto cleanup;
        }
    }

    MakerStatus status;
    status = maker_media_init(&decoder->media, desc->url);
    if (status != MAKER_STATUS_OK) {
        MAKER_OUT_OF_MEMORY;
        goto cleanup;
    }

    if (maker_demuxer_init(
            &decoder->demuxer,
            &decoder->media,
            &(MakerDemuxerOptions) {
                .max_video_frame_count = 16,
            }
        )
        != MAKER_STATUS_OK) {
        goto cleanup;
    }

    if (decoder->demuxer.picture_queue.stream_index != -1) {
        if (maker_video_decoder_init(
                &decoder->video,
                &decoder->media,
                &decoder->demuxer.picture_queue
            )
            != MAKER_STATUS_OK) {
            goto cleanup;
        }
    }

    decoder->demux_worker.data         = user_decoder;
    decoder->video_decoder_worker.data = user_decoder;

    user_decoder->internal_state = decoder;
    user_decoder->is_initialized = TRUE;

    return maker__decoder_start(user_decoder);

cleanup:
    maker_decoder_uninit(user_decoder);

    return MAKER_STATUS_ERROR;
}

void maker_decoder_uninit(MakerDecoder* user_decoder)
{
    if (user_decoder == NULL) return;
    if (user_decoder->is_initialized == TRUE) return;

    MAKER_LOG_DEBUG("gooo");

    maker__decoder_stop(user_decoder);

    MakerDecoderInternal* decoder = (MakerDecoderInternal*)user_decoder->internal_state;

    maker_video_decoder_uninit(&decoder->video);
    maker_demuxer_uninit(&decoder->demuxer);
    maker_media_uninit(&decoder->media);

    if (decoder->use_local_context) {
        maker_context_uninit(decoder->desc.context);
        maker_free(decoder->desc.context);
    }

    maker_free(decoder);
    maker_clear(user_decoder, sizeof(*user_decoder));
}

MakerStatus maker_decoder_get_media_info(MakerDecoder* user_decoder, MakerMediaInfo* info)
{
    MAKER_CHECK(user_decoder);
    MAKER_CHECK(user_decoder->is_initialized);
    MAKER_CHECK(info);
    MakerDecoderInternal* decoder = (MakerDecoderInternal*)user_decoder->internal_state;
    memcpy(info, &decoder->media.info, sizeof(*info));
    return MAKER_STATUS_OK;
}

MakerStatus maker_decoder_seek(MakerDecoder* user_decoder, u64 timestamp)
{
    MAKER_CHECK(user_decoder);
    MAKER_CHECK(user_decoder->is_initialized);

    MakerDecoderInternal* decoder = (MakerDecoderInternal*)user_decoder->internal_state;
    MakerDemuxer*         demuxer = &decoder->demuxer;

    if (demuxer->needs_seek) {
        MAKER_LOG_DEBUG("Decoder is already seeking.");
        return MAKER_STATUS_OK;
    }

    demuxer->needs_seek     = TRUE;
    demuxer->seek_timestamp = timestamp;
    demuxer->seek_flags &= ~AVSEEK_FLAG_BYTE;

    return MAKER_STATUS_OK;
}

// MakerStatus maker_decoder_set_playback_time(MakerDecoder* decoder, u32 time_ms)
// {
//     MAKER_CHECK(decoder);

//     MakerDecoderInternal* decoder = (MakerDecoderInternal*)user_decoder->internal_state;
//     MakerClock*           clock    = &internal->clock;

//     u32 time_s  = time_ms / 1000;
//     u32 time_ns = (time_ms - ((time_ms / 1000) * 1000)) * 1000000;

//     MakerTime time = { 0 };
//     if (maker_get_time(&time) != 0) {
//         return MAKER_STATUS_ERROR;
//     };

//     clock->start_time->tv_sec  = time.tv_sec - time_s;
//     clock->start_time->tv_nsec = time.tv_nsec - time_ns;

//     return MAKER_STATUS_OK;
// }

// static MakerStatus maker__context_video_refresh(MakerDecoder* decoder)
// {
//     MAKER_CHECK(decoder);

//     // MakerDecoderInternal* internal      = (MakerDecoderInternal*)decoder->internal_state;
//     // MakerFrameQueue*      picture_queue = &internal->video.frame_queue;

//     u32 time_spent_ms;
//     // printf("timespent=%i\n", time_spent_ms);
//     if (maker_decoder_get_playback_time(decoder, &time_spent_ms) != MAKER_STATUS_OK) {
//         return MAKER_STATUS_ERROR;
//     }

//     // real64 time_spent = ((real64)time_spent_ms) / 1000.0;
//     // real64 time;

//     // for (;;) {
//     //     if (mk_frame_queue_remaining_frame_count(picture_queue) < 2) {
//     //         // do nothing
//     //     } else {
//     //         MakerFrameQueueItem* next = maker_frame_queue_peek_next(picture_queue);

//     //         int stream_index
//     //             = context->decoder.media->streams[MK_TRACK_TYPE_VIDEO];
//     //         AVStream* stream = context->decoder.media->context->format
//     //                                ->streams[stream_index];

//     //         time = av_q2d(stream->time_base) * next->pts;

//     //         if (time <= time_spent) {
//     //             maker_frame_queue_drop(picture_queue); // drop frame
//     //         } else {
//     //             break;
//     //         }
//     //     }
//     // }

//     return MAKER_STATUS_OK;
// }

static MakerStatus maker_decoder_queue(MakerDecoder* user_decoder)
{
    MAKER_CHECK(user_decoder);
    MAKER_CHECK(user_decoder->is_initialized);

    MakerDecoderInternal* decoder = (MakerDecoderInternal*)user_decoder->internal_state;
    MakerContextInternal* context = (MakerContextInternal*)decoder->desc.context->internal_state;

    MakerStatus status = MAKER_STATUS_OK;

    if (context->desc.create_worker) {
        context->desc.create_worker(maker_decoder_demux, user_decoder);
        context->desc.create_worker(maker_decoder_decode_video, user_decoder);
    } else {
        MakerWorkerStatus expected = MAKER_WORKER_STATUS_IDLE;

        if (maker_demuxer_can_run(&decoder->demuxer)
            && MAKER_ATOMIC_COMPARE_EXCHANGE(
                &decoder->demux_worker.status,
                &expected,
                MAKER_WORKER_STATUS_PENDING
            )) {
            status = maker_thread_pool_queue_job(
                &context->thread_pool,
                maker_decoder_demux,
                &decoder->demux_worker
            );
            if (status != MAKER_STATUS_OK) {
                goto the_end;
            }
        }

        if (maker_video_decoder_can_run(&decoder->video)
            && MAKER_ATOMIC_COMPARE_EXCHANGE(
                &decoder->video_decoder_worker.status,
                &expected, MAKER_WORKER_STATUS_PENDING
            )) {
            status = maker_thread_pool_queue_job(
                &context->thread_pool,
                maker_decoder_decode_video,
                &decoder->video_decoder_worker
            );
            if (status != MAKER_STATUS_OK) {
                goto the_end;
            }
        }
    }

the_end:
    return status;
}

u32 maker_decoder_get_video_frame(MakerDecoder* user_decoder, MakerVideoFrame* target)
{
    MAKER_CHECK(user_decoder);
    MAKER_CHECK(target);

    MakerDecoderInternal* decoder       = (MakerDecoderInternal*)user_decoder->internal_state;
    MakerFrameQueue*      picture_queue = &decoder->video.frame_queue;

    MAKER_UNUSED(maker_decoder_queue);

    if (picture_queue->frame_count < 5) {
        maker_decoder_queue(user_decoder);
    }

    // if (picture_queue->frame_count == 0) {
    //     MAKER_LOG_ERROR("no frame found");
    //     return MAKER_STATUS_ERROR;
    // }

    // MakerFrameQueueItem* item = maker_frame_queue_peek_last(picture_queue);
    MakerFrameQueueItem* item = maker_frame_queue_peek_readable(picture_queue);

    // printf("time=%llu\n", item->pts);

    AVFrame* src_frame = item->frame;

    if (maker_video_decoder_yuv2rgb(&decoder->video, target, src_frame) != MAKER_STATUS_OK) {
        MAKER_LOG_ERROR("Failed to to convert video frame");
        return MAKER_STATUS_ERROR;
    }

    maker_frame_queue_pop_readable(picture_queue);

    return 0;
}
