#!/bin/bash

set -o errexit


source <(chrones shell enable program-2)


chrones_start foo
chrones_stop
