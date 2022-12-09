# Copyright 2020-2022 Laurent Cabaret
# Copyright 2020-2022 Vincent Jacques

from __future__ import annotations

from typing import Dict, Iterable, List, Tuple
import dataclasses

import matplotlib.pyplot as plt

from ..monitoring import result as monitoring_result


def make_graph(output_file):
    results = monitoring_result.load()

    origin_timestamp = results.main_process.started_between_timestamps[0]

    fig, axes = plt.subplots(
        2, 1, squeeze=True,
        sharex=True,
    )
    (chrones_ax, cpu_ax) = axes

    chrones_ticks_ys = []
    chrones_ticks_labels = []
    bottom_y = 0
    def plot_chrones(chrone_events: Iterable[monitoring_result.ChroneEvent]):
        nonlocal chrones_ticks_ys
        nonlocal chrones_ticks_labels
        nonlocal bottom_y

        @dataclasses.dataclass
        class Thread:
            stack: List[monitoring_result.ChroneEvent]
            chrones: Dict[str, Tuple[float, float]]
            first_event: monitoring_result.ChroneEvent
            last_event: monitoring_result.ChroneEvent

        threads = {}
        for event in chrone_events:
            thread = threads.setdefault(event.thread_id, Thread([], {}, None, None))
            if thread.first_event is None:
                thread.first_event = event
            thread.last_event = event

            if event.__class__ == monitoring_result.StopwatchStart:
                thread.stack.append(event)
            elif event.__class__ == monitoring_result.StopwatchStop:
                start_event = thread.stack.pop()
                assert start_event.label is None
                bars = thread.chrones.setdefault(start_event.function_name, [])
                bars.append((start_event.timestamp - origin_timestamp, event.timestamp - start_event.timestamp))
            elif event.__class__ == monitoring_result.StopwatchSummary:
                pass
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

    def plot_processes(process: monitoring_result.Process):
        nonlocal chrones_ticks_ys
        nonlocal chrones_ticks_labels
        nonlocal bottom_y
        plot_chrones(process.load_chrone_events())
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
        chrones_ticks_labels.append(process.command[-30:])
        bottom_y += 2

    iter_processes(results.main_process, before=plot_processes)
    chrones_ax.set_yticks(chrones_ticks_ys, labels=chrones_ticks_labels)

    def plot_cpu(process: monitoring_result.Process):
        metrics = process.instant_metrics
        cpu_ax.plot(
            [m.timestamp - origin_timestamp for m in metrics],
            [m.cpu_percent for m in metrics],
            ".-",
            label=process.command[-30:],
        )

    iter_processes(results.main_process, before=plot_cpu)
    cpu_ax.legend()
    cpu_ax.set_xlim(left=0)
    cpu_ax.set_xlabel("Time (s)")
    cpu_ax.set_ylim(bottom=0)
    cpu_ax.set_ylabel("CPU (%)")

    fig.savefig(output_file, dpi=120)
    plt.close(fig)


def iter_processes(process, *, before=lambda _: None, after=lambda _: None):
    before(process)
    for child in process.children:
        iter_processes(child, before=before, after=after)
    after(process)
