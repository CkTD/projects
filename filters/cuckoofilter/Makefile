CFLAGS = -I../murmur3 -I../bitmap -Wall -g
LDFLAGS = 
LDLIBS = -lm

all: example

example: example.o  ../murmur3/murmur3.o ../bitmap/bitmap.o cuckoofilter.o 
	$(CC) -o $@ $^ $(LDLIBS)

clean:
	rm -f *.o example
