#!/bin/bash

# Copyright 2020-2022 Laurent Cabaret
# Copyright 2020-2022 Vincent Jacques

set -o errexit


source <(chrones instrument shell enable program-2)


chrones_start foo
chrones_stop
