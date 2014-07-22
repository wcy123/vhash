#pragma once
#include <cassert>  // for assert
#include <utility>  // for std::pair
#include <cstddef>  // for size_t ptrdiff_t
#include <functional> // for std::equal_to and std::hash
#include <memory>    // for  struct allocator_traits;
#include <algorithm> // for min
#include <type_traits>
// initial size of buckets
#define VHASH_INIT_N_OF_BUCKET 8

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
#define MAX_PROBE_ATTEMPT (n_of_bucket)

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
#define COLLECT_STAT  do {                              \
        n_of_total_probe += i;                          \
        n_of_total_probe_2 += i*i;  assert(i*i >= i);   \
        n_of_probe_sample ++;                           \
    }while(0);


namespace voba {
    template<
        class Key,
        class T,
        class Hash = std::hash<Key>, // std::hash(const char*) has poor performance
        class KeyEqual = std::equal_to<Key>,
        class Allocator = std::allocator< typename std::conditional<std::is_empty<T>::value , Key, std::pair<const Key, T>  > :: type >
        > class unordered_map;
    
    template <typename T,bool empty>
    class allocate_bucket;
#define AB allocate_bucket<unordered_map,std::is_empty<mapped_type>::value >::op
    
    template<
        class Key,
        class T,
        class Hash,
        class KeyEqual,
        class Allocator
        > class unordered_map
    {
    public:
        typedef Key key_type;
        typedef T mapped_type;
        typedef typename std::conditional<std::is_empty<T>::value , Key, std::pair<const Key, T>  > :: type value_type;
        typedef std::size_t size_type;
        typedef std::ptrdiff_t difference_type;
        typedef Hash hasher;
        typedef KeyEqual key_equal;
        typedef Allocator allocator_type;
        typedef value_type& reference;
        typedef const value_type& const_reference;
        typedef typename std::allocator_traits<Allocator>::pointer pointer;
        typedef typename std::allocator_traits<Allocator>::const_pointer const_pointer;
    private:
      enum state {
        START, V0, V1, NORMAL, END
      };
    public:
        template<typename T1, typename T2, typename T3>
        struct unordered_map_iterator {
        public:
            unordered_map_iterator()
              : me(nullptr),s(0), flag(END)
                {
                }
            ~unordered_map_iterator() {}
        private:
            unordered_map_iterator(T1 * m, bool is_end)
                : me(m),s(0), flag(is_end?END:START)
                {
                    find_next();
                }
            unordered_map_iterator(T1 * m, size_type __s, state f)
                : me(m),s(__s), flag(f)
                {
                }
        public:
            T2 operator*() const
                {
                    switch(flag){
                    case NORMAL:
                        return me->bucket[s];
                    case V0:
                        return *me->v0;
                    case V1:
                        return *me->v1;
                    default:
                        assert(false);
                    }
                    abort();
                }
            T3 operator->() const
                {
                    return &(operator*());
                }
            unordered_map_iterator& operator++()
                {
                    find_next();
                    return *this;
                }
            unordered_map_iterator operator++(int) { unordered_map_iterator tmp(*this); ++*this; return tmp; }
            
            // Comparison.
            template<typename It>
            bool operator==(const It& it) const {
                switch(flag){
                case NORMAL:
                case V1:
                case V0:
                    return flag == it.flag && s == it.s;
                case END:
                    return flag == it.flag;
                default:
                    assert(false);
                }
                abort();
                return true;
            }
            bool operator!=(const unordered_map_iterator& it) const { return ! (*this == it); }

        private:
            T1* me;
            size_type s;
            state flag;
            friend class unordered_map;
        private:
            void find_next()
                {
                    // note, no "break" at each branch of "case"
                    switch(flag){
                    case START:
                        if(me->v0){ flag = V0; return ;}
                    case V0:
                        if(me->v1){ flag = V1; return ;}
                    case V1:
                        s = static_cast<size_type>(-1);
                    case NORMAL:
                        for(++s; s < me->get_n_of_bucket();++s){
                            if(me->is_occupied(me->bucket[s].first)){
                                flag = NORMAL;
                                return;
                            }
                        }
                    default:
                        flag = END;
                    }
                }
        };
        typedef unordered_map_iterator<unordered_map, reference,pointer> iterator;
        typedef unordered_map_iterator<const unordered_map, const_reference,const_pointer> const_iterator;
        typedef iterator  local_iterator;
        typedef const_iterator const_local_iterator;
    public:
        explicit unordered_map( size_type bucket_count = VHASH_INIT_N_OF_BUCKET,
                                const Hash& hash_v = Hash(),
                                const KeyEqual& equal_v = KeyEqual(),
                                const Allocator& alloc = Allocator() )
            :
            hash_m(hash_v)
            ,allocator(alloc)
            ,equal(equal_v)
            ,v0(nullptr)
            ,v1(nullptr)
            ,n_of_bucket(my__log2(bucket_count))
            ,bucket(AB(allocator,get_n_of_bucket()))
            ,n_of_elt(assert_empty_not_eq_delete.zero) // prevent assert_empty_not_eq_delete from optimizing
            ,n_of_grow(0)
            ,n_of_shrink(0)
            ,n_of_total_probe(0)
            ,n_of_total_probe_2(0)
            ,n_of_probe_sample(0)
            {
            }
        explicit unordered_map( size_type bucket_count,
                                const Allocator& alloc = Allocator() ):
            unordered_map(bucket_count, Hash(), KeyEqual(), alloc)
            {
            }
        explicit unordered_map( size_type bucket_count,
                                const Hash& hash,
                                const Allocator& alloc = Allocator() ):
            unordered_map(bucket_count, hash, KeyEqual(), alloc)
            {
            }
        explicit unordered_map( const Allocator& alloc ):
            unordered_map(VHASH_INIT_N_OF_BUCKET, Hash(), KeyEqual(), alloc)
            {
            }
        template< class InputIt >
        unordered_map( InputIt first, InputIt last,
                       size_type bucket_count = VHASH_INIT_N_OF_BUCKET,
                       const Hash& hash = Hash(),
                       const KeyEqual& equal = KeyEqual(),
                       const Allocator& alloc = Allocator()):
          unordered_map(std::max(static_cast<size_type>(last-first),static_cast<size_type>(VHASH_INIT_N_OF_BUCKET)), Hash(), KeyEqual(), alloc)
            {
                insert(first,last);
            }
        template< class InputIt >
        unordered_map( InputIt first, InputIt last,
                       size_type bucket_count,
                       const Allocator& alloc = Allocator() )
            :unordered_map(first, last, bucket_count, Hash(), KeyEqual(), alloc)
            {
            }
        template< class InputIt >
        unordered_map( InputIt first, InputIt last,
                       size_type bucket_count,
                       const Hash& hash,
                       const Allocator& alloc = Allocator() )
            :unordered_map(first, last, bucket_count, hash, KeyEqual(), alloc)
            {
            }
        unordered_map( const unordered_map& other )
            : hash_m(other.hash_m)
            ,allocator(other.allocator)
            ,equal(other.equal)
            ,v0(other.v0)
            ,v1(other.v1)
            ,n_of_bucket(other.n_of_bucket)
            ,bucket(AB(allocator,get_n_of_bucket()))
            ,n_of_elt(assert_empty_not_eq_delete.zero) // prevent assert_empty_not_eq_delete from optimizing
            ,n_of_grow(0)
            ,n_of_shrink(0)
            ,n_of_total_probe(0)
            ,n_of_total_probe_2(0)
            ,n_of_probe_sample(0)
            {
                //assert(0&&"todo");
            }
        unordered_map( const unordered_map& other, const Allocator& alloc )
            : hash_m(other.hash_m)
            ,allocator(alloc)
            ,equal(other.equal)
            ,v0(other.v0)
            ,v1(other.v1)
            ,n_of_bucket(other.n_of_bucket)
            ,bucket(AB(allocator,get_n_of_bucket()))
            ,n_of_elt(assert_empty_not_eq_delete.zero) // prevent assert_empty_not_eq_delete from optimizing
            ,n_of_grow(0)
            ,n_of_shrink(0)
            ,n_of_total_probe(0)
            ,n_of_total_probe_2(0)
            ,n_of_probe_sample(0) {
            // TODO
        }
        unordered_map( unordered_map&& other )
            : hash_m(other.hash_m)
            ,allocator(other.allocator)
            ,equal(other.equal)
            ,v0(other.v0)
            ,v1(other.v1)
            ,n_of_bucket(other.n_of_bucket)
            ,bucket(other.bucket)
            ,n_of_elt(assert_empty_not_eq_delete.zero) // prevent assert_empty_not_eq_delete from optimizing
            ,n_of_grow(0)
            ,n_of_shrink(0)
            ,n_of_total_probe(0)
            ,n_of_total_probe_2(0)
            ,n_of_probe_sample(0) {
        }
        unordered_map( unordered_map&& other, const Allocator& alloc )
            {
                assert(0&&"not implemented");
            }
        unordered_map( std::initializer_list<value_type> init,
                       size_type bucket_count = VHASH_INIT_N_OF_BUCKET,
                       const Hash& hash = Hash(),
                       const KeyEqual& equal = KeyEqual(),
                       const Allocator& alloc = Allocator() ):
            unordered_map(init.begin(), init.end(),
                          bucket_count,
                          hash,equal,alloc) {}
        unordered_map( std::initializer_list<value_type> init,
                       size_type bucket_count,
                       const Allocator& alloc = Allocator() ):
            unordered_map(init.begin(), init.end(),
                          bucket_count,
                          Hash(),
                          KeyEqual(),
                          alloc) {}
        unordered_map( std::initializer_list<value_type> init,
                       size_type bucket_count,
                       const Hash& hash,
                       const Allocator& alloc = Allocator() ):
            unordered_map(init.begin(), init.end(),
                          bucket_count,
                          hash, KeyEqual(),alloc) {}
        unordered_map&
        operator=(const unordered_map __ht)
            {
                swap(__ht);
                return *this;
            }

        unordered_map&
        operator=(unordered_map&& ht)
            {
                clear();
                swap(ht);
                return *this;
            }

        unordered_map&
        operator=(std::initializer_list<value_type> __l)
            {
                insert(__l.begin(), __l.end());
                return *this;
            }
        ~unordered_map() noexcept
            {
                release_v0v1(v0);
                release_v0v1(v1);
                release_bucket();
            }
        
        void swap(unordered_map& x)
            {
                std::swap(v0,x.v0);
                std::swap(v1,x.v1);
                std::swap(n_of_bucket,x.n_of_bucket);
                std::swap(bucket,x.bucket);
                std::swap(n_of_elt,x.n_of_elt);
                std::swap(n_of_grow,x.n_of_grow);
                std::swap(n_of_shrink,x.n_of_shrink);
                std::swap(n_of_total_probe,x.n_of_total_probe);
                std::swap(n_of_total_probe_2,x.n_of_total_probe_2);
                std::swap(n_of_probe_sample,x.n_of_probe_sample);
            }
        void clear()
            {
                release_v0v1(v0);
                release_v0v1(v1);
                release_bucket();
                n_of_bucket = my__log2(VHASH_INIT_N_OF_BUCKET);
                bucket = AB(allocator,get_n_of_bucket());                
                n_of_elt = 0;
                n_of_grow = 0;
                n_of_shrink = 0;
                n_of_total_probe = 0;
                n_of_total_probe_2 = 0;
                n_of_probe_sample = 0;
            }
        template< class InputIt >
        void
        insert( InputIt first, InputIt last)
            {
                for(;first!=last;++first){
                    insert(*first);
                }
                return;
            }
        value_type* insert(const value_type & a )
            {
                return my_insert(a);
            }
        iterator find(const key_type& k)
            {
                return my_find<unordered_map&, iterator>(*this,k);
            }
        const_iterator
        find(const key_type& k) const
            {
                return my_find<const unordered_map&, const_iterator>(*this,k);
            }
        bool
        erase(iterator __position)
            {
                return my_erase(__position); 
            }
        bool
        erase(const key_type & key)
            {
                iterator i = find(key);
                if(i != end()){
                    return my_erase(i);
                }
                return false;
            }
        allocator_type get_allocator() const noexcept
            {
                return allocator;
            }
        hasher
        hash_function() const
            { return hash_m; }

        key_equal
        key_eq() const
            { return equal; }

        void max_load_factor(float) {}
        bool empty() const noexcept
            {
                return !(n_of_elt != 0 ||  v0 != nullptr || v1 !=nullptr);
            }
        size_type size() const noexcept
            {
                return n_of_elt + (v0 != nullptr?1:0) + (v1 != nullptr?1:0);
            }
        size_type max_size() const noexcept
            {
                return get_n_of_bucket();
            }
        iterator begin() noexcept
            {
                return iterator(this,false);
            }
        const_iterator begin() const noexcept
            {
                return const_iterator(this,false);
            }
        iterator end() noexcept
            {
                return iterator(this,true);
            }
        const_iterator end() const noexcept
            {
                return const_iterator(this,true);
            }
        
    private:
        hasher hash_m;
        allocator_type allocator;
        key_equal equal;
        value_type * v0;
        value_type * v1;
        size_type n_of_bucket;
        value_type * bucket;
        size_type n_of_elt;
        size_type n_of_grow;
        size_type n_of_shrink;
        size_type n_of_total_probe;
        size_type n_of_total_probe_2;
        size_type n_of_probe_sample;
    private:
    public:
        static const key_type EMPTY;
        static const key_type DELETED;
    private:
        static size_type my__log2(size_type a)
            {
                size_type r = 0;
                while(a>>=1) r++;
                return r;
            }
        inline size_type get_n_of_bucket() const noexcept
            {
                return (1<<n_of_bucket);
            }
        class Assert_EMPTY_not_eq_DELETE {
        public:
            explicit Assert_EMPTY_not_eq_DELETE():
                zero(0)
                {
                    key_equal equal;
                    if(equal(unordered_map::EMPTY, unordered_map::DELETED)){
                        assert(false && "EMPTY should not equal to DELETED");
                        abort();
                    }
                }
            int zero;
        };
        static Assert_EMPTY_not_eq_DELETE assert_empty_not_eq_delete;

        bool is_empty(const key_type& k) const noexcept
            {
                return equal(k,EMPTY);;
            }
    
        bool is_deleted(const key_type& k) const noexcept
            {
                return equal(k,DELETED);
            }
        bool is_occupied(const key_type& k) const noexcept
            {
                return !is_empty(k) && !is_deleted(k);
            }
        size_t s_mod_n_of_bucket(size_t s) const noexcept
            {
                return s & (get_n_of_bucket() - 1);
            }
        void assign_v0v1(value_type* &p, const value_type & v)
            {
                if(p==nullptr){
                    p = allocator.allocate(1);
                    new(p) value_type(v);
                }else{
                    assign(p,v);
                }
            }
        void release_v0v1(value_type* &p)
            {
                if(p){
                    p->~value_type();
                    allocator.deallocate(p,1);
                    p = nullptr;
                }
            }
        void release_bucket()
            {
                for(size_type i = 0; i < get_n_of_bucket(); ++i){
                    bucket[i].~value_type();
                }
                allocator.deallocate(bucket,get_n_of_bucket());
            }
        void assign(value_type* p, const value_type & v )
            {
                p->~value_type();
                new(p) value_type(v);
            }
        value_type *
        find_insert_pos(const key_type& k, size_t max_probe)
            {
                key_type k2 = k;
                const size_type s0  = s_mod_n_of_bucket(hash_m(k2));
                size_type       s   = s0;
                size_type       ret = s;
                size_type       i   = 0;
                while(i< max_probe){
                    if(is_empty(bucket[s].first)){
                        if(ret == s0) {
                            // all slot are occupied.
                            ret = s;
                        }
                        break;
                    }else if(is_deleted(bucket[s].first)){ // found a deleted slot.
                        ret = s;
                    }else if(equal(bucket[s].first,k)){
                        ret = s;
                        break;
                    }
                    i++;
                    VHASH_PROBE(s,i);
                    s = s_mod_n_of_bucket(s);
                }
                COLLECT_STAT;
                if(i == max_probe){
                    return NULL;
                }
                assert(((is_deleted(bucket[ret].first) || is_empty(bucket[ret].first)) || 
                        (equal(bucket[ret].first, k))));
                return &(bucket[ret]);
            }
        value_type* my_insert(const value_type& a)
            {
                const key_type &k = a.first;
                if(is_empty(k)) {
                    assign_v0v1(v0,a);
                    return v0;
                }
                if(is_deleted(k)) {
                    assign_v0v1(v1,a);
                    return v1;
                }
                value_type * p = nullptr;
                const size_t max_probe = MAX_PROBE_ATTEMPT;
                int i = 0;
                for(i = 0, p = find_insert_pos(k,max_probe);
                    p == NULL && i < MAX_GROW_ATTEMPT;
                    p = find_insert_pos(k,max_probe)){
                    grow();
                }
                if(!p){
                    assert(0 && "out of bucket");
                    abort();
                }
                if(!is_occupied(p->first)){
                    n_of_elt++;
                }
                assign(p,a);
                return p;
            }
        template<typename T1, typename Iter>
        static Iter my_find(T1 me, const key_type & k)
            {
                if(me.v0 && me.equal(me.v0->first, k)) return Iter(&me,0,V0);
                if(me.v1 && me.equal(me.v1->first, k)) return Iter(&me,0,V1);
                const size_type s0 = me.s_mod_n_of_bucket(me.hash_m(k));
                size_type i = 0;
                size_type s = s0;
                const size_type max_probe = me.n_of_bucket;
                while(i < max_probe && !me.is_empty(me.bucket[s].first)){
                    if(me.equal(me.bucket[s].first,k)){
                        return Iter(&me,s, NORMAL);
                    }
                    i++;
                    VHASH_PROBE(s,i);
                }
                return Iter(&me,i, END);
            }
        bool
        my_erase(iterator __position)
            {
                switch(__position.flag)
                {
                case V0: release_v0v1(v0); break;
                case V1: release_v0v1(v1); break;
                case NORMAL:
                    assign(bucket + __position.s, make_pair(DELETED,mapped_type()));
                    n_of_elt --;
                    if(n_of_elt * VHASH_SHRINK_THRESHOLD < get_n_of_bucket()){
                        shrink();
                    }
                    break;
                default:
                    assert(false);
                }
                return true;
            }

        void grow()
            {
                n_of_grow ++;
                resize(1);
                return;
            }
        void resize(int newsize)
            {
                if(0)fprintf(stderr,__FILE__ ":%d:[%s] resize when load factor is %f(%ld,%ld)\n", __LINE__, __FUNCTION__,
                             (double)n_of_elt/get_n_of_bucket(),
                             n_of_elt,
                             get_n_of_bucket());
                value_type * old_bucket = bucket;
                size_type old_size = get_n_of_bucket();
                n_of_bucket += newsize;
                bucket = AB(allocator,get_n_of_bucket());
                assert(bucket);
                for(size_type n = 0 ; n < old_size; ++n){
                    value_type * kv = &old_bucket[n];
                    if(!is_occupied(kv->first)) continue;
                    const size_type s0 = s_mod_n_of_bucket(hash_m(static_cast<const Key>(kv->first)));
                    size_type i = 0;
                    size_type s = s0;
                    const size_type max_probe =  MAX_PROBE_ATTEMPT;
                    while(i < max_probe && !is_empty(bucket[s].first)){
                        i++;
                        VHASH_PROBE(s,i);
                    }
                    COLLECT_STAT;
                    assert(i < get_n_of_bucket());
                    assign(&bucket[s],*kv);
                }
                allocator.deallocate(old_bucket,old_size);
                return;
            }
        void shrink()
            {
                this->n_of_shrink ++;
                resize(-1);
                return;
            }

    };
    template<class Key, class T, class Hash, class KeyEqual, class Allocator >
    const Key unordered_map<Key,T, Hash, KeyEqual, Allocator>::EMPTY = Key(0);
    template<class Key, class T, class Hash, class KeyEqual, class Allocator >
    const Key unordered_map<Key,T, Hash, KeyEqual, Allocator>::DELETED = Key(1);
    template<class Key, class T, class Hash, class KeyEqual, class Allocator >
    typename unordered_map<Key,T, Hash, KeyEqual, Allocator>::Assert_EMPTY_not_eq_DELETE
    unordered_map<Key,T, Hash, KeyEqual, Allocator>::assert_empty_not_eq_delete;

