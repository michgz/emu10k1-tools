; Mixer for Emu10k1
; Author: Robert Mazur <robertmazur@yahoo.com>
; Date: Jan 09, 2002
; Version 1.1

; This program is free software; you can redistribute it and/or modify
; it under the terms of the GNU General Public License as published by
; the Free Software Foundation; either version 2 of the License, or
; (at your option) any later version.

; Add signal from center channel to front left and right channels

;========================================

	name	"C2F"

	include	"emu_constants.inc"

;========================================
L		io			; Left
R		io			; Right
C		io			; Center

;========================================
;		Start
;========================================
	macs	L, L, C, C_2^30		; L = L + C/2
	macs	R, R, C, C_2^30		; R = R + C/2
	end
;========================================
