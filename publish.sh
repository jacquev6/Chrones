#!/bin/bash

# Copyright 2020-2022 Laurent Cabaret
# Copyright 2020-2022 Vincent Jacques

set -o errexit

cd "$(dirname "${BASH_SOURCE[0]}")"


# This script assumes that
#   pip install --upgrade build twine
# has been run beforehand

python3 -m build

# twine upload dist/*
