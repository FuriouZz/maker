package build

import flags "./flags"
import "core:fmt"
import "core:os"
import "core:strings"

VERBOSE :: #config(VERBOSE, false)

Context :: struct {
    commands:  [dynamic]Command,
    cli:       flags.Parsed_Args,
    user_data: rawptr,
}

Command_Callback :: proc(ctx: Context) -> os.Error

Command :: struct {
    name:     string,
    callback: Command_Callback,
}

init_context :: proc(ctx: ^Context) {
    ctx.commands = make([dynamic]Command)
}

dispose_context :: proc(ctx: Context) {
    delete(ctx.commands)
}

@(private = "file")
_add_command_with_proc :: proc(
    ctx: ^Context,
    callback: Command_Callback,
    name := #caller_expression(callback),
) {
    if strings.starts_with(name, "proc(") {
        panic(
            "Cannot accept anonymous procedure, use add_command(&ctx, name, procedure)",
        )
    }
    _add_command_with_name_and_proc(ctx, name, callback)
}

_add_command_with_name_and_proc :: proc(
    ctx: ^Context,
    name: string,
    callback: Command_Callback,
) {
    append(&ctx.commands, Command{name = name, callback = callback})
}

add_command :: proc {
    _add_command_with_name_and_proc,
    _add_command_with_proc,
}

run_context :: proc(ctx: Context) {
    ctx := ctx // enable mutation
    ctx.cli = flags.parse_args(os.args[1:])
    defer flags.dispose_parsed_args(ctx.cli)

    if len(ctx.cli.args) == 0 {
        fmt.println("command is missing")
        return
    }

    for command in ctx.commands {
        if ctx.cli.args[0] == command.name {
            err := command.callback(ctx)
            if err != nil {
                fmt.panicf("%#v", err)
            }
            break
        }
    }
}

