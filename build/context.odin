package build

import "core:fmt"
import "core:os"

VERBOSE :: #config(VERBOSE, false)

Context :: struct {
    commands:  [dynamic]Command,
    cli:       Parsed_Args,
    user_data: rawptr,
}

Command_Callback :: proc(ctx: Context)

Command :: struct {
    command:  string,
    callback: Command_Callback,
}

init_context :: proc() -> Context {
    return {commands = make([dynamic]Command)}
}

dispose_context :: proc(ctx: Context) {
    delete(ctx.commands)
}

add_command :: proc(
    ctx: ^Context,
    command: string,
    callback: Command_Callback,
) {
    append(&ctx.commands, Command{command = command, callback = callback})
}

run_context :: proc(ctx: Context) {
    ctx := ctx // enable mutation
    ctx.cli = parse_args(os.args[1:])
    defer dispose_parsed_args(ctx.cli)

    if len(ctx.cli.args) == 0 {
        fmt.println("command is missing")
        return
    }

    for command in ctx.commands {
        if ctx.cli.args[0] == command.command {
            command.callback(ctx)
        }
    }
}

