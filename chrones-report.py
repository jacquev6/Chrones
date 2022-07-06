#!/usr/bin/env python3
# Copyright 2020-2022 Laurent Cabaret
# Copyright 2020-2022 Vincent Jacques

from typing import Optional, List
import argparse
import collections
import contextlib
import csv
import dataclasses
import functools
import itertools
import json
import statistics
import sys
import textwrap
import unittest


def main(argv):
    parser = argparse.ArgumentParser(description="See each command's help for details")
    commands = parser.add_subparsers(title="commands")

    self_test_parser = commands.add_parser("self-test", help="run chrones.py's tests")
    self_test_parser.set_defaults(command=self_test_command)

    summaries_parser = commands.add_parser(
        "summaries",
        help="generate functions usage summaries",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        description=textwrap.dedent(summaries_command.__doc__),
    )
    summaries_parser.set_defaults(command=summaries_command)
    summaries_parser.add_argument("events_file")

    args = parser.parse_args(argv[1:])
    args.command(args)


def self_test_command(_):
    unittest.main(argv=sys.argv[0:1] + sys.argv[2:])


def summaries_command(args):
    """
    Generate functions usage summaries

    The output is in JSON format, on stdout.
    It's a list of summary objects. One summary object for each Chrone stopwatch, with the following attributes:

        - "function" (and optionally "label"): the identifiers of the stopwatch
        - "executions_count": the total number of times this stopwatch was run
        - "average_duration_ms" (if executions_count > 1)
        - "min_duration_ms", "median_duration_ms" and "max_duration_ms" (if executions_count > 1)
        - "total_duration_ms"
    """
    with open_events_file(args.events_file) as events:
        summaries = sorted(
            make_multi_process_summaries(events),
            key=lambda summary: (summary.executions_count, -summary.total_duration),
        )
        json.dump(
            [summary.json() for summary in summaries],
            sys.stdout,
            sort_keys=False,
            indent=4,
        )
        print()


@contextlib.contextmanager
def open_events_file(events_file):
    with open(events_file) as f:
        yield filter(None, (make_event(line) for line in csv.reader(f)))


def make_event(line):
    process_id = line[0]
    thread_id = line[1]
    timestamp = int(line[2])
    if line[3] == "sw_start":
        function_name = line[4]
        label = None if line[5] == "-" else line[5]
        index = None if line[6] == "-" else int(line[6])
        return StopwatchStart(process_id, thread_id, timestamp, function_name, label, index)
    elif line[3] == "sw_stop":
        return StopwatchStop(process_id, thread_id, timestamp)
    elif line[3] == "sw_summary":
        function_name = line[4]
        label = None if line[5] == "-" else line[5]
        executions_count = int(line[6])
        average_duration = int(line[7])
        duration_standard_deviation = int(line[8])
        min_duration = int(line[9])
        median_duration = int(line[10])
        max_duration = int(line[11])
        total_duration = int(line[12])
        return StopwatchSummary(
            process_id,
            thread_id,
            timestamp,
            function_name,
            label,
            executions_count,
            average_duration,
            duration_standard_deviation,
            min_duration,
            median_duration,
            max_duration,
            total_duration,
        )
    else:
        return None


@dataclasses.dataclass
class Event:
    process_id: str
    thread_id: str
    timestamp: int


@dataclasses.dataclass
class StopwatchStart(Event):
    function_name: str
    label: Optional[str]
    index: Optional[int]


@dataclasses.dataclass
class StopwatchStop(Event):
    pass


# Very similar to 'class Summary' a few lines below, but conceptually different, so no factorization
@dataclasses.dataclass
class StopwatchSummary(Event):
    function_name: str
    label: Optional[str]
    executions_count: int
    average_duration: int
    duration_standard_deviation: int
    min_duration: int
    median_duration: int
    max_duration: int
    total_duration: int


