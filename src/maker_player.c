#include "libavcodec/packet.h"
#include "libavutil/frame.h"
#include <maker/maker_player.h>

void mk_player_open_media(MKPlayer *player, MKMedia *media) {
  int err;

  AVFormatContext *format_context = avformat_alloc_context();

  if (!format_context) {
    fprintf(stderr, "Failed to create AVFormatContext\n");
  }

  err = avformat_open_input(&format_context, media->filename, NULL, NULL);
  if (err < 0) {
    fprintf(stderr, "Failed to open file. Cause: %s\n", av_err2str(err));
    goto fail;
  }

  err = avformat_find_stream_info(format_context, NULL);
  if (err < 0) {
    fprintf(stderr, "Failed to find stream info. Cause: %s\n", av_err2str(err));
    goto fail;
  }

  int video_stream = -1;
  int audio_stream = -1;

  for (unsigned int i = 0; i < format_context->nb_streams; i++) {
    AVStream *stream = format_context->streams[i];
    if (stream->codecpar->codec_type == AVMEDIA_TYPE_VIDEO)
      video_stream = i;
    if (stream->codecpar->codec_type == AVMEDIA_TYPE_AUDIO)
      audio_stream = i;
  }

  if (video_stream == -1 && audio_stream == -1) {
    fprintf(stderr, "No video or audio stream found\n");
    goto fail;
  }

  if (video_stream > -1) {
    AVCodecParameters *video_codec_params =
        format_context->streams[video_stream]->codecpar;

    const AVCodec *video_codec =
        avcodec_find_decoder(video_codec_params->codec_id);
    if (!video_codec) {
      fprintf(stderr, "Failed to find video codec\n");
      goto fail;
    }

    AVCodecContext *video_codec_context = avcodec_alloc_context3(video_codec);
    if (!video_codec_context) {
      fprintf(stderr, "Failed to allocate a video codec context\n");
      goto fail;
    }

    err =
        avcodec_parameters_to_context(video_codec_context, video_codec_params);
    if (err < 0) {
      fprintf(
          stderr,
          "Failed to transfer codec parameters to codec context. Cause: %s\n",
          av_err2str(err)
      );
      goto video_fail;
    }

    err = avcodec_open2(video_codec_context, video_codec, NULL);
    if (err < 0) {
      fprintf(
          stderr, "Failed to initialize codec context. Cause: %s\n",
          av_err2str(err)
      );
      goto video_fail;
    }

    goto video_success;

  video_fail:
    avcodec_free_context(&video_codec_context);
    goto fail;

  video_success:
    player->video.width = video_codec_context->width;
    player->video.height = video_codec_context->height;
    player->video.codec = video_codec_context;
    player->video.stream_index = video_stream;
    player->video.has_stream = true;
    media->video.width = player->video.width;
    media->video.height = player->video.height;
  }

  if (audio_stream > -1) {
    AVCodecParameters *audio_codec_params =
        format_context->streams[audio_stream]->codecpar;

    const AVCodec *audio_codec =
        avcodec_find_decoder(audio_codec_params->codec_id);
    if (!audio_codec) {
      fprintf(stderr, "Failed to find audio codec\n");
      goto fail;
    }

    AVCodecContext *audio_codec_context = avcodec_alloc_context3(audio_codec);
    if (!audio_codec_context) {
      fprintf(stderr, "Failed to allocate a audio codec context\n");
      goto fail;
    }

    err =
        avcodec_parameters_to_context(audio_codec_context, audio_codec_params);
    if (err < 0) {
      fprintf(
          stderr,
          "Failed to transfer codec parameters to codec context. Cause: %s\n",
          av_err2str(err)
      );
      goto audio_fail;
    }

    err = avcodec_open2(audio_codec_context, audio_codec, NULL);
    if (err < 0) {
      fprintf(
          stderr, "Failed to initialize codec context. Cause: %s\n",
          av_err2str(err)
      );
      goto audio_fail;
    }

    goto audio_success;

  audio_fail:
    avcodec_free_context(&audio_codec_context);
    goto fail;

  audio_success:
    player->audio.sample_rate = audio_codec_context->sample_rate;
  }

  goto success;

fail:
  avformat_free_context(format_context);

success:
  player->format_context = format_context;
  player->decoder.yuv_frame = av_frame_alloc();
}

