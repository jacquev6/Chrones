# Copyright 2020-2022 Laurent Cabaret
# Copyright 2020-2022 Vincent Jacques

import pickle
import shlex
import subprocess
import click
import matplotlib.pyplot as plt

# @todo How could I refer to these as `instrumentation.shell` and `instrumentation.cpp`?
from .instrumentation import shell as shell_instrumentation
from .instrumentation import cpp as cpp_instrumentation
from .monitoring.runner import Runner, RunResults, Process


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
    runner = Runner(interval=0.2)
    result = runner.run(list(command))
    with open("run-result.pickle", "wb") as f:
        pickle.dump(result, f)
    # @todo Propagate exit code


@main.command
def report():
    with open("run-result.pickle", "rb") as f:
        results : RunResults = pickle.load(f)

    fig, ax = plt.subplots(1, 1)

    def plot_cpu(process: Process):
        metrics = process.instant_metrics
        ax.plot(
            [m.timestamp for m in metrics],
            [m.cpu_percent for m in metrics],
            ".-",
            label=shlex.join(process.command) or "*empty*",
        )
        for child_process in process.children:
            plot_cpu(child_process)

    plot_cpu(results.main_process)

    ax.legend()
    ax.set_xlim(left=0)
    ax.set_xlabel("Time (s)")
    ax.set_ylim(bottom=0)
    ax.set_ylabel("CPU (%)")

    fig.savefig("example.png", dpi=120)
    plt.close(fig)
