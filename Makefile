MAIN = monitor
CC = g++
INCDIRS = -I/usr/include/gstreamer-1.0 -I/usr/lib/gstreamer-1.0/include -I/usr/include/glib-2.0 -I/usr/lib/glib-2.0/include -I/usr/local/include/echoprint
CXXFLAGS = $(COMPILERFLAGS) -O3 -march=native -pipe -std=c++0x -Wall -g $(INCDIRS)
CFLAGS = -g $(INCDIRS)
LIBS = -lboost_system -lboost_filesystem -lpthread -lgstreamer-1.0 -lgobject-2.0 -lglib-2.0 -lcodegen -lvorbisenc -lvorbis -logg

prog: $(MAIN)

$(MAIN).o : $(MAIN).cpp

build/%.o : %.cpp
	$(CC) -o $@ -c $(CPPFLAGS) $(CXXFLAGS) $<

$(MAIN) : build/$(MAIN).o build/Streamer.o
	$(CC) -o $(MAIN) $^ $(LIBS)

.PHONY: clean
clean:
	rm -f build/*.o
