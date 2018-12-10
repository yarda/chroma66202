PRG=chroma66202
PRG2=pwrtest
PRG3=pwrtest2

DESTDIR=
PREFIX=/usr/local
BINDIR=$(PREFIX)/bin

CFLAGS=-Wall

.PHONY: all clean install

all: $(PRG) $(PRG2) $(PRG3)

$(PRG): $(PRG).o
	$(CC) -o $(PRG) $(PRG).o

$(PRG2): $(PRG2).o
	$(CC) -o $(PRG2) $(PRG2).o

$(PRG3): $(PRG3).o
	$(CC) -o $(PRG3) $(PRG3).o

install: $(PRG) $(PRG2)
	mkdir -p $(DESTDIR)$(BINDIR)
	install -p -m 0755 $(PRG) $(DESTDIR)$(BINDIR)/
	install -p -m 0755 $(PRG2) $(DESTDIR)$(BINDIR)/

clean:
	rm -f $(PRG).o $(PRG2).o $(PRG) $(PRG2)
