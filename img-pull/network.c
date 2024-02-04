#include "network.h"
#include <curl/curl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


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

    char* h1 = "Accept: application/vnd.docker.distribution.manifest.v2+json";
    char* h2 = "Accept: application/vnd.docker.distribution.manifest.list.v2+json";
    char* h3 = "Accept: application/vnd.docker.distribution.manifest.v1+json";
    char* h4 = "Accept: application/vnd.oci.image.manifest.v1+json";
    
    
    curl = curl_easy_init();
    

    if(curl) {
        curl_easy_setopt(curl, CURLOPT_URL, url);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writefunc);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &s);
        struct curl_slist *list = NULL;
        list = curl_slist_append(list,h1);
        list = curl_slist_append(list,h2);
        list = curl_slist_append(list,h3);
        list = curl_slist_append(list,h4);

        if (bearer_token != NULL) {
            list = curl_slist_append(list, bearer_token);
        }
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, list);
        res = curl_easy_perform(curl);
                

        if (res != CURLE_OK) {
            perror("question");
            fprintf(stderr, "Error from curl: %i %s\n", res, curl_easy_strerror(res));
        } 
        if (list != NULL) {
            curl_slist_free_all(list);
        }
        curl_easy_cleanup(curl);
    }
    
    return s.ptr;
}


size_t write_handler_disk(void *contents, size_t size, size_t nitems, FILE *file) {
  return fwrite(contents, size, nitems, file);
}
// int download_file(char *uri, char *file, char *bearer_token) {
//     CURL *curl;
//     FILE *fp;
//     CURLcode res;

//     curl = curl_easy_init();

//     if (curl) {

//         struct curl_slist *list = NULL;

//         list = curl_slist_append(list, bearer_token);
//         curl_easy_setopt(curl, CURLOPT_HTTPHEADER, list);

//         fp = fopen(file,"w");

//          if ((fp = fopen(file, "w")) == NULL) {
//           perror("Error: Could open destination for writing\n");
//           printf(file);
//           return -1;
//         }


//         curl_easy_setopt(curl, CURLOPT_URL, uri);
//         curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_handler_disk);
//         curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);
//         res = curl_easy_perform(curl);

//         if (list != NULL) {
//             curl_slist_free_all(list);
//         }

//         curl_easy_cleanup(curl);


//         fflush(fp);
//         fclose(fp);

//     }
//     return 0;
// }

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
