#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include "vhash.h"
#define EMPTY 0
#define DELETED 1
#define OCCUPIED 2

// initial size of buckets
#define VHASH_INIT_N_OF_BUCKET 3

// shrink threshold
#define VHASH_SHRINK_THRESHOLD  8

// in case of collision and no avaiable bucket is found, we attempt to
// resize the bucket. if it happens again, grow again, but no more
// than MAX_GROW_ATTEMPT times.
#define MAX_GROW_ATTEMPT 3

// maximum number of probe attempts. By default, h->n_of_bucket, it
// means that only after scanning the whole bucket, we are sure we
// cannot find the avaiable bucket.
// when #define VHASH_PROBE(s,i)  s = s0 ^ (1<<(i));
// MAX_PROBE_ATTEMPT should be better  (h->n_of_grow + log2(VHASH_INIT_N_OF_BUCKET))
// 
#define MAX_PROBE_ATTEMPT (h->flag.n_of_bucket)

// linear probe
// #define VHASH_PROBE(s,i)  s = s + 1;

// hash quadratic probing 
// #define VHASH_PROBE(s,i)  s = s + i;
// s(i) = s(i-1) + i
// s1 = s0 + 1 = s0 + 1
// s2 = s1 + 2 = s0 + 3;
// s3 = s2 + 3 = s0 + 6;
// s3 = s2 + 4 = s0 + 10;

// my probe
// #define VHASH_PROBE(s,i)  s = s0 ^ i;
// the size of bucket must be the power of 2. 
// and the maximum number of probe is log2(bucket size)
//
#define VHASH_PROBE(s,i)  s = s0 ^ (1<<(i-1));

#ifdef  USE_BDW_GC
#include <gc.h>
/* the memory must be intialized with zeros */
#define MALLOC GC_MALLOC
#define REALLOC GC_REALLOC
#define FREE GC_FREE
#else
static inline void *my_malloc(size_t size)
{
    return calloc(1,size);
}
#define MALLOC my_malloc
#define REALLOC realloc
#define FREE free
#endif

inline static int is_empty(hk_t k)
{
    return k == EMPTY;
}

inline static int is_deleted(hk_t k)
{
    return k == DELETED;
}

inline static int is_occupied(hk_t k)
{
    return !is_empty(k) && !is_deleted(k);
}
static inline int compare_key_long(hk_t k1, hk_t k2)
{
    return k1 == k2;
}
static inline size_t hash_long(hk_t k)
{
    return (size_t) k;
}
static inline int compare_key_string(hk_t k1, hk_t k2)
{
    int ret = 0;
    if( k1 == k2 ){
        ret = 1;
    }else if(k1 == 0 || k2 == 0){
        ret = 0;
    }else{
        ret = strcmp((const char *) k1, (const char*) k2) == 0;
    }
    return ret;
}
static inline uint32_t SuperFastHash (const char * data, int len);
static inline size_t hash_string(hk_t k)
{
    return (size_t) SuperFastHash((const char*)k, (int)
                                  strlen((const char*)k));
}
static inline int compare_key_obj(hk_t k1, hk_t k2,size_t n)
{
    return k1 == k2 || memcmp((const void*)k1,(const void*)k2,n);
}
static inline size_t hash_obj(hk_t k,size_t n)
{
    return (size_t) SuperFastHash((const char*)k, (int) n);
}

