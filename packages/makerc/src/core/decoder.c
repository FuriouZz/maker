#include "maker_internal.h"

inline MakerDecoderInternal* maker__decoder_internal(MakerDecoder* user_decoder)
{
    return (MakerDecoderInternal*)user_decoder->internal_state;
}

MakerStatus maker_decoder_demux(void* data)
{
    MakerDecoder*         user_decoder = (MakerDecoder*)data;
    MakerDecoderInternal* decoder      = maker__decoder_internal(user_decoder);

    MAKER_LOG_INFO("demux start");
    MakerStatus status = maker_demuxer_start(
        &decoder->demuxer,
        &(MakerDemuxerOptions) {
            .max_video_frame_count = 16,
            .should_wait           = FALSE,
        }
    );
    MAKER_LOG_INFO("demux stopped");

    return status;
}

MakerStatus maker_decoder_decode_video(void* data)
{
    MakerDecoder*         user_decoder = (MakerDecoder*)data;
    MakerDecoderInternal* decoder      = maker__decoder_internal(user_decoder);

    MAKER_LOG_INFO("decode_video start");
    MakerStatus status = maker_video_decoder_start(
        &decoder->video,
        &(MakerVideoDecoderOptions) {
            .should_wait = FALSE,
        }
    );
    MAKER_LOG_INFO("decode_video stopped");

    return status;
}

static MakerStatus maker__decoder_start(MakerDecoder* user_decoder)
{
    MAKER_CHECK(user_decoder);

    MakerDecoderInternal* decoder = maker__decoder_internal(user_decoder);

    if (decoder->desc.use_playback) {
        maker_clock_start(&decoder->clock);
    }

    maker_demuxer_setup(&decoder->demuxer);

    return MAKER_STATUS_OK;
}

static MakerStatus maker__decoder_stop(MakerDecoder* user_decoder)
{
    MAKER_CHECK(user_decoder);

    MakerDecoderInternal* decoder = maker__decoder_internal(user_decoder);
    decoder->aborted              = TRUE;

    if (decoder->desc.use_playback) {
        maker_clock_pause(&decoder->clock);
    }

    maker_demuxer_stop(&decoder->demuxer);
    maker_video_decoder_stop(&decoder->video);
    return MAKER_STATUS_OK;
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
        goto fail;
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
            goto cleanup_decoder;
        }
    }

    MakerStatus status;
    status = maker_media_init(&decoder->media, desc->url);
    if (status != MAKER_STATUS_OK) {
        MAKER_OUT_OF_MEMORY;
        goto cleanup_context;
    }

    if (decoder->media.info.streams[MAKER_TRACK_TYPE_VIDEO] != -1) {
        if (maker_video_decoder_init(&decoder->video, &decoder->media) != MAKER_STATUS_OK) {
            goto cleanup_media;
        }
    }

    if (maker_demuxer_init(&decoder->demuxer, &decoder->media, &decoder->video) != MAKER_STATUS_OK) {
        goto cleanup_video_decoder;
    }

    user_decoder->internal_state = decoder;
    user_decoder->is_initialized = TRUE;

    return maker__decoder_start(user_decoder);

    // cleanup_demuxer:
    //     maker_demuxer_uninit(&decoder->demuxer);

cleanup_video_decoder:
    maker_video_decoder_uninit(&decoder->video);

cleanup_media:
    maker_media_uninit(&decoder->media);

cleanup_context:
    if (decoder->use_local_context) {
        maker_context_uninit(decoder->desc.context);
        maker_free(decoder->desc.context);
    }

cleanup_decoder:
    maker_free(decoder);

fail:
    return MAKER_STATUS_ERROR;
}

MakerStatus maker_decoder_uninit(MakerDecoder* user_decoder)
{
    MAKER_CHECK(user_decoder);
    MAKER_CHECK(user_decoder->is_initialized);

    if (maker__decoder_stop(user_decoder) != MAKER_STATUS_OK) {
        return MAKER_STATUS_ERROR;
    }

    MakerDecoderInternal* decoder = maker__decoder_internal(user_decoder);

    maker_demuxer_uninit(&decoder->demuxer);
    maker_video_decoder_uninit(&decoder->video);
    maker_media_uninit(&decoder->media);

    if (decoder->use_local_context) {
        maker_context_uninit(decoder->desc.context);
        maker_free(decoder->desc.context);
    }

    maker_free(decoder);

    user_decoder->is_initialized = FALSE;

    return MAKER_STATUS_OK;
}