int mk_player_decode_video(MKPlayer *player, AVPacket *packet) {
  int err;
  AVCodecContext *video_codec_context = player->video.codec;
  AVFrame *frame = player->decoder.yuv_frame;

  err = avcodec_send_packet(video_codec_context, packet);
  if (err < 0) {
    fprintf(stderr, "Failed to send packet. Cause: %s\n", av_err2str(err));
    return -1;
  }

  err = avcodec_receive_frame(video_codec_context, frame);

  if (err == 0) {
    return 0;
  }

  if (err == AVERROR(EAGAIN) || err == AVERROR_EOF) {
    return err;
  }

  fprintf(
      stderr, "Error while receiving a frame from the decoder. Cause: %s\n",
      av_err2str(err)
  );

  return -1;
}

int mk_player_decode(MKPlayer *player) {
  int err;
  AVPacket *packet = av_packet_alloc();
  AVFormatContext *format_context = player->format_context;
  MKPlayerVideoStream *video = &player->video;

  for (;;) {
    err = av_read_frame(format_context, packet);
    if (err < 0) {
      goto fail;
    }

    if (packet->stream_index == video->stream_index) {
      err = mk_player_decode_video(player, packet);

      if (err == AVERROR(EAGAIN) || err == AVERROR_EOF) {
        goto success;
      }
    }
  }

fail:
  av_packet_free(&packet);
  return -1;

success:
  av_packet_free(&packet);
  return 0;
}

int mk_player_decode_one(MKPlayer *player) {
  int err;
  AVPacket *packet = av_packet_alloc();
  AVFormatContext *format_context = player->format_context;
  MKPlayerVideoStream *video = &player->video;

  for (;;) {
    err = av_read_frame(format_context, packet);
    if (err < 0) {
      goto fail;
    }

    if (packet->stream_index == video->stream_index) {
      err = mk_player_decode_video(player, packet);

      if (err == 0) {
        goto success;
      }

      if (err == AVERROR(EAGAIN) || err == AVERROR_EOF) {
        goto success;
      }
    }
  }

fail:
  av_packet_free(&packet);
  return -1;

success:
  av_packet_free(&packet);
  return 0;
}

void mk_player_create_image_data(
    MKPlayer *player, MKPlayerImageData *image_data
) {
  const int format = AV_PIX_FMT_RGBA;
  AVCodecContext *codec_context = player->video.codec;
  struct SwsContext *sws_context = sws_getContext(
      codec_context->width, codec_context->height, codec_context->pix_fmt,
      codec_context->width, codec_context->height, format, SWS_BILINEAR, NULL,
      NULL, NULL
  );

  int size = av_image_get_buffer_size(
      format, player->video.width, player->video.height, 1
  );
  image_data->frame = av_frame_alloc();
  image_data->buffer = malloc(size * sizeof(uint8_t));
  image_data->buffer_size = size;
  image_data->width = player->video.width;
  image_data->height = player->video.height;
  image_data->format = format;
  image_data->sws_context = sws_context;
}

void mk_player_free_image_data(MKPlayerImageData *data) {
  free(data->buffer);
  av_frame_free(&data->frame);
  sws_freeContext(data->sws_context);
  data->buffer_size = 0;
  data->width = -1;
  data->height = -1;
  data->format = -1;
}

int mk_player_get_pixel(MKPlayer *player, MKPlayerImageData *data) {
  int err;
  AVCodecContext *codec_context = player->video.codec;
  MKPlayerDecoder *decoder = &player->decoder;
  struct SwsContext *sws_context = data->sws_context;
  AVFrame *src_frame = decoder->yuv_frame;
  AVFrame *dst_frame = data->frame;

  if (codec_context->width != data->width ||
      codec_context->height != data->height) {
    fprintf(
        stderr, "MKPlayerImageData does not match AVCodecContext dimensions\n"
    );
    return -1;
  }

  err = av_image_alloc(
      dst_frame->data, dst_frame->linesize, codec_context->width,
      codec_context->height, data->format, 1
  );

  if (err < 0) {
    fprintf(stderr, "Failed to allocate image. Cause: %s\n", av_err2str(err));
    return -1;
  }

  sws_scale(
      sws_context, (const uint8_t *const *)src_frame->data, src_frame->linesize,
      0, codec_context->height, dst_frame->data, dst_frame->linesize
  );

  err = av_image_copy_to_buffer(
      data->buffer, data->buffer_size, (const uint8_t *const *)dst_frame->data,
      dst_frame->linesize, data->format, data->width, data->height, 1
  );

  if (err < 0) {
    fprintf(
        stderr, "Failed to copy image to buffer. Cause: %s\n", av_err2str(err)
    );
    return -1;
  }

  return 0;
}
