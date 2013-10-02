#ifndef _HASH_H_
#define _HASH_H_

#include <stdint.h>
#include <stddef.h>

#ifdef    __cplusplus
extern "C" {
#endif

typedef uint32_t    HASH_VAL;
typedef uint16_t    HASH_KEY_LEN;
#define HASH_KEY_MAX_LEN    UINT16_MAX

#define hashsize(n) ((unsigned long int)1<<(n))
#define hashmask(n) (hashsize(n)-1)

uint32_t hash(const void *key, size_t length, const uint32_t initval);

#ifdef    __cplusplus
}
#endif

#endif // _HASH_H_

