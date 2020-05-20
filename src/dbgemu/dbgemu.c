/*********************************************************************
 *     dbgemu.c - debuging/hacking tool for the emu10k1 registers
 *     Copyright (C) 2000 Daniel Bertrand
 *
 ********************************************************************* 
 *     This program is free software; you can redistribute it and/or 
 *     modify it under the terms of the GNU General Public License as 
 *     published by the Free Software Foundation; either version 2 of 
 *     the License, or (at your option) any later version. 
 * 
 *     This program is distributed in the hope that it will be useful, 
 *     but WITHOUT ANY WARRANTY; without even the implied warranty of 
 *     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the 
 *     GNU General Public License for more details. 
 * 
 *     You should have received a copy of the GNU General Public 
 *     License along with this program; if not, write to the Free 
 *     Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, 
 *     USA. 
 ********************************************************************* 
*/

#include <string.h>

#include <stdio.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/soundcard.h>
#include <linux/types.h>
#include <fcntl.h>
#include <stdlib.h>
#include "dbgemu.h"

int dump_fn0(int offset)
{
	int ret;
	struct mixer_private_ioctl ctl;
	

	if ((fd2 = open("/dev/mixer", O_RDWR, 0)) == -1) {
		perror("open");
		exit(EXIT_FAILURE);
	}


	ctl.cmd = CMD_READFN0;
	ctl.val[0]=offset;
	ret = ioctl(fd2, SOUND_MIXER_PRIVATE3, &ctl);	
	if (ret < 0) {
		perror("SOUND_MIXER_PRIVATE3");
		exit(EXIT_FAILURE);
	}
	
	close(fd2);
	return ctl.val[2];
}

int load_fn0(int offset,__u32 value)
{
	int ret;
	struct mixer_private_ioctl ctl;

	if ((fd2 = open("/dev/mixer", O_RDWR, 0)) == -1) {
		perror("open");
		exit(EXIT_FAILURE);
	}


	ctl.cmd = CMD_WRITEFN0;
	ctl.val[0]=offset;
	ctl.val[1]=value;
	ret = ioctl(fd2, SOUND_MIXER_PRIVATE3, &ctl);	
	if (ret < 0) {
		perror("SOUND_MIXER_PRIVATE3");
		exit(EXIT_FAILURE);
	}
	
	close(fd2);
	return 0;
}

void load_ac97(int reg, int val)
{
	load_fn0(0x1e,(int)reg&0xff);
	load_fn0(0x1c, (reg<<16)|val);
	printf("AC97 Reg 0x%02x  --> 0x%04x\n" ,reg,0xffff&dump_fn0(0x1c));
}
int dump_ac97(int reg){


	load_fn0(0x1e,(int)reg&0xff);

	printf("AC97 Reg 0x%02x  --> 0x%04x\n" ,reg,0xffff&dump_fn0(0x1c));

	return 0;
}

	
int dump_all_fn0()
{
	int ret,i;
	struct mixer_private_ioctl ctl;
	__u32  fn0_reg[8];

	if ((fd2 = open("/dev/mixer", O_RDWR, 0)) == -1) {
		perror("open");
		exit(EXIT_FAILURE);
	}

	ctl.cmd = CMD_READFN0;
	
	
	for(i=0;i<8;i++){
		ctl.val[0]=i*4;
		ret = ioctl(fd2, SOUND_MIXER_PRIVATE3, &ctl);	
		if (ret < 0) {
			perror("SOUND_MIXER_PRIVATE3");
			exit(EXIT_FAILURE);
		}
		fn0_reg[i]=ctl.val[2];
		//printf("0x%02x: 0x%08x\n",i*4,ctl.val[2]);

	}

	
	for (i = 0; i <6  ; i++) 
		printf("0x%02x   0x%08x  %s\n", 4*i, fn0_reg[i], pci_desc[i]);
	
	printf("0x%02x         0x%02x  %s\n", 0x18, ((__u8 *)fn0_reg)[0x18],    pci_desc[6]);
	printf("0x%02x         0x%02x  %s\n", 0x19, ((__u8 *)fn0_reg)[0x19],    pci_desc[7]);
	printf("0x%02x       0x%04x  %s\n",   0x1a, ((__u16 *)fn0_reg)[0x1a/2], pci_desc[8]);
	printf("0x%02x       0x%04x  %s\n",   0x1c, ((__u16 *)fn0_reg)[0x1c/2], pci_desc[9]);
	printf("0x%02x         0x%02x  %s\n", 0x1e, ((__u8 *)fn0_reg)[0x1e],    pci_desc[10]);
	printf("0x%02x         0x%02x  %s\n", 0x1f, ((__u8 *)fn0_reg)[0x1f],    pci_desc[11]);

	close(fd2);
	return 0;
}

