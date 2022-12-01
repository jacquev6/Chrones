#!/usr/bin/env bash
# Copyright 2020-2022 Laurent Cabaret
# Copyright 2020-2022 Vincent Jacques

set -o errexit

cd "$(dirname "${BASH_SOURCE[0]}")"


if ! diff -r builder build/builder >/dev/null 2>&1 || ! diff Makefile build/Makefile >/dev/null 2>&1
then
  rm -rf build
  mkdir build
  docker build --tag chrones-builder builder
  cp -r builder build
  cp Makefile build
fi


docker run \
  --rm --interactive --tty \
  --user $(id -u):$(id -g) `# Avoid creating files as 'root'` \
  --network none `# Ensure the repository is self-contained (except for the "docker build" phase)` \
  --volume "$PWD:/workdir" --workdir /workdir \
  chrones-builder \
    make "$@"
