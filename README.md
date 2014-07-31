vhash
=====

A drop in replacement for stl::unordered_map and stl::set

Why it is faster
================
As we known, there are [two probing algorithms](http://en.wikipedia.org/wiki/Quadratic_probing) for open address hashing table.

1. linear probing

```
probing sequenc: H+1, H+2, H+3, ... , H+k

s[0] = H
s[1] = H + 1
...
s[k] = H + k 
i.e.
s[k] = s[k-1] + 1
```

2. quadratic probing
```
probing sequenc: H+1, H+3, H+6, ... , H+k*k*1/2 - k*1/2

s[0] = H
s[1] = H + 1
...
s[k] = H + k*k*1/2 - k*1/2 
i.e.
s[k] = s[k-1] + k
```

`vhash` uses a new probing algorithm ``` s[k] = s[0] ^ (1<<(k-1)) ```
If `s[k]` is a 32 bit ingeter, we get `s[1]` by flipping the first bit
of `s[0]`, `s[2]` by flipping the second bit of `s[0]`. The maximum
number of probes is `log2(n_of_bucket)`, while other two algorithms,
the maximum number of probs is `n_of_bucket`. When we couldnot find a
avaiable bucket, we try to enlarge the buckets.  Because we do not
probe all avaiable buckets, in average, it is hardly to have load
factor `n_of_elt/n_of_bucket`, which is larger than 0.5. In a special
case, if we add number sequencely, there is no collision at all, so
the load factor becomes 1 before enlarging buckets.


benchmark
=========
http://htmlpreview.github.io/?https://github.com/wcy123/vhash/blob/test_publish_chart_html/charts.html

benchmark code https://github.com/mackstann/hash-table-shootout.git

how to add `vhash` into the benchmark
--------------------------------------

1. change the `Makefile` as below
     
```makefile
build/vhash: src/vhash_test.cc src/template.c
  g++ -std=c++11 -O2 -I ~/d/working/vhash  src/vhash_test.cc -o build/vhash  -lm
```

2. add `vhash_test.cc` into `src`

```c
#include <inttypes.h>
#include <vhash.h>
typedef voba::unordered_map<int64_t, int64_t> hash_t;
struct my_equal {
  bool operator()(const char * a, const char * b){
    return a == b;
  }
};
struct my_hash {
  size_t operator()(const char * a){
    //return static_cast<size_t>(reinterpret_cast<const int64_t>(a)>>4);
    return static_cast<size_t>(reinterpret_cast<const int64_t>(a) >> 4);
  }
};
typedef voba::unordered_map<const char *, int64_t, my_hash> str_hash_t;
#define SETUP hash_t hash; str_hash_t str_hash;
#define INSERT_INT_INTO_HASH(key, value) hash.insert(hash_t::value_type(key, value))
#define DELETE_INT_FROM_HASH(key) hash.erase(key)
#define INSERT_STR_INTO_HASH(key, value) str_hash.insert(str_hash_t::value_type(key, value))
#define DELETE_STR_FROM_HASH(key) str_hash.erase(key)
#include "template.c"

```


