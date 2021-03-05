#!/bin/bash

set -o errexit

pwd
cd "builder"
pwd

now=$(date '+%y%m%d-%H%M%S')

docker build builder --tag chrones-builder

#docker run \
#  --rm --tty --interactive \
#  --volume "$PWD:/chrones" --workdir /chrones \
#  chrones-builder \
#  bash -c "
#set -o errexit
#
#mkdir -p build
#make test
#"
