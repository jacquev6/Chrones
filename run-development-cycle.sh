#!/bin/bash

# Copyright 2020-2022 Laurent Cabaret
# Copyright 2020-2022 Vincent Jacques

set -o errexit
cd "$(dirname "${BASH_SOURCE[0]}")/"


docker build --build-arg UID=$(id -u) development --tag chrones-development


if docker run --rm --gpus all nvidia/cuda:11.2.2-base-ubuntu20.04 nvidia-smi >/dev/null 2>&1
then
  # Option '--pid=host' lets nvidia-smi see processes
  # (https://github.com/NVIDIA/nvidia-docker/issues/179#issuecomment-597701603)
  gpu_arguments="--gpus all --pid=host --env CHRONES_DEV_USE_GPU=1"
else
  echo "************************************************************************"
  echo "** The NVidia Docker runtime does not seem to be properly configured. **"
  echo "** This machine may not even have a GPU. The cycle will proceed,      **"
  echo "** but TESTS USING A GPU WILL BE SKIPPED.                             **"
  echo "************************************************************************"
  gpu_arguments=""
fi


docker run \
  --rm --interactive --tty \
  --volume "$PWD:/wd" --workdir /wd \
  $gpu_arguments \
  chrones-development \
    python3 development/cycle.py "$@"
