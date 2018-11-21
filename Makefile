PRG=chroma66202
PRG2=pwrtest

DESTDIR=
PREFIX=/usr/local
BINDIR=$(PREFIX)/bin

CFLAGS=-Wall

.PHONY: all clean install

all: $(PRG) $(PRG2)

$(PRG): $(PRG).o
	$(LD) $(LDFLAGS) -o $(PRG) $(PRG).o

$(PRG2): $(PRG2).o
	$(LD) $(LDFLAGS) -o $(PRG2) $(PRG2).o

install: $(PRG) $(PRG2)
	mkdir -p $(DESTDIR)$(BINDIR)
	install -p -m 0755 $(PRG) $(DESTDIR)$(BINDIR)/
	install -p -m 0755 $(PRG2) $(DESTDIR)$(BINDIR)/

clean:
	rm -f $(PRG).o $(PRG2).o $(PRG) $(PRG2)
