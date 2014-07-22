#include <iostream>
#include <vector>
#include <algorithm>
#include <iterator>
#include <functional>
#include <deque>
using namespace std;

#include <functional>
#include "vhash.h"

using namespace voba;
using namespace std;

#include <vector>
#include <bitset>
#include <string>
#include <utility>
 
struct Key {
  std::string first;
  std::string second;
};
 
struct KeyHash {
  std::size_t operator()(const Key& k) const
    {
      return std::hash<std::string>()(k.first) ^
        (std::hash<std::string>()(k.second) << 1);
    }
};
struct KeyEqual {
  bool operator()(const Key& lhs, const Key& rhs) const
    {
      return lhs.first == rhs.first && lhs.second == rhs.second;
    }
};
template<>
const std::string voba::unordered_map<std::string, std::string>::EMPTY("0");
template<>
const std::string voba::unordered_map<std::string, std::string>::DELETED("1");
template<>
const Key voba::unordered_map<Key, std::string,
                              KeyHash,
                              KeyEqual
                              >::EMPTY = Key({"a","b"});
template<>
const Key voba::unordered_map<Key, std::string,
                              KeyHash,
                              KeyEqual
                              >::DELETED = Key({"deleted","deleted"});
template <class A>
void dump(const A & x)
{
  for(typename A::const_iterator i = x.begin();
      i != x.end();
      ++i){
    cout << i->first << " : " << i->second << endl;
  }
}
void test_1()
{
    // default constructor: empty map
    voba::unordered_map<std::string, std::string> m1;
    return;
}
void test_2()
{
    voba::unordered_map<int, std::string> m2 =
        {
            {0, "0"},
            {1, "1"},
            {2, "2"},
            {3, "3"},
            {4, "4"},
            {5, "5"},
            {6, "6"},
            {7, "7"},
            {8, "8"},
            {9, "9"},
            {10, "10"},
        };
    dump(m2);
    for(typename voba::unordered_map<int, std::string>::iterator i = m2.begin();
        i != m2.end();
        ++i){
        voba::unordered_map<int, std::string>::iterator it = m2.find(i->first);
        cout << i->first << " : " << i->second << " ||  "
             << it->first << " : " << it->second
             << endl;
    }
    m2.clear();
    cerr <<  __FILE__ << ":" << __LINE__ << " [" << __FUNCTION__<< "] "
         << "m2.size() "  << m2.size() << " "
         << "m2.empty() "  << m2.empty() << " "
         << endl;
    while(!m2.empty()){
        cerr <<  __FILE__ << ":" << __LINE__ << " [" << __FUNCTION__<< "] "
             << "erasing " << m2.begin()->first << " "
             << endl;
        m2.erase(m2.find(m2.begin()->first));
        dump(m2);
    }
    dump(m2);
}
void test_3()
{
    typedef voba::set<int> a;
    typedef voba::unordered_map<int, int> b;
    cerr <<  __FILE__ << ":" << __LINE__ << " [" << __FUNCTION__<< "] "
         << "a::value_type "  << sizeof(a::value_type) << " "
         << "b::value_type "  << sizeof(b::value_type) << " "
         << endl;
}
int main()
{
    test_3();
//    dump(m1);
//list constructor
 
// // copy constructor
//     voba::unordered_map<int, std::string> m3 = m2;
 
// // move constructor
//     voba::unordered_map<int, std::string> m4 = std::move(m2);
 
// // range constructor
//    // std::vector<std::pair<std::bitset<8>, int>> v = { {0x12, 1}, {0x01,-1} };
//    // voba::unordered_map<std::bitset<8>, double> m5(v.begin(), v.end());
 
// //constructor for a custom type
//    voba::unordered_map<Key, std::string, KeyHash, KeyEqual> m6 = {
//        { {"John", "Doe"}, "example"},
//        { {"Mary", "Sue"}, "another"}
//     };
//    if(0) {
//      voba::unordered_map<Key, std::string, KeyHash, KeyEqual>::iterator it =
//        m6.begin();
//      it++;
//      cout << it->first.first << endl;
//    }
//    if(0) {
//      voba::unordered_map<Key, std::string, KeyHash, KeyEqual>::const_iterator it =
//        m6.cbegin();
//      it++;
//      cout << it->first.first << endl;
//    }
}


// Local Variables:
// mode:c++
// coding: undecided-unix
// End:
