package build_exe

import b "../../build"
import "base:runtime"
import "core:fmt"
import "core:mem"
import "core:os"

PROFILE :: #config(PROFILE, "debug")
TARGET_DIR :: "target/" + PROFILE

libavcodec := b.C_Artifact {
    name             = "avcodec",
    filename         = "libavcodec.62.dylib",
    output_dir       = "vendors/ffmpeg/build/lib",
    definition_paths = {"vendors/ffmpeg/build/include"},
}

libavdevice := b.C_Artifact {
    name             = "avdevice",
    filename         = "libavdevice.62.dylib",
    output_dir       = "vendors/ffmpeg/build/lib",
    definition_paths = {"vendors/ffmpeg/build/include"},
}

libavfilter := b.C_Artifact {
    name             = "avfilter",
    filename         = "libavfilter.11.dylib",
    output_dir       = "vendors/ffmpeg/build/lib",
    definition_paths = {"vendors/ffmpeg/build/include"},
}

libavformat := b.C_Artifact {
    name             = "avformat",
    filename         = "libavformat.62.dylib",
    output_dir       = "vendors/ffmpeg/build/lib",
    definition_paths = {"vendors/ffmpeg/build/include"},
}

libavutil := b.C_Artifact {
    name             = "avutil",
    filename         = "libavutil.60.dylib",
    output_dir       = "vendors/ffmpeg/build/lib",
    definition_paths = {"vendors/ffmpeg/build/include"},
}

libswresample := b.C_Artifact {
    name             = "swresample",
    filename         = "libswresample.6.dylib",
    output_dir       = "vendors/ffmpeg/build/lib",
    definition_paths = {"vendors/ffmpeg/build/include"},
}

libswscale := b.C_Artifact {
    name             = "swscale",
    filename         = "libswscale.9.dylib",
    output_dir       = "vendors/ffmpeg/build/lib",
    definition_paths = {"vendors/ffmpeg/build/include"},
}

artifact_libmaker := b.C_Artifact {
    name             = "maker",
    type             = .SharedLibrary,
    filename         = "libmaker.dylib",
    output_dir       = TARGET_DIR,
    definition_paths = {"src/include"},
}

artifact_test_media := b.C_Artifact {
    name       = "test_media",
    type       = .Executable,
    filename   = "test_media.bin",
    output_dir = TARGET_DIR,
}

artifact_test_decoder := b.C_Artifact {
    name       = "test_decoder",
    type       = .Executable,
    filename   = "test_decoder.bin",
    output_dir = TARGET_DIR,
}

artifact_test_custom_thread := b.C_Artifact {
    name       = "test_custom_thread",
    type       = .Executable,
    filename   = "test_custom_thread.bin",
    output_dir = TARGET_DIR,
}

target_maker := b.C_Target {
    name      = "maker",
    sources   = {
        "src/core/clock.c",
        "src/core/context.c",
        "src/core/decoder.c",
        "src/core/demuxer.c",
        "src/core/error.c",
        "src/core/frame_queue.c",
        "src/core/video_frame.c",
        "src/core/media.c",
        "src/core/media_info.c",
        "src/core/packet_queue.c",
        "src/core/pixel_format.c",
        "src/core/thread.c",
        "src/core/thread_pool.c",
        "src/core/util.c",
        "src/core/video_converter.c",
        "src/core/video_decoder.c",
        "src/core/wait_group.c",
    },
    libraries = {
        "avcodec",
        "avdevice",
        "avfilter",
        "avformat",
        "avutil",
        "swresample",
        "swscale",
    },
}

target_test_media := b.C_Target {
    name      = "test_media",
    sources   = {"tests/media.c"},
    libraries = {"maker"},
}

target_test_decoder := b.C_Target {
    name      = "test_decoder",
    sources   = {"tests/decoder.c"},
    libraries = {"maker"},
}

target_test_custom_thread := b.C_Target {
    name      = "test_custom_thread",
    sources   = {"tests/custom_thread.c"},
    libraries = {"maker", "avcodec"},
}

