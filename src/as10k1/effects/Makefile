AS =../as10k1
include ../../Makefile.config

%.bin : %.asm
	$(AS)  $< 

DSP := $(shell ls | grep -v ac3pass |grep .asm | grep -v asm\~ | sed s/asm/bin/ )

all: ac3pass.bin dsp 

install: all
	mkdir -p $(data_dir)
	install -m 664 *.bin $(data_dir)
	install -m 664 README $(data_dir)


clean:
	rm -f *~ core .depend *.bin

dsp: $(DSP)

ac3pass.bin: ac3pass.asm emu_constants.inc
	$(AS) -n ac3pass.asm

