#!/bin/bash

# Copyright 2020-2022 Laurent Cabaret
# Copyright 2020-2022 Vincent Jacques

set -o errexit
cd "$(dirname "${BASH_SOURCE[0]}")/"


docker build --build-arg UID=$(id -u) development --tag chrones-development

docker run \
  --rm --interactive --tty \
  --volume "$PWD:/wd" --workdir /wd \
  --gpus all \
  --pid=host `# Let nvidia-smi see processes (https://github.com/NVIDIA/nvidia-docker/issues/179#issuecomment-597701603)` \
  chrones-development \
    python3 development/cycle.py "$@"
