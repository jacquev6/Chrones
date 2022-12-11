#!/bin/bash

# Copyright 2020-2022 Laurent Cabaret
# Copyright 2020-2022 Vincent Jacques

set -o errexit
trap 'echo "Error on ${BASH_SOURCE[0]}:$LINENO"' ERR


rm -f chrones\ *


# We don't really test anything here, but this will make interface changes explicit in 'git diff'

chrones --help >'chrones --help'

chrones instrument --help >'chrones instrument --help'
chrones instrument c++ --help >'chrones instrument c++ --help'
chrones instrument c++ header-location --help >'chrones instrument c++ header-location --help'

chrones instrument shell --help >'chrones instrument shell --help'
chrones instrument shell enable --help >'chrones instrument shell enable --help'

chrones run --help >'chrones run --help'

chrones report --help >'chrones report --help'
