# Copyright 2020-2022 Laurent Cabaret
# Copyright 2020-2022 Vincent Jacques

from __future__ import annotations

from typing import List, Optional, Tuple
import csv
import dataclasses
import glob
import json
import os
import shlex
import unittest

import dacite


dataclass = dataclasses.dataclass(kw_only=True, frozen=True, slots=True)


# WARNING: if you make changes in these classes after version 1.0.0 is published,
# you may have to change the `format_version` in `RunResults.save`.

@dataclass
class MemoryInstantMetrics:
    rss: float


@dataclass
class InputOutputoInstantMetrics:
    read_chars: int
    write_chars: int


@dataclass
class ContextSwitchInstantMetrics:
    voluntary: int
    involuntary: int


@dataclass
class ProcessInstantMetrics:
    timestamp: float
    threads: int
    cpu_percent: float
    user_time: float
    system_time: float
    memory: MemoryInstantMetrics
    open_files: int
    io: InputOutputoInstantMetrics
    context_switches: ContextSwitchInstantMetrics
    gpu_percent: Optional[float]
    gpu_memory: Optional[float]


@dataclass
class ChroneEvent:
    process_id: str
    thread_id: str
    timestamp: int


@dataclass
class StopwatchStart(ChroneEvent):
    function_name: str
    label: Optional[str]
    index: Optional[int]


@dataclass
class StopwatchStop(ChroneEvent):
    pass


@dataclass
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


@dataclass
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


@dataclass
class MainProcessGlobalMetrics:
    user_time: float
    system_time: float
    minor_page_faults: int
    major_page_faults: int
    input_blocks: int
    output_blocks: int
    voluntary_context_switches: int
    involuntary_context_switches: int


@dataclass
class MainProcess(Process):
    exit_code: int
    global_metrics: MainProcessGlobalMetrics


@dataclass
class SystemInstantMetrics:
    timestamp: float
    host_to_device_transfer_rate: Optional[float]
    device_to_host_transfer_rate: Optional[float]


@dataclass
class System:
    instant_metrics: List[SystemInstantMetrics]


@dataclass
class RunSettings:
    gpu_monitored: bool


@dataclass
class RunResults:
    run_settings: RunSettings
    system: System
    main_process: MainProcess

    def save(self, logs_dir):
        data = dataclasses.asdict(self)
        # If you have to change the format_version, you *must* add an integration test
        # with data in the previous format, to prove we can still load it.
        file_content = dict(format_version=1, data=data)
        with open(os.path.join(logs_dir, "run-result.json"), "w") as f:
            json.dump(file_content, f)

    @classmethod
    def load(cls) -> RunResults:
        with open("run-result.json") as f:
            file_content = json.load(f)
        assert file_content["format_version"] == 1
        data = file_content["data"]
        return dacite.from_dict(data_class=cls, data=data, config=dacite.Config(cast=[Tuple]))


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
                timestamp=375e-9,
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
                timestamp=375e-9,
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
                timestamp=375e-9,
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
                timestamp=375e-9,
            ),
        )

    def test_stopwatch_summary(self):
        self.assertEqual(
            make_chrone_event(["process_id", "thread_id", "375", "sw_summary", "function_name", "label", 10, 9, 8, 7, 6, 5, 4]),
            StopwatchSummary(
                process_id="process_id",
                thread_id="thread_id",
                timestamp=375e-9,
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
                timestamp=375e-9,
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

