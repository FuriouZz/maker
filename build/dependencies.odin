package build

import "core:os"

Dependency :: struct {
	name:             string,
	install_dir:      string,
	build_dir:        string,
	install_commands: [][]string,
	build_commands:   [][]string,
}

add_dependency :: proc(desc: Dependency) -> os.Error {
	for cmd in desc.install_commands {
		exec(cmd[:], desc.install_dir) or_return
	}

	for cmd in desc.build_commands {
		exec(cmd[:], desc.build_dir) or_return
	}

	return nil
}

