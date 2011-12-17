include $(CONCURRIT_HOME)/common.mk

TARGET=search
SRCS=search.cpp

all: $(TARGET)

$(TARGET): $(SRCS) 
	g++ $(INCDIR) $(LIBDIR) -gdwarf-2 -O0 -Wall -o $(TARGET) $(TEST_LIBS) \
	    ./search.cpp
	g++ $(INCDIR) $(LIBDIR) -gdwarf-2 -O0 -Wall -c -o $(TARGET).o \
	    ./search.cpp
	ar rcs $(TARGET).a $(TARGET).o
	
clean:
	rm $(TARGET)
	rm $(TARGET).o
	rm $(TARGET).a
	
test:
	./$(TARGET)
