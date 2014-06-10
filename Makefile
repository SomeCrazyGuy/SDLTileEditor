cc = g++
cflags = -Wall -Wextra -O0 -g3
ldflags = -lSDL2

all: default
default:
	$(cc) $(cflags) -o sdleditor sdleditor.cxx $(ldflags)	

clean:
	rm -v sdleditor *.o