void print_constants(int offset)
{

	if(offset<OUTPUT_BASE)
		printf("0x%03x: Input    -> %s\n",offset, dsp_in_name[offset]);
	else if(offset<CONSTANTS_BASE)
		printf("0x%03x: Output   -> %s\n",offset,dsp_out_name[offset-OUTPUT_BASE]);
	else if(offset<HW_REG_BASE)
		printf("0x%03x: Constant -> 0x%08x\n",offset,constants[offset-CONSTANTS_BASE]);
	else if(offset<(HW_REG_BASE+HW_REG_SIZE))
		printf("0x%03x: Special  -> %s\n",offset,dsp_hw_name[offset-HW_REG_BASE]);
	else
		printf("0x%03x: Unknown  -> Unknown\n",offset);
}


void dump_code_detailed(long int offset)
{
	copr_buffer buf;
	
	int loword, hiword;
	int op, z, w, x, y;
	int i;
	
	if(offset<0x400){
		printf("Error: offset out of range%s",map);

		exit(0);
	}
	//get instruction:
	buf.command = CMD_READ;
	buf.offs = offset;
	buf.len = 2;

	if (ioctl(fd, SNDCTL_COPR_LOAD, &buf) == -1) {
		perror("SNDCTL_COPR_LOAD");
	}
	
	loword = ((__u32 *) buf.data)[i];
	hiword = ((__u32 *) buf.data)[i + 1];
	op = (hiword >> 20) & 0x1f;
	z = (hiword >> 10) & 0x3ff;
	w = hiword & 0x3ff;
	x = (loword >> 10) & 0x3ff;
	y = loword & 0x3ff;
	
	printf
		("Current instruction:\n 0x%03x   %s  0x%03x, 0x%03x, 0x%03x, 0x%03x\n",
		 buf.offs,op_codes[ op],
		 z, w, x, y);
	printf("Description:\n %s\n\n",op_description[op]);
	printf("Registers used by instruction:\n   Addr   Type        Contents\nR: ");
	z>=0x100? dump_reg(z,1): print_constants(z);
	printf("A: ");
	w>=0x100? dump_reg(w,1): print_constants(w);
	printf("X: ");
	x>=0x100? dump_reg(x,1): print_constants(x);
	printf("Y: ");
	y>=0x100? dump_reg(y,1): print_constants(y);
	
	
}

__u32 load_ptr(__u32 reg, __u32 channel, __u32 data){
	int ret;
	struct mixer_private_ioctl ctl;

	if ((fd2 = open("/dev/mixer", O_RDWR, 0)) == -1) {
		perror("open");
		exit(EXIT_FAILURE);
	}

	ctl.cmd = CMD_WRITEPTR;
	ctl.val[0]=reg;
	ctl.val[1]=channel;
	ctl.val[2]=data;
	
	ret = ioctl(fd2, SOUND_MIXER_PRIVATE3, &ctl);
	if (ret < 0) {
		perror("SOUND_MIXER_PRIVATE3");
		return ret;
	}
	
	
	close(fd2);
	return ctl.val[2];
	


}

