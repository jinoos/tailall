/*
 * Author   : Jinoos Lee (jinoos@gmail.com)
 * Date     : 2013/10/07
 * Version  : 0.1
 * URL      : https://github.com/jinoos/tailall
 */

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
#include <unistd.h>

#include "tailall.h"

#define EVENT_SIZE  ( sizeof (struct inotify_event) )
#define BUF_LEN     ( 1024 * ( EVENT_SIZE + 16 ) )

int main( int argc, char **argv )
{
    char dir[MAX_DIR_NAME_LENGTH]; /* monitoring directory name */
    int fd, ret;
    struct stat stat;
    tailall_t *ta;

    fd = inotify_init();

    assert(fd > 0);

    if(argc < 2)
    {
        debugf("Watching the current directory\n");
        strcpy (dir, "./");
    }else
    {
        debugf("Watching '%s' directory\n", argv[1]);
        strcpy (dir, argv[1]);
    }

    ret = lstat(dir, &stat);
    if(ret < 0)
    {
        errf("%s %s\n", strerror(errno), dir);
        exit(-1);
    }

    if(S_ISDIR(stat.st_mode))
    {
        if(dir[(strlen(dir)-1)] != '/')
        {
            strcat(dir, "/");
        }

        ta = tailall_init(dir);
        assert(ta != NULL);

        ret = scan_dir(ta, dir);

        watching(ta);

        exit(0);
    }else if(S_ISREG(stat.st_mode))
    {
        errf("Not allow to tail just a file. %s\n", dir);
        help();
        exit(-1);

    }else
    {
        help();
        exit(-1);
    }

    return 0;
}

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

    folder_table_t  *folder_table;

    folder_table = folder_table_init(FOLDER_TABLE_DEFAULT_POWER);
    assert(folder_table != NULL);

    inotify_fd = inotify_init();
    assert(inotify_fd >= 0);
    
    ta = calloc(sizeof(tailall_t), 1);
    assert(ta != NULL);

    ta->inotify = inotify_fd;
    ta->path = strdup(path);
    ta->folder_table = folder_table;
    
    return ta;
}

file_t* file_init(folder_t *folder, const char *name)
{
    assert(folder != NULL);

    char buf[MAX_DIR_NAME_LENGTH];
    file_t *file;
    int fd, ret;

    strcpy(buf, folder->path);
    strcat(buf, name);

    debugf("file_t init %s\n", buf);

    fd = open(buf, O_RDONLY);
    if(fd < 0)
    {
        debugf("%s %s\n", strerror(errno), buf);
        return NULL;
    }

    // move to end of the file
    ret = lseek(fd, 0, SEEK_END);
    assert(ret >= 0);

    file = calloc(sizeof(file_t), 1);
    assert(file != NULL);

    file->folder = folder;
    file->name = strdup(name);
    file->fd   = fd;

    return file;
}

void file_free(file_t *file)
{
    assert(file != NULL);

    if(file->name != NULL)
        free(file->name);

    close(file->fd);

    free(file);
}

folder_t* folder_init(tailall_t *ta, const char *path)
{
    assert(ta != NULL);
    assert(path != NULL);

    folder_t *folder;

    folder = calloc(sizeof(folder_t), 1);
    folder->ta = ta;
    assert(folder != NULL);

    folder->path = strdup(path);

    // IN_DONT_FOLLOW       : Don't dereference pathname if it is a symbolic link
    folder->wd = inotify_add_watch(ta->inotify, path,   
                                                        IN_CREATE |
                                                        IN_CLOSE_WRITE |
                                                        IN_DELETE |
                                                        IN_DELETE_SELF |
                                                        IN_MODIFY |
                                                        IN_MOVE_SELF |
                                                        IN_MOVED_FROM |
                                                        IN_MOVED_TO
                                                        );
    assert(folder->wd >= 0);

    return folder;
}

void folder_free(folder_t *folder)
{
    assert(folder != NULL);

    debugf("folder_free %s\n", folder->path);

    file_t *file, *file2;
    int res;

    folder_data_del(folder->ta->folder_table, folder->path);

    file = folder->file_first;

    while(file != NULL)
    {
        file2 = file->next;
        file_free(file);
        file = file2;
    }

    res = inotify_rm_watch(folder->ta->inotify, folder->wd);
    if(res < 0)
    {
        warnf("%s\n", strerror(errno));
    }

    free(folder->path);
    free(folder);
}


folder_t* folder_find(tailall_t *ta, const char *path)
{
    assert(path != NULL);

    folder_data_t *folder_data;

    folder_data = folder_data_get(ta->folder_table, path);
    if(folder_data == NULL)
        return NULL;

    return (folder_t *)folder_data->data;
}

