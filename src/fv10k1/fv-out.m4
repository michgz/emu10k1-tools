;;     fv-out.m4 - fv10k1 package
;;     Generates output patches for fv10k1.
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
;; $Id: fv-out.m4,v 1.1 2001/09/28 01:56:20 dbertrand Exp $

  name "Freeverb"

  include "emu_constants.inc"
  include "fv-routes.inc"
  include "fv-controls.inc"
  include "fv-basstreble.inc"

dly_b2	sta 0,0   ; storage for additional bass/treble filters
dly_t2	sta 0,0
dly_b3	sta 0,0   
dly_t3	sta 0,0
dly_b4	sta 0,0   
dly_t4	sta 0,0

reflr dyn 2
refll dyn 2
reverbr dyn 1
reverbl dyn 1
ptr dyn 2
ptl dyn 2
  
define(`PASTE',`$1$2')

define(`PT',`PASTE(pt,LINE)_$1'))
define(`REVOUT',`fvrev_$1')
define(`REVLEVEL',PASTE(reverb_,LINE))
define(`REFLLEVEL',PASTE(refl_,LINE))
define(`MAXLEVEL',`PASTE(maxlev_,LINE)$1')
define(`REFL',`fvrefl_$1$2')

define(`LINE2',ifelse(LINE,`f',`r',`f')) ; other line is LINE2

; output is sum of IN*PT(level),REVOUT*REVLEVEL,REFL(LINE,CHANNEL)
ifdef(`TWOSPEAKER',`; and REFL(LINE2,CHANNEL)   [two-speaker mode]')

ior IO
iol IO

  ;;; apply reflection levels and bass/treble
  macs reflr,C_0,REFL(LINE,r),REFLLEVEL
  macs refll,C_0,REFL(LINE,l),REFLLEVEL
  ifdef(`TWOSPEAKER',`
  macs reflr,reflr,REFL(LINE2,r),REFLLEVEL   ; two-speaker mode - add in other line
  macs refll,refll,REFL(LINE2,l),REFLLEVEL')
  ifelse(LINE,`r',`; apply rear reflections gain
    macints reflr,C_0,reflr,refl_rgain
    macints refll,C_0,refll,refl_rgain
  ')
  
  ;;; apply reverb levels
  macs  reverbr,C_0,REVOUT(r),REVLEVEL
  macs  reverbl,C_0,REVOUT(l),REVLEVEL
  ifelse(LINE,`r',`; apply rear reverb gain
  macints reverbr,C_0,reverbr,reverb_rgain
  macints reverbl,C_0,reverbl,reverb_rgain')

  ifelse(LINE,`r',`;;; apply additional rear bass/treble filters to input
  macs ptr,C_0,ior,PT(level)
  macs ptl,C_0,iol,PT(level)
  acc3 ptl+1,C_0,C_0,C_0
  acc3 ptr+1,C_0,C_0,C_0
  test PT(defeat)
  bne .endinput
  basstreblesep ptr,ptr+1,ptr,PT(bass),PT(treble),dly_b1,dly_t1
  basstreblesep ptl,ptl+1,ptl,PT(bass),PT(treble),dly_b2,dly_t2
.endinput
  ;;; apply rear passthru gain
  macints ptr,C_0,ptr,ptr_gain
  macints ptl,C_0,ptl,ptr_gain
  macints ptl+1,C_0,ptl+1,ptr_gain
  macints ptr+1,C_0,ptr+1,ptr_gain
  ')

  ;;; write in+reverb_reflections to output    
  ;;; use macmv accumulation for extra precision
makeoutput MACRO io,refl,reverb,passthru
  macs C_0,C_0,C_0,C_0
  macmv tmp,tmp,refl,C_max
  ifelse(LINE,`r',`
    macmv tmp,C_0,passthru,C_max
    macmv tmp,C_0,passthru+1,C_max
  ',`
    macmv tmp,C_0,io,PT(level)
  ')
  macs io,ACCUM,reverb,C_max
  ENDM
  makeoutput ior,reflr,reverbr,ptr
  makeoutput iol,refll,reverbl,ptl
  
  ;;; maintain the maximum level 
maxlevel MACRO io,maxlev
  tstneg tmp,io,io,C_0             ; tmp = abs(io)
  limit maxlev,tmp,tmp,maxlev  ; maxlevel=max(tmp,maxlevel)
  ENDM
  maxlevel ior.o,MAXLEVEL(r)
  maxlevel iol.o,MAXLEVEL(l)
  
  END
