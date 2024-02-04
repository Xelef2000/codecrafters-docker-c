#define _GNU_SOURCE
#include <sys/wait.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <libgen.h>
#include <sys/stat.h>
#include "file_operations.h"
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



int init_docker_image(char *image, char* tag, char *dir) {

    char* token = get_docker_token(image);
    if(token == NULL) {
        return -1;
    }

    struct layer_annotations** layer_list;
    if ((layer_list =  get_manifest(image,tag,token)) == NULL) {
        return -1;
    }

    // printf("\n");
    // print_layer_annotations(layer_list[0]);
    int result = 0;
    int index = 0;




    while (1) {
        struct layer_annotations* id = layer_list[index];
        if (id == NULL) {
            // printf("null");
            break;
        }

        printf("--> pulling %s\n", id->digest);
        if (docker_get_layer(token, dir, "library", image, id->digest) != 0) {
            result = -1;
        }
    // free our id memory
        free_layer_annotations(layer_list[index++]);
    }

    free(layer_list);
    free(token);
    return 0;
}

int main(int argc, char *argv[]){
    setbuf(stdout, NULL);
    char* image = "busybox";
    char* tag = "latest";
    char* dir = "./layers";

    init_docker_image(image, tag, dir);
    
    return 0;
}