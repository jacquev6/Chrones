<!--
Copyright 2020-2022 Laurent Cabaret
Copyright 2020-2022 Vincent Jacques
-->

*Chrones* is a software development tool to visualize runtime statistics (CPU percentage, GPU percentage, memory usage, *etc.*) about your program and correlate them with the phases of your program.

It aims at being very simple to use and provide useful information out of the box<!-- @todo(later) *and* at being customizable to your specific use cases -->.

Here is an example of graph produced by *Chrones* about a shell script launching a few executables (see exactly how this image is generated [at the end of this Readme](#code-of-the-example-image)):

![Example](integration-tests/readme-example/report.png)

*Chrones* was sponsored by [Laurent Cabaret](https://cabaretl.pages.centralesupelec.fr/en/publications/) from the [MICS](http://www.mics.centralesupelec.fr/) and written by [Vincent Jacques](https://vincent-jacques.net).

It's licensed under the [MIT license](http://choosealicense.com/licenses/mit/).
Its [documentation and source code](https://github.com/jacquev6/Chrones) are on GitHub.

Questions? Remarks? Bugs? Want to contribute? Open [an issue](https://github.com/jacquev6/Chrones/issues) or [a discussion](https://github.com/jacquev6/Chrones/discussions)!

<!-- @todo(later) Insert paragraph about Chrones' clients? -->

# Conceptual overview

*Chrones* consist of three parts: instrumentation (optional), monitoring and reporting.

The instrumentation part of *Chrones* runs inside your program after you've modified it.
It's used as a library for your programming language.
To use it, you add one-liners to the functions you want to know about.
After that, your program logs insider timing information about these functions.

The monitoring part is a wrapper around your program.
It runs your program as you instruct it to, preserving its access to the standard input and outputs, the environment, and its command-line.
While doing so, it monitors your program's whole process tree and logs resource usage metrics.

The reporting part reads the logs produced by the instrumentation and monitoring, and produces human-readable reports including graphs.

The instrumentation part is completely optional.
You can use the monitoring part on non-instrumented programs,
or even on partially instrumented programs like a shell script calling an instrumented executable and a non-instrumented executable.
The graphs produced by *Chrones*' reporting will just miss information about your program's phases.

We've chosen the command-line as the main user interface for *Chrones*' to allow easy integration into your automated workflows.
<!-- @todo(later) It can also be used as a Python library for advanced use-cases. -->

Please note that *Chrones* currently only works on Linux.
Furthermore, the C++ instrumentation requires g++.
We would gladly accept contributions that extend *Chrones*' usability.

*Chrones*' instrumentation libraries are available for <!-- @todo(later) Python,--> C++ and the shell language.

# Expected performance

The instrumentation part of *Chrones* accurately measures and reports durations down to the millisecond.
Its monitoring part takes samples a few times per second.
No nanoseconds in this project; *Chrones* is well suited for programs that run at least a few seconds.

Overhead introduced by *Chrones* in C++ programs is less than a second per million instrumented blocks.
Don't use it for functions called billions of times.

# Get started

## Install *Chrones*

The monitoring and reporting parts of *Chrones* are distributed as a [Python package on PyPI](https://pypi.org/project/Chrones/).
Install them with `pip install Chrones`.

<details>
<summary>And at the moment that's all you need. <small>(Click the arrow for more information)</small></summary>

The instrumentation parts are distributed in language-specific ways.

The Python version comes with the `Chrones` Python packages you've just installed.

The C++ and shell languages don't really have package managers, so the C++ and shell versions happen to also be distributed within the Python package.

Versions for other languages will be distributed using the appropriate packages managers.
</details>

## (Optional) Instrument your code

### Concepts

The instrumentation libraries are based on the following concepts:

#### Coordinator

The *coordinator* is a single object that centralizes measurements and writes them into a log file.

It also takes care of enabling or disabling instrumentation: the log will be created if and only if it detects it's being run inside *Chrones*' monitoring.
This lets you run your programm outside *Chrones*' monitoring as if it was not instrumented.

#### Chrone

A *chrone* is the main instrumentation tool.
You can think of it as a stopwatch that logs an event when it's started and another event when it's stoped.

Multiple chrones can be nested.
This makes them particularly suitable to instrument [structured code](https://en.wikipedia.org/wiki/Structured_programming) with blocks and functions (*i.e.* the vast majority of modern programs).
From the log of the nested chrones, *Chrones*' reporting is able to reconstruct the evolution of the call stack(s) of the program.

Chrones have three identifying attributes: a *name*, an optional *label* and an optional *index*.
The three of them are used in reports to distinguish between chrones.
Here is their meaning:

- In languages that support it, the name is set automatically from the name of the enclosing function.
In languages that don't, we strongly recommend that you use the same convention: a chrone's name comes from the closest named piece of code.
- It sometimes makes sense to instrument a block inside a function.
The label is here to identify those blocks.
- Finaly, when these blocks are iterations of a loop, you can use the index to distinguish them.

See `simple.cpp` at the end of this Readme for a complete example.

<!-- @todo(v1.0.0) #### Mini-chrone -->

<!-- @todo(v1.0.0) Define, explain the added value -->

### Language-specific instructions

The *Chrones* instrumentation library is currently available for the following languages:

#### Shell

First, import *Chrones* and initialize the coordinator with:

    source <(chrones instrument shell enable program-name)

where `program-name` is... the name of your program.

You can then use the two functions `chrones_start` and `chrones_stop` to instrument your shell functions:

    function foo {
        chrones_start foo

        # Do something

        chrones_stop
    }

`chrones_start` accepts one mandatory argument: the `name`, and two optional ones: the `label` and `index`.
See their description in the [Concepts](#concepts) section above.

#### C++

First, `#include <chrones.hpp>`.
The header is distributed within *Chrones*' Python package.
You can get is location with `chrones instrument c++ header-location`, that you can pass to the `-I` option of you compiler.
For example, ``g++ -I`chrones instrument c++ header-location` foo.cpp -o foo``.

`chrones.hpp` uses variadic macros with `__VA_OPT__`, so if you need to set your `-std` option, you can use either `gnu++11` or `c++20` or later.

Create the coordinator at global scope, before your `main` function:

    CHRONABLE("program-name")

where `program-name` is... the name of your program.

You can then instrument functions and blocks using the `CHRONE` macro:

    int main() {
        CHRONE();

        {
            CHRONE("loop");
            for (int i = 0; i != 100; ++i) {
                CHRONE("iteration", i);
                // Do something
            }
        }
    }

Then `CHRONE` macro accepts zero to two arguments: the optional label and index. See their description in the [Concepts](#concepts) section above.
In the example above, all three chrones will have the same name, `"int main()"`.
`"loop"` and `"iteration"` will be the respective labels of the last two chrones, and the last chrone will also have an index.

*Chrones*' instrumentation can be statically disabled by passing `-DCHRONES_DISABLED` to the compiler.
In that case, all macros provided by the header will be empty and your code will compile exactly as if it was not using *Chrones*.

Troubleshooting tip: if you get an `undefined reference to chrones::global_coordinator` error, double-check you're linking with the translation unit that calls `CHRONABLE`.

Known limitations:

- `CHRONE` must not be used outside `main`, *e.g.* in constructors and destructors of static variables

<!-- @todo(later) #### Python

First, import *Chrones*' decorator: `from chrones.instumentation import chrone`.

Then, decorate your functions:

    @chrone
    def foo():
        # Do something

You can also instrument blocks that are not functions:

    with chrone("bar"):
        # Do something

@todo(later) Name, label, and index -->

## Run using `chrones run`

Compile your executable(s) if required.
Then launch them using `chrones run -- your_program --with --its --options`.

Everything before the `--` is interpreted as options for `chrones run`.
Everything after is passed as-is to your program.
The standard input and output are passed unchanged to your program.
The exit code of `chrones run` is the exit code of `your_program`.

Have a look at `chrones run --help` for its detailed usage.

## Generate report

Run `chrones report` to generate a report in the current directory.

Have a look at `chrones report --help` for its detailed usage.

<!-- @todo(later) ## Use *Chrones* as a library

Out of the box, *Chrones* produces generic reports and graphs, but you can customize them by using *Chrones* as a Python library. -->

# Code of the example image

As a complete example, here is the shell script that the image at the top of this Readme is about (named `example.sh`):

<!-- @todo(v1.0.0) Show more things in the example graph -->
<!-- @todo(v1.0.0) Make the Gantt-ish diagram more readable -->

<!-- START example.sh --><!--
    #!/bin/bash

    set -o errexit
    trap 'echo "Error on ${BASH_SOURCE[0]}:$LINENO"' ERR
--><!-- STOP -->
<!-- EXTEND example.sh -->
    source <(chrones instrument shell enable example)


    chrones_start sleep-then-run-single
    sleep 0.5

    dd status=none if=/dev/random of=in.dat bs=1M count=3

    chrones_start run-single
    ./single
    chrones_stop

    chrones_stop

    chrones_start sleep
    sleep 0.7
    chrones_stop
<!-- STOP -->
<!-- CHMOD+X example.sh -->

And the various executables called by the script:

- `single.cpp`:

<!-- START single.cpp -->
    #include <time.h>

    #include <chrones.hpp>

    CHRONABLE("single");

    void do_some_ios() {
      CHRONE();

      char megabyte[1024 * 1024];

      std::ifstream in("in.dat");

      for (int i = 0; i != 4; ++i) {
        in.read(megabyte, sizeof(megabyte));
        usleep(500'000);
        std::ofstream out("out.dat");
        out.write(megabyte, sizeof(megabyte));
        usleep(500'000);
      }
    }

    void something_long() {
      CHRONE();

      for (int i = 0; i < 256; ++i) {
        volatile double x = 3.14;
        for (int j = 0; j != 1'000'000; ++j) {
          x = x * j;
        }
      }
    }

    void something_else() {
      CHRONE();

      usleep(500'000);
    }

    int main() {
      CHRONE();

      do_some_ios();

      {
        CHRONE("loop");
        for (int i = 0; i != 2; ++i) {
          CHRONE("iteration", i);

          something_else();
          something_long();
        }
      }

      something_else();
    }
<!-- STOP -->

This code is built using `make` and the following `Makefile`:

<!-- START run.sh --><!--
    #!/bin/bash

    set -o errexit
    trap 'echo "Error on ${BASH_SOURCE[0]}:$LINENO"' ERR

    rm -f run-results.json example.*.chrones.csv single.*.chrones.csv


    make
--><!-- STOP -->
<!-- CHMOD+X run.sh -->

<!-- START Makefile -->
    all: single

    single: single.cpp
    	g++ -std=c++20 -O3 -I`chrones instrument c++ header-location` single.cpp -o single
<!-- STOP -->
<!-- EXTEND Makefile -->

    single: Makefile
<!-- STOP -->

It's executed like this:

<!-- EXTEND run.sh -->
    chrones run -- ./example.sh
<!-- STOP -->

And the report is created like this:

<!-- EXTEND run.sh -->
    chrones report
<!-- STOP -->

# Known limitations

## Impacts of instrumentation

Adding instrumentation to your program will change what's observed by the monitoring:

- data is continuously output to the log file and this is visible in the "I/O" graph of the report
- the log file is also counted in the "Open files" graph
- in C++, an additional thread is launched in your process, visible in the "Threads" graph

## Non-monotonous system clock

*Chrones* does not handle Leap seconds well. But who does, really?

## Multiple GPUs

Machines with more than one GPU are not yet supported.
<!-- @todo(later) Support machines with several GPUs -->

# Developing *Chrones* itself

You'll need a Linux machine with:
- a reasonably recent version of Docker
- a reasonably recent version of Bash

<!-- @todo(later) Support developing on a machine without a GPU. -->
Oh, and for the moment, you need an NVidia GPU, with drivers installed and `nvidia-container-runtime` configured.

To build everything and run all tests:

    ./run-development-cycle.sh --long

To skip particularly long tests:

    ./run-development-cycle.sh

Or even:

    ./run-development-cycle.sh --quick

To [bump the version number](semver.org) and publish on PyPI:

    ./publish.sh [patch|minor|major]
