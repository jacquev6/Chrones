# Copyright 2020-2022 Laurent Cabaret
# Copyright 2020-2022 Vincent Jacques

from __future__ import annotations

import os


def enable(program_name):
    if os.environ.get("CHRONES_ENABLED"):
        yield f"chrones_filename={program_name}.$!.chrones.csv"

        yield "function chrones_start {"
        yield '  echo "$!,0,$(date +%s%N),sw_start,$1,${2:--},${3:--}" >>$chrones_filename'
        yield "}"

        yield "function chrones_stop {"
        yield '  echo "$!,0,$(date +%s%N),sw_stop" >>$chrones_filename'
        yield "}"
    else:
        yield "function chrones_start {"
        yield "  true"
        yield "}"

        yield "function chrones_stop {"
        yield "  true"
        yield "}"
