#include <stdlib.h>
#include <string.h>

#include "hashtable.h"

hashtable_t* hashtable_init(int hash_power_size, pthread_mutex_t *lock)
{
    hashtable_t *table = calloc(sizeof(hashtable_t), 1);

    table->idx = calloc(hashsize(hash_power_size), sizeof(void *));

    if(table->idx == NULL)
    {
        free(table);
        return NULL;
    }

    table->power = hash_power_size;

    table->lock = lock;

    return table;
}

hashtable_data_t* _hashtable_data_init(HASHTABLE_DATA_KEY key_type, char *key, void *data, void (*cb_data_free)(void *))
{
    if(key == NULL)
        return NULL;

    HASH_KEY_LEN len = strlen(key);

    if(len == 0)
        return NULL;

    hashtable_data_t *hdata = calloc(sizeof(hashtable_data_t), 1);

    if(hdata == NULL)
        return NULL;

    hdata->len = len;
    hdata->data = data;
    hdata->key_type = key_type;
    hdata->cb_free = cb_data_free;

    if(key_type == REALLOC_KEY)
    {
        hdata->key = calloc(1, len+1);
        memcpy(hdata->key, key, len);
    }else
    {
        hdata->key = key;
    }

    return hdata;
}


hashtable_data_t* hashtable_data_init(char *key, void *data, void (*cb_data_free)(void *))
{
    return _hashtable_data_init(POINTER_KEY, key, data, cb_data_free);
}

hashtable_data_t* hashtable_data_init_alloc(char *key, void *data, void (*cb_data_free)(void *))
{
    return _hashtable_data_init(REALLOC_KEY, key, data, cb_data_free);
}

void hashtable_data_free(hashtable_data_t *hdata)
{
    if(hdata == NULL)
        return;

    if(hdata->cb_free != NULL)
    {
        hdata->cb_free(hdata->data);
    }

    if(hdata->key_type == REALLOC_KEY)
    {
        free(hdata->key);
    }

    free(hdata);

    return;
}

hashtable_data_t* hashtable_get(hashtable_t *table, const char *key)
{
    if(table == NULL || key == NULL)
        return NULL;

    return hashtable_get2(table, key, strlen(key));
}

hashtable_data_t* hashtable_get2(hashtable_t *table, const char *key, const HASH_KEY_LEN len)
{
    if(table->lock != NULL)
    {
        pthread_mutex_lock(table->lock);
    }

    HASH_VAL hval = hash(key, (size_t)len, 0);

    hashtable_data_t *data = table->idx[hval & hashmask(table->power)];

    while(data != NULL)
    {
        if(strncmp(data->key, key, (size_t)len) == 0)
        {
            break;
        }
        data = data->next;
    }

    if(table->lock != NULL)
    {
        pthread_mutex_unlock(table->lock);
    }

    return data;
}

hashtable_data_t* hashtable_set(hashtable_t *table, hashtable_data_t *data)
{
    if(table == NULL || data == NULL)
        return NULL;

    if(table->lock != NULL)
    {
        pthread_mutex_lock(table->lock);
    }

    HASH_VAL hval = hash(data->key, data->len, 0);
    hashtable_data_t *idx = table->idx[hval & hashmask(table->power)];
    hashtable_data_t *prev = NULL;

    while(idx != NULL)
    {
        if(strncmp(idx->key, data->key, data->len) == 0)
        {
            if(table->lock != NULL)
            {
                pthread_mutex_unlock(table->lock);
            }

            return NULL;
        }

        prev = idx;
        idx = idx->next;
    }

    if(prev == NULL)
    {
        table->idx[hval & hashmask(table->power)] = data;
    }else
    {
        prev->next = data;
    }

    table->data_count++;

    if(table->lock != NULL)
    {
        pthread_mutex_unlock(table->lock);
    }

    return data;
}

hashtable_data_t* hashtable_replace(hashtable_t *table, hashtable_data_t *new_data)
{
    if(table == NULL || new_data == NULL)
        return NULL;

    if(table->lock != NULL)
    {
        pthread_mutex_lock(table->lock);
    }

    HASH_VAL hval = hash(new_data->key, new_data->len, 0);
    hashtable_data_t *idx = table->idx[hval & hashmask(table->power)];
    hashtable_data_t *prev = NULL;

    while(idx != NULL)
    {
        if(strncmp(idx->key, new_data->key, new_data->len) == 0)
        {
            new_data->next = idx->next;

            if(prev == NULL)
            {
                table->idx[hval & hashmask(table->power)] = new_data;
            }else
            {
                prev->next = new_data;
            }

            if(table->lock != NULL)
            {
                pthread_mutex_unlock(table->lock);
            }

            hashtable_data_free(idx);
            return new_data;
        }

        prev = idx;
        idx = idx->next;
    }

    if(prev == NULL)
    {
        table->idx[hval & hashmask(table->power)] = new_data;
    }else
    {
        prev->next = new_data;
    }

    table->data_count++;

    if(table->lock != NULL)
    {
        pthread_mutex_unlock(table->lock);
    }

    return new_data;
}

void hashtable_del(hashtable_t *table, const char *key)
{
    if(table == NULL || key == NULL)
        return;

    return hashtable_del2(table, key, strlen(key));
}

void hashtable_del2(hashtable_t *table, const char *key, const HASH_KEY_LEN len)
{
    if(table == NULL || key == NULL)
        return;

    if(table->lock != NULL)
    {
        pthread_mutex_lock(table->lock);
    }

    HASH_VAL hval = hash(key, (size_t)len, 0);
    hashtable_data_t *idx = table->idx[hval & hashmask(table->power)];
    hashtable_data_t *prev = NULL;

    while(idx != NULL)
    {
        if(strncmp(idx->key, key, (size_t)len) == 0)
        {
            // first item.
            if(prev == NULL)
            {
                table->idx[hval & hashmask(table->power)] = idx->next;
            }else
            {
                prev->next = idx->next;
            }

            table->data_count--;
            idx->next = NULL;

            if(table->lock != NULL)
            {
                pthread_mutex_unlock(table->lock);
            }

            hashtable_data_free(idx);
            return;
        }

        prev = idx;
        idx = idx->next;
    }

    if(table->lock != NULL)
    {
        pthread_mutex_unlock(table->lock);
    }

    return;
}

