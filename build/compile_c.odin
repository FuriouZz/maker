package build

import "base:runtime"
import "core:fmt"
import "core:os"
import "core:slice"
import "core:strings"

C_Build_Mode :: enum {
    External,
    Executable,
    SharedLibrary,
    StaticLibrary,
    Object,
}

C_Target :: struct {
    name:        string,
    definitions: []string,
    sources:     []string,
    flags:       []string,
    libraries:   []string,
}

C_Artifact :: struct {
    name:             string,
    type:             C_Build_Mode,
    filename:         string,
    output_dir:       string,
    definition_paths: []string,
}

C_Compiler :: struct {
    cwd:       string,
    command:   string,
    targets:   map[string]C_Target,
    artifacts: map[string]C_Artifact,
}

init_c_compiler :: proc(compiler: ^C_Compiler) {
    cwd, _ := os.get_working_directory(context.allocator)
    compiler.cwd = cwd
    compiler.command = "gcc"
}

uninit_c_compiler :: proc(compiler: ^C_Compiler) {
    delete(compiler.cwd)
    delete(compiler.targets)
    delete(compiler.artifacts)
}

add_c_target :: proc(ctx: ^C_Compiler, target: C_Target) {
    ctx.targets[target.name] = target
}

add_c_artifact :: proc(ctx: ^C_Compiler, artifact: C_Artifact) {
    ctx.artifacts[artifact.name] = artifact
}

get_c_target :: proc(
    ctx: ^C_Compiler,
    name: string,
) -> (
    target: ^C_Target,
    ok: bool,
) {
    return &ctx.targets[name]
}

get_c_artifact :: proc(
    ctx: ^C_Compiler,
    name: string,
) -> (
    artifact: ^C_Artifact,
    ok: bool,
) {
    return &ctx.artifacts[name]
}

compile_c_artifact :: proc(
    ctx: ^C_Compiler,
    artifact_name: string,
    target_name: string,
) -> os.Error {
    if artifact, ok := get_c_artifact(ctx, artifact_name); ok {
        if target, ok := get_c_target(ctx, target_name); ok {
            return compile_target_with_artifact(ctx, target, artifact)
        }
    }
    return .Not_Exist
}

execute_c_artifact :: proc(
    ctx: ^C_Compiler,
    artifact_name: string,
) -> os.Error {
    if artifact, ok := get_c_artifact(ctx, artifact_name); ok {
        return try_exec(
            fmt.tprintf("%s/%s", artifact.output_dir, artifact.filename),
        )
    }
    return .Not_Exist
}

@(private = "file")
add_definition_paths :: proc(cmd: ^[dynamic]string, artifact: ^C_Artifact) {
    for path in artifact.definition_paths {
        inc := fmt.tprintf("-I%s", path)
        if !slice.contains(cmd[:], inc) {
            append(cmd, inc)
        }
    }
}

@(private = "file")
add_target_paths :: proc(cmd: ^[dynamic]string, artifact: ^C_Artifact) {
    inc := fmt.tprintf("-L%s", artifact.output_dir)
    if !slice.contains(cmd[:], inc) {
        append(cmd, inc)
    }
}

@(private = "file")
compile_target_with_artifact :: proc(
    ctx: ^C_Compiler,
    target: ^C_Target,
    artifact: ^C_Artifact,
) -> os.Error {
    if artifact.type == .External {
        return nil
    }

    ensure_dir(artifact.output_dir)

    cmd := make([dynamic]string)
    defer delete(cmd)

    if artifact.type == .StaticLibrary {
        append(&cmd, "ar", "rcs")
        append(
            &cmd,
            fmt.tprintf("%s/%s", artifact.output_dir, artifact.filename),
        )

        for input in target.sources {
            output, _ := strings.replace(input, ".c", ".o", 1)
            output = os.join_path(
                {artifact.output_dir, output},
                context.temp_allocator,
            ) or_return

            output_dir, _ := os.split_path(output)
            ensure_dir(output_dir)

            cmd2: [dynamic]string
            append(&cmd2, "cc")
            append(&cmd2, ..target.flags[:])
            for name in target.libraries {
                dep := get_c_artifact(ctx, name) or_continue
                add_definition_paths(&cmd2, dep)
            }
            add_definition_paths(&cmd2, artifact)
            append(&cmd2, "-c", input, "-o", output)

            try_exec(cmd2[:]) or_return

            append(&cmd, output)
        }

        try_exec(cmd[:]) or_return
        return nil
    }

    if len(ctx.command) == 0 {
        append(&cmd, "cc")
    } else {
        append(&cmd, ctx.command)
    }

    if artifact.type == .SharedLibrary {
        append(&cmd, "-shared")
        append(
            &cmd,
            fmt.tprintf("-Wl,-install_name,@rpath/%s", artifact.filename),
        )
    } else if artifact.type == .Object {
        append(&cmd, "-c")
    }

    append(&cmd, ..target.flags[:])
    add_definition_paths(&cmd, artifact)

    for name in target.libraries {
        dep := get_c_artifact(ctx, name) or_continue
        add_definition_paths(&cmd, dep)

        if artifact.type == .Executable || artifact.type == .SharedLibrary {
            add_target_paths(&cmd, dep)
            append(&cmd, fmt.tprintf("-l%s", dep.name))
        }

        if dep.type == .StaticLibrary {
            for name in target.libraries {
            }
        }
    }

    append(
        &cmd,
        "-o",
        fmt.tprintf("%s/%s", artifact.output_dir, artifact.filename),
    )
    append(&cmd, ..target.sources[:])

    try_exec(cmd[:]) or_return

    return nil
}

