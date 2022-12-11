#!/bin/bash

# Copyright 2020-2022 Laurent Cabaret
# Copyright 2020-2022 Vincent Jacques

set -o errexit
trap 'echo "Error on ${BASH_SOURCE[0]}:$LINENO"' ERR


rm -f *.chrones.csv


# De-activated during compilation
g++ -std=gnu++11 program-1.cpp -I$(chrones instrument c++ header-location) -DCHRONES_DISABLED -o program-1

# Standalone: no log
./program-1
if [ -f program-1.*.chrones.csv ]
then
  false
fi

# In monitoring: no log
chrones run -- ./program-1
if [ -f program-1.*.chrones.csv ]
then
  false
fi


# Activated during compilation
g++ -std=gnu++11 program-1.cpp -I$(chrones instrument c++ header-location) -o program-1

# Standalone: still no log
./program-1
if [ -f program-1.*.chrones.csv ]
then
  false
fi

# In monitoring: log!
chrones run -- ./program-1
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
