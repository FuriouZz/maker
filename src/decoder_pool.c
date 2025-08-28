#include "decoder_pool.h"
#include "error.h"
#include "frame_queue.h"
#include "libavcodec/avcodec.h"
#include "libavcodec/packet.h"
#include "libavformat/avformat.h"
#include "maker/maker.h"
#include "media_pool.h"
#include "packet_queue.h"
#include "pool.h"
#include "util.h"
#include <stdio.h>

_MK_PRIVATE MKDecoder*
mk__lookup_decoder(MKDecoderPool* pool, uint32_t slot_id)
{
    uint32_t   index   = mk_pool_get_index(slot_id);
    MKDecoder* decoder = &pool->items[index];
    if (decoder->slot.id == slot_id) {
        return decoder;
    }
    return NULL;
}

/**
 * @return 0 when no video stream, 1 when initialized and -1 on error
 */
_MK_PRIVATE int
mk__create_video_decoder(MKVideoDecoder2* video, MKMedia2* media)
{
    int status;

    video->packet = NULL;
    video->codec  = NULL;
    video->format = NULL;

    int stream_index = media->streams[MK_TRACK_TYPE_VIDEO];
    if (stream_index < 0) {
        return 0;
    }

    AVFormatContext*   format = media->format;
    AVStream*          stream = format->streams[stream_index];
    AVCodecParameters* params = stream->codecpar;

    status = mk_init_packet_queue(&video->packet_q);
    if (status != 0) {
        return -1;
    }

    status = mk_init_frame_queue(&video->frame_q, &video->packet_q, 16, 1);
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
    video->packet = packet;

    video->format = format;

    return 1;

cleanup_codec_context:
    avcodec_free_context(&video->codec);
    video->codec = NULL;

cleanup_frame_q:
    mk_uninit_frame_queue(&video->frame_q);

cleanup_packet_q:
    mk_uninit_packet_queue(&video->packet_q);

    return -1;
}

_MK_PRIVATE void mk__destroy_video_decoder(MKVideoDecoder2* video)
{
    MK_ASSERT(video);

    video->format = NULL;

    av_packet_free(&video->packet);
    video->packet = NULL;

    avcodec_free_context(&video->codec);
    video->codec = NULL;

    mk_uninit_frame_queue(&video->frame_q);
    mk_uninit_packet_queue(&video->packet_q);
}

void mk_decoder_pool_init(MKDecoderPool* pool)
{
    MK_ASSERT(pool);
    mk_pool_init(&pool->pool, MK_MAX_MEDIA_POOL_SIZE);

    pool->items = mk_malloc(sizeof(MKDecoder) * pool->pool.size);
    MK_ASSERT(pool->items);
}

void mk_decoder_pool_uninit(MKDecoderPool* pool)
{
    MK_ASSERT(pool);
    mk_pool_uninit(&pool->pool);

    if (pool->items != NULL) {
        mk_free(pool->items);
        pool->items = NULL;
    }
}

MKDecoderHandle mk_decoder_pool_alloc_decoder(MKDecoderPool* pool)
{
    MK_ASSERT(pool);

    MKDecoderHandle handle = { 0 };

    uint32_t        index = mk_pool_alloc_index(&pool->pool);
    if (index != 0) {
        MKDecoder* decoder = &pool->items[index];
        mk_pool_alloc_slot(&pool->pool, &decoder->slot, index);
        handle.slot_id = decoder->slot.id;
    } else {
        MK_ERROR("Decoder pool exhausted");
    }

    return handle;
}

void mk_decoder_pool_dealloc_decoder(MKDecoderPool* pool, MKDecoderHandle* handle)
{
    MK_ASSERT(pool);

    MKDecoder* decoder = mk__lookup_decoder(pool, handle->slot_id);
    if (decoder != NULL) {
        mk_pool_dealloc_slot(&pool->pool, &decoder->slot);
        mk_clear(&decoder->slot, sizeof(decoder->slot));
    } else {
        MK_WARN("MKDecoderHandle is invalid.");
    }
}

void mk_decoder_pool_init_decoder(
    MKDecoderPool* pool, MKDecoderHandle* handle, MKMedia2* media
)
{
    MK_ASSERT(pool);
    MK_ASSERT(handle);
    MK_ASSERT(media && media->slot.state == MK_RESOURCESTATE_VALID);

    MKDecoder* decoder = mk__lookup_decoder(pool, handle->slot_id);
    if (decoder != NULL && decoder->slot.state == MK_RESOURCESTATE_ALLOC) {
        int status = mk__create_video_decoder(&decoder->video, media);
        if (status < 0) {
            decoder->slot.state = MK_RESOURCESTATE_FAILED;
            MK_WARN("Failed to create video decoder.\n");
            return;
        }

        decoder->slot.state = MK_RESOURCESTATE_VALID;
    } else {
        MK_WARN("MKDecoderHandle is invalid");
    }
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
        MK_WARN("MKDecoderHandle is invalid");
    }
}
