LDIR=./hiredis
DEPS=config.h serverlog.h daemon.h server.h

CC=cc
CFLAGS=-Wall -g 


SERVER_NAME= logserver
SERVER_OBJS = config.o server.o daemon.o serverlog.o woker.o db.o

EVENTLOOP_OBJ = ae.o anet.o zmalloc.o

$(SERVER_NAME) : $(SERVER_OBJS) $(EVENTLOOP_OBJ) ./hiredis/libhiredis.a
	$(CC) $(CFLAGS) -L$(LDIR)  -o $@ $^ -lhiredis

./hiredis/libhiredis.a:
	cd hiredis && make libhiredis.a


%.o: %.c 
	$(CC) $(CFLAGS) -c -o $@ $^

.PHONY: clean 
clean:
	rm -f *.o $(SERVER_NAME)

.PHONY: remake
remake:
	make clean
	make
