	name "2-channel Vol"

Vol_L control  #1,0,#1
Vol_R control  #1,0,#1
	
signal_l	IO
signal_r	IO
	
	macs  signal_l,$40,signal_l,Vol_L
	macs  signal_r,$40,signal_r,Vol_R

		end
	

	
