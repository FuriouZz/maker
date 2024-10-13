#ifndef MAKER_PLAYER_H
#define MAKER_PLAYER_H

#include <maker/maker_media.h>
#include <maker/maker_mutex.h>
#include <maker/maker_pool.h>
#include <maker/maker_thread.h>

#include <stdbool.h>
#include <stdint.h>

typedef struct MKPlayer {
  bool paused;

  bool stop_requested;
  MKMutex *stop_mutex;

  MKCond *continue_read_thread;
  MKThread *read_packet_thread;

  MKMediaPool media_pool;
} MKPlayer;

extern MKPlayer *mk_player_alloc(void);

#endif
