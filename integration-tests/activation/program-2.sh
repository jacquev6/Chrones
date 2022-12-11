#!/bin/bash

set -o errexit


source <(chrones instrument shell enable program-2)


chrones_start foo
chrones_stop
