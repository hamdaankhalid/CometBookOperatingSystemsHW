/* Write a program that creates 2 children procs, and connects standard output of one to standard input of 
 * another using system call pipe()*/

#include <unistd.h>
#include <stdio.h>

int main() {
	// setup inter process communication via pipe
	
	// read write file descriptors
	int rwfd[2];
	char* message = "Hello, from write proc";
    char buffer[100];

	if (pipe(rwfd) == -1) {
		perror("error in interproc pipe setup \n");
		return -1;
	}

	int pid1 = fork();
	
	if (pid1 < 0) {
		perror("Failed to make child proc 1 \n");
		return -1;
	} else if (pid1 == 0) {
		// child proc 1 reads the data
		// close the write end of pipe
		close(rwfd[1]);
		
		dup2(rwfd[0], STDIN_FILENO); // Attach the read end of the pipe to stdin

        // Read from stdin (the pipe)
        fgets(buffer, sizeof(buffer), stdin);
        printf("Child 1 received: %s\n", buffer);
		close(rwfd[0]); // close read end of pipe
	} else {
		// parent proc
		int pid2 = fork();

		if (pid2 < 0) {
			perror("Failed to make child proc 2 \n");
			return -1;
		} else if (pid2 == 0) {
			// child proc 2 writes the data
			// close read end of pipe
			printf("Child 2 writing data to proc 1 \n");

			close(rwfd[0]);
			dup2(rwfd[1], STDOUT_FILENO);
			printf("%s", message);

			close(rwfd[1]); // close write end of pipe
		}
	}
	// parent proc again
	return 0;
}