class MakeEventTestCase(unittest.TestCase):
    def test_stopwatch_start(self):
        self.assertEqual(
            make_event(["process_id", "thread_id", "375", "sw_start", "function_name", "label", "0"]),
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
            make_event(["process_id", "thread_id", "375", "sw_start", "function_name", "label", "-"]),
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
            make_event(["process_id", "thread_id", "375", "sw_start", "function_name", "-", "-"]),
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
            make_event(["process_id", "thread_id", "375", "sw_stop"]),
            StopwatchStop(
                process_id="process_id",
                thread_id="thread_id",
                timestamp=375,
            ),
        )

    def test_stopwatch_summary_no_label(self):
        self.assertEqual(
            make_event(["process_id", "thread_id", "375", "sw_summary", "function_name", "label", 10, 9, 8, 7, 6, 5, 4]),
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
            make_event(["process_id", "thread_id", "375", "sw_summary", "function_name", "-", 10, 9, 8, 7, 6, 5, 4]),
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


def make_multi_process_summaries(events):
    (all_durations, all_summaries) = functools.reduce(
        merge_durations_and_summaries,
        (
            extract_multi_threaded_durations(process_events)
            for _, process_events in itertools.groupby(events, key=lambda e: e.process_id)
        ),
    )
    yield from all_summaries
    for (key, durations) in all_durations.items():
        yield Summary(
            key[0],
            key[1],
            len(durations),
            statistics.mean(durations),
            statistics.stdev(durations) if len(durations) > 1 else 0,
            min(durations),
            statistics.median(durations),
            max(durations),
            sum(durations),
            # @todo (not needed by Laurent for now) Add summaries per process and per thread
        )


@dataclasses.dataclass
class Summary:
    function_name: str
    label: Optional[str]
    executions_count: int
    average_duration: int
    duration_standard_deviation: int
    min_duration: int
    median_duration: int
    max_duration: int
    total_duration: int

    def json(self):
        d = collections.OrderedDict()
        d["function"] = self.function_name
        if self.label is not None:
            d["label"] = self.label
        d["executions_count"] = self.executions_count
        if self.executions_count > 1:
            d["average_duration_ms"] = to_ms(self.average_duration)
            d["duration_standard_deviation_ms"] = to_ms(self.duration_standard_deviation)
            d["min_duration_ms"] = to_ms(self.min_duration)
            d["median_duration_ms"] = to_ms(self.median_duration)
            d["max_duration_ms"] = to_ms(self.max_duration)
        d["total_duration_ms"] = to_ms(self.total_duration)
        return d


def to_ms(duration_ns):
    return (duration_ns // 10_000) / 100  # ms with 2 decimals


def merge_durations_and_summaries(a, b):
    (durations_a, summaries_a) = a
    (durations_b, summaries_b) = b

    durations_merged = dict(durations_a)
    for (key, b_durations) in durations_b.items():
        merged_durations = durations_merged.setdefault(key, [])
        merged_durations += b_durations

    summaries_merged = summaries_a + summaries_b

    return (durations_merged, summaries_merged)


def extract_multi_threaded_durations(events):
    extractor = MultiThreadedDurationsExtractor()
    for event in events:
        extractor.process(event)
    return extractor.result


class MultiThreadedDurationsExtractor:
    def __init__(self):
        self.__extractors_per_thread = {}

    def process(self, event):
        extractor = self.__extractors_per_thread.setdefault(event.thread_id, SingleThreadedDurationsExtractor())
        extractor.process(event)

    @property
    def result(self):
        if self.__extractors_per_thread:
            return functools.reduce(
                merge_durations_and_summaries,
                (extractor.result for extractor in self.__extractors_per_thread.values()),
            )
        else:
            return [{}, []]


class SingleThreadedDurationsExtractor:
    def __init__(self):
        self.__stack = []
        self.__durations = {}
        self.__summaries = []

    def process(self, event):
        if event.__class__ == StopwatchStart:
            self.__stack.append(event)
        elif event.__class__ == StopwatchStop:
            start_event = self.__stack.pop()
            duration = event.timestamp - start_event.timestamp
            assert duration >= 0
            durations = self.__durations.setdefault((start_event.function_name, start_event.label), [])
            durations.append(duration)
        elif event.__class__ == StopwatchSummary:
            self.__summaries.append(Summary(
                function_name=event.function_name,
                label=event.label,
                executions_count=event.executions_count,
                average_duration=event.average_duration,
                duration_standard_deviation=event.duration_standard_deviation,
                min_duration=event.min_duration,
                median_duration=event.median_duration,
                max_duration=event.max_duration,
                total_duration=event.total_duration,
            ))
        else:
            assert False

    @property
    def result(self):
        assert len(self.__stack) == 0
        return (self.__durations, self.__summaries)


class ExtractDurationsTestCase(unittest.TestCase):
    def extract_durations(self, events):
        extractor = MultiThreadedDurationsExtractor()
        for event in events:
            extractor.process(event)
        return extractor.result[0]

    def extract_summaries(self, events):
        extractor = MultiThreadedDurationsExtractor()
        for event in events:
            extractor.process(event)
        return extractor.result[1]

    def test_empty(self):
        self.assertEqual(self.extract_durations([]), {})
        self.assertEqual(self.extract_summaries([]), [])

    def test_summaries(self):
        self.assertEqual(
            self.extract_summaries([
                StopwatchSummary("p", "t", 42, "f", None, 12, 11, 10, 9, 8, 7, 6),
                StopwatchSummary("p", "t", 42, "f", "label", 412, 411, 410, 49, 48, 47, 46),
            ]),
            [
                Summary("f", None, 12, 11, 10, 9, 8, 7, 6),
                Summary("f", "label", 412, 411, 410, 49, 48, 47, 46),
            ],
        )

    def test_single_duration(self):
        self.assertEqual(
            self.extract_durations([
                StopwatchStart("p", "t", 1234, "f", None, None),
                StopwatchStop("p", "t", 1534),
            ]),
            {("f", None): [300]},
        )

    def test_duration_with_label(self):
        self.assertEqual(
            self.extract_durations([
                StopwatchStart("p", "t", 1184, "f", "label", None),
                StopwatchStop("p", "t", 1534),
            ]),
            {("f", "label"): [350]},
        )

    def test_durations_loop(self):
        self.assertEqual(
            self.extract_durations([
                StopwatchStart("p", "t", 100, "f", "label", 1),
                StopwatchStop("p", "t", 200),
                StopwatchStart("p", "t", 250, "f", "label", 2),
                StopwatchStop("p", "t", 300),
                StopwatchStart("p", "t", 310, "f", "label", 3),
                StopwatchStop("p", "t", 460),
            ]),
            {("f", "label"): [100, 50, 150]},
        )

    def test_nested_durations(self):
        self.assertEqual(
            self.extract_durations([
                StopwatchStart("p", "t", 1234, "f", None, None),
                StopwatchStart("p", "t", 1334, "g", None, None),
                StopwatchStop("p", "t", 1434),
                StopwatchStop("p", "t", 1534),
            ]),
            {
                ('f', None): [300],
                ('g', None): [100],
            },
        )

    def test_multi_thread_durations(self):
        self.assertEqual(
            self.extract_durations([
                StopwatchStart("p", "t_a", 1234, "f", None, None),
                StopwatchStart("p", "t_b", 1334, "g", None, None),
                StopwatchStop("p", "t_a", 1434),
                StopwatchStop("p", "t_b", 1584),
            ]),
            {
                ('f', None): [200],
                ('g', None): [250],
            },
        )

    def test_concurrent_durations(self):
        self.assertEqual(
            self.extract_durations([
                StopwatchStart("p", "t_a", 1234, "f", None, None),
                StopwatchStart("p", "t_b", 1334, "f", None, None),
                StopwatchStop("p", "t_a", 1434),
                StopwatchStop("p", "t_b", 1584),
            ]),
            {
                ('f', None): [200, 250],
            },
        )


if __name__ == "__main__":
    main(sys.argv)
