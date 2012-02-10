include $(CONCURRIT_HOME)/common.mk

TARGET=libconcurrit

CONCURRIT_SRCS=$(wildcard $(CONCURRIT_SRCDIR)/*.cpp)
CONCURRIT_OBJS=$(patsubst %.cpp, %.o, $(subst $(CONCURRIT_SRCDIR), $(CONCURRIT_OBJDIR), $(CONCURRIT_SRCS))) 
CONCURRIT_HEADERS=$(wildcard $(CONCURRIT_INCDIR)/*.h)

DEFINES=-DDPOR
FLAGS=-g -Wall -Winline -fPIC -O2 \
		-Werror=uninitialized -Werror=unused -Werror=return-type -Werror=parentheses

# other flags that can be used:
#-finline-functions # this gives a lot of warnings in Linux

STD=#-std=c++0x

all: makedirs $(CONCURRIT_LIBDIR)/$(TARGET).so

$(CONCURRIT_LIBDIR)/$(TARGET).so: $(CONCURRIT_OBJS)
	g++ $(CONCURRIT_LIB_FLAGS) $(STD) $(DEFINES) $(FLAGS) -shared -o $@ $^
	ar rcs $(CONCURRIT_LIBDIR)/$(TARGET).a $^

$(CONCURRIT_OBJDIR)/%.o: $(CONCURRIT_SRCDIR)/%.cpp $(CONCURRIT_HEADERS)
	g++ $(CONCURRIT_INC_FLAGS) $(STD) $(DEFINES) $(FLAGS) -c -o $@ $(CONCURRIT_SRCDIR)/$*.cpp
	
makedirs:
	mkdir -p $(CONCURRIT_BINDIR)
	mkdir -p $(CONCURRIT_LIBDIR)
	mkdir -p $(CONCURRIT_OBJDIR)
	
clean:
	rm -rf $(CONCURRIT_BINDIR)
	rm -rf $(CONCURRIT_LIBDIR)
	rm -rf $(CONCURRIT_OBJDIR)
