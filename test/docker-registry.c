#include <stdio.h>
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



int print_layer_annotations(struct layer_annotations* layer){
  if(!layer){
    return -1;
  }
  printf("Digest: %s\nArch: %s\nOS: %s\n",layer->digest,layer->architecture,layer->os);
  return 0;
}

int free_layer_annotations(struct layer_annotations* layer){
  free(layer->digest);
  free(layer->architecture);
  free(layer->os);
  free(layer);
  return 0;
}

int docker_get_layer(char *token, char *dir, char *repo, char *image,
                     char *id) {
  size_t size = strlen(DOCKER_REGISTRY_IMAGES_URI) + strlen(repo) +
                strlen(image) + strlen("blobs") + strlen(id) + strlen("////");
  char *full_uri = malloc(size + 1);
  strcpy(full_uri, DOCKER_REGISTRY_IMAGES_URI);
  strcat(full_uri, "/");
  strcat(full_uri, repo);
  strcat(full_uri, "/");
  strcat(full_uri, image);
  strcat(full_uri, "/");
  strcat(full_uri, "blobs");
  strcat(full_uri, "/");
  strcat(full_uri, id);

  // printf("sss");
  char *file_name = make_file_from_id(id);
  // printf("sss");

  size = strlen(dir) + strlen(file_name);
  char *file = malloc(size + 2);
  strcpy(file, dir);
  strcat(file, "/");
  strcat(file, file_name);
  size = strlen("Authorization: Bearer ") + strlen(token);
  char *bearer_token = malloc(size + 1);
  strcpy(bearer_token, "Authorization: Bearer ");
  strcat(bearer_token, token);
  // printf("full uri: %s\n", full_uri);
  // printf("filename: %s\n", file);
  if (download_file(full_uri, file, bearer_token) == -1) {
    printf("Error downloading layer!");
  } else {
    // untar the file and delete the original
    untar(file, 1, 1);
  }
  free(file_name);
  free(full_uri);
  return 0;
}

char *make_file_from_id(char *id) {
  // layer id's look like this:
  // sha256:a3ed95caeb02ffe68cdd9fd84406680ae93d633cb16422d00e8a7c22955b46d4
  // sha256:538721340ded10875f4710cad688c70e5d0ecb4dcd5e7d0c161f301f36f79414
  // our file name will be everything after the :
  char *file = malloc(strlen(id));
  char *p = strstr(id, ":");
  strcpy(file, p + 1);
  return file;
}

int countString(const char *haystack, const char *needle){
    int count = 0;
    const char *tmp = haystack;
    while((tmp = strstr(tmp, needle)))
    {
        count++;
        tmp++;
    }
    return count;
}

struct layer_annotations** parse_layers(char *response_content) {
  if (response_content == NULL) {
    return NULL;
    perror("GOT NULL PTR");
  }
  int list_size = countString(response_content, "annotations");
  struct layer_annotations** layer_list = malloc((list_size * sizeof(struct layer_annotations*)) + 1);

  int index = 0;
  layer_list[0] = NULL;
  int size = 0;
  char* current_position = response_content;

  while(1){
    char* slayer = strstr(current_position, "annotations");
    if (slayer == NULL) {
      if(index==0){
        perror("Empty Response");
      }
      break;
    }

    struct layer_annotations* layer = malloc(sizeof(struct layer_annotations));
    

    char* sdigest = strstr(slayer, "digest")+9;
    char* edigest = strstr(sdigest, "\"");
    size = (edigest - sdigest)+1;
    char* digest =  malloc(size);
    memset(digest, 0, size);
    strncpy(digest, sdigest, size - 1);
    
    layer->digest=digest;
  
    char* sarch = strstr(slayer, "architecture")+15;
    char* earch = strstr(sarch, "\"");
    size = (earch - sarch) + 1;
    char* arch = malloc(size);
    memset(arch, 0, size);
    strncpy(arch,sarch, size -1);

    layer->architecture=arch;

    char* sos = strstr(slayer, "os")+5;
    char* eos = strstr(sos, "\"");
    size = (eos - sos) + 1;
    char* os = malloc(size);
    memset(os, 0, size);
    strncpy(os,sos, size -1);

    layer->os=os;

    // print_layer_annotations(layer);
    current_position = slayer+12;
    layer_list[index] = layer;
    index++;
    layer_list[index] = NULL;
  }
  

  return layer_list;
}

struct layer_annotations** parse_layers_compat(char *response_content) {
  if (response_content == NULL) {
    return NULL;
    perror("GOT NULL PTR");
  }
  printf(response_content);
  int list_size = countString(response_content, "blobSum");
  struct layer_annotations** layer_list = malloc((list_size * sizeof(struct layer_annotations*)) + 1);
  int size = 0;

  int index = 0;
  layer_list[0] = NULL;
  
  char* current_position = response_content;

  while(1){
    char* slayer = strstr(current_position, "blobSum");
    if (slayer == NULL) {
      if(index==0){
        perror("Empty Response");
      }
      break;
    }

    struct layer_annotations* layer = malloc(sizeof(struct layer_annotations));
    

    char* sdigest = strstr(slayer, "blobSum")+11;
    char* edigest = strstr(sdigest, "\"");
    size = (edigest - sdigest)+1;
    char* digest =  malloc(size);
    memset(digest, 0, size);
    strncpy(digest, sdigest, size - 1);
    
    layer->digest=digest;
  

    layer->architecture=NULL;

   

    layer->os=NULL;

    print_layer_annotations(layer);
    current_position = edigest;
    layer_list[index] = layer;
    index++;
    layer_list[index] = NULL;
  }
  

  return layer_list;
}



char* parse_token(char* response){
    if (response == NULL) {
        return NULL;
    }

    char *token_start = strstr(response, "token");
    if (token_start == NULL) {
        return NULL;
    }
    //move pointer to start of token
    token_start += 8;
    
    char *token_end = strstr(token_start, "\"");
    if (token_end == NULL) {
        return NULL;
    }
    int size = token_end - token_start;
    char* token = malloc(size+1);
    memset(token, 0, size+1);
    strncpy(token, token_start,size);

    return token;
}

char* get_docker_token(char* scope){
    int request_url_len = DOCKER_TK_REQUEST_URL_LEN + strlen(scope);
    char* url = malloc(request_url_len + 1);
    sprintf(url, "https://auth.docker.io/token?service=registry.docker.io&scope=repository:library/%s:pull", scope);
    url[request_url_len] = '\0';
    char* response = get_request(url, NULL);

    char* token = parse_token(response);
    if(token  == NULL){
        perror("Failed to parse Token\n");
    }

    free(response);
    free(url);
    return token;
}



struct layer_annotations** get_manifest(char* image, char* tag, char* token){
    int request_url_len = DOCKER_MF_REQUEST_URL_LEN + strlen(image) + strlen(tag);
    char* url = malloc(request_url_len + 1);
    sprintf(url, "https://registry.hub.docker.com/v2/library/%s/manifests/%s", image, tag);
    
    

    char *bearer_token;
    int token_len = AUTH_BEARER_TXT_LEN + strlen(token);
    bearer_token = malloc(token_len);
    sprintf(bearer_token, "Authorization: Bearer %s", token);
    // printf(url);
    // printf(bearer_token);

    
    char* response = get_request(url, bearer_token);
    printf(response);

   
    struct layer_annotations** layer_list;
    layer_list = parse_layers_compat(response);
  

    free(response);
    free(bearer_token);
    free(url);
    return layer_list;
}

