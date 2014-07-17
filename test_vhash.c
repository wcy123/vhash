#define _POSIX_C_SOURCE 
//#define _GNU_SOURCE
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include "vhash.h"
//#include "vhash_imp.h"
/* void test_is_overload() */
/* { */
/*   for(size_t N = 32; N < 65536; N = N*2){ */
/*     int count = 0; */
/*     for(size_t n = 0; n < N; ++n){ */
/*       if(is_overload(n,N)){ */
/*         fprintf(stderr,__FILE__ ":%d:[%s] %ld %ld %f\n", __LINE__, __FUNCTION__,n,N, (double)n/N); */
/*         if(count ++ >= 0){ */
/*           break; */
/*         } */
/*       } */
/*     } */
/*   } */
/*   return; */
/* } */
void test_0()
{
    vhash_t * h = vhash_create(VHASH_KEY_LONG,0);
    vhash_insert(h,0,0);
    vhash_insert(h,1,1);
    vhash_insert(h,2,2);
    assert(vhash_size(h) == 2);
}
void test_1()
{
    vhash_t * h = vhash_create(VHASH_KEY_LONG,0);
    for(size_t i = 0 ; i < 7; i ++){
        vhash_insert(h, i + 7, i );
        vhash_print(h,stdout);
    }
    assert(vhash_size(h) == 7);
}
void test_2()
{
    vhash_t * h = vhash_create(VHASH_KEY_LONG,0);
    for(size_t i = 0 ; i < 9; i ++){
        vhash_insert(h, 6 + (i+1)*8, i );
    }
    vhash_print(h,stdout);
    assert(vhash_size(h) == 9);
    for(size_t i = 0 ; i < 9; i ++){
        hk_t k = 6 + (i+1)*8;
        pair_t * p = vhash_get(h,k);
        fprintf(stderr,__FILE__ ":%d:[%s] 0x%lx -> 0x%lx\n", __LINE__, __FUNCTION__
                ,k
                ,p->v);
        assert(p && p->k == k && p->v == (hv_t)i);
    }
    for(size_t i = 0 ; i < 9; i ++){
        hk_t k = 6 + (i+1)*8;
        pair_t * p = vhash_delete(h,k);
        assert(p);
        fprintf(stderr,__FILE__ ":%d:[%s] %ld\n", __LINE__, __FUNCTION__,
                vhash_size(h));
        assert(vhash_size(h) == 9 - i -1);
    }
    vhash_print(h,stdout);
    assert(vhash_size(h) == 0);
    size_t n_of_grow = h->n_of_grow;
    for(size_t i = 0 ; i < 9; i ++){
        vhash_insert(h, 6 + (i+1)*8, i );
    }
    vhash_print(h,stdout);
    assert(vhash_size(h) == 9);  
    assert(h->n_of_grow == n_of_grow);  
}
int test_3(int argc, char *argv[])
{
    static char buf[1024];
    vhash_t * h = vhash_create(VHASH_KEY_STRING,0);
    FILE * fp = fopen(argv[1],"r");
    assert(fp);
    char * s;
    if(1) 
        while( (s = fgets(buf,1024,fp)) != NULL){
            char * t;
            char * save;
            int i = 0;
            while((t = strtok_r(i==0?s:NULL, " \n\t!={}\",()", &save)) != NULL){
                char * t2 = strdup(t);
                pair_t * p = vhash_get(h,(hk_t) t2);
                if(p){
                    p->v++;
                }else{
                    vhash_insert(h,(hk_t)t2,1);
                }
                i++;
            }
        }
    fclose(fp);
    fp = stdout;
    for(size_t s = vhash_iter_start(h);
        s !=vhash_iter_end(h);
        s = vhash_iter_next(h,s)){
        pair_t * p = vhash_iter_get(h,s);
        fprintf(stderr,__FILE__ ":%d:[%s]\t%d\t%s\n", __LINE__, __FUNCTION__,
                (int)p->v,
                (const char *) p->k);
    }
    vhash_destroy(h);
    return 0;
}
int main(int argc, char *argv[])
{
    return test_3(argc,argv);
}
/* Local Variables: */
/* mode:c */
/* c-basic-offset: 4*/
/* End: */
