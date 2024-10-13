#include <maker/maker_decoder.h>

#include <libavcodec/avcodec.h>

#include <maker/maker_mutex.h>
#include <maker/maker_thread.h>

#include <stdint.h>

#include "maker_internal.h"

typedef struct MKDecoderPacketList {
  AVPacket *packet;
  int serial;
} MKDecoderPacketList;

typedef struct MKDecoderQueue {
  MKDecoderPacketList list;
  int packet_count;
  int size;
  uint64_t duration;
  int abort_request;
  int serial;
  MKMutex *mutex;
  MKCond *cond;
} MKDecoderQueue;

typedef struct MKDecoder {
  MKDecoderQueue video_queue;

  /* Open audio/video stream and read packet */
  MKThread *read_packet_thread;

  /* Decode video packet and prepare them for display */
  MKThread *video_thread;
} MKDecoder;

_MAKER_PRIVATE int read_packet_thread(void *data) {
  MKDecoder *decoder = data;

  MKMutex *wait_mutex = mk_mutex_create();
  if (!wait_mutex) {
    goto fail;
  }

  AVPacket *packet = av_packet_alloc();
  if (!packet) {
    goto fail;
  }

fail:
  return -1;

  return 0;
}
