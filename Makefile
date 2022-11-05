#Makefile
#by Scythe

CC = gcc
CFLAGS = -O2 -Wall -fanalyzer
OBJS = func.o tcp.o
BIN = netpipe
MAN = netpipe.1
PREFIX = /usr/local
BINPATH = bin
MANPATH = man/man1

default: netpipe

all: $(OBJS) $(BIN) strip

.c.o:
	$(CC) $(CFLAGS) -c $<

func.o: func.c func.h

tcp.o: func.o tcp.c tcp.h netpipe.h

netpipe: $(OBJS) netpipe.c netpipe.h
	$(CC) $(CFLAGS) $(OBJS) -o netpipe netpipe.c

clean:
	rm -f $(OBJS)
	rm -f $(BIN)

strip: $(BIN)
	strip -s $(BIN)

install: strip
	cp $(BIN) $(PREFIX)/$(BINPATH)
	cp $(MAN) $(PREFIX)/$(MANPATH)

uninstall:
	rm -f $(PREFIX)/$(BINPATH)/$(BIN) 
	rm -f $(PREFIX)/$(MANPATH)/$(MAN) 