install :: proc(ctx: b.Context) -> os.Error {
    install_bindgen() or_return
    install_ffmpeg(ctx) or_return
    return nil
}

build :: proc(ctx: b.Context) -> os.Error {
    cc := cast(^b.C_Compiler)ctx.user_data

    if target, ok := b.get_c_target(cc, "maker"); ok {
        if artifact, ok := b.get_c_artifact(cc, "maker"); ok {
            b.compile_c_artifact(cc, artifact.name, target.name) or_return

            path := fmt.tprintf(
                "%s/%s",
                artifact.output_dir,
                artifact.filename,
            )

            // Remove symbols
            if PROFILE == "release" {
                command := fmt.tprintf("strip -x -S %s", path)
                b.try_exec(command) or_return
            }

            // Change LD_LOAD_DYLIB
            for key in target.libraries {
                if lib, ok := b.get_c_artifact(cc, key); ok {
                    input := fmt.tprintf("%s/%s", lib.output_dir, lib.filename)
                    output := fmt.tprintf(
                        "%s/%s%s",
                        artifact.output_dir,
                        os.short_stem(input),
                        os.ext(input),
                    )

                    os.copy_file(output, input) or_return

                    command := fmt.tprintf(
                        "install_name_tool -change %s/%s @rpath/%s%s %s",
                        cc.cwd,
                        input,
                        os.short_stem(input),
                        os.ext(input),
                        path,
                    )
                    b.try_exec(command) or_return
                }
            }
        }
    }

    return nil
}

test :: proc(ctx: b.Context) -> os.Error {
    build(ctx)

    cc := cast(^b.C_Compiler)ctx.user_data
    name := fmt.tprintf("test_%s", ctx.cli.flags["test"])

    b.compile_c_artifact(cc, name, name) or_return
    b.execute_c_artifact(cc, name) or_return

    return nil
}

bindgen :: proc(ctx: b.Context) -> os.Error {
    cc := cast(^b.C_Compiler)ctx.user_data

    if artifact, ok := b.get_c_artifact(cc, "maker"); ok {
        b.try_exec(
            "vendors/odin-c-bindgen/build/bin/bindgen bindgen.sjson",
        ) or_return
    }

    return nil
}

export :: proc(rctx: b.Context) -> os.Error {
    build(rctx) or_return

    os.mkdir_all("../editor/libs")

    dir := os.open(TARGET_DIR) or_return
    it := os.read_directory_iterator_create(dir)

    for entry in os.read_directory_iterator(&it) {
        if entry.type != .Regular {continue}
        if os.ext(entry.fullpath) != ".dylib" {continue}
        os.copy_file(
            fmt.tprintf("../editor/libs/%s", entry.name),
            entry.fullpath,
        ) or_return
    }

    return nil
}

bear :: proc(_: b.Context) -> os.Error {
    return b.try_exec(
        "bear -- odin run build.odin -define:VERBOSE=true -file -- build",
    )
}

install_ffmpeg :: proc(ctx: b.Context) -> os.Error {
    cc := cast(^b.C_Compiler)ctx.user_data
    source_dir := fmt.tprintf("%s/vendors/ffmpeg/sources", cc.cwd)
    build_dir := fmt.tprintf("%s/vendors/ffmpeg/build", cc.cwd)
    return b.add_dependency(
        {
            name = "ffmpeg",
            install_commands = {
                {"rm", "-rf", "vendors/ffmpeg"},
                {
                    "git",
                    "clone",
                    "https://git.ffmpeg.org/ffmpeg.git",
                    "--depth=1",
                    "--branch=n8.0",
                    "vendors/ffmpeg/sources",
                },
            },
            build_dir = source_dir,
            build_commands = {
                {
                    fmt.tprintf("%s/configure", source_dir),
                    fmt.tprintf("--prefix=\"%s\"", build_dir),
                    "--disable-programs",
                    "--disable-static",
                    "--disable-doc",
                    "--enable-shared",
                    "--enable-pic",
                    "--enable-cross-compile",
                    "--enable-swscale",
                    "--enable-debug=2",
                },
                {"make"},
                {"make", "install"},
            },
        },
    )
}

