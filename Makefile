include ./common.mk

TARGET=libconcurrit

SRCS=coroutine.cpp exception.cpp group.cpp lp.cpp modular.cpp scenario.cpp schedule.cpp serialize.cpp suite.cpp thread.cpp vc.cpp
HEADERS=concurrit.h api.h channel.h common.h coroutine.h exception.h group.h lp.h modular.h result.h scenario.h schedule.h serialize.h sharedaccess.h suite.h thread.h until.h vc.h
OBJS=$(SRCS:%.cpp=%.o)

FLAGS=-g -Wall -fPIC -Winline -finline-functions -O2

STD=#-std=c++0x

CONCURRIT_SRCS=$(addprefix $(CONCURRIT_SRCDIR)/, $(SRCS))
CONCURRIT_HEADERS=$(addprefix $(CONCURRIT_INCDIR)/, $(HEADERS))
CONCURRIT_OBJS=$(addprefix $(CONCURRIT_OBJDIR)/, $(OBJS))

all: makedirs $(CONCURRIT_BINDIR)/$(TARGET).so

$(CONCURRIT_BINDIR)/$(TARGET).so: $(CONCURRIT_OBJS)
	g++ $(INCDIR) $(LIBDIR) $(STD) $(DEFINES) $(FLAGS) -shared -o $@ $(LIBS) $^
	ar rcs $(CONCURRIT_BINDIR)/$(TARGET).a $^

$(CONCURRIT_OBJDIR)/%.o: $(CONCURRIT_SRCDIR)/%.cpp $(CONCURRIT_HEADERS)
	g++ $(INCDIR) $(STD) $(DEFINES) $(FLAGS) -c -o $@ $(CONCURRIT_SRCDIR)/$*.cpp
	
makedirs:
	mkdir -p $(CONCURRIT_BINDIR)
	mkdir -p $(CONCURRIT_OBJDIR)
	
clean:
	rm -rf $(CONCURRIT_OBJDIR)
	rm -rf $(CONCURRIT_BINDIR)
