#!/usr/bin/env python3
# Copyright 2020-2022 Laurent Cabaret
# Copyright 2020-2022 Vincent Jacques

from email.policy import strict
import json
import sys


def main(summaries):
    expected = [
        ('virtual void ChronesTest_ActualFile_Test::TestBody()', None, 1),
        ('virtual void ChronesTest_ActualFile_Test::TestBody()', "intermediate 'wwwwww'", 1),
        ('void actual_file_f()', None, 1),
        ('virtual void ChronesTest_ActualFile_Test::TestBody()', 'intermediate, "xxxxxx"', 1),
        ('void actual_file_f()', 'loop a', 6),
        ('static void foo::Bar::actual_file_g(int, float, std::pair<int, float>*)', None, 6),
        ('static void foo::Bar::actual_file_g(int, float, std::pair<int, float>*)', "mini", 6),
        ('void actual_file_f()', 'loop b', 8),
        ('void actual_file_h()', None, 8),
        ('void actual_file_h()', "mini", 8),
    ]

    assert len(summaries) == len(expected), (summaries, expected)
    for summary, (expected_function, expected_label, expected_count) in zip(summaries, expected):
        assert summary["function"] == expected_function, (summary, expected_function)
        assert summary.get("label") == expected_label, (summary, expected_label)
        assert summary["executions_count"] == expected_count, (summary, expected_count)


if __name__ == "__main__":
    with open(sys.argv[1]) as f:
        summaries = json.load(f)
    main(summaries)
