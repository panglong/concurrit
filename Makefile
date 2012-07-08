include $(CONCURRIT_HOME)/common.mk

CC=g++

TARGET=libconcurrit

CONCURRIT_SRCS=$(wildcard $(CONCURRIT_SRCDIR)/*.cpp)
CONCURRIT_OBJS=$(patsubst %.cpp, %.o, $(subst $(CONCURRIT_SRCDIR), $(CONCURRIT_OBJDIR), $(CONCURRIT_SRCS))) 
CONCURRIT_HEADERS=$(wildcard $(CONCURRIT_INCDIR)/*.h)

FLAGS=-g -fPIC -gdwarf-2 -O3 -fexceptions -rdynamic \
		$(CONCURRIT_C_STD) \
		-w -Werror=uninitialized -Werror=unused -Werror=return-type -Werror=parentheses \
		-DDPOR
		# -Wall -Winline 

# other flags that can be used:
#-finline-functions # this gives a lot of warnings in Linux

all: makedirs lib remote

remote: makedirs server client loader controller

lib: makedirs dummy $(CONCURRIT_LIBDIR)/$(TARGET).so

############################

$(CONCURRIT_LIBDIR)/$(TARGET).so: $(CONCURRIT_OBJS)
	$(CC) $(CONCURRIT_LIB_FLAGS) $(FLAGS) -shared -o $@ $^
#	ar rcs $(CONCURRIT_LIBDIR)/$(TARGET).a $^
	$(CONCURRIT_HOME)/scripts/compile_pintool.sh CONCURRIT_DEBUG_FLAGS='$(CONCURRIT_DEBUG_FLAGS)'

$(CONCURRIT_OBJDIR)/%.o: $(CONCURRIT_SRCDIR)/%.cpp $(CONCURRIT_HEADERS)
	$(CC) $(CONCURRIT_INC_FLAGS) $(FLAGS) -c -o $@ $(CONCURRIT_SRCDIR)/$*.cpp
	
############################

#$(CONCURRIT_LIBDIR)/$(TARGET).so: $(CONCURRIT_SRCS) $(CONCURRIT_HEADERS)
#	$(CC) $(CONCURRIT_INC_FLAGS) $(CONCURRIT_LIB_FLAGS) $(FLAGS) -shared -o $@ $(CONCURRIT_SRCS)
#	$(CONCURRIT_HOME)/scripts/compile_pintool.sh CONCURRIT_DEBUG_FLAGS='$(CONCURRIT_DEBUG_FLAGS)'

############################

dummy: makedirs $(CONCURRIT_LIBDIR)/libdummy.so

$(CONCURRIT_LIBDIR)/libdummy.so: $(CONCURRIT_HOME)/dummy/dummy.cpp
	$(CC) -I$(CONCURRIT_INCDIR) $(CONCURRIT_LIB_FLAGS) $(FLAGS) -shared -o $@ $^

############################

server: makedirs $(CONCURRIT_LIBDIR)/libserver.so

$(CONCURRIT_LIBDIR)/libserver.so: $(CONCURRIT_HOME)/remote/server.cpp
	$(CC) $(CONCURRIT_INC_FLAGS) $(FLAGS) $(CONCURRIT_LIB_FLAG) -L$(CONCURRIT_LIBDIR) -shared -o $@ $^

############################

client: makedirs $(CONCURRIT_LIBDIR)/libclient.so

$(CONCURRIT_LIBDIR)/libclient.so: $(CONCURRIT_HOME)/remote/client.cpp
	$(CC) $(CONCURRIT_INC_FLAGS) $(FLAGS) $(CONCURRIT_LIB_FLAG) -L$(CONCURRIT_LIBDIR) -shared -o $@ $^

############################

loader: makedirs $(CONCURRIT_BINDIR)/testloader

$(CONCURRIT_BINDIR)/testloader: $(CONCURRIT_HOME)/remote/loader.cpp
	$(CC) $(CONCURRIT_INC_FLAGS) $(FLAGS) $(CONCURRIT_LIB_FLAG) -L$(CONCURRIT_LIBDIR) -o $@ $^

############################

controller: makedirs $(CONCURRIT_BINDIR)/testcontroller

$(CONCURRIT_BINDIR)/testcontroller: $(CONCURRIT_HOME)/remote/testcontroller.cpp
	$(CC) $(CONCURRIT_INC_FLAGS) $(FLAGS) $(CONCURRIT_LIB_FLAG) -L$(CONCURRIT_LIBDIR) -o $@ $^

############################

makedirs:
	mkdir -p $(CONCURRIT_BINDIR)
	mkdir -p $(CONCURRIT_LIBDIR)
	mkdir -p $(CONCURRIT_OBJDIR)
	mkdir -p $(CONCURRIT_WORKDIR)

.PHONY: clean clean-remote	
clean:
	rm -rf $(CONCURRIT_BINDIR)
	rm -rf $(CONCURRIT_LIBDIR)
	rm -rf $(CONCURRIT_OBJDIR)
	rm -rf $(CONCURRIT_WORKDIR)
	
clean-remote:
	rm -f $(CONCURRIT_BINDIR)/testloader $(CONCURRIT_BINDIR)/testcontroller $(CONCURRIT_LIBDIR)/libserver.so $(CONCURRIT_LIBDIR)/libclient.so

clean-lib:
	rm -rf $(CONCURRIT_OBJDIR)
	rm -f $(CONCURRIT_LIBDIR)/libconcurrit.so $(CONCURRIT_LIBDIR)/libdummy.so
	