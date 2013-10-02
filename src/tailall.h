#ifndef _TALLALL_H_
#define _TALLALL_H_

#include "hashtable.h"

#define file_table_t        hashtable_t
#define file_data_t         hashtable_data_t

#define file_table_init(x)  hashtable_init(x,NULL)
#define file_data_init(x,y) hashtable_data_init(x,y,NULL)
#define file_data_get(x,y)  hashtable_get(x,y)
#define file_data_set(x,y)  hashtable_set(x,y)
#define file_data_del(x,y)  hashtable_del(x,y)
#define file_data_free(x)   { x->data->path != NULL ? free(x->data->path):; x->data != NULL ? free(x->data):; hashtable_free(x); }

#define folder_table_t          hashtable_t
#define folder_data_t           hashtable_data_t

#define folder_table_init(x)    hashtable_init(x,NULL)
#define folder_data_init(x,y)   hashtable_data_init(x,y,NULL)
#define folder_data_get(x,y)    hashtable_get(x,y)
#define folder_data_set(x,y)    hashtable_set(x,y)
#define folder_data_del(x,y)    hashtable_del(x,y)
#define folder_data_free(x)     { close(x->data->fd); free(x->data->path); free(x->data); hashtable_free(x); }

typedef struct _file_t file_t;
typedef struct _file_t
{
    char    *path;
    int     fd;
    file_t  *next;
} file_t;

typedef struct _folder_t
{
    char    *path;
    int     wd;     // watch desc
    file_t  *files;
} folder_t;

#define FOLDER_TABLE_DEFAULT_POWER  14
#define FILE_TABLE_DEFAULT_POWER    16

typedef struct _tailall_t
{
    char            *path;
    folder_table_t  *folder_table;
    file_table_t    *file_table;
    int             inotify;
} tailall_t;

// Integer to char*, same with strdup
char* intdup(const int i);

tailall_t* tailall_init(const char *path);

file_t* file_init(const char *path);

folder_t* folder_init(tailall_t *ta, const char *path);


#endif // _TALLALL_H_
