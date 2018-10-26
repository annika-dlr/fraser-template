
SRCS ?=
PROG ?=
BINDIR ?=
OBJDIR ?=

SYSTEMC_INC_DIR  ?=
SYSTEMC_LDFLAGS  ?=
SYSTEMC_LIBS     ?=

CC=gcc
CXX=g++
RM=rm -f

INCLUDES = -I../../ -I../ -I/usr/local/include -I../../fraser/src -I../../models -I../../../cpp -I../../../systemc/proc_element $(SYSTEMC_INC_DIR)
CXXFLAGS := -std=c++1y -g -Wall ${INCLUDES}
LDFLAGS = -L/usr/local/lib -L/usr/lib/x86_64-linux-gnu $(SYSTEMC_LDFLAGS)
LIBS= -lzmq -lboost_serialization -lboost_system -lboost_filesystem -lboost_thread -lpugixml $(SYSTEMC_LIBS)

vpath %.cpp $(dir $(SRCS))

SRCNAMES = $(notdir $(SRCS))
OBJS = $(patsubst %.cpp, $(OBJDIR)/%.o, $(SRCNAMES))

$(BINDIR)/$(PROG): $(OBJS)
	$(CXX) $(OBJS) $(CXXFLAGS) -o $@ $(LIBS) $(LDFLAGS)

$(OBJS): | $(OBJDIR)

$(OBJDIR):
	@mkdir -p $@

$(OBJDIR)/%.o : %.cpp
	@mkdir -p $(BINDIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@ $(LIBS) $(LDFLAGS)

.PHONY:
clean:
	$(RM) $(OBJS)
	$(RM) $(BINDIR)/$(PROG)