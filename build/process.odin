package build

import "core:fmt"
import "core:os"
import "core:strings"

@(private = "package")
Exec_Options :: struct {
    verbose:     bool,
    working_dir: string,
}

exec_string :: proc(command: string, options: Exec_Options = {}) -> os.Error {
    args := strings.split(command, " ")
    defer delete(args)
    return exec_string_list(args, options)
}

exec_string_list :: proc(
    command: []string,
    options: Exec_Options = {},
) -> os.Error {
    if options.verbose || VERBOSE {
        cmd := strings.join(command, " ") or_return
        defer delete(cmd)
        fmt.println(cmd)
    }

    ps := os.process_start(
        {
            command = command,
            working_dir = options.working_dir,
            stdout = os.stdout,
            stderr = os.stderr,
            stdin = os.stdin,
        },
    ) or_return

    state := os.process_wait(ps) or_return

    return nil
}

exec :: proc {
    exec_string,
    exec_string_list,
}

