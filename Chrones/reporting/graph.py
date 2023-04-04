# Copyright 2020-2022 Laurent Cabaret
# Copyright 2020-2022 Vincent Jacques

from __future__ import annotations

from typing import Dict, List, Optional, Tuple
import dataclasses

import matplotlib.pyplot as plt

from ..monitoring import result as monitoring_result


def make_graph(output_file):
    results = monitoring_result.RunResults.load()

    origin_timestamp = results.main_process.started_between_timestamps[0]

    gantt_grapher = GantGrapher(results)

    n = 10 if results.run_settings.gpu_monitored else 7
    fig, axes = plt.subplots(
        n, 1, squeeze=False,
        sharex=True,
        figsize=(12, 4 * n + gantt_grapher.get_height() / 10), layout="constrained",
        height_ratios=[gantt_grapher.get_height() / 15] + [1 for _ in range(n - 1)],
    )
    if results.run_settings.gpu_monitored:
        (
            (chrones_ax,),
            (cpu_ax,), (threads_ax,),
            (rss_ax,),
            (gpu_ax,), (gpu_mem_ax,), (gpu_transfers_ax,),
            (inputs_ax,), (outputs_ax,), (open_files_ax,)
        ) = axes
    else:
        (
            (chrones_ax,),
            (cpu_ax,), (threads_ax,),
            (rss_ax,),
            (inputs_ax,), (outputs_ax,), (open_files_ax,)
        ) = axes

    gantt_grapher.draw(chrones_ax)

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

    axes[-1][0].set_xlim(left=0)
    axes[-1][0].set_xlabel("Time (s)")

    fig.savefig(output_file, dpi=120)
    plt.close(fig)


class GantGrapher:
    @dataclasses.dataclass
    class Thread:
        stack: List[monitoring_result.ChroneEvent]
        chrones: Dict[str, Tuple[float, float]]
        first_event: Optional[monitoring_result.ChroneEvent]
        last_event: Optional[monitoring_result.ChroneEvent]

    def __init__(self, results: monitoring_result.RunResults):
        self.__results = results
        self.__origin_timestamp = results.main_process.started_between_timestamps[0]
        self.__prepare()

    def __prepare(self):
        self.__processes = []
        iter_processes(self.__results.main_process, before=lambda p: self.__processes.append(p))
        self.__threads = {process.pid: self.__prepare_threads(process) for process in self.__processes}

    def __prepare_threads(self, process: monitoring_result.Process):
        threads = {}
        for event in process.load_chrone_events():
            thread = threads.setdefault(event.thread_id, GantGrapher.Thread([], {}, None, None))
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
                        [start_event.function_name, start_event.label],
                    )
                )
                chrones = thread.chrones.setdefault(name, [])
                chrones.append((start_event, event))
            elif event.__class__ == monitoring_result.StopwatchSummary:
                pass
            else:
                assert False
        threads = list(threads.values())
        assert all(t.stack == [] for t in threads)

        return threads

    def get_height(self):
        return sum(
            2 + sum(3 + len(thread.chrones) for thread in self.__threads[process.pid])
            for process in self.__processes
        ) - 1

    def draw(self, ax):
        top_y = 0

        for process in self.__processes:
            start_x = (process.started_between_timestamps[0] + process.started_between_timestamps[1]) / 2 - self.__origin_timestamp
            end_x = (process.terminated_between_timestamps[0] + process.terminated_between_timestamps[1]) / 2 - self.__origin_timestamp
            width = end_x - start_x

            threads_height = sum(3 + len(thread.chrones) for thread in self.__threads[process.pid])
            process_height = 1 + threads_height

            ax.broken_barh([(start_x, width)], (top_y - process_height, process_height), color="#ff8f8f")
            ax.text(x=start_x, y=top_y - 0.5, s=process.command, ha="left", va="center")

            self.__plot_threads(top_y - 1, self.__threads[process.pid], ax)

            top_y -= 1 + process_height

        assert top_y + 1 == -self.get_height()

        ax.set_ylim(top=0, bottom=top_y + 1)
        ax.set_yticks([])

    def __plot_threads(self, top_y, threads: List[GantGrapher.Thread], ax):
        for (thread_index, thread) in enumerate(filter(lambda t: t.chrones, threads)):
            start_x = thread.first_event.timestamp - self.__origin_timestamp
            end_x = thread.last_event.timestamp - self.__origin_timestamp
            width = end_x - start_x

            thread_height = 2 + len(thread.chrones)

            ax.broken_barh([(start_x, width)], (top_y - thread_height, thread_height), color="#8fff8f")
            ax.text(x=start_x, y=top_y - 0.5, s=f"Thread {thread_index}", ha="left", va="center")

            self.__plot_chrones(start_x, top_y - 1, thread.chrones, ax)

            top_y -= 1 + thread_height

    def __plot_chrones(self, left_x, top_y, chrones, ax: plt.Axes):
        for (name, events) in sorted(chrones.items(), key=lambda kv: kv[1][0][0].timestamp):
            bars = [(start_event.timestamp - self.__origin_timestamp, stop_event.timestamp - start_event.timestamp) for (start_event, stop_event) in events]
            ax.broken_barh(bars, (top_y - 1, 1), color="#8f8fff", edgecolor="black")
            ax.text(x=left_x, y=top_y - 0.5, s=name, ha="left", va="center")

            top_y -= 1


def iter_processes(process, *, before=lambda _: None, after=lambda _: None):
    before(process)
    for child in process.children:
        iter_processes(child, before=before, after=after)
    after(process)
