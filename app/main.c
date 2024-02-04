#define _GNU_SOURCE
#include <stdio.h>
#include <sys/wait.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <libgen.h>
#include <sys/stat.h>
#include "file_operations.h"
#include <dirent.h> 
#include <sched.h>
#include <curl/curl.h>
#include "network.h"



#define BUFFER_SIZE 4096
#define READ_DESCRIPTOR 0
#define WRITE_DESCRIPTOR 1

struct child_args {
  char *command;
  char *const *cmd_args;
  char* img;
  const int *stdout_pipe;
  const int *stderr_pipe;
};




int child_process(void *clone_arg){
	struct child_args *clone_args = (struct child_args *)clone_arg;
	char *command = clone_args->command;
	char *const *cmd_args = clone_args->cmd_args;
	const int *pipe_std_out = clone_args->stdout_pipe;
	const int *pipe_std_err = clone_args->stderr_pipe;

	

	if (prepare_working_dir(command, clone_args->img) == EXIT_FAILURE) {
		perror("Error creating and changing docker directory!\n");
		return EXIT_FAILURE;
	}


	// replace std err and out with pipes
	dup2(pipe_std_out[WRITE_DESCRIPTOR], STDOUT_FILENO);
	dup2(pipe_std_err[WRITE_DESCRIPTOR], STDERR_FILENO);
	// close un needed ends
	close(pipe_std_out[READ_DESCRIPTOR]);
	close(pipe_std_err[READ_DESCRIPTOR]);
	// execv(basename(command), cmd_args);
	execv(command, cmd_args);
}




// Usage: your_docker.sh run <image> <command> <arg1> <arg2> ...
int main(int argc, char *argv[]) {
	// Disable output buffering
	setbuf(stdout, NULL);

	// create pipes
	int pipe_std_out[2], pipe_std_err[2];

	if(pipe(pipe_std_out) < 0 || pipe(pipe_std_err) < 0){
		perror("Failed to create pips");
		return 1;
	}
	
	char *command = argv[3];
	
	void *pchild_stack = malloc(1024 * 1024);

	struct child_args ca;
	
	ca.command = command;
	ca.cmd_args = &argv[3];
	ca.stdout_pipe = pipe_std_out;
	ca.stderr_pipe = pipe_std_err;
	ca.img = argv[2];
	
	int child_pid = clone(child_process, pchild_stack+(1024 * 1024), CLONE_NEWPID, (void *)&ca);
	
	if (child_pid == -1) {
	    perror("Error forking!");
	    return 1;
	}

	// parent
	//close write ends
	close(pipe_std_out[WRITE_DESCRIPTOR]);
	close(pipe_std_err[WRITE_DESCRIPTOR]);
	char buffer[BUFFER_SIZE];
	
	int n_bytes;
	
	while ((n_bytes = read(pipe_std_out[0], buffer, sizeof(buffer))) > 0) {
		write(STDOUT_FILENO, buffer, n_bytes);
	}
	
	while ((n_bytes = read(pipe_std_err[0], buffer, sizeof(buffer))) > 0) {
		write(STDERR_FILENO, buffer, n_bytes);
	}
	int child_status, exit_status;
	waitpid(child_pid, &child_status, 0);
	exit_status = WEXITSTATUS(child_status);
	exit(exit_status);

	

	return 0;
}
