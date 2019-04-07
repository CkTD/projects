CFLAGS = -I../murmur3 -I../bitmap -Wall -g
LDFLAGS = 
LDLIBS = -lm

all: example

example: example.o bloomfilter.o ../murmur3/murmur3.o ../bitmap/bitmap.o
	$(CC) $(CFLAGS) -o $@ $^ $(LDLIBS)

clean:
	rm -f *.o example 
