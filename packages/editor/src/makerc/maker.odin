package makerc

foreign import lib "../../libs/libmaker.dylib"


MAKER_VERSION_MAJOR :: 0
MAKER_VERSION_MINOR :: 0
MAKER_VERSION_PATCH :: 1
MAKER_VERSION_EXTRA :: ""
MAKER_VERSION :: "0.0.1"

/* ---- typedefs ----
*/
Status :: enum i32 {
    ERROR = -1,
    OK    = 0,
}

TrackType :: enum u32 {
    VIDEO    = 0,
    AUDIO    = 1,
    SUBTITLE = 2,
    COUNT    = 3,
}

PixelFormat :: enum u32 {
    UNKNOWN = 0,
    YUV420P = 1,
    RGBA    = 2,
    RGB     = 3,
}

MediaInfo :: struct {
    streams:      [3]i32,
    video_width:  u32,
    video_height: u32,
    video_format: PixelFormat,
}

VideoFrameDesc :: struct {
    format: PixelFormat,
    width:  u32,
    height: u32,
}

VideoFrame :: struct {
    buffer:      ^u8,
    buffer_size: i32,
    format:      PixelFormat,
    width:       u32,
    height:      u32,
    is_valid:    bool, /* boolean */
}

Context :: struct {
    internal_state: rawptr,
    is_initialized: u8, /* boolean */
}

ContextDesc :: struct {
    thread_count:  u32,
    create_worker: proc "c" (
        task: proc "c" (decoder: rawptr) -> Status,
        decoder: rawptr,
    ),
}

Decoder :: struct {
    internal_state: rawptr,
    is_initialized: u8, /* boolean */
}

DecoderDesc :: struct {
    use_playback: u8,
    url:          cstring,
    _context:     ^Context,
    context_desc: ^ContextDesc,
}

@(default_calling_convention = "c", link_prefix = "maker_")
foreign lib {
    media_info_init :: proc(info: ^MediaInfo, url: cstring) -> Status ---
    media_info_uninit :: proc(info: ^MediaInfo) -> Status ---
    video_frame_init :: proc(data: ^VideoFrame, desc: ^VideoFrameDesc) -> Status ---
    video_frame_uninit :: proc(data: ^VideoFrame) -> Status ---
    video_frame_save_pgm :: proc(target: ^VideoFrame, output: cstring) -> Status ---
    video_frame_save_ppm :: proc(target: ^VideoFrame, output: cstring) -> Status ---
    decoder_init :: proc(decoder: ^Decoder, desc: ^DecoderDesc) -> Status ---
    decoder_uninit :: proc(decoder: ^Decoder) -> Status ---
    decoder_get_media_info :: proc(decoder: ^Decoder, info: ^MediaInfo) -> Status ---
    decoder_get_video_frame :: proc(decoder: ^Decoder, target: ^VideoFrame) -> u32 ---
    decoder_get_playback_time :: proc(decoder: ^Decoder, time_ms: ^u32) -> Status ---
    decoder_seek :: proc(decoder: ^Decoder, seconds: u64) -> Status ---
    context_init :: proc(_context: ^Context, desc: ^ContextDesc) -> Status ---
    context_uninit :: proc(_context: ^Context) -> Status ---
}

