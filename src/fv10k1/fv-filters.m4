;;     fv-filters.m4 - fv10k1 package
;;     defines delay lines and GPRs for Freeverb's comb and allpass filters
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
;; $Id: fv-filters.m4,v 1.1 2001/09/28 01:56:20 dbertrand Exp $

ifdef(`REDUCED',`; This is a shorter version that only uses 32 address lines')

ifelse(
; I use m4 to do some arithmetic, to get around as10k1's limited syntax.
; All numbers here are taken direct from Freeverb, but scaled
; by 1.09 to get from Freeverb 44KHz sampling to EMU10K1's 48KHz 
)

dnl ; this macro multiplies its input by 1.09 (~ 48/44)
define(`scale',`eval($1+($1*9)/100)')

dnl ; comb filter & allpass filter stereo spread constant
define(`ss',`scale(23)')

dnl ; Padding of delay lines (to get around missed read/write problems)
dnl ; Ideally, this should be 0
define(`pad',`20')

dnl ; Defines delay lines for a generic pair of filters
define(`FILTERBLOCK',`
$1l$2 delay eval(scale($3)+pad)
$1r$2 delay eval(scale($3)+ss+pad)
w$1l$2 twrite $1l$2,0
w$1r$2 twrite $1r$2,0
r$1l$2 tread $1l$2,eval(scale($3))
r$1r$2 tread $1r$2,eval(scale($3)+ss)

')

dnl ; Defines delay lines for a pair of comb filters
define(`COMB',`
csl$1 sta 0
csr$1 sta 0
FILTERBLOCK(cd,$1,$2)')

dnl ; Defines delay lines for a pair of allpass filters
define(`ALLPASS',`FILTERBLOCK(ap,$1,$2)')

;;; macro to apply & accumulate a comb filter
comb MACRO dest,rdelay,wdelay,filterstore
  acc3 dest,dest,rdelay,C_0
  interp filterstore,rdelay,damp,filterstore
  macs wdelay,input,filterstore,roomsize
  endm

;;; macro to apply an allpass filter
allpass MACRO dest,rdelay,wdelay
  macs wdelay,dest,rdelay,allpassfeed
  macs1 dest,rdelay,dest,C_max
  endm

define(`MAKECOMBS',`
ifdef(`REDUCED',`;;; Freeverb reduced mode - 4 comb filters only
COMB(5,1422)
COMB(6,1491)
COMB(7,1557)
COMB(8,1617)
;;; define macro to apply all comb filters to land r
do_comb_filters MACRO l,r
  comb l,rcdl5,wcdl5,csl5
  comb l,rcdl6,wcdl6,csl6
  comb l,rcdl7,wcdl7,csl7
  comb l,rcdl8,wcdl8,csl8
  comb r,rcdr5,wcdr5,csr5
  comb r,rcdr6,wcdr6,csr6
  comb r,rcdr7,wcdr7,csr7
  comb r,rcdr8,wcdr8,csr8
  ENDM
',`;;; Freeverb full mode - 8 comb filters
COMB(1,1116)
COMB(2,1188)
COMB(3,1277)
COMB(4,1356)
COMB(5,1422)
COMB(6,1491)
COMB(7,1557)
COMB(8,1617)
do_comb_filters MACRO l,r
  comb l,rcdl1,wcdl1,csl1
  comb l,rcdl2,wcdl2,csl2
  comb l,rcdl3,wcdl3,csl3
  comb l,rcdl4,wcdl4,csl4
  comb l,rcdl5,wcdl5,csl5
  comb l,rcdl6,wcdl6,csl6
  comb l,rcdl7,wcdl7,csl7
  comb l,rcdl8,wcdl8,csl8
  comb r,rcdr1,wcdr1,csr1
  comb r,rcdr2,wcdr2,csr2
  comb r,rcdr3,wcdr3,csr3
  comb r,rcdr4,wcdr4,csr4
  comb r,rcdr5,wcdr5,csr5
  comb r,rcdr6,wcdr6,csr6
  comb r,rcdr7,wcdr7,csr7
  comb r,rcdr8,wcdr8,csr8
  ENDM
')')

define(`MAKEALLPASSES',`;;; 4 allpass filters
ALLPASS(1,556)
ALLPASS(2,441)
ALLPASS(3,341)
ALLPASS(4,225)
do_allpass_filters MACRO l,r
  allpass l,rapl1,wapl1
  allpass l,rapl2,wapl2
  allpass l,rapl3,wapl3
  allpass l,rapl4,wapl4
  allpass r,rapr1,wapr1
  allpass r,rapr2,wapr2
  allpass r,rapr3,wapr3
  allpass r,rapr4,wapr4
  ENDM
')

dnl ; Define the filters in different order for short or full Freeverb

ifdef(`REDUCED',`
;;; Freeverb reduced mode - comb filter delay lines go ahead, since for 
;;; reduced Freeverb we do not want to use internal TRAM -- it may be out 
;;; of address lines thanks to the reflection engine, so external TRAM 
;;; is preferrable. Hence longer delays (for comb filters) go first.
MAKECOMBS
;;; Now define the allpass filter delay lines
MAKEALLPASSES
',`;`
;;; Freeverb full mode - allpass filter delay lines go ahead, since
;;; we'd rather use internal TRAM. Hence shorter delays  go first.
MAKEALLPASSES
;;; Now define the comb filter delay lines
MAKECOMBS
;'')

  END
