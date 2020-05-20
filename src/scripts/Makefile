include ../Makefile.config

conf_avail := $(shell find $(script_dir) -name emu10k1.conf -print)

install:
ifeq ($(conf_avail),)
	install -m 644  emu10k1.conf $(script_dir)
else
	@echo "configuration file already installed..."
endif
	install -m 755 emu-script $(script_dir)

