package avformat

import "core:c"

when ODIN_OS == .Darwin {
	foreign import avformat "../../../vendors/ffmpeg/build/lib/libavformat.dylib"
}

Context :: struct {}

InputFormat :: struct {}

Dictionary :: struct {}

Codec :: struct {}

MediaType :: enum c.int {
	Unknown = -1,
	Video,
	Audio,
	Data,
	Subtitle,
	Attachment,
	TypeCount,
}

@(default_calling_convention = "c")
foreign avformat {

}

@(default_calling_convention = "c", link_prefix = "avformat_")
foreign avformat {
	alloc_context :: proc() -> ^Context ---
	free_context :: proc(ctx: ^Context) ---
	open_input :: proc(ps: ^^Context, url: cstring, fmt: ^InputFormat, options: ^Dictionary) -> c.int ---
	find_stream_info :: proc(ctx: ^Context, options: ^Dictionary) -> c.int ---
}

@(default_calling_convention = "c", link_prefix = "av_")
foreign avformat {
	find_best_stream :: proc(ctx: ^Context, type: MediaType, wanted_stream_nb: c.int, related_stream: c.int, decoder_ret: ^^Codec, flags: c.int) -> c.int ---
}

