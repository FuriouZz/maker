package maker

foreign import lib "maker.dylib"


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

Media :: struct {
	streams:      [3]i32,
	video_width:  u32,
	video_height: u32,
}

PixelFormat :: enum i32 {
	UNKNOWN = -1,
	YUV420P = 0,
	RGBA    = 1,
	RGB     = 2,
}

ImageData :: struct {
	buffer:      ^u8,
	buffer_size: i32,
	format:      PixelFormat,
	width:       u32,
	height:      u32,
	is_valid:    bool, /* boolean */
}

ImageDataDesc :: struct {
	format: PixelFormat,
	width:  u32,
	height: u32,
}

Decoder :: struct {}

DecoderOptions :: struct {
	use_threads:   bool,
	create_thread: proc "c" (task: proc "c" (^Decoder) -> Status, user_decoder: ^Decoder) -> Status,
}

@(default_calling_convention="c", link_prefix="maker_")
foreign lib {
	media_open           :: proc(url: cstring) -> ^Media ---
	media_free           :: proc(media: ^Media) ---
	image_data_alloc     :: proc(desc: ^ImageDataDesc) -> ^ImageData ---
	image_data_free      :: proc(data: ^ImageData) ---
	image_data_init      :: proc(data: ^ImageData, desc: ^ImageDataDesc) -> Status ---
	image_data_uninit    :: proc(data: ^ImageData) ---
	image_data_save_pgm  :: proc(target: ^ImageData, output: cstring) ---
	image_data_save_ppm  :: proc(target: ^ImageData, output: cstring) ---
	decoder_alloc        :: proc(url: cstring, options: ^DecoderOptions) -> ^Decoder ---
	decoder_free         :: proc(decoder: ^Decoder) ---
	decoder_start        :: proc(decoder: ^Decoder) -> Status ---
	decoder_stop         :: proc(decoder: ^Decoder) -> Status ---
	decoder_get_frame    :: proc(decoder: ^Decoder, target: ^ImageData) -> u32 ---
	decoder_seek         :: proc(decoder: ^Decoder, seconds: i32) -> Status ---
	decoder_demux        :: proc(user_decoder: ^Decoder) -> Status ---
	decoder_decode_video :: proc(user_decoder: ^Decoder) -> Status ---
}

