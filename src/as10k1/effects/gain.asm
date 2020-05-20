	name "Gain"

;;; The next line controls the amount of gain (must be an integer)
gain	constant	4

signal	io

	macints	signal, $40, signal, gain ; signal= 0 + signal * gain
	end
