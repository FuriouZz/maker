package application

import GPU "../graphics"
import "base:intrinsics"
import "core:fmt"
import SDL "vendor:sdl3"
import "vendor:wgpu"
import "vendor:wgpu/sdl3glue"

OnReadyCallback :: #type proc(app: ^Application)
OnFrameCallback :: #type proc(app: ^Application, dt: f32)
OnResizeCallback :: #type proc(app: ^Application)
OnFinishCallback :: #type proc(app: ^Application)

Application :: struct {
    desc:   ApplicationDesc,
    window: ^SDL.Window,
    gpu:    GPU.Context,
}

ApplicationDesc :: struct {
    on_ready:  OnReadyCallback,
    on_frame:  OnFrameCallback,
    on_resize: OnResizeCallback,
    on_finish: OnFinishCallback,
}

init :: proc(
    $T: typeid,
    desc: ApplicationDesc,
) where intrinsics.type_is_subtype_of(T, Application) {
    app := cast(^Application)new(T)

    if !SDL.Init({.VIDEO}) {
        fmt.panicf("SDL.Init error: ", SDL.GetError())
    }

    app.desc = desc
    // app.desc.ready :=
    //     app.desc.on_ready if app.desc.on_ready != nil else OnReadyCallback(proc(app: ^Application) {})
    // app.desc.frame :=
    //     app.desc.on_frame if app.desc.on_frame != nil else OnFrameCallback(proc(app: ^Application, dt: f32) {})
    // app.desc.resize :=
    //     app.desc.on_resize if app.desc.on_resize != nil else OnResizeCallback(proc(app: ^Application) {})
    // app.desc.finish :=
    //     app.desc.on_finish if app.desc.on_finish != nil else OnFinishCallback(proc(app: ^Application) {})

    app.window = SDL.CreateWindow(
        "WGPU Native Triangle",
        800,
        600,
        {.RESIZABLE, .HIGH_PIXEL_DENSITY},
    )

    if app.window == nil {
        fmt.panicf("SDL.CreateWindow error: ", SDL.GetError())
    }

    instance := wgpu.CreateInstance(nil)
    if instance == nil {
        panic("[wgpu] not supported")
    }

    surface := sdl3glue.GetSurface(instance, app.window)
    GPU.context_init(&app.gpu, instance, surface, ready, app)

    ready :: proc(userdata: rawptr) {
        app := cast(^Application)userdata
        _run(app)
    }
}

get_size :: proc(app: ^Application) -> (u32, u32) {
    w, h: i32
    SDL.GetWindowSizeInPixels(app.window, &w, &h)
    return u32(w), u32(h)
}

@(private = "file")
_run :: proc(app: ^Application) {

    if app.desc.on_ready != nil {app.desc.on_ready(app)}

    now := SDL.GetPerformanceCounter()
    last: u64
    dt: f32
    main_loop: for {
        last = now
        now = SDL.GetPerformanceCounter()
        dt = f32((now - last) * 1000) / f32(SDL.GetPerformanceFrequency())

        e: SDL.Event
        for SDL.PollEvent(&e) {
            #partial switch (e.type) {
            case .QUIT:
                break main_loop
            case .WINDOW_RESIZED, .WINDOW_PIXEL_SIZE_CHANGED:
                w, h := get_size(app)
                GPU.context_resize_surface(&app.gpu, w, h)
                if app.desc.on_resize != nil {app.desc.on_resize(app)}
            }
        }

        if app.desc.on_frame != nil {app.desc.on_frame(app, dt)}
    }

    if app.desc.on_finish != nil {app.desc.on_finish(app)}

    GPU.context_uninit(&app.gpu)

    SDL.DestroyWindow(app.window)
    SDL.Quit()
}

