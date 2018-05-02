all: sort lists

sort: sort.cpp
	g++ -std=c++11 -o sort -O3 sort.cpp

lists: lists.cpp
	g++ -std=c++11 -o lists -O3 lists.cpp

clean:
	rm sort lists
