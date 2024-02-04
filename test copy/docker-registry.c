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
  char *file_name = make_file_from_id(id);
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

char **add_string_to_array(char **array, int *size, const char *string) {
  char **new_arr = realloc(array, (*size + 1) * sizeof(char *));
  new_arr[*size] = malloc(strlen(string) + 1);
  strcpy(new_arr[*size], string);
  *size += 1;
  return new_arr;
}

char **parse_layers(char *response_content) {
  // printf(response_content);
  if (response_content == NULL) {
    return NULL;
  }
  char **list = NULL;
  int list_size = 0;
  while (1) {
    // find the open segment
    char *pstart = strstr(response_content, "digest");
    if (pstart == NULL) {
      break;
    }
    // find the closing tag
    char *pend = strstr(pstart + 9, "\"");
    if (pend == NULL) {
      break;
    }
    int size = 72;
    char *id = malloc(size + 1);
    memset(id, 0, size + 1);
    strncpy(id, pstart + 9, size - 10);
    list = add_string_to_array(list, &list_size, id);
        // printf(id);

    // move the response content pointer on
    response_content = pend;
  }
  // indicate the end of our list with a NULL
  list = add_string_to_array(list, &list_size, "end");
  list[list_size - 1] = NULL;

  return list;
}


int parse_token(char* response, char* token){
    if (response == NULL) {
        token = NULL;
        return -1;
    }

    char *token_start = strstr(response, "token");
    if (token_start == NULL) {
        return -1;
    }
    //move pointer to start of token
    token_start += 8;
    char *token_end = strstr(token_start, "\"");
    if (token_end == NULL) {
        return -1;
    }
    
    int size = token_end - token_start;
    strncpy(token, token_start,size);

    return 0;
}

int get_docker_token(char* scope, char* token){
    int request_url_len = DOCKER_TK_REQUEST_URL_LEN + strlen(scope);
    char* url = malloc(request_url_len + 1);
    sprintf(url, "https://auth.docker.io/token?service=registry.docker.io&scope=repository:library/%s:pull", scope);
    url[request_url_len] = '\0';
    char* response;
    response = get_response(url, NULL);

    if(parse_token(response, token) < 0){
        perror("Failed to parse Token\n");
        return EXIT_FAILURE;
    }
    // printf(response);
    free(response);
    free(url);
    return EXIT_SUCCESS;
}

char** get_manifest(char* image, char* tag, char* token){
    int request_url_len = DOCKER_MF_REQUEST_URL_LEN + strlen(image) + strlen(tag);
    char* url = malloc(request_url_len + 1);
    sprintf(url, "https://registry.hub.docker.com/v2/library/%s/manifests/%s", image, tag);
    
    char *bearer_token;
    int token_len = AUTH_BEARER_TXT_LEN + strlen(token);
    bearer_token = malloc(token_len + 1);
    sprintf(bearer_token, "Authorization: Bearer %s", token);

    char* response;
    response = get_response(url, bearer_token);
    printf(response);

    char** layer_ids;
    layer_ids = parse_layers(response);
  

    free(response);
    free(bearer_token);
    free(url);
    return 0;
}

char **docker_enumerate_layers(char *token, char *repo, char *image,
                               char *tag) {
  char **layer_ids = NULL;
  size_t size = strlen(DOCKER_REGISTRY_IMAGES_URI) + strlen(repo) +
                strlen(image) + strlen(tag) + strlen("////") +
                strlen("manifests");
  char *full_uri = malloc(size + 1);
  strcpy(full_uri, DOCKER_REGISTRY_IMAGES_URI);
  strcat(full_uri, "/");
  strcat(full_uri, repo);
  strcat(full_uri, "/");
  strcat(full_uri, image);
  strcat(full_uri, "/");
  strcat(full_uri, "manifests");
  strcat(full_uri, "/");
  strcat(full_uri, tag);
  char *bearer_token = NULL;
  if (token != NULL) {
    size = strlen("Authorization: Bearer ") + strlen(token);
    bearer_token = malloc(size + 1);
    strcpy(bearer_token, "Authorization: Bearer ");
    strcat(bearer_token, token);
  }
  char *content;
  if ((content = get_response(full_uri, bearer_token)) != NULL) {
    // printf(content);
    layer_ids = parse_layers(content);
    free(content);
  }
  if (bearer_token != NULL) {
    free(bearer_token);
  }
  // puts(full_uri);
  return layer_ids;
}