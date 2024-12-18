#ifndef MAKER_PLAYER_FFPLAY_H
#define MAKER_PLAYER_FFPLAY_H

#include "libavcodec/codec_par.h"
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/fifo.h>
#include <libswscale/swscale.h>
#include <maker/maker_media.h>
#include <maker/maker_mutex.h>
#include <maker/maker_pool.h>
#include <maker/maker_thread.h>

#include <stdbool.h>
#include <stdint.h>

#define VIDEO_PICTURE_QUEUE_SIZE 3
#define SUBPICTURE_QUEUE_SIZE 16
#define SAMPLE_QUEUE_SIZE 9
#define FRAME_QUEUE_SIZE                                                       \
  FFMAX(                                                                       \
      SAMPLE_QUEUE_SIZE,                                                       \
      FFMAX(VIDEO_PICTURE_QUEUE_SIZE, SUBPICTURE_QUEUE_SIZE)                   \
  )

typedef struct MKPlayerLogger {
  void (*func)(
      const char *tag,              // always "mk_player"
      uint32_t log_level,           // 0=panic, 1=error, 2=warning, 3=info
      uint32_t log_item_id,         // mk_player_LOGITEM_*
      const char *message_or_null,  // a message string, may be
                                    // nullptr in release mode
      uint32_t line_nr,             // line number in video_player.h
      const char *filename_or_null, // source filename, may be
                                    // nullptr in release mode
      void *user_data
  );
  void *user_data;
} MKPlayerLogger;

typedef struct MKPlayerDesc {
  MKPlayerLogger logger;
} MKPlayerDesc;

typedef struct MKPlayerFrame {
  AVFrame *frame;
  int width;
  int height;
  double pts;      // presentation timestamp for the frame
  double duration; // estimated duration of the frame
  int64_t pos;     // byte position of the frame in the input file
} MKPlayerFrame;

typedef struct MKPlayerPacketList {
  AVPacket *pkt;
  int serial;
} MKPlayerPacketList;

typedef struct MKPlayerPacketQueue {
  MKMutex *mutex;
  MKCond *cond;
  AVFifo *packet_list;
  int abort_request;

  int serial;
  int size;
  int nb_packet;
  uint64_t duration;
} MKPlayerPacketQueue;

typedef struct MKPlayerFrameQueue {
  MKPlayerFrame queue[FRAME_QUEUE_SIZE];
  int size;
  int max_size;
  int keep_last;
  MKMutex *mutex;
  MKCond *cond;
  MKPlayerPacketQueue *pktq;
} MKPlayerFrameQueue;

typedef struct MKPlayerDecoder {
  AVPacket *packet;
  MKPlayerPacketQueue *queue;
  MKThread *decoder_thread;
  AVCodecContext *codec;
  MKCond *empty_queue_cond;
  uint64_t start_pts;
  int paquet_serial;
} MKPlayerDecoder;

typedef struct MKPlayerState {
  bool paused;

  bool stop_requested;
  MKMutex *stop_mutex;

  MKCond *continue_read_thread;
  MKThread *read_packet_thread;

  MKMediaPool media_pool;

  AVFormatContext *format;

  struct {
    bool has_stream;
    int stream_index;
    AVCodecParameters *params;
    MKPlayerFrameQueue frame_queue;
    MKPlayerPacketQueue packet_queue;
  } video;
} MKPlayerState;

extern void mk_player_setup(MKPlayerDesc *desc);

extern MKPlayerState *mk_player_alloc(void);

extern void mk_player_free(MKPlayerState *player);
#endif
