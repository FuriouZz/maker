#include "libavcodec/avcodec.h"
#include "libavcodec/packet.h"
#include "libavutil/avutil.h"
#include "libavutil/error.h"
#include "maker/ffmpeg_decoder.h"
#include "maker/ffmpeg_frame_queue.h"
#include "maker/ffmpeg_packet_queue.h"
#include "maker/mutex.h"
#include "maker/thread.h"
#include "maker/utils_mem.h"

int mk_ffmpeg_decoder_init(
    MKFFMpegDecoder* decoder, AVCodecContext* codec_context,
    MKFFMpegPacketQueue* packet_queue, MKCond* is_empty_signal
)
{
    mk_clear(decoder, sizeof(MKFFMpegDecoder));

    decoder->packet.packet = av_packet_alloc();
    if (!decoder->packet.packet) {
        return AVERROR(ENOMEM);
    }
    decoder->packet.serial = -1;

    decoder->codec_context = codec_context;
    decoder->packet_queue = packet_queue;
    decoder->is_empty_signal = is_empty_signal;
    decoder->start_pts = AV_NOPTS_VALUE;

    return 0;
}

void mk_ffmpeg_decoder_destroy(MKFFMpegDecoder* decoder)
{
    av_packet_free(&decoder->packet.packet);
    avcodec_free_context(&decoder->codec_context);
}

int mk_ffmpeg_decoder_start(
    MKFFMpegDecoder* decoder, MKThreadFunction callback,
    const char* thread_name, void* data
)
{
    mk_ffmpeg_packet_queue_start(decoder->packet_queue);
    decoder->thread = mk_thread_create(callback, thread_name, data);
    if (!decoder->thread) {
        return AVERROR(ENOMEM);
    }
    return 0;
}

void mk_ffmpeg_decoder_abort(
    MKFFMpegDecoder* decoder, MKFFMpegFrameQueue* frame_queue
)
{
    decoder->packet_queue->is_aborted = 1;
    mk_ffmpeg_frame_queue_trigger_changes(frame_queue);
    mk_thread_wait(decoder->thread, NULL);
    decoder->thread = NULL;
    mk_ffmpeg_packet_queue_flush(decoder->packet_queue);
}
