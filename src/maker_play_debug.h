#ifndef MAKER_PLAY_DEBUG_H
#define MAKER_PLAY_DEBUG_H
#include "stdio.h"

#include "libavformat/avformat.h"
#include "libavutil/avutil.h"

#include "maker_internal.h"

_MAKER_PRIVATE void _mk_play_debug_stream(AVStream *stream) {
  fprintf(
      stdout, "AVStream->time_base before open coded %d/%d\n",
      stream->time_base.num, stream->time_base.den
  );
  fprintf(
      stdout, "AVStream->r_frame_rate before open coded %d/%d\n",
      stream->r_frame_rate.num, stream->r_frame_rate.den
  );
  fprintf(stdout, "AVStream->start_time %lld\n", stream->start_time);
  fprintf(stdout, "AVStream->duration %lld\n", stream->duration);
}

_MAKER_PRIVATE void
_mk_play_debug_codec(const AVCodec *codec, AVStream *stream) {
  if (codec == NULL) {
    fprintf(stderr, "Couldn't find codec info");
    exit(1);
  }

  if (codec->type == AVMEDIA_TYPE_VIDEO) {
    AVCodecParameters *codec_parameters = stream->codecpar;
    fprintf(
        stdout, "Video Codec: resolution %d x %d\n", codec_parameters->width,
        codec_parameters->height
    );
  }
}
#endif
