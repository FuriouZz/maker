package build

import "core:fmt"
import "core:os"
import "core:strings"

@(private = "package")
Exec_Options :: struct {
    verbose:        bool,
    working_dir:    string,
    panic_on_error: bool,
}

@(private = "file")
_catch_err :: proc(err: os.Error, panic_on_error: bool) {
    if err != nil {
        if panic_on_error {
            fmt.panicf("%#v", err)
        } else {
            fmt.eprintf("%#v", err)
        }
    }
}

try_exec_string :: proc(
    command: string,
    options: Exec_Options = {},
) -> os.Error {
    args := strings.split(command, " ")
    defer delete(args)
    return try_exec_string_list(args, options)
}

try_exec_string_list :: proc(
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

exec_string :: proc(
    command: string,
    options: Exec_Options = {panic_on_error = true},
) {
    err := try_exec_string(command, options)
    if err != nil {
        if (options.panic_on_error) {
            fmt.panicf("%#v", err)
        } else {
            fmt.eprintf("%#v", err)
        }
    }
}

exec_string_list :: proc(
    command: []string,
    options: Exec_Options = {panic_on_error = true},
) {
    err := try_exec_string_list(command, options)
    if err != nil {
        if (options.panic_on_error) {
            fmt.panicf("%#v", err)
        } else {
            fmt.eprintf("%#v", err)
        }
    }
}

try_exec :: proc {
    try_exec_string,
    try_exec_string_list,
}

exec :: proc {
    exec_string,
    exec_string_list,
}

