#include "maker/maker.h"
#include <stdio.h>
#include <unistd.h>

int main(void)
{

    // Current solution
    MKContext*      context = mk_context_init();
    MKMediaHandle   media   = mk_context_open_input(context, "./tests/video.mp4");
    MKDecoderHandle decoder = mk_context_create_decoder(context, &media);

    // // Decoding part
    mk_context_start_decoding(context);
    mk_context_get_video_frame(context, &decoder);

    // sleep(5);

    mk_context_stop_decoding(context);
    mk_context_drop_decoder(context, &decoder);
    mk_context_uninit(context);

    return 0;
}
