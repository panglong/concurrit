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

all: makedirs dummy remote $(CONCURRIT_LIBDIR)/$(TARGET).so

############################

$(CONCURRIT_LIBDIR)/$(TARGET).so: $(CONCURRIT_OBJS)
	$(CC) $(CONCURRIT_LIB_FLAGS) $(FLAGS) -shared -o $@ $^
#	ar rcs $(CONCURRIT_LIBDIR)/$(TARGET).a $^
	$(CONCURRIT_HOME)/scripts/compile_pintool.sh CONCURRIT_DEBUG_FLAGS='$(CONCURRIT_DEBUG_FLAGS)'

$(CONCURRIT_OBJDIR)/%.o: $(CONCURRIT_SRCDIR)/%.cpp $(CONCURRIT_HEADERS)
	$(CC) $(CONCURRIT_INC_FLAGS) $(FLAGS) -c -o $@ $(CONCURRIT_SRCDIR)/$*.cpp
	
############################

dummy: makedirs $(CONCURRIT_LIBDIR)/libdummy.so

$(CONCURRIT_LIBDIR)/libdummy.so: $(CONCURRIT_HOME)/dummy/dummy.cpp
	$(CC) -I$(CONCURRIT_INCDIR) -g -gdwarf-2 -O1 -w -fPIC -shared -o $@ $^

############################

remote: makedirs $(CONCURRIT_LIBDIR)/libremote.so

$(CONCURRIT_LIBDIR)/libremote.so: $(CONCURRIT_HOME)/remote/remote.cpp
	$(CC) $(CONCURRIT_INC_FLAGS) $(CONCURRIT_LIB_FLAGS) $(FLAGS) -shared -o $@ $^

############################

makedirs:
	mkdir -p $(CONCURRIT_BINDIR)
	mkdir -p $(CONCURRIT_LIBDIR)
	mkdir -p $(CONCURRIT_OBJDIR)
	mkdir -p $(CONCURRIT_WORKDIR)

.PHONY: clean	
clean:
	rm -rf $(CONCURRIT_BINDIR)
	rm -rf $(CONCURRIT_LIBDIR)
	rm -rf $(CONCURRIT_OBJDIR)
	rm -rf $(CONCURRIT_WORKDIR)
