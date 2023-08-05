# Completely Fair Share Scheduler

After reading chapter 9 of comet book I have decided to write my own implementation of the CFS scheduler.


A program that opens a queue to intake "mock" processes and runs them based on the CFS scheduler policy each mock program is read from a file and is of the type MockProc with it's own IO simulation and CPU simulation.
The CFS scheduler simulates running them and then prints a summary when all procs have finished
The file tells us what processes will be run and the order at which they arrive.

Each line in the file is of the type.
'''
proc_name, delay_in_arrival_from_last, total_time, num_randomly_timed_io_interrupts
'''

## Build and run:

1. make
2. ./cfs ./proc_simulation_1.txt

*** To redirect logs ***
1. touch <logfile name>
2. ./cfs ./proc_simulation_1.txt > <logfile name>

*** To analyze scheduler execution ***
1. make analyze
2. ./analyze <logfile>
