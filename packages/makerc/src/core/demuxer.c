#include "maker_internal.h"

static MakerStatus maker__demuxer_demux(MakerDemuxer* demuxer)
{
    MAKER_CHECK(demuxer);

    AVPacket*        packet = demuxer->packet;
    AVFormatContext* format = demuxer->format;
    MakerStatus      status = MAKER_STATUS_OK;
    i32              ret    = 0;
    MakerMutex       lock   = { 0 };

    u32 max_video_frame_count = demuxer->options.max_video_frame_count;

    status = maker_mutex_init(&lock);
    if (status != MAKER_STATUS_OK) {
        goto the_end;
    }

    for (;;) {
        if (MAKER_ATOMIC_LOAD(&demuxer->is_aborted) == TRUE) {
            break;
        }

        if (demuxer->needs_seek) {
            demuxer->needs_seek = FALSE;
            if (avformat_seek_file(format, -1, INT64_MIN, demuxer->seek_timestamp, INT64_MAX, AVSEEK_FLAG_BYTE) < 0) {
                MAKER_LOG_ERROR("Failed to seek");
                continue;
            }
            demuxer->is_eof = FALSE;
        }

        if (maker_packet_queue_count(&demuxer->picture_queue) >= max_video_frame_count) {
            break;
        }

        ret = av_read_frame(format, packet);

        if (ret < 0) {
            if (ret == AVERROR_EOF && !demuxer->is_eof) {
                demuxer->is_eof = TRUE;
                MAKER_LOG_DEBUG("End Of File");
            }
            break;
        } else {
            demuxer->is_eof = FALSE;
        }

        if (packet->stream_index == demuxer->picture_queue.stream_index) {
            MAKER_LOG_DEBUG("Put video packet");
            status = maker_packet_queue_put(&demuxer->picture_queue, packet);
            if (status != MAKER_STATUS_OK) {
                MAKER_LOG_ERROR("Cannot write packet");
                goto cleanup;
            }
        }
    }

cleanup:
    MAKER_ATOMIC_STORE(&demuxer->is_aborted, TRUE);
    maker_mutex_uninit(&lock);
    maker_cond_signal(&demuxer->abort_signal);

the_end:
    return status;
}

void maker_demuxer_setup(MakerDemuxer* demuxer)
{
    MAKER_ASSERT(demuxer);

    if (demuxer->picture_queue.stream_index > -1) {
        maker_packet_queue_start(&demuxer->picture_queue);
    }
}

MakerStatus maker_demuxer_start(MakerDemuxer* demuxer)
{
    MAKER_CHECK(demuxer);

    bool expected = TRUE;
    if (MAKER_ATOMIC_COMPARE_EXCHANGE(&demuxer->is_aborted, &expected, FALSE) == FALSE) {
        MAKER_LOG_DEBUG("Demuxer is busy.");
        return MAKER_STATUS_ERROR;
    }

    return maker__demuxer_demux(demuxer);
}

MakerStatus maker_demuxer_stop(MakerDemuxer* demuxer)
{
    MAKER_CHECK(demuxer);

    bool expected = FALSE;
    if (MAKER_ATOMIC_COMPARE_EXCHANGE(&demuxer->is_aborted, &expected, TRUE)) {
        maker_packet_queue_stop(&demuxer->picture_queue);
    }

    MAKER_LOG_DEBUG("Demuxer is already pending.");
    return MAKER_STATUS_OK;
}

MakerStatus maker_demuxer_init(MakerDemuxer* demuxer, MakerMedia* media, MakerDemuxerOptions* options)
{
    MAKER_CHECK(demuxer);
    MAKER_CHECK(media);

    maker_clear(demuxer, sizeof(*demuxer));
    demuxer->is_aborted = TRUE;

    if (options != NULL) {
        memcpy(&demuxer->options, options, sizeof(*options));
    }

    MAKER_ASSERT(demuxer->options.max_video_frame_count > 0);

    if (maker_packet_queue_init(&demuxer->picture_queue) != MAKER_STATUS_OK) {
        goto cleanup;
    }

    demuxer->packet = av_packet_alloc();
    if (demuxer->packet == NULL) {
        MAKER_OUT_OF_MEMORY;
        goto cleanup;
    }

    if (maker_cond_init(&demuxer->abort_signal) != MAKER_STATUS_OK) {
        goto cleanup;
    }

    demuxer->picture_queue.stream_index = media->info.streams[MAKER_TRACK_TYPE_VIDEO];
    demuxer->format                     = media->format;

    return MAKER_STATUS_OK;

cleanup:
    maker_demuxer_uninit(demuxer);
    return MAKER_STATUS_ERROR;
}

void maker_demuxer_uninit(MakerDemuxer* demuxer)
{
    if (demuxer == NULL) return;

    MAKER_LOG_DEBUG("Will uninit demuxer");

    bool expected = FALSE;
    if (MAKER_ATOMIC_COMPARE_EXCHANGE(&demuxer->is_aborted, &expected, TRUE)) {
        MAKER_LOG_DEBUG("Wait demuxer");
        maker_mutex_lock(&demuxer->abort_lock);
        maker_cond_wait(&demuxer->abort_signal, &demuxer->abort_lock);
        maker_mutex_unlock(&demuxer->abort_lock);
    }

    MAKER_LOG_DEBUG("Uninit demuxer");
    maker_mutex_uninit(&demuxer->abort_lock);
    maker_cond_uninit(&demuxer->abort_signal);
    av_packet_free(&demuxer->packet);
    maker_packet_queue_uninit(&demuxer->picture_queue);

    maker_clear(demuxer, sizeof(*demuxer));
    demuxer->is_aborted = TRUE;
}

bool maker_demuxer_can_run(MakerDemuxer* demuxer)
{
    MAKER_ASSERT(demuxer);
    return MAKER_ATOMIC_LOAD(&demuxer->is_aborted)
        && maker_packet_queue_count(&demuxer->picture_queue) < demuxer->options.max_video_frame_count;
}
