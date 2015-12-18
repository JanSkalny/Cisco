CC=gcc
CXX=g++
SVN_REV=$(shell svnversion -n .)
CFLAGS= -g -Wall -O0
LIBS= 
DEPEND := makedepend
SRCS= main.cpp Worker.cpp LineTelnet.cpp Line.cpp LineTelnetClient.cpp InterfaceMgr.cpp CommandParser.cpp Command.cpp
OBJS= main.o Worker.o LineTelnet.o Line.o LineTelnetClient.o InterfaceMgr.o CommandParser.o Command.o
DEPS=

OSNAME=$(shell uname -s)
CPUNAME=$(shell uname -m)

ifeq ($(OSNAME),Linux)
	LIBS := $(LIBS) -pthread
endif

all: switch dep undep

dep: $(SRCS)
	-$(DEPEND) $(CFLAGS) $(SRCS) 2> /dev/null

undep:
	-$(DEPEND) $(CFLAGS) 2> /dev/null

# binarky
switch: $(OBJS)
	$(CXX) $(CFLAGS) $(LIBS) -o switch $(OBJS)

# code
%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<
%.o: %.cpp
	$(CXX) $(CFLAGS) -c -o $@ $<

wc:
	-wc *.c *.h *.cpp

# cleanup
clean:
	$(DEPEND) $(COPTS)
	-rm -f switch $(OBJS)

# DO NOT DELETE
