package build

import "core:fmt"
import "core:os"
import "core:strings"

main :: proc() {
    if os.exists("build.odin") {
        command: [dynamic]string

        buf: [128]byte

        PROFILE := "debug"
        VERBOSE := false

        if profile, err := os.lookup_env_buf(buf[:], "PROFILE"); err == nil {
            if profile == "debug" || profile == "release" {
                PROFILE = strings.clone(profile)
            }
        }

        if verbose, err := os.lookup_env_buf(buf[:], "VERBOSE"); err == nil {
            VERBOSE = verbose == "1"
        }

        append_elems(&command, "odin", "run", "build.odin", "-file")
        append_elems(&command, fmt.tprintf("-define:PROFILE=%s", PROFILE))
        append_elems(&command, fmt.tprintf("-define:VERBOSE=%t", VERBOSE))
        append_elems(&command, "--")
        append(&command, ..os.args[1:])
        exec(command[:])
    }
}