    struct Empty {};
    template<typename T>
    class allocate_bucket<T,false> {
    public:
        static typename T::value_type* op(typename T::allocator_type alloc,typename T::size_type s)
            {
                typename T::value_type* ret =  alloc.allocate(s);
                for(size_t i = 0 ; i < s; ++i){
                    new(ret+i) typename T::value_type(
                        T::EMPTY,
                        typename T::mapped_type());
                }
                return ret;
            }
    };
    template<typename T>
    class allocate_bucket<T,true> {
    public:
        static typename T::value_type* op(typename T::allocator_type alloc,typename T::size_type s)
            {
                typename T::value_type* ret =  alloc.allocate(s);
                for(size_t i = 0 ; i < s; ++i){
                    new(ret+i) typename T::value_type(T::EMPTY);
                }
                return ret;
            }
    };
    template<
        class Key,
        class Hash = std::hash<Key>, // std::hash(const char*) has poor performance
        class KeyEqual = std::equal_to<Key>,
        class Allocator = std::allocator< Key >
        > 
    using set = unordered_map<Key,Hash,KeyEqual,Allocator>;
}
namespace std {
    template<class Key, class T, class Hash = std::hash<Key>, class KeyEqual = std::equal_to<Key>,
             class Allocator = std::allocator<typename std::conditional<std::is_empty<T>::value , Key, std::pair<const Key, T>  > :: type > >
    void swap(voba::unordered_map<Key,T,Hash,KeyEqual,Allocator>& a, voba::unordered_map<Key,T,Hash,KeyEqual,Allocator>& b)
    {
        a.swap(b);
    }
}

// Local Variables:
// mode:c++
// coding: undecided-unix
// End:
