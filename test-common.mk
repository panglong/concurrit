include $(CONCURRIT_HOME)/common.mk

# variables to set: TARGET, SRCS, optional: HEADERS

all: $(TARGET)

TEST_INC_FLAGS=$(PROGRAM_INC_FLAGS) $(CONCURRIT_TEST_INC_FLAGS)
TEST_LIB_FLAGS=$(PROGRAM_LIB_FLAGS) $(CONCURRIT_TEST_LIB_FLAGS)
TEST_FLAGS=$(TEST_INC_FLAGS) $(TEST_LIB_FLAGS) -gdwarf-2 -O0 -Wall

$(TARGET): $(SRCS) $(HEADERS)
	g++ $(TEST_FLAGS) -o $(TARGET) $^
	g++ $(TEST_FLAGS) -c -o $(TARGET).o $^
	ar rcs $(TARGET).a $(TARGET).o
	
clean:
	rm $(TARGET)
	rm $(TARGET).o
	rm $(TARGET).a
	
test:
	./$(TARGET)
