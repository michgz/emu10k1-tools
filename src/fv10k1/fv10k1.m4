;;     fv10k1.m4 - fv10k1 package
;;     This implements Jezar Wakefield's Freeverb algorithm
;;         
;;     Copyright (C) 2001 Oleg Smirnov <smirnov@astron.nl>
;;    
;;     This program is free software; you can redistribute it and/or modify
;;     it under the terms of the GNU General Public License as published by
;;     the Free Software Foundation; either version 2 of the License, or
;;     (at your option) any later version.
;; 
;;     This program is distributed in the hope that it will be useful,
;;     but WITHOUT ANY WARRANTY; without even the implied warranty of
;;     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
;;     GNU General Public License for more details.
;; 
;;     You should have received a copy of the GNU General Public License
;;     along with this program; if not, write to the Free Software
;;     Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
;; 
;; $Id: fv10k1.m4,v 1.1 2001/09/28 01:56:20 dbertrand Exp $

  name "Freeverb"
  
  include "emu_constants.inc"
  include "fv-routes.inc"
  include "fv-controls.inc"
  include "fv-basstreble.inc"
  
	; IO lines right/left
ior io
iol io
	
ifdef(`ROOM',`  
  ; delay lines for room reflections and reverb predelay use ITRAM
dlyr delay &.084   
dlyl delay &.084   
  ; Freeverb filters (reduced set) go into XTRAM
  include "fv-filters-reduced.inc"
',`
  ; No room reflection support - load full set of Freeverb filters and put 
  ; them into ITRAM first
  include "fv-filters.inc"
  ; delay lines for channel & reverb predelay will use XTRAM
dlyr delay &2   
dlyl delay &2   
')

writer twrite dlyr,0
writel twrite dlyl,0
oreadr tread dlyr,64     ; use 64 samples to avoid some TRAM glitches
oreadl tread dlyl,64
revreadr tread dlyr,64
revreadl tread dlyl,64
	
input dyn 1         ; wet reverb input [== (inl+inr)*gain ]

dly_b2	sta 0,0   ; storage for second bass/treble filter
dly_t2	sta 0,0

  ifdef(`ROOM',`;;; pull in room reflections code
  include "refl-ROOM.inc"')

  ;;; update TRAM read addresses from control GPRs
  acc3 oreadr.a,delay_r,writer.a,C_0
  acc3 oreadl.a,delay_l,writel.a,C_0
  acc3 revreadr.a,revdelay,writer.a,C_0
  acc3 revreadl.a,revdelay,writel.a,C_0

  ;;; init reverb outputs (and clear ACCUM for code below)
  macs fvrev_l,C_0,C_0,C_0
  macs fvrev_r,C_0,C_0,C_0

  ;;; accumulate reverb inputs ( predelayed R+L * revgain )
  ;;; and at the same time pass input to output w/delay
  macmv ior,oreadr,revreadr,revgain
  macs input,ACCUM,revreadl,revgain
	acc3 iol,oreadl,C_0,C_0
  acc3 writer,ior,C_0,C_0
  acc3 writel,iol,C_0,C_0

  ;;; apply & accumulate comb filters
  do_comb_filters fvrev_l,fvrev_r
  
  ;;; apply allpass filters
  do_allpass_filters fvrev_l,fvrev_r
  
  ;;; feed accumulated values to outputs, multiplying by wet & dry controls
  interp tmp,fvrev_l,wet1,fvrev_r
  interp tmpout,fvrev_r,wet1,fvrev_l
  macs fvrev_l,tmp,revreadl,dry
  macs fvrev_r,tmpout,revreadr,dry
  
  ;;; apply bass/treble controls to output
  test revdefeat
  bne .skipbasstreble
  basstreble fvrev_l,fvrev_l,revbass,revtreble,dly_b1,dly_t1
  basstreble fvrev_r,fvrev_r,revbass,revtreble,dly_b2,dly_t2
.skipbasstreble

  ;;; reset level meters at specified interval (use DBAC to track it)
  andxor tmp,DBAC,level_interval,C_0
  bne .skipreset
  acc3 maxlev_fr,C_0,C_0,C_0
  acc3 maxlev_fl,C_0,C_0,C_0
  acc3 maxlev_rr,C_0,C_0,C_0
  acc3 maxlev_rl,C_0,C_0,C_0
.skipreset
  
  END
