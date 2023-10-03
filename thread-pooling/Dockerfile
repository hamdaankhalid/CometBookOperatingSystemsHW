# Use the official Ubuntu base image
FROM ubuntu:latest AS Dependencies

# Set the working directory inside the container
WORKDIR /usr/src/app

# Install GCC (GNU Compiler Collection)
RUN apt-get update && apt-get install -y gcc make


FROM Dependencies

# Copy your C code into the container
COPY . .

# Compile the C code
RUN make

# Execute the compiled program when the container starts
CMD ["./build/simple_use"]

