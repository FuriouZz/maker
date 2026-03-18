#!/usr/bin/env sh

DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )

if [ ! -f "$DIR/_build" ]; then
  odin build $DIR/build -out:_build
fi

cd $DIR/packages/$1

shift 1

$DIR/_build $@
