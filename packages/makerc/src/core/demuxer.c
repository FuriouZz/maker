#include "maker_internal.h"

static MakerStatus maker__demuxer_demux(MakerDemuxer* demuxer, MakerDemuxerOptions* options)
{
    MAKER_CHECK(demuxer);

    MakerVideoDecoder* video             = demuxer->video;
    AVPacket*          packet            = demuxer->packet;
    AVFormatContext*   format            = demuxer->format;
    MakerStatus        status            = MAKER_STATUS_OK;
    i32                ret               = 0;
    MakerMutex         lock              = { 0 };
    u32                video_frame_count = 0;
    bool               should_wait       = TRUE;

    if (options != NULL) {
        video_frame_count = options->max_video_frame_count;
        should_wait       = options->should_wait;
    }

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

        if (video_frame_count > 0) {
            if (video->packet_queue.packet_count >= video_frame_count) {
                if (should_wait) {
                    maker_cond_wait(&demuxer->signal, &lock);
                    continue;
                } else {
                    break;
                }
            }
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

        if (packet->stream_index == video->stream_index) {
            MAKER_LOG_DEBUG("Put video packet");
            status = maker_packet_queue_put(&video->packet_queue, packet);
            if (status != MAKER_STATUS_OK) {
                MAKER_LOG_ERROR("Cannot write packet");
                goto cleanup;
            }
        }
    }

cleanup:
    MAKER_ATOMIC_STORE(&demuxer->is_aborted, TRUE);
    maker_mutex_uninit(&lock);

the_end:
    return status;
}

MakerStatus maker_demuxer_init(MakerDemuxer* demuxer, MakerMedia* media, MakerVideoDecoder* video_decoder)
{
    MAKER_CHECK(demuxer);
    MAKER_CHECK(media);
    MAKER_CHECK(video_decoder);

    maker_clear(demuxer, sizeof(*demuxer));
    demuxer->format     = NULL;
    demuxer->packet     = NULL;
    demuxer->video      = NULL;
    demuxer->is_eof     = FALSE;
    demuxer->is_aborted = TRUE;

    AVPacket* packet = av_packet_alloc();
    if (packet == NULL) {
        MAKER_OUT_OF_MEMORY;
        goto fail;
    }

    MakerCond signal = { 0 };
    if (maker_cond_init(&signal) != MAKER_STATUS_OK) {
        goto cleanup_packet;
    }

    demuxer->video  = video_decoder;
    demuxer->format = media->format;
    demuxer->packet = packet;
    demuxer->signal = signal;

    return MAKER_STATUS_OK;

cleanup_packet:
    av_packet_free(&packet);

fail:
    return MAKER_STATUS_ERROR;
}

void maker_demuxer_uninit(MakerDemuxer* demuxer)
{
    if (demuxer == NULL) return;

    av_packet_free(&demuxer->packet);
    maker_cond_uninit(&demuxer->signal);
    demuxer->format     = NULL;
    demuxer->packet     = NULL;
    demuxer->video      = NULL;
    demuxer->is_eof     = FALSE;
    demuxer->is_aborted = TRUE;
}

void maker_demuxer_setup(MakerDemuxer* demuxer)
{
    MAKER_ASSERT(demuxer);

    if (demuxer->video != NULL && demuxer->video->is_aborted) {
        maker_packet_queue_start(&demuxer->video->packet_queue);
    }
}

MakerStatus maker_demuxer_start(MakerDemuxer* demuxer, MakerDemuxerOptions* options)
{
    MAKER_CHECK(demuxer);

    bool expected = TRUE;
    if (MAKER_ATOMIC_COMPARE_EXCHANGE(&demuxer->is_aborted, &expected, FALSE) == FALSE) {
        return MAKER_STATUS_BUSY;
    }

    return maker__demuxer_demux(demuxer, options);
}

MakerStatus maker_demuxer_stop(MakerDemuxer* demuxer)
{
    MAKER_CHECK(demuxer);

    bool expected = FALSE;
    if (MAKER_ATOMIC_COMPARE_EXCHANGE(&demuxer->is_aborted, &expected, TRUE) == FALSE) {
        return MAKER_STATUS_OK;
    }

    if (demuxer->video != NULL) {
        maker_packet_queue_stop(&demuxer->video->packet_queue);
    }
    maker_cond_signal(&demuxer->signal);

    return MAKER_STATUS_OK;
}
