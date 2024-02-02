#include <stdio.h>
#include <sys/wait.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <libgen.h>
#include <sys/stat.h>
#include "file_operations.h"

int main(int argc, char *argv[])
{
    char *src = "docker-explorer";
    char *dest = "dest";
    char pwd[400];
    getcwd(pwd, 400);
    printf(pwd);
    copy_bin(src, dest);

    
    return 0;
}