#include "sokol_log.h"

#define MAKER_DEBUG
#include "../src/maker_player.h"

int main(int argc, char *argv[]) {
    (void)argc;
    (void)argv;
    mplayer_setup(&(mplayer_desc){.logger.func = slog_func});

    const mplayer_media media = mplayer_open_file(argv[1]);
    mplayer_decode_media(&media);

    return 0;
}
