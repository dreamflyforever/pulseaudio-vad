
LIBS=`pkg-config libpulse-simple --cflags --libs` -lm 
args=""

all: kiss_fftr.o kiss_fft.o main.o
	gcc -std=gnu99 kiss_fftr.o kiss_fft.o main.o  -o main ${LIBS}

kiss_fftr.o: kiss_fftr.c kiss_fftr.h kiss_fftr.h _kiss_fft_guts.h
	gcc $(ARGS) -c kiss_fftr.c -o kiss_fftr.o

kiss_fft.o: kiss_fft.c _kiss_fft_guts.h
	gcc $(ARGS) -c kiss_fft.c -o kiss_fft.o


main.o: main.c
	 gcc $(ARGS) -std=gnu99 -c main.c

clean: main
	rm -rf *.o
