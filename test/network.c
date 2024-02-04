#include "network.h"
#include <curl/curl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


struct response_content {
  char *content;
  size_t size;
};

struct string {
  char *ptr;
  size_t len;
};

void init_string(struct string *s) {
  s->len = 0;
  s->ptr = malloc(s->len+1);
  if (s->ptr == NULL) {
    fprintf(stderr, "malloc() failed\n");
    exit(EXIT_FAILURE);
  }
  s->ptr[0] = '\0';
}

size_t writefunc(void *ptr, size_t size, size_t nmemb, struct string *s)
{
  size_t new_len = s->len + size*nmemb;
  s->ptr = realloc(s->ptr, new_len+1);
  if (s->ptr == NULL) {
    fprintf(stderr, "realloc() failed\n");
    exit(EXIT_FAILURE);
  }
  memcpy(s->ptr+s->len, ptr, size*nmemb);
  s->ptr[new_len] = '\0';
  s->len = new_len;

  return size*nmemb;
}


char* get_request(char* url,char *bearer_token){
    CURL *curl;
    CURLcode res;
    struct string s;
    init_string(&s);
    
    // char* content = NULL;
    // printf(bearer_token);
    printf("aaaaaaaaaaaaaaaaaaaaaaaa");
    curl = curl_easy_init();
    printf("bbbbbbbbbbbbbbbbbbbbbbbb");
    if(curl) {
        curl_easy_setopt(curl, CURLOPT_URL, url);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writefunc);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &s);
        struct curl_slist *list = NULL;
        

        if (bearer_token != NULL) {
            list = curl_slist_append(list, bearer_token);
            curl_easy_setopt(curl, CURLOPT_HTTPHEADER, list);
        }
        res = curl_easy_perform(curl);
                

        if (res != CURLE_OK) {
            perror("question");
            fprintf(stderr, "Error from curl: %i %s\n", res, curl_easy_strerror(res));
            // printf(s.ptr);
        } else {
            // content = malloc(s.len + 1);
            // memset(content,0,s.len);
            // strcpy(content, s.ptr);
            // printf(s.ptr);
        }
        // free(s.ptr);
        if (list != NULL) {
            curl_slist_free_all(list);
        }
        curl_easy_cleanup(curl);
    }
    
    return s.ptr;
}



int download_file(char *uri, char *file, char *bearer_token) {
    // printf(bearer_token);
  int result = -1;
  FILE *fptr;
  CURL *curl_handle = curl_easy_init();
  char url2[200];
  strcpy(url2, uri);
  if (curl_handle) {
    if ((fptr = fopen(file, "w")) == NULL) {
      perror("Error: Could open destination for writing");
      result = -1;
    } else {
      printf("curl: url: %s\n", url2);
      curl_easy_setopt(curl_handle, CURLOPT_URL, (void *)url2);
      curl_easy_setopt(curl_handle, CURLOPT_FOLLOWLOCATION, 1L);
      // set the headers
      struct curl_slist *list = NULL;
      if (bearer_token != NULL) {
        list = curl_slist_append(list, bearer_token);
        curl_easy_setopt(curl_handle, CURLOPT_HTTPHEADER, list);
      }
      curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, write_handler_disk);
      curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, (void *)fptr);
      curl_easy_setopt(curl_handle, CURLOPT_VERBOSE, 0);
      // printf("curl: performing operation\n");
      CURLcode res = curl_easy_perform(curl_handle);
      if (res != CURLE_OK) {
        fprintf(stderr, "curl_easy_perform() failed: %i %s\n", res,
                curl_easy_strerror(res));
      }
      if (list != NULL) {
        curl_slist_free_all(list);
      }
      fflush(fptr);
      fclose(fptr);
    }
    curl_easy_cleanup(curl_handle);
    result = 0;
  } else {
    printf("Could not download file\n");
    result = -1;
  }
  return result;
}


size_t write_handler_mem(void *data, size_t size, size_t nmemb, void *arg) {
  size_t chunk_size = size * nmemb;
  struct response_content *res = (struct response_content *)arg;
  char *ptr = realloc(res->content, res->size + chunk_size + 500);
  if (ptr == NULL) {
    perror("Error: Out of memory\n");
    return 0;
  }
  res->content = ptr;
  memcpy(&(res->content[res->size]), data, chunk_size);
  res->size += chunk_size;
  res->content[res->size] = 0;
  return chunk_size;
}


size_t write_handler_disk(void *contents, size_t size, size_t nitems, FILE *file) {
  return fwrite(contents, size, nitems, file);
}