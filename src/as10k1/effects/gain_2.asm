	name "2-channel Gain"

;;; The next line controls the amount of gain (must be an integer)
gain	constant	4

	
signal1	io
signal2	io

 	
	macints	signal1, $40, signal1, gain ; signal= 0 + signal * gain
	macints	signal2, $40, signal2, gain


	end
