#!/bin/bash

# Copyright 2020-2022 Laurent Cabaret
# Copyright 2020-2022 Vincent Jacques

set -o errexit
cd "$(dirname "${BASH_SOURCE[0]}")/"


image=$(docker build --build-arg UID=$(id -u) development --quiet)

docker run \
  --rm --interactive --tty \
  --volume "$PWD:/wd" --workdir /wd \
  --gpus all \
  --pid=host `# Let nvidia-smi see processes (https://github.com/NVIDIA/nvidia-docker/issues/179#issuecomment-597701603)` \
  $image \
    python3 development/cycle.py "$@"
