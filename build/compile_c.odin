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
    target:           string,
    target_dir:       string,
    definition_paths: []string,
}

C_Build_Context :: struct {
    command:   string,
    targets:   map[string]C_Target,
    artifacts: map[string]C_Artifact,
}

add_c_target :: proc(ctx: ^C_Build_Context, target: C_Target) {
    ctx.targets[target.name] = target
}

add_c_artifact :: proc(ctx: ^C_Build_Context, artifact: C_Artifact) {
    ctx.artifacts[artifact.name] = artifact
}

get_c_target :: proc(
    ctx: ^C_Build_Context,
    name: string,
) -> (
    target: ^C_Target,
    ok: bool,
) {
    return &ctx.targets[name]
}

get_c_artifact :: proc(
    ctx: ^C_Build_Context,
    name: string,
) -> (
    artifact: ^C_Artifact,
    ok: bool,
) {
    return &ctx.artifacts[name]
}

compile_c_target :: proc(
    ctx: ^C_Build_Context,
    target_name: string,
    artifact_name: string,
) -> os.Error {
    target, t_ok := get_c_target(ctx, target_name)
    artifact, a_ok := get_c_artifact(ctx, artifact_name)
    if t_ok && a_ok {
        compile_target_with_artifact(ctx, target, artifact) or_return
    }
    return nil
}

execute_c_target :: proc(
    ctx: ^C_Build_Context,
    artifact_name: string,
) -> os.Error {
    artifact, ok := get_c_artifact(ctx, artifact_name)
    if !ok {
        return .Not_Exist
    }
    command := fmt.tprintf("%s/%s", artifact.target_dir, artifact.target)
    return exec(command)
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
    inc := fmt.tprintf("-L%s", artifact.target_dir)
    if !slice.contains(cmd[:], inc) {
        append(cmd, inc)
    }
}

add_c_dependency_artifact :: proc(
    cmd: ^[dynamic]string,
    target: ^C_Artifact,
    dep: ^C_Artifact,
) {
    add_definition_paths(cmd, dep)

    if target.type == .Executable || target.type == .SharedLibrary {
        append(cmd, fmt.tprintf("-l%s", dep.name))
        add_target_paths(cmd, dep)
    }
}

@(private = "file")
compile_target_with_artifact :: proc(
    ctx: ^C_Build_Context,
    target: ^C_Target,
    artifact: ^C_Artifact,
) -> os.Error {
    if artifact.type == .External {
        return nil
    }

    ensure_dir(artifact.target_dir)

    cmd: [dynamic]string

    if artifact.type == .StaticLibrary {
        append(&cmd, "ar", "rcs")
        append(
            &cmd,
            fmt.tprintf("%s/%s", artifact.target_dir, artifact.target),
        )

        for input in target.sources {
            output, _ := strings.replace(input, ".c", ".o", 1)
            output = os.join_path(
                {artifact.target_dir, output},
                context.temp_allocator,
            ) or_return

            output_dir, _ := os.split_path(output)
            ensure_dir(output_dir)

            cmd2: [dynamic]string
            append(&cmd2, "cc")
            append(&cmd2, ..target.flags)
            for name in target.libraries {
                dep := get_c_artifact(ctx, name) or_continue
                add_definition_paths(&cmd2, dep)
            }
            add_definition_paths(&cmd2, artifact)
            append(&cmd2, "-c", input, "-o", output)

            exec(cmd2[:]) or_return

            append(&cmd, output)
        }

        exec(cmd[:]) or_return
        return nil
    }

    if len(ctx.command) == 0 {
        append(&cmd, "cc")
    } else {
        append(&cmd, ctx.command)
    }

    if artifact.type == .SharedLibrary {
        append(&cmd, "-shared")
    } else if artifact.type == .Object {
        append(&cmd, "-c")
    }

    append(&cmd, ..target.flags)
    add_definition_paths(&cmd, artifact)

    for name in target.libraries {
        dep := get_c_artifact(ctx, name) or_continue
        add_definition_paths(&cmd, dep)

        if artifact.type == .Executable || artifact.type == .SharedLibrary {
            append(&cmd, fmt.tprintf("-l%s", dep.name))
            add_target_paths(&cmd, dep)
        }

        if dep.type == .StaticLibrary {
            for name in target.libraries {
            }
        }
    }

    append(
        &cmd,
        "-o",
        fmt.tprintf("%s/%s", artifact.target_dir, artifact.target),
    )
    append(&cmd, ..target.sources)

    exec(cmd[:]) or_return

    return nil
}

