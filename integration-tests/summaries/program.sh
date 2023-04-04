#!/bin/bash

# Copyright 2020-2022 Laurent Cabaret
# Copyright 2020-2022 Vincent Jacques

set -o errexit


source <(chrones instrument shell enable program)


chrones_start loop
for i in $(seq 5)
do
  chrones_start sleep
  sleep 0.1
  chrones_stop
done
chrones_stop