install_bindgen :: proc() -> os.Error {
    return b.add_dependency(
        {
            name = "odin-c-bindgen",
            install_commands = {
                {"rm", "-rf", "vendors/odin-c-bindgen"},
                {
                    "git",
                    "clone",
                    "git@github.com:karl-zylinski/odin-c-bindgen.git",
                    "--depth=1",
                    "--rev=408a6f4e3c35a17e4517dc374c5c7edd19081e9f",
                    "vendors/odin-c-bindgen/sources",
                },
                {"mkdir", "-p", "vendors/odin-c-bindgen/build/bin"},
            },
            build_dir = "vendors/odin-c-bindgen/sources",
            build_commands = {
                {"odin", "build", "src", "-out:../build/bin/bindgen"},
            },
        },
    )
}

add_cflags :: proc(
    mode: b.C_Build_Mode,
    allocator := context.temp_allocator,
) -> []string {
    cflags := make([dynamic]string, allocator)

    append(
        &cflags,
        "-std=c99",
        "-Wall",
        "-Wextra",
        "-Werror",
        "-Wunused",
        "-g",
        "-O2",
    )

    when PROFILE == "debug" {
        append(
            &cflags,
            "-fsanitize=address",
            // "-fsanitize=memory",
            // "-fsanitize=thread",
            "-DMAKER_DEBUG",
        )
    }


    #partial switch mode {
    case .SharedLibrary:
        append(&cflags, "-fvisibility=hidden", "-fPIC", "-pedantic")
    case .Executable:
        append(&cflags, "-Wl,-rpath,@executable_path")
    }

    return cflags[:]
}

main :: proc() {
    track: mem.Tracking_Allocator
    mem.tracking_allocator_init(&track, context.allocator)
    context.allocator = mem.tracking_allocator(&track)
    defer {
        if len(track.allocation_map) > 0 {
            fmt.eprintf(
                "=== %v allocations not freed: ===\n",
                len(track.allocation_map),
            )
            for _, entry in track.allocation_map {
                fmt.eprintf("- %v bytes @ %v\n", entry.size, entry.location)
            }
        }
        mem.tracking_allocator_destroy(&track)
    }

    cc: b.C_Compiler
    b.init_c_compiler(&cc)
    defer b.uninit_c_compiler(&cc)

    b.add_c_artifact(&cc, libavcodec)
    b.add_c_artifact(&cc, libavdevice)
    b.add_c_artifact(&cc, libavfilter)
    b.add_c_artifact(&cc, libavformat)
    b.add_c_artifact(&cc, libavutil)
    b.add_c_artifact(&cc, libswresample)
    b.add_c_artifact(&cc, libswscale)

    b.add_c_artifact(&cc, artifact_libmaker)
    b.add_c_artifact(&cc, artifact_test_media)
    b.add_c_artifact(&cc, artifact_test_decoder)
    b.add_c_artifact(&cc, artifact_test_custom_thread)

    target_maker.flags = add_cflags(.SharedLibrary)
    target_test_media.flags = add_cflags(.Executable)
    target_test_decoder.flags = add_cflags(.Executable)
    target_test_custom_thread.flags = add_cflags(.Executable)

    b.add_c_target(&cc, target_maker)
    b.add_c_target(&cc, target_test_media)
    b.add_c_target(&cc, target_test_decoder)
    b.add_c_target(&cc, target_test_custom_thread)

    ctx: b.Context
    b.init_context(&ctx)
    defer b.dispose_context(ctx)

    b.add_command(&ctx, install)
    b.add_command(&ctx, build)
    b.add_command(&ctx, export)
    b.add_command(&ctx, bindgen)
    b.add_command(&ctx, bear)
    b.add_command(&ctx, test)

    ctx.user_data = &cc
    b.run_context(ctx)
}

