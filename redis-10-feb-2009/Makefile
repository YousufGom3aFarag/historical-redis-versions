# Makefile for wbox
# Copyright (C) 2007 Salvatore Sanfilippo <antirez@invece.org>
# All Rights Reserved
# Under the GPL license version 2

DEBUG?= -g
CFLAGS?= -O2 -Wall -W -DSDS_ABORT_ON_OOM
CCOPT= $(CFLAGS)

OBJ = adlist.o ae.o anet.o dict.o redis.o sds.o
PRGNAME = redis-server

all: redis-server

# Deps (use make dep to generate this)
adlist.o: adlist.c adlist.h
ae.o: ae.c ae.h
anet.o: anet.c anet.h
dict.o: dict.c dict.h
redis.o: redis.c ae.h sds.h anet.h dict.h adlist.h
sds.o: sds.c sds.h

redis-server: $(OBJ)
	$(CC) -o $(PRGNAME) $(CCOPT) $(DEBUG) $(OBJ)

.c.o:
	$(CC) -c $(CCOPT) $(DEBUG) $(COMPILE_TIME) $<

clean:
	rm -rf $(PRGNAME) *.o

dep:
	$(CC) -MM *.c
