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

    origin_timestamp = results.main_process.started_between_timestamps[0]

    fig, axes = plt.subplots(
        2, 1, squeeze=True,
        sharex=True,
    )
    (chrones_ax, cpu_ax) = axes

    chrones_ticks_ys = []
    chrones_ticks_labels = []
    process_index = 0
    def plot_chrones(process: Process):
        nonlocal chrones_ticks_ys
        nonlocal chrones_ticks_labels
        nonlocal process_index
        # print(process.command, process.started_between_timestamps[0], process.started_between_timestamps[1], process.terminated_between_timestamps)
        chrones_ax.broken_barh(
            [(process.started_between_timestamps[0] - origin_timestamp, process.started_between_timestamps[1] - process.started_between_timestamps[0])],
            (process_index, 1),
            color="grey"
        )
        chrones_ax.broken_barh(
            [(process.started_between_timestamps[1] - origin_timestamp, process.terminated_between_timestamps[0] - process.started_between_timestamps[1])],
            (process_index, 1),
            color="blue"
        )
        chrones_ax.broken_barh(
            [(process.terminated_between_timestamps[0] - origin_timestamp, process.terminated_between_timestamps[1] - process.terminated_between_timestamps[0])],
            (process_index, 1),
            color="grey"
        )
        chrones_ticks_ys.append(process_index + 0.5)
        chrones_ticks_labels.append(shlex.join(process.command)[-40:] or "*empty*")
        # chrones_ticks_labels.append(process.pid)
        process_index += 2
        for child_process in process.children:
            plot_chrones(child_process)

    plot_chrones(results.main_process)
    chrones_ax.set_yticks(chrones_ticks_ys, labels=chrones_ticks_labels)


    def plot_cpu(process: Process):
        metrics = process.instant_metrics
        cpu_ax.plot(
            [m.timestamp - origin_timestamp for m in metrics],
            [m.cpu_percent for m in metrics],
            ".-",
            label=shlex.join(process.command)[-40:] or "*empty*",
            # label=process.pid,
        )
        for child_process in process.children:
            plot_cpu(child_process)

    plot_cpu(results.main_process)
    cpu_ax.legend()
    cpu_ax.set_xlim(left=0)
    cpu_ax.set_xlabel("Time (s)")
    cpu_ax.set_ylim(bottom=0)
    cpu_ax.set_ylabel("CPU (%)")

    fig.savefig("example.png", dpi=120)
    plt.close(fig)
