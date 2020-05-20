;;; signal inversion effect
	name "Invert"


gain	constant	$80000000

signal	io

	macs	signal, $40, signal, gain ; signal= 0 + signal * -1
	end
