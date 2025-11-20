package build_exe

import "../../build"

main :: proc() {
	build.VERBOSE = true
	build.exec({"odin", "test", "./tests", "-collection:maker=vendors", "-debug"})
}

