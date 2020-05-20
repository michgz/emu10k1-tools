include ./Makefile.config

all: As10k1 Play-ac3 Mixer

Play-ac3:
	cd play-ac3 && $(MAKE)
As10k1:
	cd as10k1 && $(MAKE)

Mixer:
	cd mixer && $(MAKE)


VERSION :=0.9.4
src_dst := emu-tools-$(VERSION)

install: all
	mkdir -p $(bindir)
	mkdir -p $(data_dir)
	mkdir -p $(man_prefix)/man1
	mkdir -p $(script_dir)
	cd as10k1 && $(MAKE) install
	cd mixer && $(MAKE) install
	cd scripts && $(MAKE) install

play-ac3:

clean: 
	cd as10k1 && $(MAKE) clean
	cd mixer && $(MAKE) clean
	cd play-ac3 && $(MAKE) clean

src-dist: clean
	rm -rf $(src_dst)
	rm -rf $(src_dst).tar.gz
	mkdir $(src_dst)

	cp -r as10k1 $(src_dst)
	cp -r mixer  $(src_dst)
	cp -r play-ac3  $(src_dst)
	cp -r dbgemu  $(src_dst)
	cp -r scripts $(src_dst)
	cp -r fv10k1 $(src_dst)

	cp Makefile Makefile.config ../GNU_GPL $(src_dst)

	mkdir $(src_dst)/docs

	cp CHANGES $(src_dst)/docs/
	cp ../docs/README-TOOLS $(src_dst)/docs/README
	cp --target-directory=$(src_dst)/docs  ../docs/dsp.txt\
	 ../docs/manuals.txt ../docs/TODO ../docs/multichannel.txt\
	 ../docs/README.FAQ 

	tar cfz $(src_dst).tar.gz $(src_dst) --exclude CVS
	rm -rf $(src_dst)
