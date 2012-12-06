
LIBS=`pkg-config libpulse-simple --cflags --libs` -lm 

all: main.c 
	gcc -std=gnu99 kiss_fft.c main.c -o main ${LIBS}

clean: main
	rm -rf main
