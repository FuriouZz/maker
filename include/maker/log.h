#ifndef MK_LOG_H
#define MK_LOG_H

#include <stdint.h>
#define MK_LOGITEMS                                                                                  \
    MK_LOGITEM(OK, "Ok")                                                                             \
    MK_LOGITEM(AVFORMAT_ALLOC_CONTEXT_FAILED, "Could not allocate memory for AVFormatContext")       \
    MK_LOGITEM(AVFORMAT_OPEN_INPUT_FAILED, "Could not open file")                                    \
    MK_LOGITEM(AVFORMAT_FIND_STREAM_INFO_FAILED, "Could not open file")                              \
    MK_LOGITEM(AVCODEC_FIND_CODEC_FAILED, "Could not find codec")                                    \
    MK_LOGITEM(AVCODEC_ALLOC_CONTEXT_FAILED, "Could not allocate memory for AVCodecContext")         \
    MK_LOGITEM(AVCODEC_OPEN_CODEC_FAILED, "Could not open codec")                                    \
    MK_LOGITEM(AVUTIL_FRAME_ALLOC_FAILED, "Could not allocate memory for AVFrame")                   \
    MK_LOGITEM(AVUTIL_PACKET_ALLOC_FAILED, "Could not allocate memory for AVPacket")                 \
    MK_LOGITEM(AVCODEC_SEND_PACKET_FAILED, "Error while sending a packet to the decoder")            \
    MK_LOGITEM(AVCODEC_RECEIVE_FRAME_FAILED, "Error while receiving a frame from the decoder")       \
    MK_LOGITEM(AVCODEC_COPY_PARAM_TO_CONTEXT_FAILED, "Failed to copy codec params to codec context") \
    MK_LOGITEM(AV_IMAGE_COPY_TO_BUFFER_FAILED, "Failed to copy AVFrame to buffer")

#define MK_LOGITEM(item, msg) MK_LOGITEM_##item,
typedef enum mk_logitem { MK_LOGITEMS } mk_logitem;
#undef MK_LOGITEM

#define MK_LOG_PANIC(code) mk_log(MK_LOGITEM_##code, 0, 0, __LINE__)
#define MK_LOG_ERROR(code) mk_log(MK_LOGITEM_##code, 1, 0, __LINE__)
#define MK_LOG_ERRORMSG(code, msg) mk_log(MK_LOGITEM_##code, 1, msg, __LINE__)
#define MK_LOG_WARN(code) mk_log(MK_LOGITEM_##code, 2, 0, __LINE__)
#define MK_LOG_WARNMSG(code, msg) mk_log(MK_LOGITEM_##code, 2, msg, __LINE__)
#define MK_LOG_INFO(code) mk_log(MK_LOGITEM_##code, 3, 0, __LINE__)
#define MK_LOG_INFOMSG(code, msg) mk_log(MK_LOGITEM_##code, 3, msg, __LINE__)

extern void mk_log(mk_logitem log_item, uint32_t log_level, const char* msg, uint32_t line_nr);

#endif