__u32 dump_ptr(int reg){

	int ret;
	struct mixer_private_ioctl ctl;
	if ((fd2 = open("/dev/mixer", O_WRONLY, 0)) == -1) {
		perror("open");
		exit(EXIT_FAILURE);
	}

	ctl.cmd = CMD_READPTR;
	ctl.val[0]=reg;
	ctl.val[1]=0;
	ret = ioctl(fd2, SOUND_MIXER_PRIVATE3, &ctl);
	if (ret < 0) {
		perror("SOUND_MIXER_PRIVATE3");
		return ret;
	}
	
	
	close(fd2);
	return ctl.val[2];
}




	
void print_dbg(){
	__u32 val,val2;
	int step_mode_on;
	
	val=dump_ptr(0x52);
	val2=(val&DBG_CONDITION_CODE)>>9;
	
	step_mode_on=(val&DBG_SINGLE_STEP)>>15 ;
	if(step_mode_on){
		
	printf("----------------------------------------------------------------------------\n"
"|Emu10k1 Status                |  Debug register 0x%08x                  |\n"
"|----------------------------------------------------------------------------|\n"
"| single step mode: \e[1;31mON\e[0m	 	  Saturation: %s\e[0m; Last addr: 0x%03x (0x%03x) |\n"
"| program pointer: 0x%03x (0x%03x)  Condition code: 0x%02x -> %s\e[0m %s\e[0m %s\e[0m %s\e[0m %s\e[0m          |\n"
" -----------------------------------------------------------------------------\n"
"\n",
	       val,

	       ((val&DBG_SATURATION_OCCURED)>>25)?"\e[1;31mYes ":"None",
	       (val&DBG_SATURATION_ADDR)>>16,
	        ((val&DBG_SATURATION_ADDR)>>16)*2+0x400, 
	       (val&DBG_SINGLE_STEP_ADDR),
	       (val&DBG_SINGLE_STEP_ADDR)*2+0x400,
	       val2,
	       
	       (val2 &0x10) ? "\e[1;32mS":"s",
	       (val2 & 0x8) ? "\e[1;32mZ":"z",
	       (val2 & 0x4) ? "\e[1;32mM":"m",
	       (val2 & 0x2) ? "\e[1;32mN":"n",
	       (val2 & 0x1) ? "\e[1;32mB":"b"
	       
	);
	dump_code_detailed(0x400+((val*2 )&0x1ff));

	}
	else
		printf("-----------------------------------------------------------------------------\n"
"|Emu10k1 Status                |  Debug register 0x%08x                  |\n"
"|----------------------------------------------------------------------------|\n"
"| single step mode: off	          Saturation: %s\e[0m; Last addr: 0x%03x (0x%03x) |\n"
"|                                                                            |\n"
" ----------------------------------------------------------------------------\n"
"\n", val, ((val&DBG_SATURATION_OCCURED)>>25)?"\e[1;31mYes ":"None",
	       (val&DBG_SATURATION_ADDR)>>16,
		       ((val&DBG_SATURATION_ADDR)>>16)*2+0x400    
		       );

	 

}

	



unsigned long dsp_code_low,dsp_code_high;

void load_reg(int, __u32);

void op(int op, int z,int  w,int  x,int  y)
{
	int  w0, w1;
        
	
	if (op >= 0x10 || op < 0x00){
		printf("illegal op code");
		exit(-1);
	}
	
        
        	
	w0 = (x << 10) | y;
	w1 = (op << 20) | (z << 10) | w;
	dsp_code_low = w0;
	dsp_code_high = w1;
	
}

void set_code(int argc,char **argv){
	int r,a,x,y,i=0,addr;

	addr=(int)strtoul(argv[3], NULL,0);
	printf("0x%03x\t",addr);
	if((addr%2)!=0){
		printf("error, address must be a even number\n");
			exit(-1);
	}
	
	
	while(i<16){
		//printf("%s,%s\n",op_codes[i],argv[3]);
		if(strcasecmp(argv[4],op_codes[i])==0)
			goto match;
		i++;
	}
	printf("bad op_code\n");
	exit(-1);
match:
	printf("OP(0x%03x,",i);
	r=(int)strtoul(argv[5], NULL,0);
	a=(int)strtoul(argv[6], NULL,0);
	x=(int)strtoul(argv[7], NULL,0);
	y=(int)strtoul(argv[8], NULL,0);
	
	printf("  0x%03x, 0x%03x, 0x%03x, 0x%03x)\n",r,a,x,y);

	op(i, r,a, x, y);
	foo=1;
	printf("0x%08x\t0x%08x\n",(int)dsp_code_low,(int)dsp_code_high);
	load_reg(addr,dsp_code_low );
	load_reg(addr+1,dsp_code_high );
	return;
}

