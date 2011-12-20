include $(CONCURRIT_HOME)/common.mk

# variables to set: TARGET, SRCS

all: $(TARGET)

$(TARGET): $(SRCS) 
	g++ $(INCDIR) $(LIBDIR) -gdwarf-2 -O0 -Wall -o $(TARGET) $(TEST_LIBS) $^
	g++ $(INCDIR) $(LIBDIR) -gdwarf-2 -O0 -Wall -c -o $(TARGET).o $^
	ar rcs $(TARGET).a $(TARGET).o
	
clean:
	rm $(TARGET)
	rm $(TARGET).o
	rm $(TARGET).a
	
test:
	./$(TARGET)
