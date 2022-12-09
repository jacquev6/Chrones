#!/usr/bin/env python3

# Copyright 2020-2022 Laurent Cabaret
# Copyright 2020-2022 Vincent Jacques

from __future__ import annotations

import glob
import shutil
import subprocess
# Please keep this script simple enough to use only the standard library

# @todo Use a dedicated virtualenv instead of assuming that
#   pip install --upgrade build twine
# has been run beforehand

# @todo Update changelog


if __name__ == "__main__":
    with open("setup.py") as f:
        setup_lines = f.readlines()
    for line in setup_lines:
        if line.startswith("version = "):
            last_version = line.rstrip()[11:-1]
    print("Last version:", last_version)

    print("Changes since:")
    subprocess.run(["git", "log", "--oneline", "--no-decorate", f"v{last_version}.."], check=True)

    last_parts = [int(p) for p in last_version.split(".")]
    new_parts = list(last_parts)
    new_parts[-1] += 1
    new_version = ".".join(str(p) for p in new_parts)
    manual_new_version = input(f"New version [{new_version}]? ")
    if manual_new_version:
        new_parts = [int(p) for p in manual_new_version.split(".")]
        assert len(new_parts) == 3
        assert new_parts > last_parts
        new_version = manual_new_version
    with open("setup.py", "w") as f:
        for line in setup_lines:
            if line.startswith("version = "):
                f.write(f"version = {repr(new_version)}\n")
            else:
                f.write(line)

    subprocess.run(["git", "add", "setup.py"], check=True)
    subprocess.run(["git", "commit", "-m", f"Publish version {new_version}"], check=True)

    subprocess.run(["git", "tag", f"v{new_version}"], check=True)
    subprocess.run(["git", "push", f"--tags"], check=True)

    shutil.rmtree("dist", ignore_errors=True)
    shutil.rmtree("Chrones.egg-info", ignore_errors=True)

    subprocess.run(["python3", "-m", "build"])
    subprocess.run(["twine", "upload"] + glob.glob("dist/*"))
