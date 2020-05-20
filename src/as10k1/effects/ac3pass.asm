; AC3 Passthrough for Emu10k1
; Author: Juha Yrjölä <jyrjola@cc.hut.fi>
; Date: Mar 04, 2001

;;; This program is free software; you can redistribute it and/or modify
;;; it under the terms of the GNU General Public License as published by
;;; the Free Software Foundation; either version 2 of the License, or
;;; (at your option) any later version.

	include "emu_constants.inc"

	name "AC3pass"

ac3_a	delay $3fff
ac3_b	delay $3fff

ac3in_a		tread ac3_a, 0
ac3in_b		tread ac3_b, 0

out_a	io
out_b	io
in_a	equ out_a
in_b	equ out_b

;out_a	equ $22
;out_b	equ $23
;out_a	equ $2a
;out_b	equ $2b

enable		control 0,0,1
count		control 0,0,1
ptr		control 0,0,1

xormask	con $70000000
masklin	con $ffff0000		; 16 MSBs
masklog	con $fffff000		; 20 MSBs
C_7	con $7
;C10000 con $10000
limit	con 1024
limit_s	con 1056
limit_a con $200f800
limit_b con $400f800

bitmask	con $f800
dbac_v	con $e000

active	sta 0
tmp	dyn 1

;	move	out_a, C_0
;	move	out_b, C_0

	test	enable
        skip    CCR,CCR,C_8,.disabled
	test	active
        skip    CCR,CCR,C_256,.activated
	and	tmp, DBAC, bitmask
	macints	C_0, tmp, C_n1, dbac_v
        skip    CCR,CCR,C_256,.disabled_b
	move	active, C_1
.activated
	and	tmp, ac3in_a, masklog		; tmp = in_a & 0xfffff000
	log	tmp, tmp, C_7, C_0		; tmp = log(tmp)
	and	tmp, tmp, masklin		; tmp = tmp & $ffff0000
	andxor	tmp, tmp, C_n1, xormask
	skip	C_0, CCR, C_4, .a_pos
	andxor	tmp, tmp, C_n1, xormask
.a_pos
	move	out_a, tmp
	
 	and	tmp, ac3in_b, masklog		; tmp = in_b & 0xfffff000
	log	tmp, tmp, C_7, C_0		; tmp = log(tmp)
	and	tmp, tmp, masklin		; tmp = tmp & $ffff0000
	andxor	tmp, tmp, C_n1, xormask
	skip	C_0, CCR, C_4, .b_pos
	andxor	tmp, tmp, C_n1, xormask
.b_pos
	move	out_b, tmp

;	macints	out_b, C_0, ptr, C10000
	
	macintw	ptr, ptr, C_1, C_n1
	
	test	ptr
        skip    CCR,CCR,C_256,.noreset
        move	ptr, limit
        macintw	count, count, C_1, C_1
        move	IRQ, C_nmax
.noreset
	move	active, C_1
	skip	C_0, C_max, C_max, .truly_the_end
.disabled
.disabled_b
	move	ptr, limit_s
	macintw	ac3in_a.a, limit_a, DBAC, C_n1
	macintw ac3in_b.a, limit_b, DBAC, C_n1
	move	active, C_0
	move	count, C_0
	move	out_a, in_a
	move	out_b, in_b
.the_end
.truly_the_end
	end
