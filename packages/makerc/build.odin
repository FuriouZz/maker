package build_exe

import "../../build"
import "core:flags"
import "core:fmt"
import "core:os"
import "core:strings"

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

// // Create static library
// libmaker_artifact := build.C_Artifact {
//     name             = "maker",
//     type             = .StaticLibrary,
//     target           = "libmaker.a",
//     target_dir       = TARGET_DIR,
//     definition_paths = {"src/include"},
// }

// // Use libmaker static library
// libtestmedia_target := build.C_Target {
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

main :: proc() {
    ctx: build.C_Context
    ctx.command = "gcc"

    build.add_c_artifact(
        &ctx,
        {
            name = "avcodec",
            target = "libavcodec.dylib",
            target_dir = "vendors/ffmpeg/build/lib",
            definition_paths = {"vendors/ffmpeg/build/include"},
        },
    )
    build.add_c_artifact(
        &ctx,
        {
            name = "avdevice",
            target = "libavdevice.dylib",
            target_dir = "vendors/ffmpeg/build/lib",
            definition_paths = {"vendors/ffmpeg/build/include"},
        },
    )
    build.add_c_artifact(
        &ctx,
        {
            name = "avfilter",
            target = "libavfilter.dylib",
            target_dir = "vendors/ffmpeg/build/lib",
            definition_paths = {"vendors/ffmpeg/build/include"},
        },
    )
    build.add_c_artifact(
        &ctx,
        {
            name = "avformat",
            target = "libavformat.dylib",
            target_dir = "vendors/ffmpeg/build/lib",
            definition_paths = {"vendors/ffmpeg/build/include"},
        },
    )
    build.add_c_artifact(
        &ctx,
        {
            name = "avutil",
            target = "libavutil.dylib",
            target_dir = "vendors/ffmpeg/build/lib",
            definition_paths = {"vendors/ffmpeg/build/include"},
        },
    )
    build.add_c_artifact(
        &ctx,
        {
            name = "swresample",
            target = "libswresample.dylib",
            target_dir = "vendors/ffmpeg/build/lib",
            definition_paths = {"vendors/ffmpeg/build/include"},
        },
    )
    build.add_c_artifact(
        &ctx,
        {
            name = "swscale",
            target = "libswscale.dylib",
            target_dir = "vendors/ffmpeg/build/lib",
            definition_paths = {"vendors/ffmpeg/build/include"},
        },
    )
    build.add_c_artifact(
        &ctx,
        {
            name = "maker",
            type = .SharedLibrary,
            target = "libmaker.dylib",
            target_dir = TARGET_DIR,
            definition_paths = {"src/include"},
        },
    )
    build.add_c_artifact(
        &ctx,
        {
            name = "test_media",
            type = .Executable,
            target = "test_media.bin",
            target_dir = TARGET_DIR,
        },
    )

    build.add_c_target(
        &ctx,
        {
            name = "maker",
            flags = CFLAGS,
            sources = {
                "src/core/clock.c",
                "src/core/decoder.c",
                "src/core/demuxer.c",
                "src/core/error.c",
                "src/core/frame_queue.c",
                "src/core/video_frame.c",
                "src/core/media.c",
                "src/core/packet_queue.c",
                "src/core/pixel_format.c",
                "src/core/thread.c",
                "src/core/thread_pool.c",
                "src/core/util.c",
                "src/core/video_converter.c",
                "src/core/video_decoder.c",
            },
            dependencies = {
                "avcodec",
                "avdevice",
                "avfilter",
                "avformat",
                "avutil",
                "swresample",
                "swscale",
            },
        },
    )
    build.add_c_target(
        &ctx,
        {
            name = "test_media",
            flags = CFLAGS,
            sources = {"tests/media.c"},
            dependencies = {"maker"},
        },
    )

    parse_args(&ctx)
}

parse_args :: proc(ctx: ^build.C_Context) -> os.Error {
    Commands :: enum {
        help,
        build,
        build_test,
        test,
        install,
        bear,
        bindgen,
    }

    Options :: struct {
        command:  Commands `args:"pos=0" usage:"Command to run"`,
        overflow: [dynamic]string `usage:"Any extra arguments go here."`,
    }

    Test_Options :: struct {
        name: string `args:"pos=0,required" usage:"Name of the test to run"`,
    }

    opts: Options
    flags.parse_or_exit(&opts, os.args, .Odin)

    test_opts: Test_Options
    if opts.command == .test {
        err := flags.parse(&test_opts, opts.overflow[:], .Odin)
        flags.print_errors(Test_Options, err, "", .Odin)
    }

    switch opts.command {
    case .build:
        build.compile_c_target(ctx, "maker", "maker") or_return

    case .build_test:
        build.compile_c_target(ctx, "test_media", "test_media") or_return

    case .install:
        install_bindgen()
        install_ffmpeg()

    case .bindgen:
        build.exec(
            {"vendors/odin-c-bindgen/build/bin/bindgen", "bindgen.sjson"},
        )
        build.exec(
            {
                "cp",
                fmt.tprintf("target/%s/libmaker.dylib", PROFILE),
                "../editor/vendors/maker/libmaker.dylib",
            },
        )

    case .bear:
        build.exec({"bear", "--", "../../_build", "build"})

    case .test:
        build.compile_c_target(ctx, test_opts.name, test_opts.name) or_return
        build.execute_c_target(ctx, test_opts.name) or_return

    case .help:
        b := strings.builder_make()

        commands: [][]string = {
            {"bear", "Generate compile_commands"},
            {"bindgen", "Generate odin bindings"},
            {"build", "Build libmaker.dylib"},
            {"build:test", "Build tests"},
            {"install", "Install dependencies (ffmpeg, odin-c-bindgen)"},
            {"test", "Run test"},
        }

        for command in commands {
            strings.write_string(
                &b,
                strings.left_justify(command[0], 10, " ") or_return,
            )
            strings.write_string(&b, "  ")
            strings.write_string(&b, command[1])
            strings.write_string(&b, "\n")
        }

        fmt.print(strings.to_string(b))

    }

    return nil
}

install_ffmpeg :: proc() -> os.Error {
    cwd, _ := os.get_working_directory(context.temp_allocator)
    defer free_all(context.temp_allocator)

    source_dir := fmt.tprintf("%s/vendors/ffmpeg/sources", cwd)
    build_dir := fmt.tprintf("%s/vendors/ffmpeg/build", cwd)
    return build.add_dependency(
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
    return build.add_dependency(
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

