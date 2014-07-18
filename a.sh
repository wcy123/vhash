set -x 
CXXFLAGS='-ggdb -O0 -Wall -pedantic -Werror'
g++ $CXXFLAGS -o a.out -std=c++11 test_vhash.c  && ./a.out



