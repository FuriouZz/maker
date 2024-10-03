#include "sokol_log.h"

#define MAKER_DEBUG
#include "../src/maker_decoder.h"

int main(int argc, char *argv[]) {
    (void)argc;
    (void)argv;
    mdecoder_setup(&(mdecoder_desc){.logger.func = slog_func});

    const mdecoder_media media = mdecoder_create_media(argv[1]);
    mdecoder_decode_media(&media);

    return 0;
}