inline static int compare(vhash_t * h, hk_t k1, hk_t k2)
{
    switch(h->flag.key_type){
    case VHASH_KEY_LONG:
        return compare_key_long(k1,k2);
    case VHASH_KEY_STRING:
        return compare_key_string(k1,k2);
    case VHASH_KEY_FIXED_SIZE_OBJ:
        return compare_key_obj(k1,k2,h->flag.key_size);
    case VHASH_KEY_OBJ_0:
        return VHASH_COMPARE_KEY_0(k1,k2);
    case VHASH_KEY_OBJ_1:
        return VHASH_COMPARE_KEY_1(k1,k2);
    case VHASH_KEY_OBJ_2:
        return VHASH_COMPARE_KEY_2(k1,k2);
    case VHASH_KEY_OBJ_3:
        return VHASH_COMPARE_KEY_3(k1,k2);
    case VHASH_KEY_OBJ_4:
        return VHASH_COMPARE_KEY_4(k1,k2);
    }
    abort();
    return 1;
}
inline static int hash(vhash_t * h, hk_t k1)
{
    switch(h->flag.key_type){
    case VHASH_KEY_LONG:
        return hash_long(k1);
    case VHASH_KEY_STRING:
        return hash_string(k1);
    case VHASH_KEY_FIXED_SIZE_OBJ:
        return hash_obj(k1,h->flag.key_size);
    case VHASH_KEY_OBJ_0:
        return VHASH_HASH_0(k1);
    case VHASH_KEY_OBJ_1:
        return VHASH_HASH_1(k1);
    case VHASH_KEY_OBJ_2:
        return VHASH_HASH_2(k1);
    case VHASH_KEY_OBJ_3:
        return VHASH_HASH_3(k1);
    case VHASH_KEY_OBJ_4:
        return VHASH_HASH_4(k1);
    }
    abort();
    return 1;
}
inline static size_t get_n_of_bucket(vhash_t * h)
{
    return (1<<h->flag.n_of_bucket);
}
inline static size_t s_mod_n_of_bucket(vhash_t * h, size_t s)
{
    return s & (get_n_of_bucket(h) - 1);
}
#define COLLECT_STAT  do {                              \
    h->n_of_total_probe += i;                           \
    h->n_of_total_probe_2 += i*i;  assert(i*i >= i);    \
    h->n_of_probe_sample ++;                            \
}while(0);
inline static pair_t * find_insert_pos(vhash_t * h, hk_t k, size_t max_probe)
{
    const size_t s0  = s_mod_n_of_bucket(h,hash(h,k));
    size_t       s   = s0;
    size_t       ret = s;
    size_t       i   = 0;
    while(i< max_probe){
        if(is_empty(h->bucket[s].k)){
            if(ret == s0){ // all slot are occupied.
                ret = s;
            }
            break;
        }else if(is_deleted(h->bucket[s].k)){ // found a deleted slot.
            ret = s;
        }else if(compare(h,h->bucket[s].k,k)){
            ret = s;
            break;
        }
        i++;
        VHASH_PROBE(s,i);
        s = s_mod_n_of_bucket(h,s);
        if(0) fprintf(stderr,__FILE__ ":%d:[%s] bucket(0x%ld) k = %ld, 0x%lx %ld 0x%lx\n", __LINE__, __FUNCTION__,
                      get_n_of_bucket(h),
                      k,
                      s0,i,s);
    }
    COLLECT_STAT;
    if(i == max_probe){
        return NULL;
    }
    assert(((is_deleted(h->bucket[ret].k) || is_empty(h->bucket[ret].k)) || 
            (compare(h,h->bucket[ret].k, k))));
    return &(h->bucket[ret]);
}
inline static
void resize(vhash_t * h, int newsize)
{
    if(0)fprintf(stderr,__FILE__ ":%d:[%s] grow when load factor is %f(%ld,%ld)\n", __LINE__, __FUNCTION__,
                 (double)h->n_of_elt/get_n_of_bucket(h),
                 h->n_of_elt,
                 get_n_of_bucket(h));
    pair_t * old_bucket = h->bucket;
    size_t old_size = get_n_of_bucket(h);
    h->flag.n_of_bucket += newsize;
    h->bucket = (pair_t*) MALLOC(sizeof(pair_t)* get_n_of_bucket(h));
    assert(h->bucket);
    for(size_t n = 0 ; n < old_size; ++n){
        pair_t * kv = &old_bucket[n];
        if(!is_occupied(kv->k)) continue;
        const size_t s0 = s_mod_n_of_bucket(h,hash(h,kv->k));
        size_t i = 0;
        size_t s = s0;
        const size_t max_probe =  MAX_PROBE_ATTEMPT;
        while(i < max_probe && !is_empty(h->bucket[s].k)){
            i++;
            VHASH_PROBE(s,i);
        }
        COLLECT_STAT;
        assert(i < get_n_of_bucket(h));
        h->bucket[s].k = kv->k;
        h->bucket[s].v = kv->v;
    }
    FREE(old_bucket);
    return;
}
inline static
void grow(vhash_t * h)
{
    h->n_of_grow ++;
    resize(h, 1);
    return;
}
inline static void shrink(vhash_t * h)
{
    h->n_of_shrink ++;
    resize(h, -1);
    return;
}

