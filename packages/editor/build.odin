package build_exe

import b "../../build"
import "core:fmt"

PROFILE :: #config(PROFILE, "debug")
TARGET_DIR :: "target/" + PROFILE

main :: proc() {
    b.ensure_dir(TARGET_DIR)
    b.exec(
        fmt.tprintf(
            "odin test ./tests -out:%s/%s -debug -extra-linker-flags:-Wl,-rpath,libs -keep-executable",
            TARGET_DIR,
            "tests",
        ),
    )
}

