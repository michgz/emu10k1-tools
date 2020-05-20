CFLAGS = -W -Wall
include ../Makefile.config

%.bin : %.asm
	$(AS)  $< 

all: dep as10k1 dsp

dep:
	$(CC) -M *.c $(CFLAGS) > .depend

-include .depend

as10k1: as10k1.o parse.o assemble.o macro.o


install: all
	cd effects && make install
	mkdir -p $(data_dir)
	install -m 755 as10k1 $(bindir)
	install -m 664 as10k1.1 $(man_prefix)/man1

clean:
	rm -f *.o *~ as10k1 core .depend
	cd effects && $(MAKE) clean
dsp:
	cd effects && $(MAKE)

