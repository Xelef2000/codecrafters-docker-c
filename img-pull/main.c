#define _GNU_SOURCE
#include <sys/wait.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <libgen.h>
#include <sys/stat.h>
// #include "file_operations.h"
#include <sched.h>
#include <curl/curl.h>
#include "network.h"
#include "docker-registry.h"

#define DOCKER_TOKEN_LEN 2660
#define DOCKER_TK_REQUEST_URL_LEN 87
#define DOCKER_TK_REQUEST_RESPONSE_LEN 5413
#define DOCKER_MF_REQUEST_URL_LEN 55
#define AUTH_BEARER_TXT_LEN 22
#define DOCKER_REGISTRY_IMAGES_URI "https://registry.hub.docker.com/v2"



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

    printf("    getting token\n");
    char* token = get_docker_token(&image_spec);
    if(token == NULL) {
        return -1;
    }
    printf("    getting manifest\n");
    char** layer_list = get_manifest(&image_spec,token);
    if(!layer_list){
        return -1;
    }

    int index = 0;
    int result = 0;
    while(layer_list[index] != NULL){
        printf("--> pulling %s\n", layer_list[index]);
        if (docker_get_layer(token, dir, image_spec.lib, image_spec.image, layer_list[index]) != 0) {
            result = -1;
        }
        free(layer_list[index++]);
        printf("\n");
    }

    free(layer_list);
    free(token);
    free(image_spec.image);
    free(image_spec.lib);
    free(image_spec.tag);
    return result;
}

int main(int argc, char *argv[]){
    setbuf(stdout, NULL);
    char* image1 = "busybox:1.36.0-glibc";
    char* image2 = "busybox:latest";
    char* image3 = "library/ubuntu:latest";
    char* image4 = "alpinelinux/ansible:latest";

    char directory_name_template[] = "./layers/boxXXXXXX";
	char *dir = mkdtemp(directory_name_template);

    // printf("-------- IMAGE 1 --------\n");
    // init_docker_image(image1, dir);
    // printf("-------- IMAGE 2 --------\n");
    // *dir = mkdtemp(directory_name_template);
    // init_docker_image(image2, dir);
    // printf("-------- IMAGE 3 --------\n");
    // *dir = mkdtemp(directory_name_template);
    // init_docker_image(image3, dir);
    // printf("-------- IMAGE 4 --------\n");
    // *dir = mkdtemp(directory_name_template);
    // init_docker_image(image4, dir);
    
    return 0;
}