vhash_t * vhash_create(key_type_t ktype, unsigned short key_size)
{
    vhash_t * r = (vhash_t *)MALLOC(sizeof(vhash_t)); 
    r->v0 = 0; 
    r->v1 = 0; 
    r->flag.key_type = (unsigned int)ktype;
    r->flag.key_size = key_size;
    r->flag.v0_exist = 0; 
    r->flag.v1_exist = 0; 
    r->flag.n_of_bucket = VHASH_INIT_N_OF_BUCKET;
    r->bucket = (pair_t *)MALLOC(sizeof(pair_t)* get_n_of_bucket(r));
    r->n_of_elt = 0;
    r->n_of_grow = 0;
    r->n_of_shrink = 0;
    r->n_of_total_probe = 0;
    r->n_of_total_probe_2 = 0;
    r->n_of_probe_sample = 0;
    return r;
}
vhash_t * vhash_insert(vhash_t * h, hk_t k, hv_t v)
{
    if(k == EMPTY) {
        h->flag.v0_exist = 1;
        h->v0 = v;
        return h;
    }
    if(k == DELETED) {
        h->flag.v1_exist = 1;
        h->v1 = v;
        return h;
    }
    pair_t * p = NULL;
    const size_t max_probe = MAX_PROBE_ATTEMPT;
    int i = 0;
    for(i = 0, p = find_insert_pos(h,k,max_probe);
        p ==NULL && i < MAX_GROW_ATTEMPT;
        p = find_insert_pos(h,k,max_probe)){
        grow(h);
    }
    if(!p){
        assert(0 && "out of bucket");
        exit(1);
    }
    if(!is_occupied(p->k)){
        h->n_of_elt++;
    }
    p->k = k;
    p->v = v;
    return h;
}
pair_t* vhash_get(vhash_t * h, hk_t k)
{
    const size_t s0 = s_mod_n_of_bucket(h,hash(h,k));
    size_t i = 0;
    size_t s = s0;
    const size_t max_probe = MAX_PROBE_ATTEMPT;
    while(i < max_probe && !is_empty(h->bucket[s].k)){
        if(compare(h,h->bucket[s].k,k)){
        //if(h->bucket[s].k == k){
            return &h->bucket[s];
        }
        i++;
        VHASH_PROBE(s,i);
    }
    return NULL;
}
pair_t* vhash_delete(vhash_t * h, hk_t k)
{
    // shrink when load < 0.25
    if(h->n_of_elt * VHASH_SHRINK_THRESHOLD < get_n_of_bucket(h)){
        shrink(h);
    }
    pair_t * p = vhash_get(h,k);
    if(p){
        p->k = DELETED;
        p->v = 0;
        h->n_of_elt --;
    }
    return p;
}

void vhash_destroy(vhash_t *h)
{
    FREE(h->bucket);
    FREE(h);
}
size_t vhash_size(vhash_t * h)
{
    size_t ret = 0;
    if(is_occupied(h->flag.v0_exist)){
        ret ++;
    }
    if(is_occupied(h->flag.v1_exist)){
        ret ++;
    }
    return h->n_of_elt + ret;
}
size_t vhash_iter_start(vhash_t * h)
{
    return 0;
}
size_t vhash_iter_end(vhash_t * h)
{
    return (size_t) -1;
}
size_t vhash_iter_next(vhash_t* h, size_t c)
{
    for(size_t s = c + 1; s < get_n_of_bucket(h); s++){
        if(!is_occupied(h->bucket[s].k)) continue;
        return s;
    }
    return (size_t) -1;
}
pair_t* vhash_iter_get(vhash_t* h, size_t c)
{
    return &(h->bucket[c]);
}
vhash_t * vhash_print(vhash_t * h, FILE * fp)
{
    fprintf(fp,"hash (%p) size: %ld\n",(void*)h,
            vhash_size(h));
    fprintf(fp,"load n/N=%f(%ld/%ld)\n",
            (double)h->n_of_elt/get_n_of_bucket(h),
            h->n_of_elt,
            get_n_of_bucket(h));
    double avg = (double)h->n_of_total_probe / h->n_of_probe_sample;
    fprintf(fp,"avg probe: %f (%ld/%ld)\n", 
            avg,
            h->n_of_total_probe,
            h->n_of_probe_sample);
    fprintf(fp,"var probe: %f\n", 
            (((double)h->n_of_total_probe_2) / h->n_of_probe_sample)
            - 
            (avg * avg));
    fprintf(fp,"# of grow: %ld\n", h->n_of_grow);
    if(h->flag.v0_exist){
        fprintf(fp,"- %ld(0x%lx) 0x%lx: 0x%lx\n",0l,0l,0l,h->v0);
    }
    if(h->flag.v1_exist){
        fprintf(fp,"- %ld(0x%lx) 0x%lx: 0x%lx\n",1l,1l,1l,h->v1);
    }
    for(size_t s = 0 ; s < get_n_of_bucket(h); s++){
        if(!is_occupied(h->bucket[s].k)) continue;
        fprintf(fp,"%ld(0x%lx) 0x%lx: 0x%lx\n",s,s,h->bucket[s].k,h->bucket[s].v);
    }
    return h;
}



