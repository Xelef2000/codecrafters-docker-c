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
    


//   printf(image);
  // 1. get the auth token
  char token[DOCKER_TOKEN_LEN+1];
  if ((get_docker_token(image, token)) < 0) {
    return -1;
  }
//   printf(token);
//   printf("ssssssssssss");
//   get_manifest(image, tag, token);
  // 2. pull the image manifest and extract the layers 
  char **layer_ids;
  if ((layer_ids =  get_manifest(image,tag,token)) == NULL) {
    return -1;
  }

// //   and finally download the layer blobs
//   int result = 0;
//   int index = 0;
//   while (1) {
//     char *id = layer_ids[index];
//     // printf(id);
//     if (id == NULL) {
//         printf("null");
//       break;
//     }
//     printf("--> pulling %s\n", id);
//     // if (docker_get_layer(token, dir, "library", image, id) != 0) {
//     //   result = -1;
//     // }
//     // free our id memory
//     free(layer_ids[index++]);
//   }

//   free(layer_ids);
//   return result;
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