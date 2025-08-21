#include "decoder.h"
#include "frame_queue.h"
#include "libavcodec/avcodec.h"
#include "libavcodec/packet.h"
#include "libavutil/error.h"
#include "mutex.h"
#include "packet_queue.h"
#include "thread.h"
#include "util.h"

int mk_decoder_init(
    MKDecoder* decoder, AVCodecContext* codec_context,
    MKPacketQueue* packet_queue, MKCond* is_empty_signal
)
{
    MK_ASSERT(decoder);
    MK_ASSERT(codec_context);
    MK_ASSERT(packet_queue);
    MK_ASSERT(is_empty_signal);

    mk_clear(decoder, sizeof(MKDecoder));

    decoder->packet.packet = av_packet_alloc();
    if (decoder->packet.packet == NULL) {
        return AVERROR(ENOMEM);
    }
    decoder->packet.serial = -1;

    decoder->codec_context = codec_context;
    decoder->packet_queue = packet_queue;
    decoder->is_empty_signal = is_empty_signal;

    return 0;
}

void mk_decoder_destroy(MKDecoder* decoder)
{
    if (decoder != NULL) {
        av_packet_free(&decoder->packet.packet);
        avcodec_free_context(&decoder->codec_context);
    }
}

int mk_decoder_start(MKDecoder* decoder)
{
    MK_ASSERT(decoder);

    int ret;
    ret = mk_thread_init(&decoder->thread);
    if (ret != 0) {
        return ret;
    }

    mk_packet_queue_start(decoder->packet_queue);

    return 0;
}

void mk_decoder_abort(MKDecoder* decoder, MKFrameQueue* frame_queue)
{
    MK_ASSERT(decoder);
    MK_ASSERT(frame_queue);

    mk_packet_queue_abort(decoder->packet_queue);
    mk_frame_queue_trigger_changes(frame_queue);
    mk_thread_wait(&decoder->thread, NULL);
    mk_packet_queue_flush(decoder->packet_queue);
}
