include $(CONCURRIT_HOME)/common.mk

TARGET=increment
SRCS=increment.cpp

all: $(TARGET)

$(TARGET): $(SRCS) 
	g++ $(INCDIR) $(LIBDIR) -gdwarf-2 -O0 -Wall -o $(TARGET) $(TEST_LIBS) \
	    ./increment.cpp
	g++ $(INCDIR) $(LIBDIR) -gdwarf-2 -O0 -Wall -c -o $(TARGET).o \
	    ./increment.cpp
	ar rcs $(TARGET).a $(TARGET).o
	
clean:
	rm $(TARGET)
	rm $(TARGET).o
	rm $(TARGET).a
	
test:
	./$(TARGET)
