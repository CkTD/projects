#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <termios.h>
#include <sys/ioctl.h>
#include <time.h>
#include <pwd.h>
#include <grp.h>
#include <limits.h>

#ifndef PATH_MAX 

#define PATH_MAX 8192

#endif

void useage()
{
    printf("useage:  ls [-l] [-a] [-R] [-h] [-i] pathname...\n");
}


typedef struct
{
    int a;
    int l;
    int R;
    int i;
    int no;
    char **files;
    int ws_col;
} config_t;

config_t config; /* global config */



static char* optString = "laRih?";
void get_config(int argc, char *argv[])
{
   
    memset(&config, 0, sizeof(config));
    struct winsize size;
    ioctl(STDIN_FILENO, TIOCGWINSZ, (char*)&size);
    config.ws_col = size.ws_col;

    int opt, index;
    while ((opt = getopt(argc,argv,optString)) != -1)
    {
        switch (opt)
        {
            case 'a':
                config.a = 1;
                break;
            case 'l':
                config.l = 1;
                break;
            case 'R':
                config.R = 1;
                break;
            case 'i':
                config.i = 1;
                break;
            case 'h':
            case '?':
                useage();
                exit(0);
                break;
        }
    }
    
    for (index = optind; index<argc; index++)
    {
        config.no++;
        config.files = realloc(config.files,sizeof(char*)*config.no);
        config.files[config.no-1] = argv[index];
    }

    if (config.no == 0)
    {
        config.no = 1;
        config.files = malloc(sizeof(char*));
        config.files[0] = ".";
    }
}




static char *link_color = "\033[1;36m";
static char *dir_color = "\033[1;34m";
static char *exec_color = "\033[1;32m";
static char *end_color = "\033[0m";
static char *no_color = "\033[0;37m";
static char *notexistlink_color = "\033[0;31m";


typedef struct 
{
    char *inode;
    char *mode;
    char *nlink;
    char *size;
    char *group;
    char *user;
    char *time;
    char *name;
    char *sylink;
    char *name_color;
    char *link_color;
    int nl;
} info;

info **infos = NULL;
int  infos_no = 0;
int  infos_size = 0;


info* alloc_info()
{
    infos_no++;
    if (infos_no > infos_size)
    {
        infos_size = infos_no *2;
        infos = realloc(infos, sizeof(info*)*infos_size);
    }
    infos[infos_no - 1] = malloc(sizeof(info));
    memset(infos[infos_no - 1], 0, sizeof(info));
    return infos[infos_no - 1];
}

void clear_info()
{
    int   n;
    info  *ifp;
    for (n = 0; n < infos_no; n++)
    {
        ifp = infos[n];

        free(ifp->inode);
        free(ifp->mode);
        free(ifp->nlink);
        free(ifp->size);
        free(ifp->group);
        free(ifp->user);
        free(ifp->time);
        free(ifp->name);
        free(ifp->sylink);
        free(ifp);
    }
    infos_no = 0;
}

/* sort the files in info buff */
void sortit()
{
    int i, j;
    info *ifp;
    for (i = 0; i < infos_no - 1; i++)
        for (j = 0;j < infos_no - 1 -i; j++)
            if ((strcmp(infos[j]->name, infos[j + 1]->name)) > 0)
            {
                ifp = infos[j];
                infos[j] = infos[j + 1];
                infos[j + 1] = ifp;
            }
}


