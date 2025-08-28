#include "error.h"
#include "util.h"
#include <_abort.h>
#include <stdio.h>
#include <string.h>

void mk_log(uint32_t code, char* message, uint32_t line, char* filename)
{
    char* code_string = NULL;

    if (code == 1) {
        code_string = "info";
    } else if (code == 2) {
        code_string = "warn";
    } else if (code == 3) {
        code_string = "error";
    } else if (code == 4) {
        code_string = "panic";
    } else {
        code_string = "log";
    }

    if (filename) {
        fprintf(stderr, "[mk][%s][%s:%d:0] %s\n", code_string, filename, line, message);
    } else {
        fprintf(stderr, "[mk][%s][line:%d] %s\n", code_string, line, message);
    }

    if (code == 4) {
        abort();
    }
}
