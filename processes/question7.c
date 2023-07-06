/* Write a program that creates a child process and the child process then closes the standard output (STDOUT_FILENO)
 * What happens if after closing standard output file descriptor the child calls printf ?
 */

#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>

int main() {
	int pid = fork();

	if (pid < 0) {
		return -1;
	} else if (pid == 0) {
		// child proc
		close(STDOUT_FILENO);
		printf("FUCK\n");
	}

	// parent proc
	return 0;
}

/*
 * In the given program, the child process closes the standard output file descriptor (STDOUT_FILENO) using the close() function. After closing the standard output, the child process attempts to call printf() to print the string "FUCK".

When the child process closes the standard output file descriptor, it essentially disconnects itself from the standard output. As a result, any subsequent attempt to write to stdout using functions like printf() will not have any effect.

In this case, when the child process calls printf("FUCK\n"), the text "FUCK" will not be displayed on the console or any other standard output stream. Since the standard output file descriptor has been closed, the output is effectively discarded.

It's important to note that even though printf() is called in the child process, the output is not displayed because the child process has closed its standard output file descriptor. The parent process, on the other hand, remains unaffected and continues its execution as usual without any impact from the child process's actions.
*/
