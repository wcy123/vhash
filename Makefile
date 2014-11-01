CXXFLAGS=-Wall -pedantic -Werror -std=c++11



all: test_vhash.out

test_vhash: test_vhash.cc vhash.h
	$(CXX) $(CXXFLAGS) -o $@ $<

test_vhash.out: test_vhash
	./test_vhash > $@

clean:
	rm test_vhash test_vhash.out *.o
