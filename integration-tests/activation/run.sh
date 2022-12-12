#!/bin/bash

# Copyright 2020-2022 Laurent Cabaret
# Copyright 2020-2022 Vincent Jacques

set -o errexit
trap 'echo "Error on ${BASH_SOURCE[0]}:$LINENO"' ERR


rm -f *.chrones.csv
make program-1-no-chrones program-1-yes-chrones


# De-activated during compilation, standalone: no log
./program-1-no-chrones
if [ -f program-1.*.chrones.csv ]
then
  false
fi

# De-activated during compilation, in monitoring: no log
chrones run -- ./program-1-no-chrones
if [ -f program-1.*.chrones.csv ]
then
  false
fi

# Activated during compilation, standalone: still no log
./program-1-yes-chrones
if [ -f program-1.*.chrones.csv ]
then
  false
fi

# Activated during compilation, in monitoring: log!
chrones run -- ./program-1-yes-chrones
test -f program-1.*.chrones.csv

# Standalone: no log
./program-2.sh
if [ -f program-2.*.chrones.csv ]
then
  false
fi

# In monitoring: log
chrones run -- ./program-2.sh
test -f program-2.*.chrones.csv
