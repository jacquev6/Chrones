# Copyright 2020-2022 Laurent Cabaret
# Copyright 2020-2022 Vincent Jacques

from __future__ import annotations

from typing import Dict, List, Optional, Tuple
import csv
import dataclasses
import glob
import pickle
import shlex
import unittest


@dataclasses.dataclass(kw_only=True, frozen=True)
class ProcessInstantMetrics:
    timestamp: float
    threads: int
    cpu_percent: float
    user_time: float
    system_time: float
    memory: Dict  # @todo Refine
    open_files: int
    io: Dict  # @todo Refine
    context_switches: Dict  # @todo Refine
    gpu_percent: float
    gpu_memory: int


@dataclasses.dataclass(kw_only=True, frozen=True)
class ChroneEvent:
    process_id: str
    thread_id: str
    timestamp: int


@dataclasses.dataclass(kw_only=True, frozen=True)
class StopwatchStart(ChroneEvent):
    function_name: str
    label: Optional[str]
    index: Optional[int]


@dataclasses.dataclass(kw_only=True, frozen=True)
class StopwatchStop(ChroneEvent):
    pass


@dataclasses.dataclass(kw_only=True, frozen=True)
class StopwatchSummary(ChroneEvent):
    function_name: str
    label: Optional[str]
    executions_count: int
    average_duration: int
    duration_standard_deviation: int
    min_duration: int
    median_duration: int
    max_duration: int
    total_duration: int


@dataclasses.dataclass(kw_only=True, frozen=True)
class Process:
    command_list: List[str]
    pid: int
    started_between_timestamps: Tuple[float, float]
    terminated_between_timestamps: Tuple[float, float]
    instant_metrics: List[ProcessInstantMetrics]
    children: List[Process]

    @property
    def command(self):
        return shlex.join(self.command_list)

    def load_chrone_events(self):
        return load_chrone_events(self.pid)


@dataclasses.dataclass(kw_only=True, frozen=True)
class MainProcessGlobalMetrics:
    user_time: float
    system_time: float
    minor_page_faults: int
    major_page_faults: int
    input_blocks: int
    output_blocks: int
    voluntary_context_switches: int
    involuntary_context_switches: int


@dataclasses.dataclass(kw_only=True, frozen=True)
class MainProcess(Process):
    exit_code: int
    global_metrics: MainProcessGlobalMetrics


@dataclasses.dataclass(kw_only=True, frozen=True)
class SystemInstantMetrics:
    timestamp: float
    host_to_device_transfer_rate: float
    device_to_host_transfer_rate: float


@dataclasses.dataclass(kw_only=True, frozen=True)
class System:
    instant_metrics: List[SystemInstantMetrics]


@dataclasses.dataclass(kw_only=True, frozen=True)
class RunResults:
    system: System
    main_process: MainProcess


def load() -> RunResults:
    with open("run-result.pickle", "rb") as f:
        return pickle.load(f)


def load_chrone_events(pid):
    chrones_file_names = glob.glob(f"*.{pid}.chrones.csv")
    if len(chrones_file_names) != 1:
        return

    with open(chrones_file_names[0]) as f:
        for line in csv.reader(f):
            yield make_chrone_event(line)


def make_chrone_event(line):
    process_id = line[0]
    thread_id = line[1]
    timestamp = int(line[2]) / 1e9
    if line[3] == "sw_start":
        return StopwatchStart(
            process_id=process_id,
            thread_id=thread_id,
            timestamp=timestamp,
            function_name=line[4],
            label=None if line[5] == "-" else line[5],
            index=None if line[6] == "-" else int(line[6]),
        )
    elif line[3] == "sw_stop":
        return StopwatchStop(
            process_id=process_id,
            thread_id=thread_id,
            timestamp=timestamp,
        )
    elif line[3] == "sw_summary":
        return StopwatchSummary(
            process_id=process_id,
            thread_id=thread_id,
            timestamp=timestamp,
            function_name=line[4],
            label=None if line[5] == "-" else line[5],
            executions_count=int(line[6]),
            average_duration=int(line[7]),
            duration_standard_deviation=int(line[8]),
            min_duration=int(line[9]),
            median_duration=int(line[10]),
            max_duration=int(line[11]),
            total_duration=int(line[12]),
        )
    else:
        assert False


class MakeChroneEventTestCase(unittest.TestCase):
    def test_stopwatch_start(self):
        self.assertEqual(
            make_chrone_event(["process_id", "thread_id", "375", "sw_start", "function_name", "label", "0"]),
            StopwatchStart(
                process_id="process_id",
                thread_id="thread_id",
                timestamp=375,
                function_name="function_name",
                label="label",
                index=0,
            ),
        )

    def test_stopwatch_start_no_index(self):
        self.assertEqual(
            make_chrone_event(["process_id", "thread_id", "375", "sw_start", "function_name", "label", "-"]),
            StopwatchStart(
                process_id="process_id",
                thread_id="thread_id",
                timestamp=375,
                function_name="function_name",
                label="label",
                index=None,
            ),
        )

    def test_stopwatch_start_no_label(self):
        self.assertEqual(
            make_chrone_event(["process_id", "thread_id", "375", "sw_start", "function_name", "-", "-"]),
            StopwatchStart(
                process_id="process_id",
                thread_id="thread_id",
                timestamp=375,
                function_name="function_name",
                label=None,
                index=None,
            ),
        )

    def test_stopwatch_stop(self):
        self.assertEqual(
            make_chrone_event(["process_id", "thread_id", "375", "sw_stop"]),
            StopwatchStop(
                process_id="process_id",
                thread_id="thread_id",
                timestamp=375,
            ),
        )

    def test_stopwatch_summary_no_label(self):
        self.assertEqual(
            make_chrone_event(["process_id", "thread_id", "375", "sw_summary", "function_name", "label", 10, 9, 8, 7, 6, 5, 4]),
            StopwatchSummary(
                process_id="process_id",
                thread_id="thread_id",
                timestamp=375,
                function_name="function_name",
                label="label",
                executions_count=10,
                average_duration=9,
                duration_standard_deviation=8,
                min_duration=7,
                median_duration=6,
                max_duration=5,
                total_duration=4,
            )
        )

    def test_stopwatch_summary_no_label(self):
        self.assertEqual(
            make_chrone_event(["process_id", "thread_id", "375", "sw_summary", "function_name", "-", 10, 9, 8, 7, 6, 5, 4]),
            StopwatchSummary(
                process_id="process_id",
                thread_id="thread_id",
                timestamp=375,
                function_name="function_name",
                label=None,
                executions_count=10,
                average_duration=9,
                duration_standard_deviation=8,
                min_duration=7,
                median_duration=6,
                max_duration=5,
                total_duration=4,
            )
        )

