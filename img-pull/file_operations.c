#include <stdio.h>
#include <sys/wait.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <libgen.h>
#include <sys/stat.h>
#include "file_operations.h"
#include <dirent.h> 


#define DIR_LEN 4096


int file_exists(char *filename) {
  struct stat buffer;   
  return (stat(filename, &buffer) == 0);
}

int copy_bin(char* command, char* dest){
	if(file_exists(command)){
		return copy_file(command, dest);
	} else{
		//TODO: implement path check
		perror("Error copying file; file not found\n");
		return -1;
	}
}



int copy_file(char* src, char* dest){

	int child_status;
	pid_t pid;
	int status;

	if (!src || !dest) {
        perror("Error opening files!\n");
    	return EXIT_FAILURE;
    }

	pid = fork();

	if (pid < 0){
		perror("Error forking\n");
		return EXIT_FAILURE;
	}

	if (pid == 0) { 
		// child
        execl("/bin/cp", "/bin/cp", src, dest, (char *)0);
    } else {
		pid_t ws = waitpid( pid, &child_status, 0);
		if (ws == -1)
        { 
			perror("Error copying files\n");
			return EXIT_FAILURE;
        }

		if( WIFEXITED(child_status)) 
        {
            status = WEXITSTATUS(child_status); 
			if(status < 0){
				perror("Error copying files; Non Zero Exit\n");
				return EXIT_FAILURE;
			}
           
        }
        else if (WIFSIGNALED(child_status)) 
        {
			perror("Error copying files; killed\n");
			return EXIT_FAILURE;
        }
        else if (WIFSTOPPED(child_status)) /* stopped */
        {
			perror("Error copying files; stopped\n");
			return EXIT_FAILURE;
        }
	}


	return EXIT_SUCCESS;
}

int prepare_working_dir(char* command){
	char directory_name_template[] = "/tmp/boxXXXXXX";
	char *directory_name = mkdtemp(directory_name_template);
	
	if (directory_name == NULL) {
		perror("Error creating temp dir\n");
		return EXIT_FAILURE;
	}

	char *executable_path = command;
	char *executable_name = basename(executable_path);

	char *new_executable_path = malloc(strlen(directory_name) + strlen(executable_name) + 2);
	
	sprintf(new_executable_path, "%s/%s", directory_name, executable_name);
	
	if (copy_bin(executable_path, new_executable_path) || chdir(directory_name)) {
		free(new_executable_path);
		return EXIT_FAILURE;
	}
	
	if (chroot(directory_name)) {
		free(new_executable_path);
		return EXIT_FAILURE;
	}
	
	free(new_executable_path);
	return EXIT_SUCCESS;
}


int untar(char *file) {
  char *file_name = basename(file);
  char *path = dirname(file);
  // construct the command
  size_t size = strlen("cd ") + strlen(path) + strlen(" && tar -xf ") +
               2*strlen(file_name) + strlen(" > /dev/null") + strlen(" && rm ");
  char *command = malloc(size + 60);
  sprintf(command,"cd %s && tar -xvhzvf %s > /dev/null && rm %s",path, file_name, file_name);
//   printf("tar command: %s\n", command);
  int val = system(command);
  free(command);
  return val;
}

int makedir(char *dir) {
  size_t size = strlen("mkdir -p ") + strlen(dir);
  char *command = malloc(size + 1);
  strcpy(command, "mkdir -p ");
  strcat(command, dir);
  return system(command);
}