file_t* folder_put_file(folder_t *folder, file_t *file)
{
    assert(folder != NULL);
    assert(file != NULL);

    file_t *filep;

    filep = folder->file_last;

    if(filep == NULL)
    {
        folder->file_first = folder->file_last = file;
    }else
    {
        folder->file_last = filep->next = file;
        file->prev = filep;
    }

    return file;
}

file_t* folder_find_file(folder_t *folder, const char *filename)
{
    assert(folder != NULL);
    assert(filename != NULL);

    file_t *file;

    file = folder->file_last;

    if(file == NULL)
        return NULL;

    do
    {
        if(strcmp(file->name, filename) == 0)
        {
            return file;
        }
    } while((file = file->next) != NULL);

    return NULL;
}

// return
//   NULL : if not exists file 
file_t* folder_remove_file(folder_t *folder, const char *filename)
{
    assert(folder != NULL);
    assert(filename != NULL);

    debugf("folder_remove_file %s %s\n", folder->path, filename);

    file_t *file = folder_find_file(folder, filename);

    if(file == NULL)
        return NULL;

    if(file == folder->file_first)
        folder->file_first = file->next;

    if(file == folder->file_last)
        folder->file_last = file->prev;

    if(file->next != NULL)
        file->next->prev = NULL;

    if(file->prev != NULL)
        file->prev->next = NULL;

    file->next = file->prev = NULL;

    return file;
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
            warnf("Ignored, due to a symbolic link \"%s\"\n", path);
            return -1;
        }

        if(S_ISFIFO(stat.st_mode))
        {
            warnf("Ignored, due to a FIFO \"%s\"\n", path);
            return -1;
        }

        if(S_ISBLK(stat.st_mode))
        {
            warnf("Ignored, due to a block device \"%s\"\n", path);
            return -1;
        }
        
        if(S_ISCHR(stat.st_mode))
        {
            warnf("Ignored, due to a charictor device \"%s\"\n", path);
            return -1;
        }

        if(S_ISSOCK(stat.st_mode))
        {
            warnf("Ignored, due to a socket \"%s\"\n", path);
            return -1;
        }
    }else
    {
        // error
        errf("%s \"%s\"\n", strerror(errno), path);
        return -1;
    }

    return -1;
}

// return
//   1 : Success
//   0 : Permission denied
//  -1 : Error
int scan_dir(tailall_t *ta, const char *path)
{
    assert(path != NULL);

    debugf("Start to scan directory %s\n", path);

    char buf[MAX_DIR_NAME_LENGTH], *bufp = buf;
    char *wdstr;
    int ret;
    DIR *dir;
    struct dirent *ent;
    folder_t *folder;
    folder_data_t *folder_data, *folder_datap;
    file_t *file;

    memset(buf, 0, MAX_DIR_NAME_LENGTH);
    strcpy(buf, path);
    bufp += strlen(path);

    folder = folder_init(ta, buf);
    assert(folder != NULL);

    wdstr = intdup(folder->wd);

    // must not be the folder in folder_table
    assert(folder_data_get(ta->folder_table, wdstr) == NULL);

    folder_data = folder_data_init(wdstr, folder);
    assert(folder_data != NULL);

    folder_datap = folder_data_set(ta->folder_table, folder_data);
    assert(folder_datap != NULL);

    dir = opendir(path);

    if(dir == NULL)
    {
        warnf("%s, %s\n", strerror(errno), path);

        if(errno & EACCES)
        {
            return 0;
        }else
        {
            exit(-1);
        }
    }

    while((ent = readdir(dir)) != NULL)
    {

        strcpy(bufp, ent->d_name);

        ret = is_dir(buf);

        if(ret > 0)
        {
            // directory
            if(is_valid_dirname(bufp))
            {
                // valid directory
                debugf("D %s\n", buf);

                strcat(buf, "/");
                scan_dir(ta, buf);
                continue;
            }

            // ignored invalid directory as '.', '..'
        }else if(ret == 0)
        {
            // file
            debugf("F %s\n", buf);
            
            file = file_init(folder, ent->d_name);
            if(file != NULL)
            {
                folder_put_file(folder, file);
            }
        }
    }

    closedir(dir);

    return 0;
}

