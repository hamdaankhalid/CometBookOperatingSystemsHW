# Build and Run
* docker build . -t pewpew && docker run pewpew


## The idea
A thread pool model where pre-initialized worker threads are used, and tasks are distributed upon.
Tasks can be fire and forgotten, or tasks can be awaited.
