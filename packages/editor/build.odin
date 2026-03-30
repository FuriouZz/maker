package build_exe

import "../../build"

main :: proc() {
    build.exec("odin test ./tests -debug -extra-linker-flags:-Wl,-rpath,libs")
}

