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
#include <ctype.h>
#include "json.h" // src https://github.com/sheredom/json.h/
// #include "jsmn.h" //https://raw.githubusercontent.com/zserge/jsmn/master/jsmn.h

#define DOCKER_TOKEN_LEN 2660
#define DOCKER_TK_REQUEST_URL_LEN 80
#define DOCKER_TK_REQUEST_RESPONSE_LEN 5413
#define DOCKER_MF_REQUEST_URL_LEN 48
#define AUTH_BEARER_TXT_LEN 22
#define DOCKER_REGISTRY_IMAGES_URI "https://registry.hub.docker.com/v2"

#define LAYER_ARCH "amd64"
#define LAYER_OS "linux"


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

int find_node(struct json_object_element_s* node, char* node_name){
  if(node->next == NULL){
      perror("Node not found in JSON");
      return -1;
    }
  while (strcmp(node->name->string, node_name))
  {
    if(node->next == NULL){
      perror("Node not found in JSON");
      return -1;
    }
    node = node->next;
  }
  return 1;
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

char* get_docker_token(struct docker_image_spec* img_spec){
    int request_url_len = DOCKER_TK_REQUEST_URL_LEN + strlen(img_spec->image) + strlen(img_spec->lib);
    char* url = malloc(request_url_len + 1);

    sprintf(url, "https://auth.docker.io/token?service=registry.docker.io&scope=repository:%s/%s:pull", img_spec->lib, img_spec->image);    
   

    char* response = get_request(url, NULL);

    char* token = parse_token(response);
    if(token  == NULL){
        perror("Failed to parse Token\n");
    }

    free(response);
    free(url);
    return token;
}


int manifest_schema_version(char* response_content){
  char* p = strstr(response_content,"schemaVersion");
  while(!isdigit(*p)){
    p++;
  }
  return *p - '0';
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

int docker_get_layer(char *token, char *dir, char *repo, char *image,
                     char *id) {
  size_t size = strlen(DOCKER_REGISTRY_IMAGES_URI) + strlen(repo) + strlen(image) + strlen("blobs") + strlen(id) + strlen("////");
  char *full_uri = malloc(size + 1);
  sprintf(full_uri, "https://registry.hub.docker.com/v2/%s/%s/blobs/%s",repo,image,id);


  char *file_name = make_file_from_id(id);


  size = strlen(dir) + strlen(file_name);

  char *file = malloc(size + 2);
  sprintf(file,"%s/%s",dir,file_name);
  printf("DIR: %s\n",dir);

  
  size = strlen("Authorization: Bearer ") + strlen(token);
  char *bearer_token = malloc(size + 1);
  sprintf(bearer_token,"Authorization: Bearer %s",token);

  if (download_file(full_uri, file, bearer_token) == -1) {
    printf("Error downloading layer!");
  } else {
    untar(file);
  }
  free(bearer_token);
  free(file);
  free(file_name);
  free(full_uri);
  return 0;
}


char** parse_schema_v2(char* response, char*token, struct docker_image_spec* img_spec){
  // char* p = strstr(response,"mediaType");
  // printf(response);
  struct json_value_s* root = json_parse(response, strlen(response));
  struct json_object_s* object = (struct json_object_s*)root->payload;
  struct json_object_element_s* element = object->start;
  while (strcmp(element->name->string, "manifests"))
  {
    element = element->next;
  }
  struct json_array_s* array = (struct json_array_s*)element->value->payload;
  struct json_array_element_s* array_element = array->start;
  while(1){
    struct json_object_element_s* element_sd = ((struct json_object_s*)(array_element->value->payload))->start;
    while(strcmp(element_sd->name->string, "platform"))
    {
      element_sd = element_sd->next;
    }
    struct json_object_s* plat = (struct json_object_s*)element_sd->value->payload;
    if(strcmp(((struct json_string_s*)(plat->start->value->payload))->string, "amd64") == 0 && strcmp(((struct json_string_s*)(plat->start->next->value->payload))->string, "linux") == 0 ){
      break;
    }
    array_element = array_element->next;
  }

  struct json_object_element_s* element_dg = ((struct json_object_s*)(array_element->value->payload))->start;
  while(strcmp(element_dg->name->string, "digest"))
  {
    element_dg = element_dg->next;
  }


  const char* manifest_digest = (((struct json_string_s*)(element_dg->value->payload))->string);


  int request_url_len = DOCKER_MF_REQUEST_URL_LEN + strlen(img_spec->image) + strlen(manifest_digest)+ strlen(img_spec->lib);
  char* url = malloc(request_url_len + 1);
  sprintf(url, "https://registry.hub.docker.com/v2/%s/%s/manifests/%s",img_spec->lib, img_spec->image,manifest_digest);
    
    

  char *bearer_token;
  int token_len = AUTH_BEARER_TXT_LEN + strlen(token)+1;
  bearer_token = malloc(token_len);
  sprintf(bearer_token, "Authorization: Bearer %s", token);

  
    
  
  char* img_manifest_json = get_request(url, bearer_token);
  if (img_manifest_json == NULL){
    perror("Failed to get image manifest");
    return NULL;
  }

  // printf(img_manifest_json);
  struct json_value_s* im_root = json_parse(img_manifest_json, strlen(img_manifest_json));
  struct json_object_s* im_object = (struct json_object_s*)im_root->payload;
  struct json_object_element_s* im_element = im_object->start;
  while (strcmp(im_element->name->string, "layers"))
  {
    im_element = im_element->next;
  }
  struct json_array_s* im_array = (struct json_array_s*)im_element->value->payload;
  
  int layer_count = array->length;
  char** layer_list = malloc(layer_count*sizeof(char*)+1);
  memset(layer_list,0,layer_count*sizeof(char*)+1);

  struct json_array_element_s* blob = im_array->start;
  for(int i = 0; i < layer_count; i++){
    if(blob == NULL){
      break;
    }
    struct json_object_s* sdf = (struct json_object_s*)blob->value->payload;
    // struct json_string_s* string = (struct json_string_s*)sdf->start->value->payload;
    struct json_object_element_s* layer_element = sdf->start;

    while (strcmp(layer_element->name->string, "digest"))
    {
    layer_element = layer_element->next;
    }
 
    char* hash = malloc(((struct json_string_s*)(layer_element->value->payload))->string_size);
    strcpy(hash,((struct json_string_s*)(layer_element->value->payload))->string);
    layer_list[i] = hash;
    
    blob = blob->next;
  }
  layer_list[layer_count] = NULL;

  free(url);
  free(bearer_token);
  free(im_root);
  free(root);
  free(img_manifest_json);
  return layer_list;
}

char** parse_schema_v1(char* response){
  // printf(response);
  struct json_value_s* root = json_parse(response, strlen(response));
  struct json_object_s* object = (struct json_object_s*)root->payload;
  struct json_object_element_s* element = object->start;
  while (strcmp(element->name->string, "fsLayers"))
  {
    element = element->next;
  }
  // printf(element->name->string);
  
  struct json_array_s* array = (struct json_array_s*)element->value->payload;

  int layer_count = array->length;
  char** layer_list = malloc(layer_count*sizeof(char*)+1);
  memset(layer_list,0,layer_count*sizeof(char*)+1);

  struct json_array_element_s* blob = array->start;
  for(int i = 0; i < layer_count; i++){
    if(blob == NULL){
      break;
    }
    struct json_object_s* sdf = (struct json_object_s*)blob->value->payload;
    struct json_string_s* string = (struct json_string_s*)sdf->start->value->payload;
    // printf(string ->string);
    // printf("\n");
    
    char* hash = malloc(string->string_size);
    strcpy(hash,string->string);
    layer_list[i] = hash;
    
    blob = blob->next;
  }
  layer_list[layer_count] = NULL;
  

  free(root);
  return layer_list;
}

char** get_manifest(struct docker_image_spec* img_spec, char* token){
    int request_url_len = DOCKER_MF_REQUEST_URL_LEN + strlen(img_spec->image) + strlen(img_spec->tag)+ strlen(img_spec->lib);
    char* url = malloc(request_url_len + 1);
    sprintf(url, "https://registry.hub.docker.com/v2/%s/%s/manifests/%s",img_spec->lib, img_spec->image, img_spec->tag);
    
    

    char *bearer_token;
    int token_len = AUTH_BEARER_TXT_LEN + strlen(token)+1;
    bearer_token = malloc(token_len);
    sprintf(bearer_token, "Authorization: Bearer %s", token);
    
    char** layer_list = NULL;
    
    char* response = get_request(url, bearer_token);
  
    if(strstr(response, "TOOMANYREQUESTS") == NULL){
      
      if(manifest_schema_version(response) == 1){

        //use v1 parser #looking for blobs
        layer_list = parse_schema_v1(response);
      } else{

        layer_list = parse_schema_v2(response,token, img_spec);
      }
    // printf(response);

    } else{
      perror("Rate Limited");
    }


    free(response);
    free(bearer_token);
    free(url);
    return layer_list;
}
