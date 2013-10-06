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

#ifdef DEBUG
#define debugf(...) {fprintf(stdout, "Debug - "); fprintf(stdout, __VA_ARGS__);}
#else
#define debugf(...) 
#endif

#define warnf(...) {fprintf(stderr, "Warning - "); fprintf(stderr, __VA_ARGS__);}
#define errf(...) {fprintf(stderr, "Error - "); fprintf(stderr, __VA_ARGS__);}
#define outf(...) fprintf(stdout, __VA_ARGS__);

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
#define folder_data_init(x,y)   hashtable_data_init(x,y,NULL)
#define folder_data_get(x,y)    hashtable_get(x,y)
#define folder_data_set(x,y)    hashtable_set(x,y)
#define folder_data_del(x,y)    hashtable_del(x,y)

typedef struct _file_t file_t;
typedef struct _folder_t folder_t;
typedef struct _tailall_t tailall_t;

typedef struct _file_t
{
    char            *name;
    int             fd;
    folder_t        *folder;
    file_t          *next;
    file_t          *prev;
} file_t;

typedef struct _folder_t
{
    tailall_t       *ta;
    char            *path;
    int             wd;     // watch desc
    file_t          *file_first;
    file_t          *file_last;
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


//
//
//

// Integer to char*, same with strdup
char*           intdup(const int i);

tailall_t*      tailall_init(const char *path);

file_t*         file_init(folder_t *folder, const char *name);
void            file_free(file_t *file);

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
int             tailing(file_t *file);
void            help();

#endif // _TALLALL_H_
