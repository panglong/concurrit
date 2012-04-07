include $(CONCURRIT_HOME)/common.mk

# variables to set: TARGET, SRCS, optional: HEADERS

TEST_INC_FLAGS+=$(CONCURRIT_TEST_INC_FLAGS) -I.
TEST_LIB_FLAGS+=$(CONCURRIT_TEST_LIB_FLAGS) -L.

TEST_FLAGS=$(TEST_INC_FLAGS) $(TEST_LIB_FLAGS) -g -gdwarf-2 -O0 -Wall $(CONCURRIT_C_STD)

CC?=g++

TARGETLIB?=

shared: $(TARGETLIB)

$(TARGET): $(TARGETLIB) $(SRCS) $(HEADERS)
	$(CC) $(TEST_FLAGS) -o $(TARGET) $^
	$(CC) $(TEST_FLAGS) -c -o $(TARGET).o $^
	ar rcs $(TARGET).a $(TARGET).o
	chmod +x $(TARGET)
	
clean:
	rm -f $(TARGET)
	rm -f $(TARGET).o
	rm -f $(TARGET).a
	rm -f lib$(TARGET).so
	
test: $(TARGET)
	$(TARGET)
	
pin: $(TARGET)
	$(CONCURRIT_HOME)/scripts/run_pintool.sh ./$(TARGET) $(ARGS)

lib$(TARGET).so: 
	g++ -g -Wall -Winline -fPIC -gdwarf-2 -O1 -shared -lpthread -lbz2 -o $@ $^

all: $(TARGET)
