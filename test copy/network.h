#ifndef NETWORK
#define NETWORK
#include <stdlib.h>
#include <stdio.h>


char *get_response(char *uri, char *bearer_token);
int download_file(char *uri, char *file, char *bearer_token);
size_t write_handler_mem(void *data, size_t size, size_t nmemb, void *arg);
size_t write_handler_disk(void *contents, size_t size, size_t nitems,FILE *file);


#endif