/*
 * Write a program that opens a file and then calls fork to create a new process. Can both the child and parent access teh file descriptor?
 * what happens if they both try to write to the file concurrently
 */

#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <string.h>

int main() {
	int fd = open("question2_file.txt", O_WRONLY);
	int forkResp = fork();
	if (forkResp < 0) {
		printf("failed to create new proc \n");
		return -1;
	} else if (forkResp == 0) {
		// CHILD PROC
		printf("CHILD PID: %d \n", (int) getpid());		
		const char* msg = "CHILD PROC WRITE \n";
		int sz = write(fd,  msg, strlen(msg));
		if (sz != strlen(msg)) {
			printf("failed to write from child proc \n");
		}
	} else {
		// PARENT PROC
		printf("PARENT PID: %d \n", (int) getpid());		
		
		const char* msg = "PARENT PROC WRITE \n";
		int sz = write(fd,  msg, strlen(msg));
		if (sz != strlen(msg)) {
			printf("failed to write from parent proc \n");
		}
	}

	int closed = close(fd);
	if (closed != 0) {
		printf("Failed to close %d \n", fd);
	} else {
		printf("Closed %d \n", fd);
	}
}

/*
 * In a UNIX-like system, when you use fork() to create a new process, both the child and parent processes will have separate copies of the file descriptor table, including the file descriptors. This means that both the child and parent can access the same file descriptor independently. However, the file offset (the position in the file) is shared between them, so if one process writes to the file, the other process will continue writing from that position.

If both the child and parent processes try to write to the file concurrently without proper synchronization, it can lead to a data race and potentially corrupt the file. Therefore, it's crucial to use synchronization mechanisms to ensure that only one process writes to the file at a time.
 * */
