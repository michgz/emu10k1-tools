; Surround Matrix for Emu10k1
; Author: Robert Mazur <robertmazur@yahoo.com>
; Date: Jan 14, 2002
; Version 1.2

; This program is free software; you can redistribute it and/or modify
; it under the terms of the GNU General Public License as published by
; the Free Software Foundation; either version 2 of the License, or
; (at your option) any later version.

;========================================

	name	"Surround"

	include "emu_constants.inc"

;========================================

delline		delay	&0.02		; 0.02 sec delay
write		twrite	delline,&0	; write at 0 sec
read		tread	delline,&0.02	; read at 0.02 sec

;----------------------------------------
ml		con	#0.575997	; lpf 7000Hz
yl		sta	0

mlp		con	#0.277015	; lpf 2500Hz
mhp		con	#3.7076e-2	; hpf 300Hz
ylp		sta	0
shp		sta	0

;----------------------------------------	
Lt		io			; Stereo Left (Left Total)
Rt		io			; Stereo Right (Right Total)
L		equ Lt			; Front  Left
R		equ Rt			; Front  Right
Ls		io
Rs		io
C		io			; Center

;----------------------------------------
tmp		dyn

	
;========================================
;		Start
;========================================
	sub	write, Lt, Rt		; delay.in = L - R 
	
	lpf	yl, ml, read 		; yl = LowPass(7kHz, delay.out) = surround
	
	move	Ls, yl
	move	Rs, yl

	add	tmp, Lt, Lt		; tmp = L + R
	
	hpf	tmp, shp, mhp, tmp	; tmp = HighPass(300Hz, tmp)
	lpf	ylp, mlp, tmp		; ylp = LowPass(2.5kHz, tmp) = center
	
	move	C, ylp

	sub	R, Rt, Ls		; R = R - surround
	sub	L, Lt, Rs		; L = L - surround

	end
;========================================

