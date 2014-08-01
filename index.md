---
layout : page
title : Introduction
---

vhash
=====

A drop in replacement for stl::unordered_map and stl::set

Why it is faster
================
As we known, there are [two probing algorithms](http://en.wikipedia.org/wiki/Quadratic_probing) for open address hashing table.

 1. linear probing
probing sequenc: $$H+1$$, $$H+2$$, $$H+3$$, $$...$$ , $$H+k$$

$$
\begin{aligned}
 s_0 &=& H + 0 \\
 s_1 &=& H + 1  \\
 &...&  \\
 s_k &=& H + k 
\end{aligned}
$$

$$
        s_k = s_{k-1} + 1
$$


 2. quadratic probing
probing sequenc: $$H$$, $$H+1$$, $$H+3$$, $$H+6$$, $$...$$ , $$H+ \frac{k(k-1)}{2}$$

$$
\begin{aligned}
 s_0 &=& H + 0 \\
 s_1 &=& H + 1  \\
 &...&  \\
 s_k &=& H + \frac{k(k-1)}{2}
\end{aligned}
$$

$$
        s_k = s_{k-1} + k
$$

`vhash` uses a new probing algorithm

$$
s_k = s_0 \oplus (1<<(k-1))
$$

$$\oplus$$ means bit-wise XOR.

If $$s_k$$ is a 32 bit ingeter, we get $$s_1$$ by flipping the first bit
of $$s_0$$, $$s_2$$ by flipping the second bit of $$s_0$$. The maximum
number of probes is $$\log_2(N)$$, where $$N$$ is the number of buckets.
For other two algorithms, the maximum number of probes is $$N$$. When no
avaiable bucket could be found, we try to enlarge the buckets.
Because we do not probe all avaiable buckets, in average, the load
factor $$n/N$$ is most likely less than 0.5, where $$n$$ is the number of
elements. However, in a special case, if we add number sequencely,
there is no collision at all, so the load factor becomes 1 before
enlarging buckets. $$N$$ is a power of 2.


benchmark
=========
[benchmark results](charts.html)

benchmark code https://github.com/mackstann/hash-table-shootout.git

how to add `vhash` into the benchmark
--------------------------------------

1. change the `Makefile` as below
     
{% highlight makefile %}
build/vhash: src/vhash_test.cc src/template.c
  g++ -std=c++11 -O2 -I ~/d/working/vhash  src/vhash_test.cc -o build/vhash  -lm
{% endhighlight %}

2. add `vhash_test.cc` into `src`

{% highlight c %}
#include <inttypes.h>
#include <vhash.h>
typedef voba::unordered_map<int64_t, int64_t> hash_t;
struct my_hash {
  size_t operator()(const char * a){
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
{% endhighlight %}

It is important to have a better hashing function. This algorithm is
very sensitive to the hashing function, whether hash value is randomly
distributed or not.  Typically, a new string pointer has 8-byte or
16-byte alignment so that the last 3 or 4 bits are always be
zero. This leads to very bad performance with the new probing
algorithm. After shifting 4-bit to left, we have satisfied
performance.




