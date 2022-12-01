#!/bin/bash

# Copyright 2020-2022 Laurent Cabaret
# Copyright 2020-2022 Vincent Jacques

set -o errexit

cd "$(dirname "${BASH_SOURCE[0]}")"


quick=false
long=false
while [[ "$#" -gt 0 ]]
do
  case $1 in
    --quick)
      quick=true;;
    --long)
      long=true;;
    *)
      echo "Unknown argument: $1"
      echo "Usage: $0 [--quick|--long]"
      exit 1;;
  esac
  shift
done
