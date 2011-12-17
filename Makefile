include ./common.mk

TARGET=libcounit

SRCS=coroutine.cpp exception.cpp group.cpp lp.cpp scenario.cpp schedule.cpp serialize.cpp suite.cpp thread.cpp vc.cpp
HEADERS=counit.h api.h channel.h common.h coroutine.h exception.h group.h lp.h result.h scenario.h schedule.h serialize.h sharedaccess.h suite.h thread.h until.h vc.h
OBJS=$(SRCS:%.cpp=%.o)

FLAGS=-g -Wall -fPIC -Winline -finline-functions -O2

STD=#-std=c++0x

COUNIT_SRCS=$(addprefix $(COUNIT_SRCDIR)/, $(SRCS))
COUNIT_HEADERS=$(addprefix $(COUNIT_INCDIR)/, $(HEADERS))
COUNIT_OBJS=$(addprefix $(COUNIT_OBJDIR)/, $(OBJS))

all: makedirs $(COUNIT_BINDIR)/$(TARGET).so

$(COUNIT_BINDIR)/$(TARGET).so: $(COUNIT_OBJS)
	g++ $(INCDIR) $(LIBDIR) $(STD) $(DEFINES) $(FLAGS) -shared -o $@ $(LIBS) $^
	ar rcs $(COUNIT_BINDIR)/$(TARGET).a $^

$(COUNIT_OBJDIR)/%.o: $(COUNIT_SRCDIR)/%.cpp $(COUNIT_HEADERS)
	g++ $(INCDIR) $(STD) $(DEFINES) $(FLAGS) -c -o $@ $(COUNIT_SRCDIR)/$*.cpp
	
makedirs:
	mkdir -p $(COUNIT_BINDIR)
	mkdir -p $(COUNIT_OBJDIR)
	
clean:
	rm -rf $(COUNIT_OBJDIR)
	rm -rf $(COUNIT_BINDIR)
