;;; Simple Stereo flanger
;;; Author:Daniel Bertrand
;;; Date: July 3, 2001

;;; This program is free software; you can redistribute it and/or modify  
;;; it under the terms of the GNU General Public License as published by  
;;; the Free Software Foundation; either version 2 of the License, or     
;;; (at your option) any later version.

;;; References:	
;;; http://www.harmony-central.com/Effects/Articles/Flanging/
		
;;;  speed( formerly "delta")=2*pi*freq/48000
;;;  this give us our delta value for a specific freq (0.1-0.3Hz is good)
		

	include "emu_constants.inc"
	name "stereo flanger"
		
in1	IO
in2	IO
	
out1	equ in1	
out2	equ in2
	
	
speed	  control  2e-05 , 0 , 1e-4 	; Controls frequency (radians)
delay   control  &7e-3  ,$1600 , 20e-3	; twice (2*) average delay (sec)
width	  control  #0.33  ,0	 ,0.5	; width control 
forward	  control  #1	 ,0      ,#1	; forward mix 
feedback  control  0.3   ,0      ,0.5	; feedback level 

;; sine generator storage spaces:	
sinx  sta  0	
cosx  sta  #0.5
			
tmp  dyn 	
tmp2 dyn
	
;;; Two Delay Lines:	
	
	
dly1   delay  &20e-3		;20msec delay line
	
write1	twrite dly1,0		; tram writes
ready1	tread dly1,0		; tram reads
reada1	tread dly1,0
	

dly2   delay  &20e-3		;20msec delay line
	
write2	twrite dly2,0		; tram writes
ready2	tread  dly2,0		; tram reads
reada2	tread  dly2,0
					
;;;The code:	
	
		
;;; two opcode sinewave generator (I love this chip!):
	macs  sinx,sinx,speed,cosx
	macs1 cosx,cosx,speed,sinx 

;;; 0.5Asint+0.5:	
	macs tmp,C_2^30,sinx,width
	
;;; calculate address:
	macs  ready1.a,write1.a,delay,tmp
	macs  ready2.a,write2.a,delay,tmp
;second addresses for interpolation:
;(interesting how the emu engineers decided that $800 wasn't a needed value) 
	macints reada1.a,ready1.a,C_8,C_256 
	macints reada2.a,ready2.a,C_8,C_256 
	
	
;;; output values:		
;;; 0x55 is 00100000 (?)
	macints tmp,C_0,reada1.a,C_LSshift; get least significant part of address
	
	interp tmp2,ready1,tmp,reada1 ;interpolate in-between the two delay line readings
	macs  out1,in1,tmp2,forward

;;; feedback and write to the delay line:
	
	macs  write1,in1,tmp2,feedback
		
	
;;; output values:		
;;; 0x55 is 00100000 (?)
	macints tmp,C_0,reada2.a,C_LSshift; get least significant part of address
	
	interp tmp2,ready2,tmp,reada2 ;interpolate in-between the two delay line readings
	macs  out2,in2,tmp2,forward

;;; feedback and write to the delay line:
	
	macs  write2,in2,tmp2,feedback
		
	
	end





