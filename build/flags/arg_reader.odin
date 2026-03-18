package flags

import "core:io"

Args_Reader :: struct {
    index: i64,
    len:   i64,
    args:  []string,
}

reader_init :: proc(r: ^Args_Reader, args: []string) {
    r.args = args
    r.index = 0
    r.len = i64(len(args))
}

reader_get_arg :: proc(r: ^Args_Reader) -> (value: string, err: io.Error) {
    if r.index >= 0 && r.index < r.len {
        value = r.args[r.index]
        err = .None
    } else {
        err = .EOF
    }
    return value, err
}

reader_next_arg :: proc(r: ^Args_Reader) -> (string, io.Error) {
    if r.index + 1 > r.len {
        return "", .EOF
    }
    r.index += 1
    return reader_get_arg(r)
}

reader_is_done :: proc(r: ^Args_Reader) -> bool {
    return r.index == r.len
}

