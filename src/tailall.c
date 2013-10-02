#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/inotify.h>
#include <assert.h>
#include <libgen.h>
#include <fcntl.h>
#include <unistd.h>
#include <dirent.h>

#include "tailall.h"

#define EVENT_SIZE  ( sizeof (struct inotify_event) )
#define BUF_LEN     ( 1024 * ( EVENT_SIZE + 16 ) )
#define MAX_DIR_NAME_LENGTH     1024
#define FILE_BUF_SIZE           4096

char* intdup(const int i)
{
    char buf[33];
    sprintf(buf, "%d", i);
    return strdup(buf);
}

tailall_t* tailall_init(const char *path)
{
    assert(path != NULL);

    tailall_t       *ta;

    int             inotify_fd;

    file_table_t    *file_table;
    folder_table_t  *folder_table;

    folder_table = folder_table_init(FOLDER_TABLE_DEFAULT_POWER);
    assert(folder_table != NULL);

    file_table = file_table_init(FILE_TABLE_DEFAULT_POWER);
    assert(file_table != NULL);

    inotify_fd = inotify_init();
    assert(inotify_fd >= 0);
    
    ta = calloc(sizeof(tailall_t), 1);
    assert(ta != NULL);

    ta->path = strdup(path);
    ta->folder_table = folder_table;
    ta->file_table = file_table;
    
    return ta;
}

file_t* file_init(const char *path)
{
    assert(path != NULL);

    file_t *file;
    int fd, ret;

    fd = open(path, O_RDONLY);
    if(fd < 0)
    {
        fprintf(stderr, "Cannot open file %s\n", path);
        return NULL;
    }

    file = calloc(sizeof(file_t), 1);
    assert(file != NULL);

    file->path = strdup(path);
    file->fd   = fd;

    ret = lseek(file->fd, 0, SEEK_END);
    assert(ret >= 0);

    return file;
}

folder_t* folder_init(tailall_t *ta, const char *path)
{
    assert(path != NULL);

    struct stat stat;
    folder_t *folder;
    int ret;

    ret = lstat(path, &stat);

    if(ret < 0)
    {
        if(ret && EACCES)
        {
            fprintf(stderr, "Error : Permission denied to watch \"%s\"\n", path);
        }else
        {
            fprintf(stderr, "Error : Cannot watch \"%s\", due to unknown reason. lstat return %d\n", path, ret);
        }

        return NULL;
    }

    assert(!S_ISREG(stat.st_mode));

    if(S_ISLNK(stat.st_mode))
    {
        fprintf(stderr, "Error : Cannot watch a Symblolic Link \"%s\"\n", path);
        return NULL;
    }

    if(S_ISFIFO(stat.st_mode))
    {
        fprintf(stderr, "Error : Cannot watch a FIFO \"%s\"\n", path);
        return NULL;
    }

    if(S_ISBLK(stat.st_mode))
    {
        fprintf(stderr, "Error : Cannot watch a Block Device \"%s\"\n", path);
        return NULL;
    }
    
    if(S_ISCHR(stat.st_mode))
    {
        fprintf(stderr, "Error : Cannot watch a Charictor Device \"%s\"\n", path);
        return NULL;
    }

    if(S_ISSOCK(stat.st_mode))
    {
        fprintf(stderr, "Error : Cannot watch a Socket \"%s\"\n", path);
        return NULL;
    }

    folder = calloc(sizeof(folder_t), 1);
    assert(folder != NULL);

    folder->path = strdup(path);

    folder->wd = inotify_add_watch(ta->inotify, path, IN_MODIFY | IN_CREATE | IN_DELETE | IN_MOVED_FROM | IN_MOVED_TO | IN_MOVE_SELF | IN_DELETE_SELF);
    assert(folder->wd >= 0);

    return folder;
}

// return
//   1 : valid name
//   0 : invalid name
int is_valid_dirname(const char *ent)
{
    assert(ent != NULL);

    if(ent[0] == '.')
        return 0;
    else
        return 1;
}

// return
//   1 : directory
//   0 : file
//  -1 : others or error
int is_dir(const char *path)
{
    assert(path != NULL);

    struct stat stat;
    int ret;

    ret = lstat(path, &stat);
    if(ret >= 0)
    {
        if(S_ISDIR(stat.st_mode))
            return 1;

        if(S_ISREG(stat.st_mode))
            return 0;

        if(S_ISLNK(stat.st_mode))
        {
            fprintf(stderr, "Ignored, due to a Symblolic Link \"%s\"\n", path);
            return -1;
        }

        if(S_ISFIFO(stat.st_mode))
        {
            fprintf(stderr, "Ignored, due to a FIFO \"%s\"\n", path);
            return -1;
        }

        if(S_ISBLK(stat.st_mode))
        {
            fprintf(stderr, "Ignored, due to a Block Device \"%s\"\n", path);
            return -1;
        }
        
        if(S_ISCHR(stat.st_mode))
        {
            fprintf(stderr, "Ignored, due to a Charictor Device \"%s\"\n", path);
            return -1;
        }

        if(S_ISSOCK(stat.st_mode))
        {
            fprintf(stderr, "Ignored, due to a Socket \"%s\"\n", path);
            return -1;
        }
    }else
    {
        // error
        fprintf(stderr, "Error, %s \"%s\"\n", strerror(errno), path);
        return -1;
    }
}

