#include "maker.h"
#include "maker_internal.h"
#include <unistd.h>

static MakerStatus maker__decoder_decode(MakerDecoder* user_decoder)
{
    MAKER_CHECK(user_decoder);

    MakerDecoderInternal* decoder = (MakerDecoderInternal*)user_decoder;
    if (decoder->desc.use_threads == TRUE) {
        return MAKER_STATUS_OK;
    }

    maker_demuxer_start(
        &decoder->demuxer,
        &(MakerDemuxerOptions) {
            .video_frame_count = 1,
        }
    );
    maker_video_decoder_start(
        &decoder->video,
        &(MakerVideoDecoderOptions) {
            .max_count = 1,
        }
    );

    return MAKER_STATUS_OK;
}

MakerDecoder* maker_decoder_alloc(char* url, MakerDecoderDesc desc)
{
    MakerDecoderInternal* decoder = maker_malloc_clear(sizeof(*decoder));
    if (decoder == NULL) {
        MAKER_OUT_OF_MEMORY;
        goto fail;
    }

    decoder->desc = desc;

    decoder->media = maker_media_open(url);
    if (decoder->media == NULL) {
        MAKER_OUT_OF_MEMORY;
        goto cleanup_decoder;
    }

    if (decoder->media->streams[MAKER_TRACK_TYPE_VIDEO] != -1) {
        if (maker_video_decoder_init(&decoder->video, decoder->media) != MAKER_STATUS_OK) {
            goto cleanup_media;
        }
    }

    if (maker_demuxer_init(&decoder->demuxer, decoder->media, &decoder->video) != MAKER_STATUS_OK) {
        goto cleanup_video_decoder;
    }

    if (decoder->desc.thread_cb == NULL && decoder->desc.use_threads == TRUE) {
        decoder->thread_pool = maker_thread_pool_alloc();
        if (decoder->thread_pool == NULL) {
            goto cleanup_demuxer;
        }
    }

    return (MakerDecoder*)decoder;

cleanup_demuxer:
    maker_demuxer_uninit(&decoder->demuxer);

cleanup_video_decoder:
    maker_video_decoder_uninit(&decoder->video);

cleanup_media:
    maker_media_free(decoder->media);

cleanup_decoder:
    maker_free(decoder);

fail:
    return NULL;
}

void maker_decoder_free(MakerDecoder* user_decoder)
{
    if (user_decoder == NULL) return;
    MakerDecoderInternal* decoder = (MakerDecoderInternal*)user_decoder;
    maker_demuxer_uninit(&decoder->demuxer);
    maker_video_decoder_uninit(&decoder->video);
    maker_media_free(decoder->media);
    maker_free(decoder);
}

MakerStatus maker_decoder_demux(MakerDecoder* user_decoder)
{
    MakerDecoderInternal* decoder = (MakerDecoderInternal*)user_decoder;
    MakerStatus           status  = maker_demuxer_start(&decoder->demuxer, NULL);
    MAKER_LOG_INFO("demux stopped");
    return status;
}

MakerStatus maker_decoder_decode_video(MakerDecoder* user_decoder)
{
    MakerDecoderInternal* decoder = (MakerDecoderInternal*)user_decoder;
    MakerStatus           status  = maker_video_decoder_start(&decoder->video, NULL);
    MAKER_LOG_INFO("decode_video stopped");
    return status;
}

