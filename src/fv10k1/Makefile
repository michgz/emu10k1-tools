# Makefile for fv10k1
#
# $Id: Makefile,v 1.1 2001/09/28 01:56:20 dbertrand Exp $

VERSION := 0.3.0

ROOMS := $(basename $(wildcard *.room))  

BASEPATCH := fv10k1

BINDIR := bin

INCLUDES := fv-routes.inc fv-controls.inc fv-basstreble.inc emu_constants.inc

ROOMPATCHES := $(foreach room,$(ROOMS),$(BASEPATCH)-$(room))

ROOMINCLUDES := $(foreach room,$(ROOMS),refl-$(room).inc)

ROOMBINARIES := $(addsuffix .bin,$(addprefix $(BINDIR)/,$(ROOMPATCHES))) 

OUTPATCHES := fv-out-f fv-out-r fv-out-f2 fv-out-r2

BINARIES := $(ROOMBINARIES) \
		$(BINDIR)/$(BASEPATCH).bin \
    $(addsuffix .bin,$(addprefix $(BINDIR)/,$(OUTPATCHES))) 

MAKEROOM := ./mkfvroom.pl

MYDIR := $(notdir $(shell pwd))

all: $(BINDIR) $(BINARIES)

$(BINDIR):
	if [ ! -d $(BINDIR) ]; then mkdir $(BINDIR); fi

# some explicit dependencies
$(BINARIES): $(INCLUDES)

$(BINDIR)/$(BASEPATCH).bin: fv-filters.inc

$(ROOMBINARIES): fv-filters-reduced.inc $(ROOMINCLUDES)

# implicit rule for assembling binaries
$(BINDIR)/%.bin: %.asm $(INCLUDES)
	as10k1 $<
	mv $*.bin $(BINDIR)

# implicit rules for converting m4 to asm
%.asm: %.m4
	m4 $< >$@

%.inc: %.m4
	m4 $< >$@

# explicit rule for making reduced Freeverb definitions
fv-filters-reduced.inc: fv-filters.m4
	m4 -DREDUCED $< >$@

# rule for making Freeverb patches with reflection support
$(BASEPATCH)-%.asm: $(BASEPATCH).m4 
	m4 -DROOM=$* $< >$@

# rules to make room includes from room reflections
calcroom: calcroom.C
	gcc -O2 $< -lm -o $@                                                        

# makes a .room.rp (room paths) from a .room (room definition) file,
# and generates an include files
refl-%.inc: %.room calcroom $(MAKEROOM) 
	./calcroom $< >/dev/null
	$(MAKEROOM) $<.rp >$@

# implicit rules for generating output patches
fv-out-%2.asm: fv-out.m4  
	m4 -DLINE=$* -DTWOSPEAKER $< >$@

fv-out-%.asm: fv-out.m4  
	m4 -DLINE=$* $< >$@

# to prevent make from deleting the auto-generated sources
.PRECIOUS: $(ROOMINCLUDES) \
	$(addsuffix .asm,$(BASEPATCH) $(ROOMPATCHES) $(OUTPATCHES)) 

tarball:
	(cd ..; tar zcvf fv10k1-$(VERSION).tar.gz --exclude RCS $(MYDIR))

clean:
	rm -f $(BINARIES) $(addsuffix .asm,$(BASEPATCH) $(ROOMPATCHES) $(OUTPATCHES)) \
    fv-filters.inc fv-filters-reduced.inc $(ROOMINCLUDES) \
		$(addsuffix .room.rp,$(ROOMS)) \
		calcroom *.bck core


