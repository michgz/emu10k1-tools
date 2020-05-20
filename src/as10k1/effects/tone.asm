;;; Bass and Treble Effect
;;; By:	   Daniel Bertrand
;;; Date:  August 14th,2001
;;; License: GPL v2
;;;
	name "Tone Control"
	include "emu_constants.inc"
	
;;; a and b coefs for bass:	
bass	control $3e4f844f, #-1, #1 
bass_1	control $84ed4cc3, #-1, #1 
bass_2	control $3cc69927, #-1, #1 
bass_3	control $7b03553a, #-1, #1 
bass_4	control $c4da8486, #-1, #1 
	
;;; a and b coef for treble:	
treble control $0125cba9, #-1, #1 
treble_1 control $fed5debd, #-1, #1 
treble_2 control $00599b6c, #-1, #1 
treble_3 control $0d2506da, #-1, #1 
treble_4 control $fa85b354, #-1, #1 

in	IO
out	equ in

tbl_d0 static 0
tbl_d1 static 0
tbl_d2 static 0
tbl_d3 static 0

bss_d0 static 0		
bss_d1 static 0		
bss_d2 static 0		
bss_d3 static 0		
	
tmp dynamic
;; bass:
	
	fracmult  tmp,$04c, in
	 
	fracmult    $0040, tmp, bass
	macmv   bss_d1, bss_d0, bss_d1, bass_2
	macmv   bss_d0, tmp , bss_d0, bass_1
	macmv   bss_d3, bss_d2, bss_d3, bass_4
	macs    bss_d2, $56, bss_d2, bass_3
	acc3    bss_d2, bss_d2, bss_d2, $40

;; treble:
	
	fracmult    $40, bss_d2, treble
	macmv   tbl_d1, tbl_d0, tbl_d1, treble_2  
	macmv   tbl_d0, bss_d2, tbl_d0, treble_1
	macmv   tbl_d3, tbl_d2, tbl_d3, treble_4
	macs    tbl_d2, $56, tbl_d2, treble_3
	macints tbl_d2, $40, tbl_d2, $46

	;;; 2x gain at output, may cause a bit of distortion at higher levels:
;	 acc3   out, tbl_d2, tbl_d2, $40

	;;; more correct. 
	move out, tbl_d2
	
	end
		
