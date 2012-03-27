include $(CONCURRIT_HOME)/common.mk

# variables to set: TARGET, SRCS, optional: HEADERS

all: $(TARGET)

TEST_INC_FLAGS+=$(CONCURRIT_TEST_INC_FLAGS) -I.
TEST_LIB_FLAGS+=$(CONCURRIT_TEST_LIB_FLAGS) -L.

TEST_FLAGS=$(TEST_INC_FLAGS) $(TEST_LIB_FLAGS) -g -gdwarf-2 -O0 -Wall $(CONCURRIT_C_STD)

CC?=g++

$(TARGET): $(SRCS) $(HEADERS)
	$(CC) $(TEST_FLAGS) -o $(TARGET) $^
	$(CC) $(TEST_FLAGS) -c -o $(TARGET).o $^
	ar rcs $(TARGET).a $(TARGET).o
	chmod +x $(TARGET)
	
clean:
	rm $(TARGET)
	rm $(TARGET).o
	rm $(TARGET).a
	
test: $(TARGET)
	$(TARGET)
	
pin: $(TARGET)
	$(CONCURRIT_HOME)/scripts/run_pintool.sh ./$(TARGET) $(ARGS)
