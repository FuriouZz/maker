package editor

import "base:runtime"
import "core:fmt"
import "core:thread"
import "core:time"
import "maker:maker"

USE_THREADS :: true

pool: thread.Pool

TaskData :: struct {
	job:     proc "c" (_: ^maker.Decoder) -> maker.Status,
	decoder: ^maker.Decoder,
}

create_task :: proc "c" (
	job: proc "c" (_: ^maker.Decoder) -> maker.Status,
	decoder: ^maker.Decoder,
) -> maker.Status {

	context = runtime.default_context()

	task := new(TaskData)
	task.job = job
	task.decoder = decoder

	thread.pool_add_task(&pool, context.allocator, proc(thread_task: thread.Task) {
			t: ^TaskData = (^TaskData)(thread_task.data)
			defer free(t)
			t.job(t.decoder)
		}, task)

	return .OK
}

main :: proc() {
	thread.pool_init(&pool, context.allocator, 2)
	thread.pool_start(&pool)

	input := cstring("../maker/tests/video.mp4")
	output := cstring("image.ppm")

	media := maker.media_open(input)
	defer maker.media_free(media)

	fmt.println("Size =", media.video_width, "x", media.video_height)

	image := maker.image_data_alloc(
		&{width = media.video_width, height = media.video_height, format = .RGBA},
	)
	defer maker.image_data_free(image)

	decoder := maker.decoder_alloc(
		input,
		&{use_threads = USE_THREADS, create_thread = create_task},
	)
	defer maker.decoder_free(decoder)

	maker.decoder_start(decoder)
	defer maker.decoder_stop(decoder)

	if USE_THREADS {time.sleep(time.Second * 2)}

	maker.decoder_get_frame(decoder, image)
	maker.image_data_save_ppm(image, output)
}

