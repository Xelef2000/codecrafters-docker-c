#include <stdio.h>
#include <sys/wait.h>
#include <unistd.h>
#include <string.h>

#define BUFFER_SIZE 4096
#define READ_DESCRIPTOR 0
#define WRITE_DESCRIPTOR 1

// Usage: your_docker.sh run <image> <command> <arg1> <arg2> ...
int main(int argc, char *argv[]) {
	// Disable output buffering
	setbuf(stdout, NULL);

	// create pipes
	int pipe_std_out[2], pipe_std_err[2];

	if(pipe(pipe_std_out) < 0 || pipe(pipe_std_err) < 0){
		printf("Failed to create pips");
		return 1;
	}
	
	char *command = argv[3];
	int child_pid = fork();
	if (child_pid == -1) {
	    printf("Error forking!");
	    return 1;
	}

	
	
	if (child_pid == 0) {
		// replace std err and out with pipes
		dup2(pipe_std_out[WRITE_DESCRIPTOR], STDOUT_FILENO);
		dup2(pipe_std_err[WRITE_DESCRIPTOR], STDERR_FILENO);
		// close un needed ends
		close(pipe_std_out[READ_DESCRIPTOR]);
		close(pipe_std_err[READ_DESCRIPTOR]);

	    execv(command, &argv[3]);
		

	} else {
		// parent

		//close write ends
		close(pipe_std_out[WRITE_DESCRIPTOR]);
		close(pipe_std_err[WRITE_DESCRIPTOR]);

		// char buffer_out[BUFFER_SIZE];
		// char buffer_err[BUFFER_SIZE];

		// memset(buffer_out, 0, sizeof(buffer_out));
		// memset(buffer_err, 0, sizeof(buffer_err));

		// int out_size = read(pipe_std_out[READ_DESCRIPTOR], buffer_out, sizeof(buffer_out));
		// int err_size = read(pipe_std_err[READ_DESCRIPTOR], buffer_err, sizeof(buffer_err));

		// buffer_out[out_size] = '\0';
		// buffer_err[err_size] = '\0';

		// write(STDOUT_FILENO, buffer_out, out_size);
		// write(STDERR_FILENO, buffer_err, err_size);

		char buffer[BUFFER_SIZE];
		// memset(buffer, 0, sizeof(buffer));
		

		int n_bytes;
		
		while ((n_bytes = read(pipe_std_out[0], buffer, sizeof(buffer))) > 0) {
			write(STDOUT_FILENO, buffer, n_bytes);
		}
		

		while ((n_bytes = read(pipe_std_err[0], buffer, sizeof(buffer))) > 0) {
			write(STDERR_FILENO, buffer, n_bytes);
		}

		wait(NULL);

	}

	return 0;
}