MakerStatus maker_decoder_get_media_info(MakerDecoder* user_decoder, MakerMediaInfo* info)
{
    MAKER_CHECK(user_decoder);
    MAKER_CHECK(user_decoder->is_initialized);
    MAKER_CHECK(info);
    MakerDecoderInternal* decoder = maker__decoder_internal(user_decoder);
    memcpy(info, &decoder->media.info, sizeof(*info));
    return MAKER_STATUS_OK;
}

MakerStatus maker_decoder_seek(MakerDecoder* user_decoder, u64 timestamp)
{
    MAKER_CHECK(user_decoder);
    MAKER_CHECK(user_decoder->is_initialized);

    MakerDecoderInternal* decoder = maker__decoder_internal(user_decoder);
    MakerDemuxer*         demuxer = &decoder->demuxer;

    if (demuxer->needs_seek) {
        return MAKER_STATUS_BUSY;
    }

    demuxer->needs_seek     = TRUE;
    demuxer->seek_timestamp = timestamp;
    demuxer->seek_flags &= ~AVSEEK_FLAG_BYTE;

    return MAKER_STATUS_OK;
}

MakerStatus maker_decoder_get_playback_time(MakerDecoder* user_decoder, u32* time_ms)
{
    MAKER_CHECK(user_decoder);
    MAKER_CHECK(user_decoder->is_initialized);
    MAKER_CHECK(time_ms);

    MakerDecoderInternal* decoder = maker__decoder_internal(user_decoder);
    MakerClock*           clock   = &decoder->clock;

    MakerTime time = { 0 };
    if (maker_get_time(&time) != MAKER_STATUS_OK) {
        return MAKER_STATUS_ERROR;
    };

    *time_ms = (time.tv_sec - clock->start_time->tv_sec) * 1000
        + (time.tv_nsec - clock->start_time->tv_nsec) / 1000000;

    return MAKER_STATUS_OK;
}

// MakerStatus maker_decoder_set_playback_time(MakerDecoder* decoder, u32 time_ms)
// {
//     MAKER_CHECK(decoder);

//     MakerDecoderInternal* decoder = maker__decoder_internal(user_decoder);
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

    MakerDecoderInternal* decoder = maker__decoder_internal(user_decoder);
    MakerContextInternal* context = maker__context_internal(decoder->desc.context);

    if (context->desc.create_worker) {
        context->desc.create_worker(maker_decoder_demux, user_decoder);
        context->desc.create_worker(maker_decoder_decode_video, user_decoder);
    } else {
        if (maker_thread_pool_queue_job(
                &context->thread_pool,
                maker_decoder_demux,
                user_decoder
            )
            != MAKER_STATUS_OK) {
            return MAKER_STATUS_ERROR;
        }

        if (maker_thread_pool_queue_job(
                &context->thread_pool,
                maker_decoder_decode_video,
                user_decoder
            )
            != MAKER_STATUS_OK) {
            return MAKER_STATUS_ERROR;
        }
    }

    return MAKER_STATUS_OK;
}

u32 maker_decoder_get_video_frame(MakerDecoder* user_decoder, MakerVideoFrame* target)
{
    MAKER_CHECK(user_decoder);
    MAKER_CHECK(target);
    // (void)maker__context_video_refresh;

    MakerDecoderInternal* decoder = maker__decoder_internal(user_decoder);
    maker_decoder_queue(user_decoder);

    // if (maker__context_video_refresh(decoder) != MAKER_STATUS_OK) {
    //     return MAKER_STATUS_ERROR;
    // }

    MakerFrameQueue*     picture_queue = &decoder->video.frame_queue;
    MakerFrameQueueItem* item          = maker_frame_queue_peek_readable(picture_queue);

    if (item == NULL) {
        MAKER_LOG_ERROR("no frame found");
        return MAKER_STATUS_ERROR;
    }

    // enum AVPixelFormat dst_format = maker_format_to_av_pixel_format(target->format);
    // AVFrame*           dst_frame  = internal->video.frame;
    AVFrame* src_frame = item->frame;

    // MakerFrameQueueItem* next = maker_frame_queue_peek_next(picture_queue);
    // if (next->pts == context->next_pts) {
    //     return item->pts;
    // }

    if (maker_video_decoder_yuv2rgb(&decoder->video, target, src_frame) != MAKER_STATUS_OK) {
        MAKER_LOG_ERROR("Failed to to convert video frame");
        return MAKER_STATUS_ERROR;
    }

    // context->next_pts = next->pts;
    // return item->pts;

    MAKER_LOG_INFO("GET FRAME");
    return 0;
}
