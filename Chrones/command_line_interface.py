# Copyright 2020-2022 Laurent Cabaret
# Copyright 2020-2022 Vincent Jacques

import subprocess
import click
import matplotlib.pyplot as plt

from .instrumentation import shell as shell_instrumentation


@click.group
def main():
    pass


@main.group
def shell():
    pass


@shell.command
@click.argument("program-name")
def activate(program_name):
    for line in shell_instrumentation.activate(program_name):
        print(line)


@main.command
@click.argument("command", nargs=-1, type=click.UNPROCESSED)
def run(command):
    subprocess.run(list(command), check=True)


@main.command
def report():
    fig, ax = plt.subplots(1, 1)

    ax.plot([0, 5], [80, 90])
    ax.set_ylim(bottom=0)

    fig.savefig("example.png", dpi=300)
    plt.close(fig)
