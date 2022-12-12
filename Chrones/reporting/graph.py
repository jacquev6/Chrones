# Copyright 2020-2022 Laurent Cabaret
# Copyright 2020-2022 Vincent Jacques

from __future__ import annotations

from typing import Dict, Iterable, List, Tuple
import dataclasses

import matplotlib.pyplot as plt

from ..monitoring import result as monitoring_result


def make_graph(output_file):
    results = monitoring_result.RunResults.load()

    origin_timestamp = results.main_process.started_between_timestamps[0]

    n = 10 if results.run_settings.gpu_monitored else 7
    fig, axes = plt.subplots(
        n, 1, squeeze=True,
        sharex=True,
        figsize=(12, 4 * n), layout="constrained",
    )
    if results.run_settings.gpu_monitored:
        (
            chrones_ax,
            cpu_ax, threads_ax,
            rss_ax,
            gpu_ax, gpu_mem_ax, gpu_transfers_ax,
            inputs_ax, outputs_ax, open_files_ax
        ) = axes
    else:
        (
            chrones_ax,
            cpu_ax, threads_ax,
            rss_ax,
            inputs_ax, outputs_ax, open_files_ax
        ) = axes

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
                name = " - ".join(
                    str(part)
                    for part in filter(
                        lambda p: p is not None,
                        [start_event.function_name, start_event.label, start_event.index],
                    )
                )
                bars = thread.chrones.setdefault(name, [])
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
                    color="orange",
                    edgecolor="black",
                )
                for (name, bars) in thread.chrones.items():
                    chrones_ax.broken_barh(bars, (bottom_y + 1, 2), edgecolor="black")
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
    cpu_ax.set_ylim(bottom=0)
    cpu_ax.set_ylabel("CPU (%)")

    def plot_threads(process: monitoring_result.Process):
        metrics = process.instant_metrics
        threads_ax.plot(
            [m.timestamp - origin_timestamp for m in metrics],
            [m.threads for m in metrics],
            ".-",
            label=process.command[-30:],
        )

    iter_processes(results.main_process, before=plot_threads)
    threads_ax.legend()
    threads_ax.set_ylim(bottom=0)
    threads_ax.set_ylabel("Threads")

    def plot_rss(process: monitoring_result.Process):
        metrics = process.instant_metrics
        rss_ax.plot(
            [m.timestamp - origin_timestamp for m in metrics],
            [m.memory.rss / 1024. / 1024. for m in metrics],
            ".-",
            label=process.command[-30:],
        )

    iter_processes(results.main_process, before=plot_rss)
    rss_ax.legend()
    rss_ax.set_ylim(bottom=0)
    rss_ax.set_ylabel("Memory - RSS (MiB)")

    if results.run_settings.gpu_monitored:
        def plot_gpu(process: monitoring_result.Process):
            metrics = process.instant_metrics
            gpu_ax.plot(
                [m.timestamp - origin_timestamp for m in metrics],
                [m.gpu_percent for m in metrics],
                ".-",
                label=process.command[-30:],
            )

        iter_processes(results.main_process, before=plot_gpu)
        gpu_ax.legend()
        gpu_ax.set_ylim(bottom=0)
        gpu_ax.set_ylabel("GPU (%)")

        def plot_gpu_mem(process: monitoring_result.Process):
            metrics = process.instant_metrics
            gpu_mem_ax.plot(
                [m.timestamp - origin_timestamp for m in metrics],
                [m.gpu_memory for m in metrics],
                ".-",
                label=process.command[-30:],
            )

        iter_processes(results.main_process, before=plot_gpu_mem)
        gpu_mem_ax.legend()
        gpu_mem_ax.set_ylim(bottom=0)
        gpu_mem_ax.set_ylabel("GPU memory (MiB)")

        gpu_transfers_ax.plot([m.timestamp - origin_timestamp for m in results.system.instant_metrics], [m.host_to_device_transfer_rate for m in results.system.instant_metrics], 'o-', label="Host to device",)
        gpu_transfers_ax.plot([m.timestamp - origin_timestamp for m in results.system.instant_metrics], [m.device_to_host_transfer_rate for m in results.system.instant_metrics], 'o-', label="Device to host",)
        gpu_transfers_ax.set_ylabel("System-wide\nGPU transfers (MB/s)")
        gpu_transfers_ax.legend()

    def plot_inputs(process: monitoring_result.Process):
        metrics = process.instant_metrics
        inputs_ax.plot(
            [m.timestamp - origin_timestamp for m in metrics],
            [m.io.read_chars / 1024. / 1024. for m in metrics],
            ".-",
            label=process.command[-30:],
        )

    iter_processes(results.main_process, before=plot_inputs)
    inputs_ax.legend()
    inputs_ax.set_ylim(bottom=0)
    inputs_ax.set_ylabel("Cumulated inputs (MiB)")

    def plot_outputs(process: monitoring_result.Process):
        metrics = process.instant_metrics
        outputs_ax.plot(
            [m.timestamp - origin_timestamp for m in metrics],
            [m.io.write_chars / 1024. / 1024. for m in metrics],
            ".-",
            label=process.command[-30:],
        )

    iter_processes(results.main_process, before=plot_outputs)
    outputs_ax.legend()
    outputs_ax.set_ylim(bottom=0)
    outputs_ax.set_ylabel("Cumulated outputs (MiB)")

    def plot_open_files(process: monitoring_result.Process):
        metrics = process.instant_metrics
        open_files_ax.plot(
            [m.timestamp - origin_timestamp for m in metrics],
            [m.open_files for m in metrics],
            ".-",
            label=process.command[-30:],
        )

    iter_processes(results.main_process, before=plot_open_files)
    open_files_ax.legend()
    open_files_ax.set_ylim(bottom=0)
    open_files_ax.set_ylabel("Open files")

    axes[-1].set_xlim(left=0)
    axes[-1].set_xlabel("Time (s)")

    fig.savefig(output_file, dpi=120)
    plt.close(fig)


def iter_processes(process, *, before=lambda _: None, after=lambda _: None):
    before(process)
    for child in process.children:
        iter_processes(child, before=before, after=after)
    after(process)
