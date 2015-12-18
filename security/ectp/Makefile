CC=gcc
SVN_REV=$(shell svnversion -n .)
CFLAGS= -Wall -g
LIBS= -lpcap
#CFLAGS= `pkg-config --cflags pcap` -Wall -g
#LIBS= `pkg-config --libs pcap`

DEPEND := makedepend
SRCS= loop.c
OBJS= loop.o
DEPS=

OSNAME=$(shell uname -s)
CPUNAME=$(shell uname -m)

ifeq ($(OSNAME),Linux)
	LIBS := $(LIBS) -pthread
endif

all: loop dep undep

dep: $(SRCS)
	-$(DEPEND) $(CFLAGS) $(SRCS) 2> /dev/null

undep:
	-$(DEPEND) $(CFLAGS) 2> /dev/null

loop: $(OBJS)
	$(CC) $(CFLAGS) $(LIBS) -o loop $(OBJS)

# code
%.o: %.c 
	$(CC) $(CFLAGS) -c -o $@ $<

# cleanup
clean:
	$(DEPEND) $(COPTS)
	-rm -f loop $(OBJS)

# DO NOT DELETE
