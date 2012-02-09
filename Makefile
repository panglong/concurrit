include ./common.mk

TARGET=libconcurrit

CONCURRIT_SRCS=$(wildcard $(CONCURRIT_SRCDIR)/*.cpp)
CONCURRIT_OBJS=$(patsubst %.cpp, %.o, $(subst $(CONCURRIT_SRCDIR), $(CONCURRIT_OBJDIR), $(CONCURRIT_SRCS))) 
CONCURRIT_HEADERS=$(wildcard $(CONCURRIT_INCDIR)/*.h)

FLAGS=-g -Wall -Winline -fPIC -O2 \
		-Werror=uninitialized -Werror=unused -Werror=return-type -Werror=parentheses

# other flags that can be used:
#-finline-functions # this gives a lot of warnings in Linux

STD=#-std=c++0x

all: makedirs $(CONCURRIT_BINDIR)/$(TARGET).so

$(CONCURRIT_BINDIR)/$(TARGET).so: $(CONCURRIT_OBJS)
	g++ $(LIBDIR) $(STD) $(DEFINES) $(FLAGS) -shared -o $@ $(LIBS) $^
	ar rcs $(CONCURRIT_BINDIR)/$(TARGET).a $^

$(CONCURRIT_OBJDIR)/%.o: $(CONCURRIT_SRCDIR)/%.cpp $(CONCURRIT_HEADERS)
	g++ $(INCDIR) $(STD) $(DEFINES) $(FLAGS) -c -o $@ $(CONCURRIT_SRCDIR)/$*.cpp
	
makedirs:
	mkdir -p $(CONCURRIT_BINDIR)
	mkdir -p $(CONCURRIT_OBJDIR)
	
clean:
	rm -rf $(CONCURRIT_OBJDIR)
	rm -rf $(CONCURRIT_BINDIR)
