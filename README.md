What is a chrone ?
==================

Apart from an extraordinary literary object found in the masterpiece [La Horde du Contrevent (french only)](https://lavolte.net/livres/la-horde-du-contrevent/), a chrone is a stopwatch to measure the execution duration of a piece of code.
A set of tools are provided to produce human-readable reports.

The ambition of Chrones is to accurately measure and report durations down to the millisecond.
No nanoseconds in this project!

How it works
============

During the execution of the program being monitored, a stream of events (*e.g.* "start stopwatch", "stop stopwatch") is recorded in a `.csv` file.
After execution, a command-line tool (`chrones-report.py`) can be used to generate various reports and visualizations.
See "Reporting" below.

Chrones makes the strong assumption that stopwatches are nested (*i.e.* "start" and "stop" events are sequenced like a well-parenthesized expression).
This is useful because stopwatches typically measure the duration of a function or code block.
That assumption allows re-creating the call stack at any point in the program's life.

Languages supported
===================

C++: full support of mono-threaded programs

Bash: full support of single-processed scripts

The arguably simple format of the `.cvs` file makes it relatively easy to support another language:
one "just" has to create a library that produces that file.

Get started
===========

C++
---

Add `c++/chrones.hpp` in your include path.

In your main source file, add:

    #include "chrones.hpp"

    CHRONABLE("name_of_your_executable")

The name above will be used as the base name of the `.csv` file.

Add `CHRONE();` at the top of the functions and blocks you want to instrument.
This macro will automatically detect the name of the function it's in.
If you need to instrument several blocks in the same function, you might want to pass it a label argument:

    void f() {
        CHRONE();
        {
            CHRONE("first block");
            // Do something long
        }
        {
            CHRONE("second block");
            // Do something long
        }
    }

If you need to instrument the successive iteration of a loop, you can pass a label and an index:

    void f() {
        CHRONE();
        for (int i = 0; i != 10; ++i) {
            CHRONE("loop", i);
            // Do something long
        }
    }

Compile, link and run your program to generate the `.csv` file.
See "reporting" below to generate reports.

If you compile `chrones.hpp` with `-DNO_CHRONES`, both macros expand to nothing, effectively removing Chrones from your program.

Bash
----

Add `bash/chrones.sh` near your script.

In your script, `source chrones.sh` then call `chrones_initialize name_of_your_script`.
That name will be used as the base name of the `.csv` file.

Call `chrones_start_stopwatch` (resp. `chrones_stop_stopwatch`) at the beginning (resp. end) of the functions and blocks you want to instrument.

`chrones_start_stopwatch` accepts a mandatory "function" argument and optional "label" and "index" arguments for use-cases similar to the ones described in the "C++" section above:

    for i in $(seq 1 10)
    do
        chrones_start_stopwatch "main" "loop" $i
        # Do something long
        chrones_stop_stopwatch

Run your script to generate the `.csv` file.
See "reporting" below to generate reports.

@todo Support the NO_CHRONES functionality?

Reporting
---------

You should now have a `name_of_your_program.chrones.csv` file.

Running `chrones-report.py summaries name_of_your_program.chrones.csv` will print out a "summaries" `.json` document.

See `chrones-report.py --help` for details.

Developing Chrones itself
=========================

Dependencies:
- a reasonably recent version of Docker
- a reasonably recent version of Bash

To run all tests:

    ./make.sh -j$(nproc)
