#ifndef ERROR_H
#define ERROR_H

#include <stdint.h>

#ifdef MK_DEBUG
#define MK_FILE __FILE__
#else
#define MK_FILE 0
#endif

#define MK_LOG(message) mk_log(0, message, __LINE__, MK_FILE)
#define MK_INFO(message) mk_log(1, message, __LINE__, MK_FILE)
#define MK_WARN(message) mk_log(2, message, __LINE__, MK_FILE)
#define MK_ERROR(message) mk_log(3, message, __LINE__, MK_FILE)
#define MK_PANIC(message) mk_log(4, message, __LINE__, MK_FILE)

extern void mk_log(uint32_t code, char* message, uint32_t line, char* filename);

#endif
