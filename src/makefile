LINK.o=$(LINK.cc)
CXXFLAGS:=-std=c++17 -fsanitize=address -g
LDLIBS:=-luv -lcurl -lpthread

uvw_curl: uvw_curl.o main.o log.o

uvw_curl.o: uvw_curl.hpp
main.o: uvw_curl.hpp