void dump_reg(long int offset,long int length)
{

	copr_buffer buf;
	
	int loword, hiword;
	int op, z, w, x, y;
	int align;
	int i;

	if(offset<0x000){
		printf("Error: offset out of range%s",map);

		exit(0);
	}
	

	while (length > 0) {
		buf.command = CMD_READ;
		buf.offs = offset;
		buf.len = length < 1000 ? length : 1000;

		if (ioctl(fd, SNDCTL_COPR_LOAD, &buf) == -1) {
			perror("SNDCTL_COPR_LOAD");
			offset += buf.len;
			length -= buf.len;
			continue;
		}

		for (i = 0; (i < buf.len) && (buf.offs + i) < 0x7ff; i++) {
			if (buf.offs+i <=0x65){
				loword = ((__u32 *) buf.data)[i];
				printf("0x%03x   0x%08x  %s\n", i + buf.offs,
				       loword,config_desc[i+buf.offs]);
			}else if (buf.offs+i <0x100){
				loword = ((__u32 *) buf.data)[i];
				printf("0x%03x   0x%08x  Not implemented\n", i + buf.offs,
				       loword);
			
			}else if (buf.offs + i < 0x400){
				loword = ((__u32 *) buf.data)[i];
				printf("0x%03x: GPR      -> 0x%08x\n", i + buf.offs,
				       loword);
			}else{
				align = (buf.offs + i) % 2;
				loword = ((__u32 *) buf.data)[i + align];
				hiword = ((__u32 *) buf.data)[i + 1 + align];
				op = (hiword >> 20) & 0x1f;
				z = (hiword >> 10) & 0x3ff;
				w = hiword & 0x3ff;
				x = (loword >> 10) & 0x3ff;
				y = loword & 0x3ff;

				printf
				    ("0x%03x   0x%08x   0x%08x   %s  0x%03x, 0x%03x, 0x%03x, 0x%03x\n",
				     i + buf.offs + align, loword, hiword,op_codes[ op],
				     z, w, x, y);
				i++;
			}
		}
		offset += buf.len;
		length -= buf.len;
	}
}
	