// http://www.azillionmonkeys.com/qed/hash.html
/*
IMPORTANT NOTE: Since there has been a lot of interest for the code below, I have decided to additionally provide it under the LGPL 2.1 license. This provision applies to the code below only and not to any other code including other source archives or listings from this site unless otherwise specified.

The LGPL 2.1 is not necessarily a more liberal license than my derivative license, but this additional licensing makes the code available to more developers. Note that this does not give you multi-licensing rights. You can only use the code under one of the licenses at a time. 

*/
#include <stdio.h>
#include <stdint.h>


#undef get16bits
#if (defined(__GNUC__) && defined(__i386__)) || defined(__WATCOMC__) \
  || defined(_MSC_VER) || defined (__BORLANDC__) || defined (__TURBOC__)
#define get16bits(d) (*((const uint16_t *) (d)))
#endif

#if !defined (get16bits)
#define get16bits(d) ((((uint32_t)(((const uint8_t *)(d))[1])) << 8)\
                      +(uint32_t)(((const uint8_t *)(d))[0]) )
#endif

uint32_t SuperFastHash (const char * data, int len) {
  uint32_t hash = len, tmp;
  int rem;

  
  if (len <= 0 || data == NULL) return 0;

  if(0){
    rem = (4 - (((intptr_t)data) & 3)) & 3;
    rem = rem > len?len:rem;
  
    switch (rem) {
    case 3: hash += get16bits (data);
      hash ^= hash << 16;
      hash ^= ((signed char)data[sizeof (uint16_t)]) << 18;
      hash += hash >> 11;
      break;
    case 2: hash += get16bits (data);
      hash ^= hash << 11;
      hash += hash >> 17;
      break;
    case 1: hash += (signed char)*data;
      hash ^= hash << 10;
      hash += hash >> 1;
    }
    len -= rem;
    data = data + rem;
  }

  rem = len & 3;
  len >>= 2;
  
  /* Main loop */
  for (;len > 0; len--) {
    hash  += get16bits (data);
    tmp    = (get16bits (data+2) << 11) ^ hash;
    hash   = (hash << 16) ^ tmp;
    data  += 2*sizeof (uint16_t);
    hash  += hash >> 11;
  }

  /* Handle end cases */
  switch (rem) {
  case 3: hash += get16bits (data);
    hash ^= hash << 16;
    hash ^= ((signed char)data[sizeof (uint16_t)]) << 18;
    hash += hash >> 11;
    break;
  case 2: hash += get16bits (data);
    hash ^= hash << 11;
    hash += hash >> 17;
    break;
  case 1: hash += (signed char)*data;
    hash ^= hash << 10;
    hash += hash >> 1;
  }

  /* Force "avalanching" of final 127 bits */
  hash ^= hash << 3;
  hash += hash >> 5;
  hash ^= hash << 4;
  hash += hash >> 17;
  hash ^= hash << 25;
  hash += hash >> 6;

  return hash;
}

/* Local Variables: */
/* mode:c */
/* c-basic-offset: 4*/
/* End: */
