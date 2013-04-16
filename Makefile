PRG=chroma66202
OBJ=$(PRG).o
CC=gcc
CFLAGS=-Wall -pedantic

.PHONY: clean

$(PRG): $(OBJ)
	$(CC) -o $(PRG) $(OBJ)

$(OBJ): $(PRG).c
	$(CC) $(CFLAGS) -c $(PRG).c -o $(OBJ) 

clean:
	rm -f *.o $(PRG)
