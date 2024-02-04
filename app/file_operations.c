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
#include "docker-registry.h"

#define DIR_LEN 4096


#define DOCKER_TOKEN_LEN 2660
#define DOCKER_TK_REQUEST_URL_LEN 87
#define DOCKER_TK_REQUEST_RESPONSE_LEN 5413
#define DOCKER_MF_REQUEST_URL_LEN 55
#define AUTH_BEARER_TXT_LEN 22
#define DOCKER_REGISTRY_IMAGES_URI "https://registry.hub.docker.com/v2"


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

int init_docker_image(char *image, char *dir) {

    struct docker_image_spec image_spec;

    //TODO Remove redundant code

    if(strchr(image, ':') == NULL && strchr(image, '/') == NULL){
       image_spec.image = malloc(strlen(image)+1);
       memset(image_spec.image,0, strlen(image)+1);
       strcpy(image_spec.image,image);

       image_spec.tag = malloc(strlen("latest")+1);
       memset(image_spec.tag,0, strlen("latest")+1);
       strcpy(image_spec.tag,"latest");

       image_spec.lib = malloc(strlen("library")+1);
       memset(image_spec.lib,0, strlen("library")+1);
       strcpy(image_spec.lib,"library");
    }

    if(strchr(image, ':') == NULL && strchr(image, '/') != NULL){
       image_spec.tag = malloc(strlen("latest")+1);
       memset(image_spec.tag,0, strlen("latest")+1);
       strcpy(image_spec.tag,"latest");

       char* libend = strchr(image, '/')+1;
       image_spec.image = malloc(strlen(libend)+1);
       memset(image_spec.image,0, strlen(libend)+1);
       strcpy(image_spec.image,libend);

       int size = libend - image;
       image_spec.lib = malloc(size);
       memset(image_spec.lib,0, size);
       strncpy(image_spec.lib, image, size -1);

    }


    if(strchr(image, ':') != NULL && strchr(image, '/') == NULL){
        char* imend = strchr(image, ':')+1;
        image_spec.tag = malloc(strlen(imend)+1);
        memset(image_spec.tag,0, strlen(imend)+1);
        strcpy(image_spec.tag,imend);

        int size = imend - image;
        image_spec.image = malloc(size);
        memset(image_spec.image,0, size);
        strncpy(image_spec.image, image, size -1);

        image_spec.lib = malloc(strlen("library")+1);
        memset(image_spec.lib,0, strlen("library")+1);
        strcpy(image_spec.lib,"library");
    }

    if(strchr(image, ':') != NULL && strchr(image, '/') != NULL){
       char* libend = strchr(image, '/')+1;
       int size = libend - image;
       image_spec.lib = malloc(size);
       memset(image_spec.lib,0, size);
       strncpy(image_spec.lib, image, size -1);

       char* imend = strchr(libend, ':')+1;
       image_spec.tag = malloc(strlen(imend)+1);
       memset(image_spec.tag,0, strlen(imend)+1);
       strcpy(image_spec.tag,imend);

       size = imend - libend;
       image_spec.image = malloc(size);
       memset(image_spec.image,0, size);
       strncpy(image_spec.image, libend, size -1);
       

    }

    // printf("    getting token\n");
    char* token = get_docker_token(&image_spec);
    if(token == NULL) {
        return -1;
    }
    // printf("    getting manifest\n");
    char** layer_list = get_manifest(&image_spec,token);
    if(!layer_list){
        return -1;
    }

    int index = 0;
    int result = 0;
    while(layer_list[index] != NULL){
        // printf("--> pulling %s\n", layer_list[index]);
        if (docker_get_layer(token, dir, image_spec.lib, image_spec.image, layer_list[index]) != 0) {
            result = -1;
        }
        free(layer_list[index++]);
        // printf("\n");
    }

    free(layer_list);
    free(token);
    free(image_spec.image);
    free(image_spec.lib);
    free(image_spec.tag);
    return result;
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

int prepare_working_dir(char* command, char* img){
	char directory_name_template[] = "/tmp/boxXXXXXX";
	char *directory_name = mkdtemp(directory_name_template);
	
	if (directory_name == NULL) {
		perror("Error creating temp dir\n");
		return EXIT_FAILURE;
	}

	if (init_docker_image(img,directory_name) < 0) {
		perror("Error Pulling img\n");
		return EXIT_FAILURE;
	}
	

	char *executable_path = command;
	char *executable_name = basename(executable_path);

	char *new_executable_path = malloc(strlen(directory_name) + strlen(executable_name) + 2);
	
	sprintf(new_executable_path, "%s/%s", directory_name, executable_name);
	
	// if (copy_bin(executable_path, new_executable_path) || chdir(directory_name)) {
	// 	free(new_executable_path);
	// 	return EXIT_FAILURE;
	// }
	
	if (chroot(directory_name)) {
		free(new_executable_path);
		return EXIT_FAILURE;
	}
	
	free(directory_name);
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