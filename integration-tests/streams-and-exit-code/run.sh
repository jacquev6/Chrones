#!/bin/bash

# Copyright 2020-2022 Laurent Cabaret
# Copyright 2020-2022 Vincent Jacques

set -o errexit
trap 'echo "Error on ${BASH_SOURCE[0]}:$LINENO"' ERR


# Exit code is preserved
chrones run -- true
test $? -eq 0

if chrones run -- bash -c "exit 42"
then
  false
else
  test $? -eq 42
fi

# Standard input and outputs are preserved
diff <(echo toto | cat -A) <(echo toto | chrones run -- cat -A)

# Standard error is preserved
diff <(bash -c "echo toto >&2" 2>&1) <(chrones run -- bash -c "echo toto >&2" 2>&1)

# Environment is barely changed
diff <(env | grep -v -e '^_=') <(chrones run -- env | grep -v -e '^_=' -e '^CHRONES_')