MakerStatus maker_decoder_start(MakerDecoder* user_decoder)
{
    MAKER_CHECK(user_decoder);

    MakerDecoderInternal* decoder = (MakerDecoderInternal*)user_decoder;

    if (decoder->desc.use_playback) {
        MakerDecoderInternal* internal = (MakerDecoderInternal*)decoder;
        maker_clock_start(&internal->clock);
    }

    if (decoder->desc.use_threads == FALSE) {
        return MAKER_STATUS_OK;
    }

    if (decoder->desc.thread_cb == NULL) {
        maker_thread_pool_init(decoder->thread_pool, 2);

        if (maker_thread_pool_queue_job(decoder->thread_pool, maker_decoder_demux, decoder) != MAKER_STATUS_OK) {
            return MAKER_STATUS_ERROR;
        }

        if (maker_thread_pool_queue_job(decoder->thread_pool, maker_decoder_decode_video, decoder) != MAKER_STATUS_OK) {
            return MAKER_STATUS_ERROR;
        }
    } else {
        decoder->desc.thread_cb(maker_decoder_demux, decoder);
        decoder->desc.thread_cb(maker_decoder_decode_video, decoder);
    }

    return MAKER_STATUS_OK;
}

MakerStatus maker_decoder_stop(MakerDecoder* user_decoder)
{
    MAKER_CHECK(user_decoder);

    MakerDecoderInternal* decoder = (MakerDecoderInternal*)user_decoder;

    if (decoder->desc.use_playback) {
        MakerDecoderInternal* internal = (MakerDecoderInternal*)decoder;
        maker_clock_pause(&internal->clock);
    }

    if (decoder->desc.use_threads == FALSE) {
        return MAKER_STATUS_OK;
    }

    maker_demuxer_stop(&decoder->demuxer);
    maker_video_decoder_stop(&decoder->video);
    return MAKER_STATUS_OK;
}

MakerStatus maker_decoder_seek(MakerDecoder* user_decoder, u64 timestamp)
{
    MAKER_CHECK(user_decoder);

    MakerDecoderInternal* decoder = (MakerDecoderInternal*)user_decoder;
    MakerDemuxer*         demuxer = &decoder->demuxer;

    if (demuxer->needs_seek) {
        return MAKER_STATUS_BUSY;
    }

    demuxer->needs_seek     = TRUE;
    demuxer->seek_timestamp = timestamp;
    demuxer->seek_flags &= ~AVSEEK_FLAG_BYTE;

    return MAKER_STATUS_OK;
}

MakerStatus maker_decoder_get_playback_time(MakerDecoder* decoder, u32* time_ms)
{
    MAKER_CHECK(decoder);
    MAKER_CHECK(time_ms);

    MakerDecoderInternal* internal = (MakerDecoderInternal*)decoder;
    MakerClock*           clock    = &internal->clock;

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

//     MakerDecoderInternal* internal = (MakerDecoderInternal*)decoder;
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

//     // MakerDecoderInternal* internal      = (MakerDecoderInternal*)decoder;
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

u32 maker_decoder_get_video_frame(MakerDecoder* decoder, MakerVideoFrame* target)
{
    MAKER_CHECK(decoder);
    MAKER_CHECK(target);
    // (void)maker__context_video_refresh;

    MakerDecoderInternal* internal = (MakerDecoderInternal*)decoder;
    maker__decoder_decode(decoder);

    // if (maker__context_video_refresh(decoder) != MAKER_STATUS_OK) {
    //     return MAKER_STATUS_ERROR;
    // }

    MakerFrameQueue*     picture_queue = &internal->video.frame_queue;
    MakerFrameQueueItem* item          = maker_frame_queue_peek_last(picture_queue);

    if (item == NULL) {
        return MAKER_STATUS_ERROR;
    }

    // enum AVPixelFormat dst_format = maker_format_to_av_pixel_format(target->format);
    // AVFrame*           dst_frame  = internal->video.frame;
    AVFrame* src_frame = item->frame;

    // MakerFrameQueueItem* next = maker_frame_queue_peek_next(picture_queue);
    // if (next->pts == context->next_pts) {
    //     return item->pts;
    // }

    if (maker_video_decoder_yuv2rgb(&internal->video, target, src_frame) != MAKER_STATUS_OK) {
        MAKER_LOG_ERROR("Failed to to convert video frame");
        return MAKER_STATUS_ERROR;
    }

    // context->next_pts = next->pts;
    // return item->pts;

    return 0;
}
