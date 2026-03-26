package build_exe

import b "../../build"
import "base:runtime"
import "core:fmt"
import "core:os"

PROFILE :: #config(PROFILE, "debug")

when PROFILE == "release" {
    TARGET_DIR :: "target/release"
    CFLAGS :: []string {
        "-std=c99",
        "-Wall",
        "-Wextra",
        "-Werror",
        "-Wunused",
        "-pedantic",
        "-fPIC",
        "-O2",
    }
} else {
    TARGET_DIR :: "target/debug"
    CFLAGS :: []string {
        "-std=c99",
        "-Wall",
        "-Wextra",
        "-Werror",
        "-Wunused",
        "-pedantic",
        "-fPIC",
        "-g",
        "-O0",
        "-fsanitize=address",
    }
}

libavcodec :: b.C_Artifact {
    name             = "avcodec",
    target           = "libavcodec.dylib",
    target_dir       = "vendors/ffmpeg/build/lib",
    definition_paths = {"vendors/ffmpeg/build/include"},
}

libavdevice :: b.C_Artifact {
    name             = "avdevice",
    target           = "libavdevice.dylib",
    target_dir       = "vendors/ffmpeg/build/lib",
    definition_paths = {"vendors/ffmpeg/build/include"},
}

libavfilter :: b.C_Artifact {
    name             = "avfilter",
    target           = "libavfilter.dylib",
    target_dir       = "vendors/ffmpeg/build/lib",
    definition_paths = {"vendors/ffmpeg/build/include"},
}

libavformat :: b.C_Artifact {
    name             = "avformat",
    target           = "libavformat.dylib",
    target_dir       = "vendors/ffmpeg/build/lib",
    definition_paths = {"vendors/ffmpeg/build/include"},
}

libavutil :: b.C_Artifact {
    name             = "avutil",
    target           = "libavutil.dylib",
    target_dir       = "vendors/ffmpeg/build/lib",
    definition_paths = {"vendors/ffmpeg/build/include"},
}

libswresample :: b.C_Artifact {
    name             = "swresample",
    target           = "libswresample.dylib",
    target_dir       = "vendors/ffmpeg/build/lib",
    definition_paths = {"vendors/ffmpeg/build/include"},
}

libswscale :: b.C_Artifact {
    name             = "swscale",
    target           = "libswscale.dylib",
    target_dir       = "vendors/ffmpeg/build/lib",
    definition_paths = {"vendors/ffmpeg/build/include"},
}

artifact_libmaker :: b.C_Artifact {
    name             = "maker",
    type             = .SharedLibrary,
    target           = "libmaker.dylib",
    target_dir       = TARGET_DIR,
    definition_paths = {"src/include"},
}

artifact_test_media :: b.C_Artifact {
    name       = "media",
    type       = .Executable,
    target     = "media.bin",
    target_dir = TARGET_DIR,
}

artifact_test_decoder :: b.C_Artifact {
    name       = "decoder",
    type       = .Executable,
    target     = "decoder.bin",
    target_dir = TARGET_DIR,
}

artifact_test_custom_thread :: b.C_Artifact {
    name       = "custom_thread",
    type       = .Executable,
    target     = "custom_thread.bin",
    target_dir = TARGET_DIR,
}

// // Create static library
// libmaker_artifact := b.C_Artifact {
//     name             = "maker",
//     type             = .StaticLibrary,
//     target           = "libmaker.a",
//     target_dir       = TARGET_DIR,
//     definition_paths = {"src/include"},
// }

// // Use libmaker static library
// libtestmedia_target := b.C_Target {
//     name         = "test_media",
//     flags        = CFLAGS,
//     sources      = {"tests/media.c"},
//     dependencies = {
//         "maker",
//         "avcodec",
//         "avdevice",
//         "avfilter",
//         "avformat",
//         "avutil",
//         "swresample",
//         "swscale",
//     },
// }

