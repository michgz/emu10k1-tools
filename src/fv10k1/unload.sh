#!/bin/bash
# $Id: unload.sh,v 1.1 2001/09/28 01:56:20 dbertrand Exp $

emu-dspmgr -uFreeverb
for n in 1 2 3 4 5 6 7 8 9; do 
  emu-dspmgr -u"Freeverb $n"
done
