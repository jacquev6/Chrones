# Copyright 2020-2022 Laurent Cabaret
# Copyright 2020-2022 Vincent Jacques

from __future__ import annotations

import os.path


def location():
    # @todo(later) Switch to https://setuptools.pypa.io/en/latest/userguide/datafiles.html#accessing-data-files-at-runtime
    return os.path.dirname(__file__)