#define MAX_COLUMN 16
void show_short()
{
    int i, j, s, e;
    
    int n;                /* column numbers */
    int sum;              /* total windows columns needed */
    
    int npc = 1;          /* numebr of files per column but last */
    int rmd = 0;          /* number of files in last column */
    int lpc[MAX_COLUMN];  /* max file name length in per column */
    

    n = ((infos_no > MAX_COLUMN)?MAX_COLUMN:infos_no);
    for (; ; n--)
    {
        /* find out rmd and npc for current n */
        if ((infos_no % n)== 0)
            rmd = 0;
        else
            rmd = n - (infos_no % n);        
        npc = (infos_no + rmd) / n;
        rmd = npc - rmd;

        if (rmd <= 0 ) continue; /* can't organise them in n column */
        
        /* check name length, make sure screen can hold them */
        sum = 0;
        for (i = 0; i < n; i++)
        {
            int max = 0;
            s = i * npc;
            e = s + ((i==(n-1))?rmd:npc);
            for (j = s; j < e; j++)
                if (infos[j]->nl > max)
                    max = infos[j]->nl;
            
            lpc[i] = max;
            sum += (max + 2);
        }
        if ((config.ws_col) < sum && (n != 1)) continue; /* names too long */
        
        break;  /* find it! */
    }
    
    /* show */
    for (i = 0; i < npc; i++)
    {
        int co = 0;
        for(j = i; j < infos_no; j += npc)
        {
            char format[16];
            sprintf(format,"%%s%%-%ds%%s",lpc[co]+2);
            printf(format, infos[j]->name_color, infos[j]->name, end_color);
            co++;
        }
        printf("\n");
    }
}

void show_long()
{
    int  i, n;
    info *ifp;
    
    int l_inode = 0;
    int l_mode = 10;
    int l_nlink = 0;
    int l_size = 0;
    int l_group = 0;
    int l_user = 0;
    int l_time = 0;
    char format[512];
        
    for (i = 0; i < infos_no; i++)
    {
        n = strlen(infos[i]->inode);
        if (n > l_inode)
            l_inode = n;
        n = strlen(infos[i]->mode);
        if (n > l_mode)
            l_mode = n;
        n = strlen(infos[i]->nlink);
        if (n > l_nlink)
            l_nlink = n;
        n = strlen(infos[i]->size);
        if (n > l_size)
            l_size = n;
        n = strlen(infos[i]->group);
        if (n > l_group)
            l_group = n;
        n = strlen(infos[i]->user);
        if (n > l_user)
            l_user = n;
        n = strlen(infos[i]->time);
        if (n > l_time)
            l_time = n;
    }
        
    sprintf(format, "%%%ds %%%ds %%%ds "
                    "%%%ds %%%ds %%%ds "
                    "%%s%%s%%s %%s%%s%%s\n",
                    l_mode, l_nlink, l_user,
                    l_group, l_size, l_time);
    for (i = 0; i < infos_no; i++)
    {
        ifp = infos[i];
        if(config.i)
        {
            char inodeformat[8];
            sprintf(inodeformat,"%%%ds ",l_inode);
            printf(inodeformat,ifp->inode);
        }
        printf(format, 
                ifp->mode, ifp->nlink, 
                ifp->user, ifp->group, ifp->size,ifp->time, 
                ifp->name_color, ifp->name, end_color, 
                (ifp->sylink==NULL)?"":ifp->link_color,
                (ifp->sylink==NULL)?"":ifp->sylink,
                (ifp->sylink==NULL)?"":end_color);
    }
}

/* print to screen all files in buffer and clear it */
void showDetail()
{
    if (!infos_no)
        return;

    sortit();

    if(config.l)
        show_long();
    else
        show_short();

    clear_info();
}

