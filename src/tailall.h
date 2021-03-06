/*
 * Author   : Jinoos Lee (jinoos@gmail.com)
 * Date     : 2013/10/07
 * URL      : https://github.com/jinoos/tailall
 */

#ifndef _TALLALL_H_
#define _TALLALL_H_

#include "hashtable.h"

#define MAX_DIR_NAME_LENGTH     8192
#define FILE_BUF_SIZE           1024*64

#define EVENT_SIZE  ( sizeof (struct inotify_event) )
#define BUF_LEN     ( 1024 * ( EVENT_SIZE + 16 ) )

#ifdef DEBUG
#define debugf(...) { fprintf(stdout, "DBUG - "); fprintf(stdout, __VA_ARGS__); fflush(stdout); }
#define debugfn(...) { fprintf(stdout, "DBUG - "); fprintf(stdout, __VA_ARGS__); fprintf(stdout, "\n"); fflush(stdout); }
#else
#define debugf(...) 
#define debugfn(...) 
#endif

#define errf(...) { fprintf(stderr, "ERR  - "); fprintf(stderr, __VA_ARGS__); fflush(stderr); }
#define errfn(...) { fprintf(stderr, "ERR  - "); fprintf(stderr, __VA_ARGS__); fprintf(stderr, "\n");}

#define warnf(...) { fprintf(stderr, "WARN - "); fprintf(stderr, __VA_ARGS__); fflush(stderr); }
#define warnfn(...) { fprintf(stderr, "WARN - "); fprintf(stderr, __VA_ARGS__); fprintf(stderr, "\n"); fflush(stderr); }

#define infof(...) { fprintf(stderr, "INFO - "); fprintf(stdout, __VA_ARGS__); fflush(stdout); }
#define infofn(...) { fprintf(stderr, "INFO - "); fprintf(stdout, __VA_ARGS__); fprintf(stdout, "\n"); fflush(stdout); }

#define outf(...) { fprintf(stdout, __VA_ARGS__); fflush(stdout); }
#define outfn(...) { fprintf(stdout, __VA_ARGS__); fprintf(stdout, "\n"); fflush(stdout); }

#define file_table_t        hashtable_t
#define file_data_t         hashtable_data_t

/*
#define file_table_init(x)  hashtable_init(x,NULL)
#define file_data_init(x,y) hashtable_data_init(x,y,NULL)
#define file_data_get(x,y)  hashtable_get(x,y)
#define file_data_set(x,y)  hashtable_set(x,y)
#define file_data_del(x,y)  hashtable_del(x,y)
#define file_data_free(x)   { free(x->data->path); free(x->data->name); free(x->data); hashtable_free(x); }
*/

#define folder_table_t          hashtable_t
#define folder_data_t           hashtable_data_t

#define folder_table_init(x)    hashtable_init(x,NULL)
#define folder_data_init(x,y)   hashtable_data_init_alloc(x,y,NULL)
#define folder_data_get(x,y)    hashtable_get(x,y)
#define folder_data_set(x,y)    hashtable_set(x,y)
#define folder_data_del(x,y)    hashtable_del(x,y)

typedef struct _file_t file_t;
typedef struct _folder_t folder_t;
typedef struct _tailall_t tailall_t;

struct _file_t
{
    char            *name;
    int             fd;
    folder_t        *folder;
    file_t          *next;
    file_t          *prev;
};

struct _folder_t
{
    tailall_t       *ta;
    char            *path;
    int             wd;     // watch desc
    file_t          *file_first;
    file_t          *file_last;
};

#define FOLDER_TABLE_DEFAULT_POWER  14
#define FILE_TABLE_DEFAULT_POWER    16
#define MALLOC_TRIM_TERM            100


struct _tailall_t
{
    char            *path;
    folder_table_t  *folder_table;
    file_table_t    *file_table;
    int             inotify;
    file_t          *last_tailing_file;
    uint64_t        tailing_count;
    char            buf[FILE_BUF_SIZE];
    char            ebuf[BUF_LEN];
};

//
//
//

// Integer to char*, same with strdup
char*           intdup(const int i);

tailall_t*      tailall_init(const char *path);

file_t*         file_init(folder_t *folder, const char *name);
void            file_free(file_t *file);
off_t           file_move_eof(file_t *file);

folder_t*       folder_init(tailall_t *ta, const char *path);
void            folder_free(folder_t *folder);
folder_t*       folder_find(tailall_t *ta, const char *path);
file_t*         folder_put_file(folder_t *folder, file_t *file);
file_t*         folder_find_file(folder_t *folder, const char *filename);
file_t*         folder_remove_file(folder_t *folder, const char *filename);
int             is_valid_dirname(const char *ent);
int             is_dir(const char *path);
int             scan_dir(tailall_t *ta, const char *path);
void            watching(tailall_t *ta);
int             tailing(tailall_t *ta, file_t *file);
void            help();

#endif // _TALLALL_H_
