PRG=chroma66202
PRG2=pwrtest
PRG3=pwrtest_energy
PRG4=acpienergy
PRG5=ipmienergy

DESTDIR=
PREFIX=/usr/local
BINDIR=$(PREFIX)/bin

CFLAGS=-Wall

.PHONY: all clean install

all: $(PRG) $(PRG2) $(PRG3) $(PRG4) $(PRG5)

ipmipwrlib.o: ipmipwrlib.c ipmipwrlib.h
	$(CC) $(CFLAGS) -c -o ipmipwrlib.o ipmipwrlib.c

$(PRG): $(PRG).o
	$(CC) $(LDFLAGS) -o $(PRG) $(PRG).o

$(PRG2): $(PRG2).o
	$(CC) $(LDFLAGS) -o $(PRG2) $(PRG2).o

$(PRG3): $(PRG3).o
	$(CC) -o $(PRG3) $(PRG3).o

$(PRG4): $(PRG4).o
	$(CC) -o $(PRG4) $(PRG4).o

$(PRG5): $(PRG5).o ipmipwrlib.o
	$(CC) -o $(PRG5) ipmipwrlib.o $(PRG5).o

install: $(PRG) $(PRG2)
	mkdir -p $(DESTDIR)$(BINDIR)
	install -p -m 0755 $(PRG) $(DESTDIR)$(BINDIR)/
	install -p -m 0755 $(PRG2) $(DESTDIR)$(BINDIR)/
	install -p -m 0755 $(PRG3) $(DESTDIR)$(BINDIR)/
	install -p -m 0755 $(PRG4) $(DESTDIR)$(BINDIR)/
	install -p -m 0755 $(PRG5) $(DESTDIR)$(BINDIR)/

clean:
	rm -f *.o $(PRG) $(PRG2) $(PRG3) $(PRG4) $(PRG5)
