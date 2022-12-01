#!/bin/bash

# Copyright 2020-2022 Laurent Cabaret
# Copyright 2020-2022 Vincent Jacques

set -o errexit

cd "$(dirname "${BASH_SOURCE[0]}")"


# @todo Use a dedicated virtualenv instead of assuming that
#   pip install --upgrade build twine
# has been run beforehand

# @todo Bump version, before or after publishing
# @todo Update changelog

rm -rf dist  Chrones.egg-info

python3 -m build

twine upload dist/*
