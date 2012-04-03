include $(CONCURRIT_HOME)/common.mk

TARGET=libconcurrit

CONCURRIT_SRCS=$(wildcard $(CONCURRIT_SRCDIR)/*.cpp)
CONCURRIT_OBJS=$(patsubst %.cpp, %.o, $(subst $(CONCURRIT_SRCDIR), $(CONCURRIT_OBJDIR), $(CONCURRIT_SRCS))) 
CONCURRIT_HEADERS=$(wildcard $(CONCURRIT_INCDIR)/*.h)

DEFINES=-DDPOR
FLAGS=-g -fPIC -gdwarf-2 -O3 -fexceptions \
		$(CONCURRIT_C_STD) \
		-w -Werror=uninitialized -Werror=unused -Werror=return-type -Werror=parentheses
		# -Wall -Winline 

# other flags that can be used:
#-finline-functions # this gives a lot of warnings in Linux

all: makedirs $(CONCURRIT_LIBDIR)/$(TARGET).so

$(CONCURRIT_LIBDIR)/$(TARGET).so: $(CONCURRIT_OBJS)
	$(CC) $(CONCURRIT_LIB_FLAGS) $(DEFINES) $(FLAGS) -shared -o $@ $^
	ar rcs $(CONCURRIT_LIBDIR)/$(TARGET).a $^
	$(CONCURRIT_HOME)/scripts/compile_pintool.sh CONCURRIT_DEBUG_FLAGS='$(CONCURRIT_DEBUG_FLAGS)'

$(CONCURRIT_OBJDIR)/%.o: $(CONCURRIT_SRCDIR)/%.cpp $(CONCURRIT_HEADERS)
	$(CC) $(CONCURRIT_INC_FLAGS) $(DEFINES) $(FLAGS) -c -o $@ $(CONCURRIT_SRCDIR)/$*.cpp
	
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
