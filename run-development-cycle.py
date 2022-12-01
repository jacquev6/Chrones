#!/usr/bin/env python3

# Copyright 2020-2022 Laurent Cabaret
# Copyright 2020-2022 Vincent Jacques

import sys
# Please keep this script simple to use only the standard library


def main(args):
    assert len(args) in [1, 2]
    if len(args) == 2:
        long = args[1] == "--long"
        quick = args[1] == "--quick"
        if not (long or quick):
            print("Unknown argument:", args[1])
            print(f"Usage: {args[0]} [--long|--quick]")
            exit(1)


if __name__ == "__main__":
    main(sys.argv)
