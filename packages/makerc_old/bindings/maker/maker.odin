package makerc

foreign import lib "libmaker.dylib"


MAKER_VERSION_MAJOR :: 0
MAKER_VERSION_MINOR :: 0
MAKER_VERSION_PATCH :: 1
MAKER_VERSION_EXTRA :: ""
MAKER_VERSION       :: "0.0.1"

/* ---- typedefs ----
*/
Status :: enum i32 {
	ERROR = -1,
	OK    = 0,
	BUSY  = 1,
}

TrackType :: enum u32 {
	VIDEO    = 0,
	AUDIO    = 1,
	SUBTITLE = 2,
	COUNT    = 3,
}

PixelFormat :: enum i32 {
	UNKNOWN = -1,
	YUV420P = 0,
	RGBA    = 1,
	RGB     = 2,
}

Media :: struct {
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

Decoder :: struct {}

DecoderDesc :: struct {
	use_playback: u8,
	use_threads:  bool,
	thread_cb:    proc "c" (task: proc "c" (decoder: ^Decoder) -> Status, decoder: ^Decoder),
}

@(default_calling_convention="c", link_prefix="maker_")
foreign lib {
	media_open                :: proc(url: cstring) -> ^Media ---
	media_free                :: proc(media: ^Media) ---
	video_frame_alloc         :: proc(desc: VideoFrameDesc) -> ^VideoFrame ---
	video_frame_free          :: proc(data: ^VideoFrame) ---
	video_frame_init          :: proc(data: ^VideoFrame, desc: VideoFrameDesc) -> Status ---
	video_frame_uninit        :: proc(data: ^VideoFrame) ---
	video_frame_save_pgm      :: proc(target: ^VideoFrame, output: cstring) -> Status ---
	video_frame_save_ppm      :: proc(target: ^VideoFrame, output: cstring) -> Status ---
	decoder_alloc             :: proc(url: cstring, desc: DecoderDesc) -> ^Decoder ---
	decoder_free              :: proc(decoder: ^Decoder) ---
	decoder_start             :: proc(decoder: ^Decoder) -> Status ---
	decoder_stop              :: proc(decoder: ^Decoder) -> Status ---
	decoder_get_video_frame   :: proc(decoder: ^Decoder, target: ^VideoFrame) -> u32 ---
	decoder_get_playback_time :: proc(decoder: ^Decoder, time_ms: ^u32) -> Status ---
	decoder_seek              :: proc(decoder: ^Decoder, seconds: u64) -> Status ---
	decoder_demux             :: proc(user_decoder: ^Decoder) -> Status ---
	decoder_decode_video      :: proc(user_decoder: ^Decoder) -> Status ---
}

