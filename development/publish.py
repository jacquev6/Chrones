# Copyright 2020-2022 Laurent Cabaret
# Copyright 2020-2022 Vincent Jacques

from __future__ import annotations
import glob
import shutil

import subprocess
import sys

import semver


def main(args):
    assert len(args) == 2
    check_cleanliness()
    (old_version, new_version) = bump_version(args[1])
    update_changelog(old_version, new_version)
    commit(new_version)
    publish_to_pypi()


def check_cleanliness():
    not_indexed = subprocess.run(["git", "diff", "--stat", "--exit-code"])
    if not_indexed.returncode != 0:
        print("ERROR: there are non-staged changes. Please 'git add' them and retry.")
        exit(1)

    branch = subprocess.run(["git", "branch", "--show-current"], stdout=subprocess.PIPE, universal_newlines=True, check=True)
    if branch.stdout.strip() != "main":
        input("WARNING: you're not on branch 'main'. Press enter to proceed, Ctrl+C to cancel.")

    not_commited = subprocess.run(["git", "diff", "--stat", "--staged", "--exit-code"], stdout=subprocess.DEVNULL)
    if not_commited.returncode != 0:
        input("WARNING: Some changes are staged but not commited. They will be included in the publication commit. Press enter to proceed, Ctrl+C to cancel.")


def bump_version(part):
    with open("setup.py") as f:
        setup_lines = f.readlines()
    for line in setup_lines:
        if line.startswith("version = "):
            old_version = semver.VersionInfo.parse(line[11:-2])

    print("Last version:", old_version)
    if part == "patch":
        new_version = old_version.bump_patch()
    elif part == "minor":
        new_version = old_version.bump_minor()
    elif part == "major":
        new_version = old_version.bump_major()
    else:
        assert False
    print("New version:", new_version)

    with open("setup.py", "w") as f:
        for line in setup_lines:
            if line.startswith("version = "):
                f.write(f"version = \"{new_version}\"\n")
            else:
                f.write(line)

    return (old_version, new_version)


def update_changelog(old_version, new_version):
    log_lines = subprocess.run(
        ["git", "log", "--oneline", "--no-decorate", f"v{old_version}.."],
        stdout=subprocess.PIPE, universal_newlines=True,
        check=True,
    ).stdout.splitlines()

    with open("CHANGELOG.md", "a") as f:
        f.write(f"\n# Version {new_version}\n\n")
        for line in log_lines:
            f.write(f"- {line.split(' ', 1)[1]}\n")

    input("Please edit CHANGELOG.md then press enter to proceed, Ctrl+C to cancel.")


def commit(new_version):
    subprocess.run(["git", "add", "setup.py", "CHANGELOG.md"], check=True)
    subprocess.run(["git", "commit", "-m", f"Publish version {new_version}"], stdout=subprocess.DEVNULL, check=True)
    subprocess.run(["git", "tag", f"v{new_version}"], check=True)
    subprocess.run(["git", "push", f"--tags"], check=True)


def publish_to_pypi():
    shutil.rmtree("dist", ignore_errors=True)
    shutil.rmtree("build", ignore_errors=True)
    shutil.rmtree("Chrones.egg-info", ignore_errors=True)
    subprocess.run(["python3", "-m", "build"])
    subprocess.run(["twine", "upload"] + glob.glob("dist/*"))


if __name__ == "__main__":
    main(sys.argv)