/* add a file to buffer */
int addDetail(const char *file, const struct stat *statbuf)
{
    info *ifp = alloc_info();

    int n, show_link;
    char c;
    char buf[PATH_MAX] = "";
    char *cp;
    struct stat reallinkstatbuf;
   
    show_link = 0;

    if(config.l)
    {
        /* file type */
        ifp->mode = malloc(sizeof(char) * 11);
        
        if(S_ISREG(statbuf->st_mode))
            c = '-';
        else if (S_ISDIR(statbuf->st_mode))
            c = 'd';
        else if (S_ISCHR(statbuf->st_mode))
            c = 'c';
        else if (S_ISBLK(statbuf->st_mode))
            c = 'b';
        else if (S_ISFIFO(statbuf->st_mode))
            c = 'f';
        else if (S_ISLNK(statbuf->st_mode))
            c = 'l';
        else if (S_ISSOCK(statbuf->st_mode))
            c = 's';
        else
            c = '?';
        ifp->mode[0] = c;


        /* mode permissions */
        if (S_IRUSR & statbuf->st_mode)
            ifp->mode[1] = 'r';
        else
            ifp->mode[1] = '-';
        if (S_IWUSR & statbuf->st_mode)
            ifp->mode[2] = 'w';
        else
            ifp->mode[2] = '-';
        if (S_IXUSR & statbuf->st_mode)
            ifp->mode[3] = 'x';
        else
            ifp->mode[3] = '-';
        if (S_IRGRP & statbuf->st_mode)
            ifp->mode[4] = 'r';
        else
            ifp->mode[4] = '-';
        if (S_IWGRP & statbuf->st_mode)
            ifp->mode[5] = 'w';
        else
            ifp->mode[5] = '-';
        if (S_IXGRP & statbuf->st_mode)
            ifp->mode[6] = 'x';
        else
            ifp->mode[6] = '-';
        if (S_IROTH & statbuf->st_mode)
            ifp->mode[7] = 'r';
        else
            ifp->mode[7] = '-';
        if (S_IWOTH & statbuf->st_mode)
            ifp->mode[8] = 'w';
        else
            ifp->mode[8] = '-';
        if (S_IXOTH & statbuf->st_mode)
            ifp->mode[9] = 'x';
        else
            ifp->mode[9] = '-';

        ifp->mode[10] = 0;

        /* inode */
        n = sprintf(buf, "%ld", statbuf->st_ino);
        buf[n] = 0;
        ifp->inode = malloc(sizeof(char)*(strlen(buf) + 2));
        strcpy(ifp->inode, buf);


        /* number of links */
        n = sprintf(buf, "%ld", statbuf->st_nlink);
        buf[n] = 0;
        ifp->nlink = malloc(sizeof(char)*(strlen(buf) + 1));
        strcpy(ifp->nlink, buf);

        /* user/group ID */
        cp = getpwuid(statbuf->st_uid)->pw_name;
        ifp->user = malloc(sizeof(char)*(strlen(cp) + 1));
        strcpy(ifp->user, cp);

        cp = getgrgid(statbuf->st_gid)->gr_name;
        ifp->group = malloc(sizeof(char)*(strlen(cp) + 1));
        strcpy(ifp->group, cp);

        /* size in bytes */
        n = sprintf(buf, "%ld", statbuf->st_size);
        ifp->size = malloc(sizeof(char)*(strlen(buf) + 1));
        strcpy(ifp->size, buf);
       
        /* mtime */
        n = sprintf(buf,"%s",ctime(&(statbuf->st_mtime)));
        ifp->time = malloc(sizeof(char)*(strlen(buf) + 1));
        strcpy(ifp->time, buf);
        ifp->time[n - 1] = 0;
    }

    if (S_ISLNK(statbuf->st_mode))
    {
        ifp->name_color = link_color;
        show_link = 1;
        
        if(config.l)
        {
            n = readlink(file, buf, PATH_MAX);
            if (n < 0)
            {
                printf("err can't read link for [%s], %s\n",file, strerror(errno));
                show_link = 0;
            }
            else
            {
                /* get symbolic link file type */
                buf[n] = 0;
                n = lstat(buf, &reallinkstatbuf);
                if (n != 0)
                {
                    printf("err can't read stat for [%s], %s\n",file, strerror(errno));
                    show_link = 0;
                }
                else
                {
                    if (S_ISLNK(reallinkstatbuf.st_mode))
                        ifp->link_color = link_color;
                    else if (S_ISDIR(reallinkstatbuf.st_mode))
                        ifp->link_color = dir_color;
                    else if (S_IXUSR & reallinkstatbuf.st_mode)
                        ifp->link_color = exec_color;
                    else
                        ifp->link_color = no_color; 
                }

                /* file symbolic link to */
                if (show_link)
                {    
                    n = strlen(buf) + 4;
                    ifp->sylink = malloc(sizeof(char)*(n));

                    sprintf(ifp->sylink, "-> %s",buf);
                }
            }
        }
    }
    else if (S_ISDIR(statbuf->st_mode))
        ifp->name_color = dir_color;
    else if (S_IXUSR & statbuf->st_mode)
        ifp->name_color = exec_color;
    else
        ifp->name_color = no_color; 


    /* file name */
    n = strlen(file);
    ifp->nl = n;
    ifp->name = malloc(sizeof(char)*(n + 1));
    ifp->name[n] = 0;
    sprintf(ifp->name,"%s", file);
    return 0;
}



