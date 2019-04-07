CFLAGS = -g -Wall
LDLIBS = -lm

all: example 

example: bitmap.o example.o
	$(CC) -o $@ $^ $(LDLIBS)

libbitmap.a: bitmap.o
	ar rc libbitmap.a bitmap.o

clean:
	rm -f *.o *.a example
