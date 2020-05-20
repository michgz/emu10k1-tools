all: dbgemu



dbgemu: dbgemu.c

	gcc -o dbgemu -Wall dbgemu.c


install: dbgemu

	install -m 755 dbgemu /usr/local/bin/
clean:
	rm -f .depend core *~ *.o dbgemu
