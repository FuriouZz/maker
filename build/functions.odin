package build

import "base:runtime"
import "core:c/libc"
import "core:fmt"
import os "core:os/os2"
import "core:strings"

VERBOSE :: #config(VERBOSE, false)

get_sources :: proc(
	dir: string,
	allocator := context.allocator,
) -> (
	files: [dynamic]string,
	err: os.Error,
) {
	cwd := os.get_working_directory(allocator) or_return

	w := os.walker_create(dir)
	for entry in os.walker_walk(&w) {
		if entry.type != .Regular {continue}
		if !strings.ends_with(entry.name, ".c") {continue}

		input := os.get_relative_path(cwd, entry.fullpath, allocator) or_return
		append_elem(&files, input)
	}

	return files, nil
}

ensure_dir :: proc(dir: string) -> os.Error {
	exec({"mkdir", "-p", dir}) or_return
	return nil
}

exec :: proc(command: []string, cwd: string = "") -> os.Error {
	when VERBOSE {
		cmd := strings.join(command, " ") or_return
		fmt.println(cmd)
		delete(cmd)
	}

	ps := os.process_start(
		{
			command = command,
			working_dir = cwd,
			stdout = os.stdout,
			stderr = os.stderr,
			stdin = os.stdin,
		},
	) or_return

	state := os.process_wait(ps) or_return
	os.process_close(ps) or_return

	return nil
}

parse_args :: proc(
	args: []string,
	allocator: runtime.Allocator,
) -> (
	map[string]string,
	[dynamic]string,
) {
	context.allocator = allocator

	entries := make(map[string]string)
	positional: [dynamic]string

	i := -1
	count := len(args)
	for {
		i = i + 1
		if i >= count {break}

		if strings.starts_with(args[i], "--") {
			arg, _ := strings.replace(args[i], "-", "", 2)

			res, err := strings.split(arg, "=")

			if len(res) == 2 {
				map_insert(&entries, res[0], res[1])
				continue
			}

			if i + 1 < count && !strings.starts_with(args[i + 1], "-") {
				map_insert(&entries, arg, args[i + 1])
				i = i + 1
				continue
			}

			continue
		}

		if strings.starts_with(args[i], "-") {
			key, _ := strings.replace(args[i], "-", "", 1)
			map_insert(&entries, key, "true")
			continue
		}

		append(&positional, args[i])
	}

	return entries, positional
}

