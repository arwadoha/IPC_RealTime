CC = gcc
CFLAGS = -Wall -pthread

supermarket_simulation: main.c
	$(CC) $(CFLAGS) -o $@ $^

clean:
	rm -f supermarket_simulation
