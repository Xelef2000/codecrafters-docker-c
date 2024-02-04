#ifndef DOCKER_REG
#define DOCKER_REG

struct layer_annotations
{
    char* digest;
    char* architecture;
    char* os;
};



int docker_get_layer(char *token, char *dir, char *repo, char *image, char *id);
char *make_file_from_id(char *id);
char **add_string_to_array(char **array, int *size, const char *string);
struct layer_annotations** parse_layers(char *response_content);
char* parse_token(char* response);
char* get_docker_token(char* scope);
struct layer_annotations** get_manifest(char* image, char* tag, char* token);
int print_layer_annotations(struct layer_annotations* layer);
int free_layer_annotations(struct layer_annotations* layer);
int countString(const char *haystack, const char *needle);


#endif