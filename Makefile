IDIR=.
MONGODIR=/home/rp/workspace/mongo-c-driver/src/
CC = gcc
CFLAGS = -Wall -I$(MONGODIR)
ALL_CFLAGS = -Wall -I$(IDIR) -I$(MONGODIR) --std=c99 -Wswitch
LDFLAGS = -lmongoc -levent
DEPS=common.h config.h mongoq.h
OBJ=common.o log.o mdb.o mongoq.o thread.o 

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(ALL_CFLAGS)

mq: $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS) $(LDFLAGS)

.PHONY: clean

clean:
	$(RM) -f *.o *~ core 
