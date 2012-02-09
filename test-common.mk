include $(CONCURRIT_HOME)/common.mk

# variables to set: TARGET, SRCS

all: $(TARGET)

$(TARGET): $(SRCS) 
	g++ $(CONCURRIT_TEST_INC_FLAGS) $(CONCURRIT_TEST_LIB_FLAGS) -gdwarf-2 -O0 -Wall -o $(TARGET) $^
	g++ $(CONCURRIT_TEST_INC_FLAGS) $(CONCURRIT_TEST_LIB_FLAGS) -gdwarf-2 -O0 -Wall -c -o $(TARGET).o $^
	ar rcs $(TARGET).a $(TARGET).o
	
clean:
	rm $(TARGET)
	rm $(TARGET).o
	rm $(TARGET).a
	
test:
	./$(TARGET)
