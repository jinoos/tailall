#ifndef _HASHTABLE_H_
#define _HASHTABLE_H_

#include <pthread.h>

#include "hash.h"

#ifdef    __cplusplus
extern "C"
{
#endif

typedef enum {REALLOC_KEY, POINTER_KEY} HASHTABLE_DATA_KEY;

typedef struct _hashtable_data hashtable_data_t;
struct _hashtable_data
{
    char                *key;
    HASHTABLE_DATA_KEY  key_type;    // if 1, need to free during clean up
    HASH_KEY_LEN        len;
    void                *data;
    hashtable_data_t    *next;
    void                (*cb_free)(void *);
};

typedef struct _hashtable
{
    int                 power;
    hashtable_data_t    **idx;
    uint64_t            data_count;
    pthread_mutex_t     *lock;
} hashtable_t;

hashtable_data_t*   hashtable_data_init(char *key, void *data, void (*cb_data_free)(void *));
hashtable_data_t*   hashtable_data_init_alloc(char *key, void *data, void (*cb_data_free)(void *));
void                hashtable_data_free(hashtable_data_t *hdata);

hashtable_t*        hashtable_init(int hash_power_size, pthread_mutex_t *lock);
hashtable_data_t*   hashtable_get(hashtable_t *table, const char *key);
hashtable_data_t*   hashtable_get2(hashtable_t *table, const char *key, const HASH_KEY_LEN len);
hashtable_data_t*   hashtable_set(hashtable_t *table, hashtable_data_t *data);

// exists data already : delete current data and set new data.
// not exists data     : same with hashtable_set()
hashtable_data_t*   hashtable_replace(hashtable_t *table, hashtable_data_t *data);
void                hashtable_del(hashtable_t *table, const char *key);
void                hashtable_del2(hashtable_t *table, const char *key, const HASH_KEY_LEN len);

#ifdef    __cplusplus
}
#endif

#endif // _HASHTABLE_H_
