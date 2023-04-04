#!/bin/bash

# Copyright 2020-2022 Laurent Cabaret
# Copyright 2020-2022 Vincent Jacques

set -o errexit
trap 'echo "Error on ${BASH_SOURCE[0]}:$LINENO"' ERR


chrones run -- ./program.sh
chrones report --with-summaries summaries.json
test $(jq -r '.[0].function' <summaries.json) == loop
test $(jq '.[0].executions_count' <summaries.json) -eq 1
test $(jq -r '.[1].function' <summaries.json) == sleep
test $(jq '.[1].executions_count' <summaries.json) -eq 5
rm run-result.json *.chrones.csv report.png summaries.json
