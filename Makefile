all: sort

sort: sort.cpp Makefile
	g++ -pthread -openmp -std=c++11 -o sort -O3 sort.cpp

clean:
	rm sort
