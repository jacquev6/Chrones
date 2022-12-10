# Copyright 2020-2022 Laurent Cabaret
# Copyright 2020-2022 Vincent Jacques

from __future__ import annotations

import pickle

import click

# @todo How could I refer to these as `instrumentation.shell` and `instrumentation.cpp`?
from .instrumentation import shell as shell_instrumentation
from .instrumentation import cpp as cpp_instrumentation
from .monitoring.runner import Runner
from .reporting.graph import make_graph


@click.group
def main():
    pass


@main.group
def config():
    pass


@config.group(name="c++")
def cpp():
    pass


@cpp.command
def header_location():
    print(cpp_instrumentation.location())


@main.group
def shell():
    pass


@shell.command
@click.argument("program-name")
def activate(program_name):
    for line in shell_instrumentation.activate(program_name):
        print(line)


@main.command
@click.argument("command", nargs=-1, type=click.UNPROCESSED)
def run(command):
    # @todo Take parameters from command-line
    runner = Runner(interval=0.2, clear_io_caches=False)
    result = runner.run(list(command))
    with open("run-result.pickle", "wb") as f:
        pickle.dump(result, f)
    # @todo Propagate exit code


@main.command
def report():
    make_graph("example.png")
