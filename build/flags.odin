package build

import "base:runtime"
import "core:flags"
import "core:strings"
import "core:testing"

@(private = "file")
DEFAULT_FLAG_VALUE :: "true"

Parsed_Args :: struct {
    flags: map[string]string,
    args:  [dynamic]string,
}

parse_args :: proc(
    args: []string,
    allocator := context.allocator,
) -> Parsed_Args {
    parsed := Parsed_Args {
        flags = make(map[string]string, allocator),
        args  = make([dynamic]string, allocator),
    }

    i := -1
    count := len(args)
    for {
        i = i + 1
        if i >= count {break}

        if strings.starts_with(args[i], "-") {
            arg := args[i][1:]

            if strings.starts_with(arg, "-") {
                append(&parsed.args, args[i])
                continue
            }

            colon_index := strings.index_byte(arg, ':')
            if colon_index != -1 {
                map_insert(
                    &parsed.flags,
                    arg[:colon_index],
                    arg[colon_index + 1:],
                )
                continue
            }

            if i + 1 < count && !strings.starts_with(args[i + 1], "-") {
                map_insert(&parsed.flags, arg, args[i + 1])
                i = i + 1
                continue
            } else {
                map_insert(&parsed.flags, args[i][1:], DEFAULT_FLAG_VALUE)
            }

            continue
        }

        append(&parsed.args, args[i])
    }

    return parsed
}

dispose_parsed_args :: proc(args: Parsed_Args) {
    delete(args.flags)
    delete(args.args)
}

@(test)
test_parse_args :: proc(t: ^testing.T) {
    args := parse_args(
        {"run", "something", "-verbose", "-profile:debug", "--what-is-that?"},
        context.temp_allocator,
    )
    defer free_all(context.temp_allocator)

    testing.expect(t, args.flags["verbose"] == "true")
    testing.expect(t, args.flags["profile"] == "debug")
    testing.expect(t, args.args[0] == "run")
    testing.expect(t, args.args[1] == "something")
    testing.expect(t, args.args[2] == "--what-is-that?")
}

