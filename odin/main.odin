package main

import "core:fmt"
import "core:strings"
import "ffmpeg:avformat"

Media :: struct {
	filename: string,
	format:   ^avformat.Context,
	streams:  [avformat.MediaType.TypeCount]i32,
}

main :: proc() {
	media, err := open(`./tests/video.mp4`)
	defer close(&media)
	fmt.println(media, err)
}

open :: proc(filename: cstring) -> (Media, i32) {
	media: Media
	media.filename = string(filename)

	ctx := avformat.alloc_context()
	ret: i32

	ret = avformat.open_input(&ctx, filename, nil, nil)
	if ret != 0 {
		return {}, ret
	}

	ret = avformat.find_stream_info(ctx, nil)
	if ret != 0 {
		return {}, ret
	}


	for i in 0 ..< 5 {
		media.streams[i] = -1
	}

	media.streams[avformat.MediaType.Video] = avformat.find_best_stream(
		ctx,
		.Video,
		-1,
		-1,
		nil,
		0,
	)
	media.streams[avformat.MediaType.Audio] = avformat.find_best_stream(
		ctx,
		.Audio,
		-1,
		media.streams[avformat.MediaType.Video],
		nil,
		0,
	)

	return media, 0
}

close :: proc(media: ^Media) {
	avformat.free_context(media.format)
}

