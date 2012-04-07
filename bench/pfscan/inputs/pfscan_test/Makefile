

SOURCES = libpth.cpp mypthread.cpp Originals.cpp ThreadInfo.cpp Initializer.cpp

HEADERS = modeltypes.h libpth.h mypthread.h Originals.h ThreadInfo.h Initializer.h 

OBJS = $(SOURCES:.cpp=.o)

all: libmodel.so

%.so: $(OBJS) %.o
	g++ -Wall -shared -g -fPIC -o $@ $^ -ldl
	cp -f $@ ../bin

%.o: %.cpp %.h $(HEADERS)
	g++ -Wall -c -g -fPIC -o $@ $<

clean:
	rm -f *.o *.so
	rm -f ../bin/*.so

