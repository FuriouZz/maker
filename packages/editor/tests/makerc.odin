package tests

import "base:runtime"
import "core:fmt"
import "core:testing"
import "core:thread"
import "core:time"
import "maker:maker"

pool: thread.Pool

TaskData :: struct {
	job:     proc "c" (_: ^maker.Decoder) -> maker.Status,
	decoder: ^maker.Decoder,
}

@(test)
decode_media_test :: proc(t: ^testing.T) {
	USE_THREADS :: true


	create_task :: proc "c" (
		job: proc "c" (_: ^maker.Decoder) -> maker.Status,
		decoder: ^maker.Decoder,
	) {
		context = runtime.default_context()

		task := new(TaskData)
		task.job = job
		task.decoder = decoder

		thread.pool_add_task(&pool, context.allocator, proc(thread_task: thread.Task) {
				t: ^TaskData = (^TaskData)(thread_task.data)
				defer free(t)
				t.job(t.decoder)
			}, task)
	}

	thread.pool_init(&pool, context.allocator, 2)
	defer thread.pool_destroy(&pool)

	thread.pool_start(&pool)
	defer thread.pool_join(&pool)

	input := cstring("../makerc/tests/video.mp4")
	output := cstring("image.ppm")

	media := maker.media_open(input)
	defer maker.media_free(media)

	fmt.println("Size =", media.video_width, "x", media.video_height)

	image := maker.video_frame_alloc(
		{width = media.video_width, height = media.video_height, format = .RGBA},
	)
	defer maker.video_frame_free(image)

	decoder := maker.decoder_alloc(input, {use_threads = USE_THREADS, thread_cb = create_task})
	defer maker.decoder_free(decoder)

	maker.decoder_start(decoder)
	defer maker.decoder_stop(decoder)

	if USE_THREADS {time.sleep(time.Second * 2)}

	maker.decoder_get_video_frame(decoder, image)
	maker.video_frame_save_ppm(image, output)
}

