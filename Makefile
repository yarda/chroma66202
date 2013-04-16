PRG=chroma66202
PRG2=pwrtest

DESTDIR=/usr/local
CFLAGS=-Wall

.PHONY: all clean install

all: $(PRG) $(PRG2)

$(PRG): $(PRG).o
	$(CC) -o $(PRG) $(PRG).o

$(PRG2): $(PRG2).o
	$(CC) -o $(PRG2) $(PRG2).o

install: $(PRG) $(PRG2)
	mkdir -p $(DESTDIR)/bin
	install -p -m 0755 $(PRG) $(DESTDIR)/bin/
	install -p -m 0755 $(PRG2) $(DESTDIR)/bin/

clean:
	rm -f $(PRG).o $(PRG2).o $(PRG) $(PRG2)
