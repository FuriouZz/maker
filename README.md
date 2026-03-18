# maker

`maker` is hobby project for learning C and [Odin](https://odin-lang.org/).

The goal is to create a simple video editor with graphics animation.

It is divided in two packages:
- `packages/makerc` provides functions to interact with ffmpeg easily
- `packages/editor` is a video editor using maker bindings

## How to use `build.odin`

`build.odin` is a script file where to all commands of the package are created

Run `build.odin`

```sh
cd packages/makerc
odin run ./build.odin -file -- [command]
```

Build then run `./build`

```sh
cd packages/makerc
odin build ./build.odin -file
./build [command]
```
