#ifndef DOCKER_REG
#define DOCKER_REG

int docker_get_layer(char *token, char *dir, char *repo, char *image, char *id);
char *make_file_from_id(char *id);
char **add_string_to_array(char **array, int *size, const char *string);
char **parse_layers(char *response_content);
int parse_token(char* response, char* token);
int get_docker_token(char* scope, char* token);
char** get_manifest(char* image, char* tag, char* token);
char **docker_enumerate_layers(char *token, char *repo, char *image,
                               char *tag);
#endif