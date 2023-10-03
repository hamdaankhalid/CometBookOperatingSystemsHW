* docker build . -t pewpew && docker run -it pewpew /bin/bash
* run the executable: ./build/simple_use


## The idea
A thread pool model where pre-initialized worker threads are used, and tasks are distributed upon.
Tasks can be fire and forgotten, or tasks can be awaited.
