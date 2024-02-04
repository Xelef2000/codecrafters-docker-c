#ifndef DOCKER_REG
#define DOCKER_REG

struct docker_image_spec{
    char* lib;
    char* image;
    char* tag;
};

char* parse_token(char* response);
char* get_docker_token(struct docker_image_spec* img_spec);
int countString(const char *haystack, const char *needle);
char** get_manifest(struct docker_image_spec* img_spec, char* token);
int docker_get_layer(char *token, char *dir, char *repo, char *image,char *id);
#endif