void load_reg(int offset, __u32 value)
{
	copr_buffer buf;
	int ret;

	if(offset<0x000 || offset>=0x1000 ){
		printf("Register address out of range\n%s",map);
		exit(-1);
	}

	buf.command = CMD_WRITE;
	buf.offs =  offset;
	buf.len = 1;

	memcpy(buf.data, &value,  sizeof(__u32));

	ret = ioctl(fd, SNDCTL_COPR_LOAD, &buf);
	if (ret < 0) {
		perror("SNDCTL_COPR_LOAD");
		exit(-1);
	}
}
/*
void dump_xtram(int offset, int length){
	copr_buffer buf;
	int i;
	buf.command = CMD_READ_TRAM;


	printf("length:%x, offs:%x\n",length,offset);

	buf.offs = offset;
	buf.len = length < 1000 ? length : 1000;
	
		if (ioctl(fd, SNDCTL_COPR_LOAD, &buf) == -1) {
			perror("SNDCTL_COPR_LOAD");
			exit(0);
		}

	for (i = 0; i < length ; i++) 
		printf("0x%05x   0x%04x\n", i+buf.offs, ((__u16 * )buf.data) [i]);

}
//FIXME: only loads one value at a time
void load_xtram(int offset, int value){
	copr_buffer buf;

	printf("Value:%x, offs:%x\n",value,offset);

	buf.command = CMD_WRITE_TRAM;
	((int *)buf.data)[0]=value;
	buf.offs = offset;
	buf.len = 1;

		if (ioctl(fd, SNDCTL_COPR_LOAD, &buf) == -1) {
			perror("SNDCTL_COPR_LOAD");
			exit(0);
		}
}

*/
void parse_args(int argc,char **argv){
	int val1,val2;
	__u32 val3;
	if (argc<2)
		printf("%s\n%s\n%s\n%s\n",version,usage,map,contact);
	else if( ( strlen(argv[1])==2 )   &&      ( argv[1][0]=='-' )    ){
		switch(argv[1][1])
		{
		case 'd'://dump:
			if (argc>2)
				switch(argv[2][0])
				{

				case 'a':
					if(argc<4){
						fprintf(stderr,"%s",usage);
						exit(EXIT_FAILURE);
					}
					val1=(int)strtoul(argv[3], NULL,16);
					dump_ac97(val1);
					break;
					
				case 'r'://dump registers
					if (argc < 5) {
						fprintf(stderr,"%s",usage);
						exit(EXIT_FAILURE);
					}
					val1=(int)strtoul(argv[3], NULL,16);
					val2=(int)strtoul(argv[4], NULL,16);
					dump_reg(val1,val2);
					break;
				case 'f'://dump fn0 register values
					dump_all_fn0();
					break;
					/*
				case 't'://dump tram values	
					if (argc < 4) {
						fprintf(stderr,"%s",usage);
						exit(EXIT_FAILURE);
					}
					val1=(int)strtoul(argv[2], NULL,16);
					val2=(int)strtoul(argv[3], NULL,16);
					dump_xtram(val1,val2);
					break;*/
				}
			else
				printf("%s\n%s\n%s\n%s\n",version,usage,map,contact);
			break;
		case 'l'://load
			switch(argv[2][0])
			{
			case 'a':
				if (argc < 5) {
					fprintf(stderr,"%s",usage);
					exit(EXIT_FAILURE);
				}
				val1=(int)strtoul(argv[3],0,16);
				val3=strtoul(argv[4],0,16);
				load_ac97(val1,val3);

				break;
				
			case 'c'://code code	
				if( argc<9){
					fprintf(stderr,"%s",usage);
					exit(EXIT_FAILURE);
				}
				set_code(argc,argv);	
				break;

			case 'r'://load register
				if (argc < 4) {
					fprintf(stderr,"%s",usage);
					exit(EXIT_FAILURE);
				}
				val1=(int)strtoul(argv[3],0,16);
				val3=strtoul(argv[4],0,16);
				load_reg(val1,val3);
				break;

				/*
			case 'x'://load a tram value
				if(argc < 4){fprintf(stderr,"%s",usage);
				exit(EXIT_FAILURE);
				}
				val1=(int)strtoul(argv[2],0,16);
				val2=(int)strtoul(argv[3],0,16);
				load_xtram(val1,val2);
				break;
			*/

			case 'f'://load a value to the fn0 registers

				if (argc < 5) {
					fprintf(stderr,"%s",usage);
					exit(EXIT_FAILURE);
				}
				val1=(int)strtoul(argv[3],0,16);
				val3=strtoul(argv[4],0,16);
				load_fn0(val1,val3);
				break;
			}
			break;
		case 'D': //debug
			if (argc>2){	
				if (!strcmp(argv[2],"step_mode"))
					load_ptr(0x52,00,  DBG_SINGLE_STEP  );
				else if (!strcmp(argv[2],"step")){
					if (argc==4){
						val1=(int)strtoul(argv[3], NULL,16);
						//printf("val1=%d",val1);
					}else
						val1=(dump_ptr(0x52)+1)&DBG_SINGLE_STEP_ADDR;
					//printf("val1=%x",val1);	
					load_ptr(0x52,00, DBG_STEP |DBG_SINGLE_STEP|val1 );	
				}
				else if (!strcmp(argv[2],"step_mode_off"))
					load_ptr(0x52,00,  0x00 );
				else if (!strcmp(argv[2],"clear_sat")){
					val1=(dump_ptr(0x52)+1) &  DBG_SINGLE_STEP;
					load_ptr(0x52,00, val1| DBG_SATURATION_OCCURED); 
				}
			}
			
			print_dbg();
			break;
		
						
		
		
		case 'v':
			printf("%s\n",version);
			break;
		case 'h':
		default:
			printf("%s\n%s\n%s\n%s\n",version,usage,map,contact);
		
		}
		   
	}else
	printf("%s\n%s\n%s\n%s\n",version,usage,map,contact);
         
	
}
int main(int argc, char **argv)
{	
	
	
	
	if ((fd = open("/dev/dsp", O_WRONLY, 0)) == -1) {
		perror("open");
		exit(EXIT_FAILURE);
	}
	parse_args(argc,argv);

	
	close(fd);
	exit(0); 
}
