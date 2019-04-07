#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <string.h>
#include <dirent.h>


ino_t  find_inode        (char* name);
char * find_current_dirname();

int main(void){
    int level = 0;
    char** path_str  = NULL;
    char* path_next = "";
    for(;;)
    {
        path_next = find_current_dirname();
        //printf("pwd_next    %s\n",pwd_next);
        if(path_next == NULL) 
            break;
        path_str = realloc(path_str, sizeof(char*)*(++level));
        path_str[level - 1] = malloc(sizeof(char)*(strlen(path_next) + 1));
        strcpy(path_str[level-1], path_next);

        chdir("..");
    }
    for (int i = level -1; i >= 0; i--)
        printf("/%s",path_str[i]);

    printf("\n");
    return 0;
}


ino_t find_inode(char *file) 
{
    struct stat statbuf;
    int ret_val = lstat(file, &statbuf);
    if (ret_val < 0) 
    {
        printf("Cannot stat %s",file);
        exit(-1);
    }
    return statbuf.st_ino;
}


char * find_current_dirname()
{
    DIR *dirp;
    struct dirent *dp;
    int current_inode = find_inode(".");
    if(current_inode == find_inode(".."))
        return NULL;
    
    if ((dirp = opendir("..")) == NULL) 
    {
        printf("Cannot open ..");
        exit(-1);
    }
    while ((dp = readdir(dirp)) != NULL) 
    {
        if(dp->d_ino == current_inode)
        {
            closedir(dirp);
            return dp->d_name;
        }
    }

    return NULL;
}
