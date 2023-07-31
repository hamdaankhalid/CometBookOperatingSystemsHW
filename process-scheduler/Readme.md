# Completely Fair Share Scheduler

After reading chapter 9 of comet book I have decided to write my own implementation of the CFS scheduler.


A program that opens a queue to intake "mock" processes and runs them based on the CFS scheduler policy each mock program is read from a file and is of the type MockProc with it's own IO simulation and CPU simulation.
The CFS scheduler simulates running them and then prints a summary when all procs have finished
The file tells us what processes will be run and the order at which they arrive.

Each line in the file is of the type.
'''
proc_name, ms_arrival_from_start, total_time, num_randomly_timed_io_interrupts
'''

Program runs 2 different binaries.
1. A long running scheduler that listens for incoming processes and simulates
their running.
2. A job sender, that parses our mock porcesses simulation file and accordingly sends jobs to run into the long running scheduler, along with mock IO interrupts for the processes based on the job description in file. It also signals when the job has finished.
