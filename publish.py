#!/usr/bin/env python3

# Copyright 2020-2022 Laurent Cabaret
# Copyright 2020-2022 Vincent Jacques

import glob
import shutil
import subprocess

# @todo Use a dedicated virtualenv instead of assuming that
#   pip install --upgrade build twine
# has been run beforehand

# @todo Bump version, before or after publishing
# @todo Update changelog

if __name__ == "__main__":
    shutil.rmtree("dist", ignore_errors=True)
    shutil.rmtree("Chrones.egg-info", ignore_errors=True)

    subprocess.run(["python3", "-m", "build"])

    subprocess.run(["twine", "upload"] + glob.glob("dist/*"))
