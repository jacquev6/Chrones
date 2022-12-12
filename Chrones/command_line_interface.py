# Copyright 2020-2022 Laurent Cabaret
# Copyright 2020-2022 Vincent Jacques

from __future__ import annotations

import os
import sys
import textwrap

import click

# @todo(later) How could I refer to these as `instrumentation.shell` and `instrumentation.cpp`?
from .instrumentation import shell as shell_instrumentation
from .instrumentation import cpp as cpp_instrumentation
from .monitoring.runner import Runner
from .reporting.graph import make_graph


@click.group(help="Chrones is a software development tool to visualize runtime statistics about your program and correlate them with the phases of your program. Please visit https://github.com/jacquev6/Chrones for more details.")
def main():
    pass


@main.group(help="Everything related to instrumentation.")
def instrument():
    pass


@instrument.group(name="c++", help="Instrumentation of C++ programs.")
def cpp():
    pass


@cpp.command(help="Display the location of the C++ header(s). Use as 'g++ -I`chrones instrument c++ header-location`'.")
def header_location():
    print(cpp_instrumentation.location())


@instrument.group(help="Instrumentation of shell scripts.")
def shell():
    pass


@shell.command(help="Display the 'chrones_*' shell fonctions. Use as 'source <(chrones instrument shell enable PROGRAM_NAME)'.")
@click.argument("program-name")
def enable(*, program_name):
    for line in shell_instrumentation.enable(program_name):
        print(line)


@main.command(help=textwrap.dedent("""\
    Run a program under Chrones' monitoring.
    Add a '--' before the command if you need to pass options to the command.

    You *may* want to clear I/O caches just before running your command.
    See for example https://stackoverflow.com/a/25336215/905845.
"""))
@click.option("--logs-dir", default=".", help="Directory where instrumentation and monitoring logs will be stored.")
@click.option("--monitor-gpu", is_flag=True, help="Measure GPU usage.")
@click.option("--monitoring-interval", type=float, default=0.2, help="Interval between two consecutives measures, in seconds.")
@click.option("--allowed-missing-samples", type=int, default=1, help="Number of successive samples that can be skipped before Chrones start displaying warnings about \"slow monitoring\".")
@click.argument("command", nargs=-1, type=click.UNPROCESSED)
def run(
    *,
    logs_dir,
    monitor_gpu,
    monitoring_interval,
    allowed_missing_samples,
    command,
):
    runner = Runner(
        monitor_gpu=monitor_gpu,
        logs_directory=logs_dir,
        monitoring_interval=monitoring_interval,
        allowed_missing_samples=allowed_missing_samples,
    )
    result = runner.run(list(command))
    result.save(logs_dir)
    if result.main_process.exit_code != 0:
        sys.exit(result.main_process.exit_code)


@main.command(help="Create a human-readable image from monitoring logs.")
@click.option("--logs-dir", default=".", help="Directory containing instrumentation and monitoring logs.")
@click.option("--output-name", default="report.png", help="Output name for the report.")
def report(*, logs_dir, output_name):
    output_name = os.path.abspath(output_name)
    os.chdir(logs_dir)
    make_graph(output_name)
    # @todo(later) Restore summaries (cf. Original chrones-report.py file)
