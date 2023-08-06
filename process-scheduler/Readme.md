# Completely Fair Share Scheduler

After reading chapter 9 of comet book I have decided to write my own implementation of the CFS scheduler.


A program that opens an IPC pipe to intake "mock" processes and runs them based on the CFS scheduler policy each mock program is read from a file and is of the type MockProc with it's own IO simulation and CPU simulation.
The CFS scheduler simulates running them and then creates a visual summary when all procs have finished
The file tells us what processes will be run and the order at which they arrive.

Each line in the user defined simulation file is of the following type.
'''
proc_name, delay_in_arrival_from_last, total_time, num_randomly_timed_io_interrupts
'''

The main simulation code is in cfs.cpp, the makefile uses that to create the Completely fair scheduler and simulation processes.
After running the simulation code, if you decide to redirect logs, you can use the analyze binary which is build by analyze.cpp
via makefile. At the end of execution of analyze it will create a BMP file, and print a legend for the colors.

## Build and run:

1. make
2. ./cfs ./proc_simulation_1.txt

*** To redirect logs ***
1. touch {logfile name}
2. ./cfs ./proc_simulation_1.txt > {logfile name}

*** To analyze scheduler execution ***
1. make analyze
2. ./analyze {logfile}

The above will create a graph and you can open it with an image viewer.
