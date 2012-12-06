
all: main.c 
	gcc -std=gnu99 main.c -o main `pkg-config libpulse-simple --cflags --libs` -lm kiss_fft.c  -g 

clean: main
	rm -rf main
