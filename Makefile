CXXFLAGS=-Wall -pedantic -Werror -std=c++11
ifneq ($(CONFIG),release)
	CFLAGS += -ggdb -O0
	CXXFLAGS += -ggdb -O0
else
	CFLAGS += -O3 -DNDEBUG
	CXXFLAGS += -O3 -DNDEBUG
endif



all: test_vhash.out

test_vhash: test_vhash.cc vhash.h
	$(CXX) $(CXXFLAGS) -o $@ $<

test_vhash.out: test_vhash
	./test_vhash > $@

clean:
	rm test_vhash test_vhash.out *.o
