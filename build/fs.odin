package build

import "base:runtime"
import "core:os"
import "core:strings"

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
    os.make_directory_all(dir)
    return nil
}

