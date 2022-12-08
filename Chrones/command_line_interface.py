# Copyright 2020-2022 Laurent Cabaret
# Copyright 2020-2022 Vincent Jacques

import dataclasses
import glob
import pickle
import shlex
from typing import Dict, List, Tuple
import click
import matplotlib.pyplot as plt

# @todo How could I refer to these as `instrumentation.shell` and `instrumentation.cpp`?
from .instrumentation import shell as shell_instrumentation
from .instrumentation import cpp as cpp_instrumentation
from .monitoring.runner import Runner, RunResults, Process
from . import report as chrones_report


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
    bottom_y = 0
    def plot_chrones(chrones_file_name):
        nonlocal chrones_ticks_ys
        nonlocal chrones_ticks_labels
        nonlocal bottom_y

        @dataclasses.dataclass
        class Thread:
            stack: List[chrones_report.Event]
            chrones: Dict[str, Tuple[float, float]]
            first_event: chrones_report.Event
            last_event: chrones_report.Event

        threads = {}
        with chrones_report.open_events_file(chrones_file_name) as events:
            for event in events:
                event.timestamp /= 1e9

                thread = threads.setdefault(event.thread_id, Thread([], {}, None, None))
                if thread.first_event is None:
                    thread.first_event = event
                thread.last_event = event

                if event.__class__ == chrones_report.StopwatchStart:
                    thread.stack.append(event)
                elif event.__class__ == chrones_report.StopwatchStop:
                    start_event = thread.stack.pop()
                    assert start_event.label is None
                    bars = thread.chrones.setdefault(start_event.function_name, [])
                    bars.append((start_event.timestamp - origin_timestamp, event.timestamp - start_event.timestamp))
                else:
                    assert False
        threads = list(threads.values())
        assert all(t.stack == [] for t in threads)

        for thread in threads:
            if thread.chrones:
                chrones_ax.broken_barh(
                    [(thread.first_event.timestamp - origin_timestamp, thread.last_event.timestamp - thread.first_event.timestamp)],
                    (bottom_y + 0.75, len(thread.chrones) * 3 - 0.5),
                    color="orange"
                )
                for (name, bars) in thread.chrones.items():
                    chrones_ax.broken_barh(bars, (bottom_y + 1, 2))
                    chrones_ticks_ys.append(bottom_y + 2)
                    chrones_ticks_labels.append(name)
                    bottom_y += 3
                bottom_y += 1

    def plot_processes(process: Process):
        nonlocal chrones_ticks_ys
        nonlocal chrones_ticks_labels
        nonlocal bottom_y
        # print(process.command, process.started_between_timestamps[0], process.started_between_timestamps[1], process.terminated_between_timestamps)
        chrones_file_names = glob.glob(f"*.{process.pid}.chrones.csv")
        if len(chrones_file_names) == 1:
            plot_chrones(chrones_file_names[0])
        chrones_ax.broken_barh(
            [(process.started_between_timestamps[0] - origin_timestamp, process.started_between_timestamps[1] - process.started_between_timestamps[0])],
            (bottom_y, 1),
            color="grey"
        )
        chrones_ax.broken_barh(
            [(process.started_between_timestamps[1] - origin_timestamp, process.terminated_between_timestamps[0] - process.started_between_timestamps[1])],
            (bottom_y, 1),
            color="blue"
        )
        chrones_ax.broken_barh(
            [(process.terminated_between_timestamps[0] - origin_timestamp, process.terminated_between_timestamps[1] - process.terminated_between_timestamps[0])],
            (bottom_y, 1),
            color="grey"
        )
        chrones_ticks_ys.append(bottom_y + 0.5)
        chrones_ticks_labels.append(shlex.join(process.command)[-40:] or "*empty*")
        # chrones_ticks_labels.append(process.pid)
        bottom_y += 2
        for child_process in process.children:
            plot_processes(child_process)

    plot_processes(results.main_process)
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
