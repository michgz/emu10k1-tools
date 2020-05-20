CC := gcc
CFLAGS := -g -O2 -Wall
# GTKCFLAGS := `gtk-config --cflags`
# GTKLIBS := `gtk-config --libs`

include ../Makefile.config

all: dep libdsp emu-dspmgr emu-config

-include .depend


dep:
	$(CC) -M emu-dspmgr.c emu-config.c $(GTKCFLAGS) > .depend


#emu-gtkcontrol.o: emu-gtkcontrol.c 
#	$(CC) $(CFLAGS) $(GTKCFLAGS)  -c emu-gtkcontrol.c

#emu-gtkcontrol: emu-gtkcontrol.o lib/libdsp.o
#	$(CC) $(CFLAGS) $(GTKLIBS) emu-gtkcontrol.o lib/libdsp.o -o $@ 

emu-config: emu-config.o lib/mix.o 

emu-dspmgr: emu-dspmgr.o lib/libdsp.o 

libdsp:
	(cd lib; $(MAKE) all)

clean:
	rm -f .depend core *~ *.o
	rm -f  emu-config emu-dspmgr emu-gtkcontrol
	(cd lib; $(MAKE) clean)

distclean: clean
	rm -f emu-config emu-dspmgr emu-gtkcontrol

install: all
	install -m 755 emu-dspmgr $(bindir)
	install -m 755 emu-config $(bindir)
#	install -m 755 emu-gtkcontrol $(bindir)

	install -m 664 doc/emu-dspmgr.1 $(man_prefix)/man1	
	install -m 664 doc/emu-config.1 $(man_prefix)/man1	


