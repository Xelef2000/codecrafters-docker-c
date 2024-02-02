#ifndef FILE_OPERATIONS
#define FILE_OPERATIONS

int copy_file(char* src, char* dest);
int copy_bin(char* command, char* dest);
int file_exist(char *filename);
int prepare_working_dir(char* command);


#endif