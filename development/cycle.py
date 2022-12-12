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

    # With Chrones NOT installed
    ############################

    subprocess.run([f"pip3", "install", "-r", "requirements.txt"], stdout=subprocess.DEVNULL, check=True)

    run_cpp_tests()
    check_copyright_notices()
    # @todo(later) Run Python linter
    # @todo(later) Run ad-hoc check for "from __future__ import" in all Python files
    # @todo(later) Sort Python imports
    # @todo(later) Sort C++ includes

    # With Chrones installed
    ########################

    subprocess.run([f"pip3", "install", "."], stdout=subprocess.DEVNULL, check=True)

    make_example_integration_test_from_readme()
    run_integration_tests()


def run_cpp_tests():
    subprocess.run(
        ["make", f"-j{max(1, multiprocessing.cpu_count() - 2)}"],
        cwd="Chrones/instrumentation/cpp",
        check=True,
    )

def run_python_tests():
    subprocess.run(
        [
            "python3", "-m", "unittest", "discover",
            "--pattern", "*.py",
            "--start-directory", "Chrones", "--top-level-directory", ".",
        ],
        check=True,
    )

def check_copyright_notices():
    file_paths = subprocess.run(["git", "ls-files"], capture_output=True, universal_newlines=True, check=True).stdout.splitlines()
    for file_path in sorted(file_paths):
        if file_path in ["Chrones/__init__.py"]:
            continue

        if os.path.basename(file_path) in [".gitignore", "report.png"]:
            continue

        if file_path.startswith("integration-tests/help/chrones ") and file_path.endswith(" --help"):
            continue

        found = {
            re.compile(r"Copyright (\(c\) )?(2020-)?2022 Laurent Cabaret"): False,
            re.compile(r"Copyright (\(c\) )?(2020-)?2022 Vincent Jacques"): False,
        }
        with open(file_path) as f:
            for line in itertools.islice(f, 10):
                line = line.strip()
                for needle in found.keys():
                    if needle.search(line):
                        found[needle] = True
                if all(found.values()):
                    break
            else:
                assert False, file_path


def run_integration_tests():
    for test_file_name in glob.glob("integration-tests/*/run.sh"):
        print(test_file_name)
        subprocess.run(
            ["./run.sh"],
            cwd=os.path.dirname(test_file_name),
            check=True,
        )


def make_example_integration_test_from_readme():
    with open("README.md") as f:
        lines = f.readlines()

    executables = []
    files = {}
    current_file_name = None
    for line in lines:
        line = line.rstrip()
        m =  re.fullmatch(r"<!-- CHMOD\+X (.+) -->", line)
        if m:
            executables.append(m.group(1))
        m =  re.fullmatch(r"(?:-->)?<!-- STOP -->", line)
        if m:
            assert current_file_name
            current_file_name = None
        if current_file_name:
            files[current_file_name].append(line)
        m = re.fullmatch(r"<!-- (START|EXTEND) (.+) -->(?:<!--)?", line)
        if m:
            current_file_name = m.group(2)
            if m.group(1) == "START":
                files[current_file_name] = []
    assert current_file_name is None, current_file_name

    for file_name, file_contents in files.items():
        file_contents = textwrap.dedent("\n".join(file_contents)) + "\n"
        file_path = os.path.join("integration-tests", "readme-example", file_name)
        try:
            with open(file_path) as f:
                needs_write = f.read() != file_contents
        except FileNotFoundError:
            needs_write = True
        if needs_write:
            with open(file_path, "w") as f:
                f.write(file_contents)
    for executable in executables:
        os.chmod(os.path.join("integration-tests", "readme-example", executable), 0o755)


if __name__ == "__main__":
    main(sys.argv)
