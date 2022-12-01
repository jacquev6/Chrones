import subprocess
import click
import matplotlib.pyplot as plt


@click.group
def main():
    pass


@main.group
def shell():
    pass


@shell.command
@click.argument("program-name")
def activate(program_name):
    print("function chrones_start {")
    print("  echo start")
    print("}")
    print("function chrones_stop {")
    print("  echo stop")
    print("}")


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
