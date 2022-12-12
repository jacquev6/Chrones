#!/bin/bash

# Copyright 2020-2022 Laurent Cabaret
# Copyright 2020-2022 Vincent Jacques

set -o errexit
trap 'echo "Error on ${BASH_SOURCE[0]}:$LINENO"' ERR


make program-1
rm -rf bar run-result.json *.chrones.csv *.png
mkdir -p toto/tutu


chrones run -- ./program-2.sh
test -f run-result.json
test -f program-1.*.chrones.csv
test -f program-2.*.chrones.csv
chrones report
test -f report.png
rm run-result.json *.chrones.csv report.png


chrones run --logs-dir bar/baz -- ./program-2.sh
test -f bar/baz/run-result.json
test -f bar/baz/program-1.*.chrones.csv
test -f bar/baz/program-2.*.chrones.csv
chrones report --logs-dir bar/baz --output-name foo.png
test -f foo.png
rm -r bar foo.png