// return
//   1 : Success
//   0 : Permission denied
//  -1 : Error
int scan_dir(tailall_t *ta, const char *path)
{
    assert(path != NULL);

    char buf[MAX_DIR_NAME_LENGTH], *bufp = buf;
    int ret;
    struct stat stat;
    DIR *dir;
    struct dirent *ent;

    memset(buf, 0, MAX_DIR_NAME_LENGTH);
    strcpy(buf, path);
    bufp += strlen(path);

    dir = opendir(path);

    if(dir == NULL)
    {
        fprintf(stderr, "Error : %s, %s\n", strerror(errno), path);

        if(errno && EACCES)
        {
            return 0;
        }else
        {
            exit(-1);
        }
    }

    while((ent = readdir(dir)) != NULL)
    {
        fprintf(stdout, "Entry name : %s\n", ent->d_name);

        strcpy(bufp, ent->d_name);

        ret = is_valid_dirname(buf);

        if(ret > 0)
        {
            // directory
            if(is_valid_dirname(bufp))
            {
                // valid directory
            }
            
            // ignored invalid directory as '.', '..'
        }else if(ret == 0)
        {
            // file
        }
    }

    closedir(dir);

    return 0;
}


int open_file(const char *filename)
{
    int fp = open(filename , O_RDONLY); 

    file_table_init(3);

    assert(fp >= 0);

    if(fp < 0)
    {
        printf("Cannot open file %s\n", filename);
        exit(-1);
    }

    char buf[FILE_BUF_SIZE];
    int ret, total;

    memset(buf, 0, FILE_BUF_SIZE);
    total = 0;


    printf("--- %d \n", errno);

    // move fp to end of file.
    ret = lseek(fp, 0, SEEK_END);
    assert(ret >= 0);

    return fp;
}

int print_once(const int fp)
{
    char buf[FILE_BUF_SIZE];
    int ret, total;

    memset(buf, 0, FILE_BUF_SIZE);
    total = 0;

    while(ret = read(fp, buf, FILE_BUF_SIZE) > 0)
    {
        printf("%d %s", ret, buf);
        memset(buf, 0, FILE_BUF_SIZE);
        total += ret;
    }

    if(ret < 0)
    {
        printf("%s\n",strerror(errno));
    }

    return total;
}

int main( int argc, char **argv )
{
    char dir[MAX_DIR_NAME_LENGTH]; /* monitoring directory name */
    int fd, rstat, ret;
    int wd; /* watch desc */
    char buffer[BUF_LEN];
    struct stat stat;
    tailall_t *ta;

    fd = inotify_init();
    assert(fd > 0);

    if(argc < 2)
    {
        fprintf (stderr, "Watching the current directory\n");
        strcpy (dir, "./");
    }else
    {
        fprintf (stderr, "Watching '%s' directory\n", argv[1]);
        strcpy (dir, argv[1]);
    }

    rstat = lstat(dir, &stat);
    assert(rstat >= 0);

    if(S_ISDIR(stat.st_mode))
    {
        printf("directory %s\n", dir);

        if(dir[(strlen(dir)-1)] != '/')
        {
            strcat(dir, "/");
        }

        printf("new dirname %s\n", dir);

        ta = tailall_init(dir);
        assert(ta != NULL);

        ret = scan_dir(ta, dir);

        printf("Scaned. %d\n", ret);
        exit(0);


    }else if(S_ISREG(stat.st_mode))
    {
        printf("file %s\n", dir);

        int fp = open_file(dir);
        assert(fp >= 0);

        int ret;

        ret = print_once(fp);

        printf("ret %d\n", ret);

    }else
    {
        printf("Unknonwn file type.\n");
    }


//    printf("dirname %s\n", dirname(strdup(dir)));
//    printf("basename %s\n", basename(strdup(dir)));

    wd = inotify_add_watch(fd, dir, IN_MODIFY | IN_CREATE | IN_DELETE | IN_MOVED_FROM | IN_MOVED_TO | IN_MOVE_SELF | IN_DELETE_SELF);

    while(1) {
        int length, i = 0;
        length = read(fd, buffer, BUF_LEN);
        if (length < 0) {
            perror("read");
        }
        while (i < length) {
            struct inotify_event *event = (struct inotify_event *) &buffer[i];
            printf ("[debug] wd=%d mask=%d cookie=%d len=%d dir=%s\n", event->wd, event->mask, event->cookie, event->len, (event->mask & IN_ISDIR)?"yes":"no");
            if (event->len) {
                if (event->mask & IN_CREATE) {
                    if (event->mask & IN_ISDIR) {
                        printf("The directory %s/%s was created.\n", dir, event->name);      
                    } else {
                        printf("The file %s/%s was created.\n", dir, event->name);
                    }
                } else if (event->mask & IN_DELETE) {
                    if (event->mask & IN_ISDIR) {
                        printf("The directory %s/%s was deleted.\n", dir, event->name);      
                    } else {
                        printf("The file %s/%s was deleted.\n", dir, event->name);
                    }
                } else if (event->mask & IN_MODIFY) {
                    if (event->mask & IN_ISDIR) {
                        printf("The directory %s/%s was modified.\n", dir, event->name);
                    } else {
                        printf("The file %s/%s was modified.\n", dir, event->name);
                    }
                } else if (event->mask & IN_MOVED_FROM || event->mask & IN_MOVED_TO || event->mask & IN_MOVE_SELF) {
                    if (event->mask & IN_ISDIR) {
                        printf("The directory %s/%s was moved.\n", dir, event->name);
                    } else {
                        printf("The file %s/%s was moved.\n", dir, event->name);
                    }
                }
            }
            i += EVENT_SIZE + event->len;
        }
    }
    /*
       inotify_rm_watch(fd, wd);
       close(fd);
     */
    return 0;
}
