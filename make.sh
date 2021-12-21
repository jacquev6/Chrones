#!/usr/bin/env bash
# Copyright 2020-2021 Laurent Cabaret
# Copyright 2020-2021 Vincent Jacques

set -o errexit

cd "$(dirname "${BASH_SOURCE[0]}")"


# One can export variables CHRONES_SKIP_* before running this script to skip some parts.
# Useful to spare some time on repeated runs.

if [[ -z $CHRONES_SKIP_BUILDER ]]
then
  docker build builder --tag chrones-builder
fi

docker run \
  --rm --interactive --tty \
  --user $(id -u):$(id -g) `# Avoid creating files as 'root'` \
  --network none `# Ensure the repository is self-contained (except for the "docker build" phase)` \
  --volume "$PWD:/wd" --workdir /wd \
  chrones-builder \
    make "$@"