void watching(tailall_t *ta)
{
    char buffer[BUF_LEN];

    while(1)
    {
        int length, i = 0;

        length = read(ta->inotify, buffer, BUF_LEN);

        while (i < length)
        {
            struct inotify_event *event = (struct inotify_event *) &buffer[i];

            debugf("wd=%d mask=%d cookie=%d len=%d dir=%s\n", event->wd, event->mask, event->cookie, event->len, (event->mask & IN_ISDIR)?"yes":"no");

            

            char *wd = intdup(event->wd);
            folder_data_t *folder_data = folder_data_get(ta->folder_table, wd);
            assert(folder_data != NULL);
            folder_t *folder = (folder_t*) folder_data->data;
            assert(folder_data != NULL);
            free(wd);

            debugf("Path : %s\n", folder->path);

            if(event->len)
            {
                char buf[MAX_DIR_NAME_LENGTH];

                //
                if (event->mask & IN_CREATE)
                {
                    if (event->mask & IN_ISDIR)
                    {
                        debugf("The directory %s%s was created.\n", folder->path, event->name);      

                        strcpy(buf, folder->path);
                        strcat(buf, event->name);
                        strcat(buf, "/");
                        scan_dir(ta, buf);
                    } else {
                        debugf("The file %s%s was created.\n", folder->path, event->name);

                        file_t *file = file_init(folder, event->name);
                        if(file != NULL)
                        {
                            folder_put_file(folder, file);
                        }
                    }

                // 
                } else if (event->mask & IN_DELETE)
                {
                    if(event->mask & IN_ISDIR)
                    {
                        debugf("The directory %s%s was deleted.\n", folder->path, event->name);      
                        strcpy(buf, folder->path);
                        strcat(buf, event->name);
                        strcat(buf, "/");

                        folder_t *folder = folder_find(ta, buf);

                        if(folder != NULL)
                        {
                            folder_free(folder);
                        }
                    }else
                    {
                        debugf("The file %s%s was deleted.\n", folder->path, event->name);

                        file_t *file = folder_remove_file(folder, event->name);

                        if(file != NULL)
                        {
                            file_free(file);
                        }
                    }

                //
                } else if (event->mask & IN_DELETE_SELF)
                {
                    if(event->mask & IN_ISDIR)
                    {
                        debugf("The directory %s%s was deleted itself.\n", folder->path, event->name);      
                        strcpy(buf, folder->path);
                        strcat(buf, event->name);
                        strcat(buf, "/");

                        folder_t *folder = folder_find(ta, buf);

                        if(folder != NULL)
                        {
                            folder_free(folder);
                        }
                    }

                //
                } else if (event->mask & IN_MODIFY || event->mask & IN_CLOSE_WRITE)
                {
                    if(event->mask & IN_ISDIR)
                    {
                        debugf("The directory %s - %s was modified.\n", folder->path, event->name);
                        // can be ignored.
                        debugf("Ignored, %s - %s/ directory modified.\n", folder->path, event->name);
                    }else
                    {
                        debugf("The file %s - %s was modified.\n", folder->path, event->name);
                        file_t *file = folder_find_file(folder, event->name);

                        if(file != NULL)
                        {
                            tailing(file);
                        }else
                        {
                            file = file_init(folder, event->name);
                            if(file != NULL)
                            {
                                file = folder_put_file(folder, file);
                            }
                        }
                    }

                //
                } else if (event->mask & IN_MOVED_FROM)
                {
                    if (event->mask & IN_ISDIR)
                    {
                        debugf("The directory %s%s was moved from.\n", folder->path, event->name);
                    } else
                    {
                        debugf("The file %s%s was moved from.\n", folder->path, event->name);
                    }

                //
                } else if (event->mask & IN_MOVED_TO)
                {
                    if (event->mask & IN_ISDIR)
                    {
                        debugf("The directory %s%s was moved to.\n", folder->path, event->name);
                    } else
                    {
                        debugf("The file %s%s was moved to.\n", folder->path, event->name);
                    }

                //
                } else if (event->mask & IN_MOVE_SELF)
                {
                    if (event->mask & IN_ISDIR)
                    {
                        debugf("The directory %s%s was moved.\n", folder->path, event->name);
                        if(folder != NULL)
                        {
                            folder_free(folder);
                        }
                    }
                }
            }

            i += EVENT_SIZE + event->len;
        }
    }

}

int tailing(file_t *file)
{
    assert(file != NULL);

    char buf[FILE_BUF_SIZE];
    int ret, total;

    memset(buf, 0, FILE_BUF_SIZE);

    total = 0;
    while( (ret = read(file->fd, buf, FILE_BUF_SIZE)) > 0)
    {
        if(total == 0)
        {
            outf("\n# %s%s %d\n", file->folder->path, file->name, ret);
        }


        fprintf(stdout, "%.*s", ret, buf);
        memset(buf, 0, FILE_BUF_SIZE);
        total += ret;
    }

    if(ret < 0)
    {
        warnf("%s\n",strerror(errno));
    }

    return total;
}


void help()
{
    outf("\n");
    outf("Usage: [DIRECTORY]\n");
    outf("\n");
    outf("Example: ./tailall \n");
    outf("\n");
    outf("Tailing all files(only normal file) under a directory such as UNIX tail command,\n");
    outf("even in sub-directories recursively.\n");
    outf("\n");
    outf("NFS(Network File System) file, symbolic link, FIFO and block device will be\n");
    outf("ignored, due to some reasons.");
    outf("\n");
    outf("DIRECTORY is the target to be watched. It watchs current directory (./), if\n");
    outf("no DIRECTORY.\n");
    outf("\n");
}
