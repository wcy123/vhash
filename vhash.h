#pragma once
#include <stdio.h>
#include <stdint.h>
#include <string.h>
// usually, key_compare function and hash function is very small and
// fast, function call overhead is relatively much higher than normal
// functions. It would be good if we can inline key_compare and hash
// function. If we use a function pointer, we have very good
// flexibility, but we lost efficiency. so we have pre-defined a set
// of commonly used key_compare function and hash function. it is not
// easy to customize your own key_compare and hash function.
// 
typedef int64_t hk_t;
typedef int64_t hv_t;
typedef enum {
    // int64_t as key type
    // key compare: k1 == k2
    // hash : return k it self
    VHASH_KEY_LONG  = 0 ,
    // hk_t is a pointer to a cstring
    // key compare: k1 == k2 || strcmp((const char*)k1,(const char*)k2) == 0
    // hash: a string hash
    VHASH_KEY_STRING = 1 ,
    // hk_t is a pointer to a fixed size object, size is defined by
    // `key_obj_size`.
    // key compare, k1 == k2 || memcmp((const void*)k1,(const void*)k2, key_obj_size)
    // hash: hash((const unsigned char*) k, key_obj_size)
    VHASH_KEY_FIXED_SIZE_OBJ = 2,
    //
    // system-wise, only possible to have 5 additional type of key
    // comparing function and hashing function.
    // they should be defined as MACRO
    //     VHASH_COMPARE_KEY_0, VHASH_HASH_0
    //     VHASH_COMPARE_KEY_1, VHASH_HASH_1
    //     VHASH_COMPARE_KEY_2, VHASH_HASH_2
    //     VHASH_COMPARE_KEY_3, VHASH_HASH_3
    //     VHASH_COMPARE_KEY_4, VHASH_HASH_4
    VHASH_KEY_OBJ_0 = 3,
    VHASH_KEY_OBJ_1 = 4,
    VHASH_KEY_OBJ_2 = 5,
    VHASH_KEY_OBJ_3 = 6,
    VHASH_KEY_OBJ_4 = 7
} key_type_t;

#ifndef VHASH_COMPARE_KEY_0
#define VHASH_COMPARE_KEY_0 compare_key_long
#endif
#ifndef VHASH_COMPARE_KEY_1
#define VHASH_COMPARE_KEY_1 compare_key_long
#endif
#ifndef VHASH_COMPARE_KEY_2
#define VHASH_COMPARE_KEY_2 compare_key_long
#endif
#ifndef VHASH_COMPARE_KEY_3
#define VHASH_COMPARE_KEY_3 compare_key_long
#endif
#ifndef VHASH_COMPARE_KEY_4
#define VHASH_COMPARE_KEY_4 compare_key_long
#endif
#ifndef VHASH_HASH_0
#define VHASH_HASH_0 hash_long
#endif
#ifndef VHASH_HASH_1
#define VHASH_HASH_1 hash_long
#endif
#ifndef VHASH_HASH_2
#define VHASH_HASH_2 hash_long
#endif
#ifndef VHASH_HASH_3
#define VHASH_HASH_3 hash_long
#endif
#ifndef VHASH_HASH_4
#define VHASH_HASH_4 hash_long
#endif


typedef struct pair_s {
    hk_t k;
    hv_t v;
} pair_t;

typedef struct vhash_s {
    pair_t * bucket;
    hv_t v0;
    hv_t v1;
    size_t n_of_elt;
    struct {
        unsigned int key_type:    8;
        unsigned int key_size:   16;
        unsigned int n_of_bucket: 6; // n_of_bucket = 0,..., 63 (1<<n_of_bucket) is at most 64 bits
        unsigned int v0_exist:    1;
        unsigned int v1_exist:    1;
    } flag;
// statistics
    size_t n_of_grow;
    size_t n_of_shrink;
    size_t n_of_total_probe;
    size_t n_of_total_probe_2;
    size_t n_of_probe_sample;
} vhash_t;

vhash_t * vhash_create(key_type_t ktype, unsigned short key_size);
vhash_t * vhash_insert(vhash_t * h, hk_t k, hv_t v);
pair_t* vhash_get(vhash_t * h, hk_t k);
size_t vhash_iter_start(vhash_t *h);
size_t vhash_iter_end(vhash_t *h);
size_t vhash_iter_next(vhash_t* h, size_t c);
pair_t* vhash_iter_get(vhash_t* h, size_t c);
// return NULL if cannot find the key.  when find the key, return
// non-NULL. the return value pointer to a deleted bucket which does
// not hold and valid data.
pair_t* vhash_delete(vhash_t * h, hk_t k);
void vhash_destroy(vhash_t *h);
size_t vhash_size(vhash_t * h);
vhash_t * vhash_print(vhash_t * h, FILE * fp);

/* Local Variables: */
/* mode:c */
/* c-basic-offset: 4*/
/* End: */
