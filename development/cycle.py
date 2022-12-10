#!/usr/bin/env python3

# Copyright 2020-2022 Laurent Cabaret
# Copyright 2020-2022 Vincent Jacques

from __future__ import annotations

import glob
import itertools
import multiprocessing
import os
import re
import subprocess
import sys
import textwrap


def main(args):
    assert len(args) in [1, 2]
    if len(args) == 2:
        long = args[1] == "--long"
        quick = args[1] == "--quick"
        if not (long or quick):
            print("Unknown argument:", args[1])
            print(f"Usage: {args[0]} [--long|--quick]")
            exit(1)
    else:
        long = False
        quick = False

    run_cpp_tests()
    # @todo Run Python tests (python3 -m unittest discover --pattern '*.py')
    # @todo Run Python linter
    # @todo Run ad-hoc check for copyright lines in all files
    # @todo Run ad-hoc check for "from __future__ import" in all Python files
    # @todo Run integration tests
    # @todo Sort Python imports
    # @todo Sort C++ includes
    build_example_from_readme(quick=quick)


def run_cpp_tests():
    subprocess.run(
        ["make", f"-j{max(1, multiprocessing.cpu_count() - 2)}"],
        cwd="Chrones/instrumentation/cpp",
        check=True,
    )


def build_example_from_readme(quick):
    subprocess.run([f"pip3", "install", "."], check=True)

    if quick:
        subprocess.run([f"./report.sh"], cwd="example", check=True)
        return
    else:
        for f in itertools.chain(
            glob.glob("example/*.chrones.csv"),
            glob.glob("example/*.pickle"),
        ):
            os.unlink(f)

        with open("README.md") as f:
            lines = f.readlines()

        files = {}
        current_file_name = None
        current = ""
        for line in lines:
            if line.rstrip() == "<!-- STOP -->":
                assert current_file_name not in files
                files[current_file_name] = current
                current_file_name = None
                current = ""
            if current_file_name:
                current += line
            m = re.fullmatch(r"<!-- START (.+) -->", line.rstrip())
            if m:
                current_file_name = m.group(1)
        assert current_file_name is None, current_file_name

        for file_name, file_contents in files.items():
            file_contents = textwrap.dedent(file_contents)
            file_path = os.path.join("example", file_name)
            try:
                with open(file_path) as f:
                    needs_write = f.read() != file_contents
            except FileNotFoundError:
                needs_write = True
            if needs_write:
                with open(file_path, "w") as f:
                    if file_path.endswith(".sh"):
                        f.write(textwrap.dedent("""\
                            #!/bin/bash
                            set -o errexit

                        """))
                    f.write(file_contents)
                os.chmod(file_path, 0o755)

        for phase in ["build", "run", "report"]:
            subprocess.run([f"./{phase}.sh"], cwd="example", check=True)


if __name__ == "__main__":
    main(sys.argv)
