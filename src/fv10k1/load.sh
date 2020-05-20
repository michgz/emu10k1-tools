#!/bin/bash
# $Id: load.sh,v 1.1 2001/09/28 01:56:20 dbertrand Exp $

patchdir=bin
input=Pcm

patch() {
  printf "%-8s%-30s" "$1:" "$2"
  if emu-dspmgr -l"$1 R" -l"$1 L" -f $patchdir/$2; then
    echo "[ OK ]"
  fi
}

# unload everything first
unload.sh

if [ "$1" = "-h" ]; then 
  echo "Usage: load.sh [room name]"
  echo "If room name is specified, use 'load.sh room_name 2' for two-speaker mode."
  exit 1
elif [ "$1" = "" ]; then
  freeverb=fv10k1
  patch $input fv10k1.bin
  patch Front fv-out-f2.bin
  patch Rear  fv-out-r2.bin
else
  room="$1"
  echo "Loading room reflections for room '$room'"
  if [ "$2" = "2" ]; then
    echo "Using two-speaker mode"
    x=2
  else
    echo "Using four-speaker mode"
  fi
  # Load input patches ahead of main module, to grab internal TRAM
  patch $input fv10k1-$room.bin
  patch Front fv-out-f$x.bin
  patch Rear  fv-out-r$x.bin
fi

# emu-dspmgr -u
