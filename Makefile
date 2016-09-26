MAIN = monitor
CC = g++
INCDIRS = 
CXXFLAGS = $(COMPILERFLAGS) -O3 -march=native -pipe -std=c++0x -Wall -g $(INCDIRS)
CFLAGS = -g $(INCDIRS)
LIBS = -lboost_system -lboost_filesystem -lpthread

prog: $(MAIN)

$(MAIN).o : $(MAIN).cpp

build/%.o : %.cpp
	$(CC) -o $@ -c $(CPPFLAGS) $(CXXFLAGS) $<

$(MAIN) : build/$(MAIN).o
	$(CC) -o $(MAIN) $^ $(LIBS)

.PHONY: clean
clean:
	rm -f build/*.o