target_maker :: b.C_Target {
    name      = "maker",
    flags     = CFLAGS,
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

target_test_media :: b.C_Target {
    name      = "media",
    flags     = CFLAGS,
    sources   = {"tests/media.c"},
    libraries = {"maker"},
}

target_test_decoder :: b.C_Target {
    name      = "decoder",
    flags     = CFLAGS,
    sources   = {"tests/decoder.c"},
    libraries = {"maker"},
}

target_test_custom_thread :: b.C_Target {
    name      = "custom_thread",
    flags     = CFLAGS,
    sources   = {"tests/custom_thread.c"},
    libraries = {"maker", "avcodec"},
}

main :: proc() {
    build_context: b.C_Build_Context
    build_context.command = "gcc"

    b.add_c_artifact(&build_context, libavcodec)
    b.add_c_artifact(&build_context, libavdevice)
    b.add_c_artifact(&build_context, libavfilter)
    b.add_c_artifact(&build_context, libavformat)
    b.add_c_artifact(&build_context, libavutil)
    b.add_c_artifact(&build_context, libswresample)
    b.add_c_artifact(&build_context, libswscale)
    b.add_c_artifact(&build_context, artifact_libmaker)
    b.add_c_artifact(&build_context, artifact_test_media)
    b.add_c_artifact(&build_context, artifact_test_decoder)
    b.add_c_artifact(&build_context, artifact_test_custom_thread)

    b.add_c_target(&build_context, target_maker)
    b.add_c_target(&build_context, target_test_media)
    b.add_c_target(&build_context, target_test_decoder)
    b.add_c_target(&build_context, target_test_custom_thread)

    ctx: b.Context
    b.init_context(&ctx)
    defer b.dispose_context(ctx)

    b.add_command(&ctx, install)
    b.add_command(&ctx, build)
    b.add_command(&ctx, bindgen)
    b.add_command(&ctx, bear)
    b.add_command(&ctx, test)

    ctx.user_data = &build_context
    b.run_context(ctx)
}

install :: proc(_: b.Context) {
    install_bindgen()
    install_ffmpeg()
}

build :: proc(ctx: b.Context) {
    build_ctx := cast(^b.C_Build_Context)ctx.user_data
    b.compile_c_target(build_ctx, target_maker.name, artifact_libmaker.name)
    b.compile_c_target(
        build_ctx,
        target_test_media.name,
        artifact_test_media.name,
    )
    b.compile_c_target(
        build_ctx,
        target_test_decoder.name,
        artifact_test_decoder.name,
    )
    b.compile_c_target(
        build_ctx,
        target_test_custom_thread.name,
        artifact_test_custom_thread.name,
    )
}

bindgen :: proc(ctx: b.Context) {
    build_ctx := cast(^b.C_Build_Context)ctx.user_data
    artifact, ok := b.get_c_artifact(build_ctx, "maker")
    if ok {
        err: os.Error

        catch_err :: proc(err: os.Error) {
            if err != nil {
                fmt.panicf("%#v", err)
            }
        }
        catch_err(
            b.exec("vendors/odin-c-bindgen/build/bin/bindgen bindgen.sjson"),
        )
        catch_err(b.ensure_dir("../editor/src/decoder"))
        catch_err(
            os.copy_file(
                fmt.tprintf(
                    "../editor/src/decoder/%s",
                    artifact_libmaker.target,
                ),
                fmt.tprintf(
                    "%s/%s",
                    artifact_libmaker.target_dir,
                    artifact_libmaker.target,
                ),
            ),
        )
    }
}

bear :: proc(_: b.Context) {
    b.exec("bear -- odin run build.odin -define:VERBOSE=true -file -- build")
}

test :: proc(ctx: b.Context) {
    build(ctx)

    build_ctx := cast(^b.C_Build_Context)ctx.user_data
    name := ctx.cli.flags["test"]

    err: os.Error
    err = b.compile_c_target(build_ctx, name, name)
    if err != nil {
        fmt.panicf("%#v", err)
    }

    err = b.execute_c_target(build_ctx, name)
    if err != nil {
        fmt.panicf("%#v", err)
    }
}

install_ffmpeg :: proc() -> os.Error {
    cwd, _ := os.get_working_directory(context.temp_allocator)
    defer free_all(context.temp_allocator)

    source_dir := fmt.tprintf("%s/vendors/ffmpeg/sources", cwd)
    build_dir := fmt.tprintf("%s/vendors/ffmpeg/build", cwd)
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
                    "./configure",
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
                {"make install"},
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
                    "--rev=807603709017926f9eaefaae8d4a8437b7a17a46",
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