char namebuf[PATH_MAX];     /* make up full path name */


int doDetail_single(const char *file, const struct stat *statbuf)
{
    addDetail(file, statbuf);
    showDetail();
    return 0;
}

int doLs(const char *file)
{
    struct stat     statbuf;
    struct dirent   *deptr;
    DIR             *dptr;
    int             rv,n;

    rv = lstat(file, &statbuf);
    if (rv != 0)
    {
        printf("error: can't get info of path %s, %s\n",file,strerror(errno));
        return -1;
    }

    // not directory
    if (!S_ISDIR(statbuf.st_mode))
        return doDetail_single(file, &statbuf);
    
    // directory
    if(config.R || config.no > 1)
        printf("%s:\n", file);

    dptr = opendir(file);
    if (dptr == NULL)
    {
        printf("error: can't open dir %s, %s\n",file,strerror(errno));
        return -1;
    }

    
    char ** dirnms = NULL;             /* all sub-dirs name */
    int dirnms_no = 0;                 /* sub-dirs number */
    int dirnmsize = 0;                 /* name list size */

    int notroot = 1;
    if(strcmp(file,"/") == 0)
        notroot = 0;
    while ((deptr = readdir(dptr)) != NULL)
    {
        strcpy(namebuf, file);
        if(notroot)
            namebuf[strlen(file)] = '/';
        strcpy(namebuf + strlen(file) + notroot, deptr->d_name);
        
        rv = lstat(namebuf, &statbuf);
        if (rv != 0)
        {
            printf("error: can't get info of file %s, %s\n",file,strerror(errno));
            return -1;
        }
      
        if ((deptr->d_name[0] == '.') && (!config.a))
            continue;

        addDetail(deptr->d_name, &statbuf);    /* ls every files and sub-dirs in current directory */

        if (config.R)  /* save all sub-dir name to dirnms for recursiev */
        {
            if ((strcmp(deptr->d_name,".")==0)||(strcmp(deptr->d_name,"..")==0))
                continue;
            
            if (S_ISDIR(statbuf.st_mode))
            {
                dirnms_no++;
                if (dirnms_no > dirnmsize)
                {
                    dirnmsize = dirnms_no * 2;
                    dirnms = realloc(dirnms,sizeof(char*) * dirnmsize);
                }
                dirnms[dirnms_no - 1] = malloc(sizeof(char)*(strlen(namebuf) + 1));
                strcpy(dirnms[dirnms_no - 1], namebuf);
            }
        }
    }
   
    showDetail();

    closedir(dptr);

    if (dirnms_no != 0)
        printf("\n");
    

    if (config.R)      /* recursive for every sub-dir and free mem */
    {
        for (n = 0; n < dirnms_no; n++)
        {
            doLs(dirnms[n]);
            free(dirnms[n]);
            if (n != dirnms_no - 1)
                printf("\n");
        }
        free(dirnms);
    }
    return 0;
}


int main(int argc, char* argv[])
{
    int i;
    get_config(argc,argv);
    for (i = 0; i < config.no; i++)
    { 
        doLs(config.files[i]);
        if(i != config.no - 1)
            printf("\n");
    }
    return 0;
}
