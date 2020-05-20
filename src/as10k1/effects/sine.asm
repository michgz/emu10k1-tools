	name "Sine wave Gen"
	include "emu_constants.inc"

in	io
out	equ  in
	

delta control $3afa691,0,$7fffffff ; controls frequency


vol   control #1,0,#1		; amplitude of sinewave 
cosx  sta #1
sinx  sta 0	

	
	macs  sinx,sinx,delta,cosx       
	macs1 cosx,cosx,delta,sinx 
	macs  out,C_0, vol,cosx

	
	end








