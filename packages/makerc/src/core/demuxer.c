#include "maker_internal.h"

static MakerStatus maker__demuxer_demux(MakerDemuxer* demuxer, MakerDemuxerOptions* options)
{
    MAKER_CHECK(demuxer);

    MakerVideoDecoder* video     = demuxer->video;
    AVPacket*          packet    = demuxer->packet;
    AVFormatContext*   format    = demuxer->format;
    MakerStatus        status    = MAKER_STATUS_ERROR;
    i32                ret       = 0;
    MakerMutex         lock      = { 0 };
    u32                remaining = 0;

    if (options != NULL) {
        remaining = options->video_frame_count;
    }

    if (maker_mutex_init(&lock) != MAKER_STATUS_OK) {
        goto the_end;
    }

    for (;;) {
        if (__atomic_load_n(&demuxer->is_aborted, __ATOMIC_SEQ_CST) == TRUE) {
            break;
        }

        if (av_fifo_can_write(video->packet_queue.fifo) == 0) {
            maker_cond_wait(&demuxer->signal, &lock);
            continue;
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
            maker_packet_queue_put(&video->packet_queue, packet, &demuxer->is_aborted);

            if (remaining > 0) {
                remaining--;
                if (remaining == 0) break;
            }
        }
    }

    bool expected = FALSE;
    bool desired  = TRUE;
    __atomic_compare_exchange(&demuxer->is_aborted, &expected, &desired, FALSE, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);

    status = MAKER_STATUS_OK;

    maker_mutex_uninit(&lock);

the_end:
    return status;
}

MakerStatus maker_demuxer_init(MakerDemuxer* demuxer, MakerMedia* media, MakerVideoDecoder* video_decoder)
{
    MAKER_CHECK(demuxer);
    MAKER_CHECK(media);
    MAKER_CHECK(video_decoder);

    MakerMediaInternal* internal_media = (MakerMediaInternal*)media;

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
    demuxer->format = internal_media->format;
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

MakerStatus maker_demuxer_start(MakerDemuxer* demuxer, MakerDemuxerOptions* options)
{
    MAKER_CHECK(demuxer);

    if (__atomic_load_n(&demuxer->is_aborted, __ATOMIC_SEQ_CST) == FALSE) {
        return MAKER_STATUS_BUSY;
    }

    __atomic_store_n(&demuxer->is_aborted, FALSE, __ATOMIC_SEQ_CST);
    maker__demuxer_demux(demuxer, options);

    return MAKER_STATUS_OK;
}

MakerStatus maker_demuxer_stop(MakerDemuxer* demuxer)
{
    MAKER_CHECK(demuxer);

    if (__atomic_load_n(&demuxer->is_aborted, __ATOMIC_SEQ_CST) == TRUE) {
        return MAKER_STATUS_OK;
    }

    __atomic_store_n(&demuxer->is_aborted, TRUE, __ATOMIC_SEQ_CST);
    maker_cond_signal(&demuxer->signal);

    return MAKER_STATUS_OK;
}
