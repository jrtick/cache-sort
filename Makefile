all: sort lists

sort: sort.cpp Makefile
	g++ -std=c++11 -fopenmp -o mergesort -O3 -D MERGESORT sort.cpp
	g++ -std=c++11 -fopenmp -o quicksort -O3 -D QUICKSORT sort.cpp
	g++ -std=c++11 -fopenmp -o samplesort -O3 -D SAMPLESORT sort.cpp
	g++ -std=c++11 -fopenmp -o kmergesort -O3 -D KMERGESORT sort.cpp

lists: lists.cpp Makefile
	g++ -std=c++11 -fopenmp -o lists -O3 lists.cpp

clean:
	rm *sort lists
