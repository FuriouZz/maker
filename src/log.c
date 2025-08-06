#include "internal.h"
#include "maker/log.h"
#include "maker/utils_str.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#if defined(MK_DEBUG)
#define MK_LOGITEM(item, msg) #item ": " msg,
_MK_PRIVATE const char* mk_log_messages[] = { MK_LOGITEMS };
#undef MK_LOGITEM
#endif

typedef struct MKLogger {
    void (*func)(
        const char* tag, // always "mk"
        uint32_t log_level, // 0=panic, 1=error, 2=warning, 3=info
        uint32_t log_item_id, // mk_LOGITEM_*
        const char* message_or_null, // a message string, may be
                                     // nullptr in release mode
        uint32_t line_nr, // line number in video.h
        const char* filename_or_null, // source filename, may be
                                      // nullptr in release mode
        void* user_data
    );
    void* user_data;
} MKLogger;

// from
// https://github.com/floooh/sokol/blob/570be17908dd019faa1c28e29208490b2172f521/sokol_log.h#L251
_MK_PRIVATE void _mk_default_logger(
    const char* tag, uint32_t log_level, uint32_t log_item_id,
    const char* message_or_null,

    uint32_t line_nr, const char* filename_or_null,

    void* user_data
)
{
    (void)(log_item_id);
    (void)(line_nr);
    (void)(user_data);

    char str_buf[512];
    char* str = str_buf;
    char* end = str_buf + sizeof(str_buf);
    char num_buf[32];

    if (tag) {
        str = mk_str_concat("[", str, end);
        str = mk_str_concat(tag, str, end);
        str = mk_str_concat("]", str, end);
    }

    const char* log_level_str;
    switch (log_level) {
    case 0:
        log_level_str = "panic";
        break;
    case 1:
        log_level_str = "error";
        break;
    case 2:
        log_level_str = "warning";
        break;
    default:
        log_level_str = "info";
        break;
    }
    str = mk_str_concat("[", str, end);
    str = mk_str_concat(log_level_str, str, end);
    str = mk_str_concat("]", str, end);

    str = mk_str_concat("[id:", str, end);
    str = mk_str_concat(
        mk_str_from_int(log_item_id, num_buf, sizeof(num_buf)), str, end
    );
    str = mk_str_concat("]", str, end);

    if (filename_or_null) {
        str = mk_str_concat(" ", str, end);
        str = mk_str_concat(filename_or_null, str, end);
        str = mk_str_concat(":", str, end);
        str = mk_str_concat(
            mk_str_from_int(line_nr, num_buf, sizeof(num_buf)), str, end
        );
        str = mk_str_concat(":0: ", str, end);
    } else {
        str = mk_str_concat("[line:", str, end);
        str = mk_str_concat(
            mk_str_from_int(line_nr, num_buf, sizeof(num_buf)), str, end
        );
        str = mk_str_concat("] ", str, end);
    }

    if (message_or_null) {
        str = mk_str_concat("\n\t", str, end);
        str = mk_str_concat(message_or_null, str, end);
    }

    str = mk_str_concat("\n\n", str, end);

    if (0 == log_level) {
        str = mk_str_concat("ABORTING because of [panic]\n", str, end);
        (void)str;
    }

    fputs(str_buf, stderr);

    if (0 == log_level) {
        abort();
    }
}

typedef struct MKLoggerGlobal {
    MKLogger logger;
} MKLoggerGlobal;

MKLoggerGlobal mk_logger_global = { .logger.func = _mk_default_logger };

void mk_log(
    mk_logitem log_item, uint32_t log_level, const char* msg, uint32_t line_nr
)
{
    if (mk_logger_global.logger.func) {
        const char* filename = 0;
#if defined(MK_DEBUG)
        filename = __FILE__;
        if (0 == msg) {
            msg = mk_log_messages[log_item];
        }
#endif
        mk_logger_global.logger.func(
            "mk", log_level, log_item, msg, line_nr, filename,
            mk_logger_global.logger.user_data
        );
    } else {
        // for log level PANIC it would be 'undefined behaviour' to continue
        if (log_level == 0) {
            abort();
        }
    }
}
