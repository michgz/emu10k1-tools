
	name  "Attenuation"

;;; The next line controls the amount of attenuation (between 0 and 1)	 
attenuation	constant  #0.25

			
signal	io

	macs	signal, $40, signal, attenuation; signal= 0 + signal * attenuation
	
	end
