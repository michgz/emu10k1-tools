#ifndef DBGEMU_H
#define DBGEMU_H


#define DBG			0x52		/* DO NOT PROGRAM THIS REGISTER!!! MAY DESTROY CHIP */
#define DBG_ZC                  0x80000000      /* zero tram counter */
#define DBG_SATURATION_OCCURED  0x02000000      /* saturation control */
#define DBG_SATURATION_ADDR     0x01ff0000      /* saturation address */
#define DBG_SINGLE_STEP         0x00008000      /* single step mode */
#define DBG_STEP                0x00004000      /* start single step */
#define DBG_CONDITION_CODE      0x00003e00      /* condition code */
#define DBG_SINGLE_STEP_ADDR    0x000001ff      /* single step address */

void dump_reg(long int offset,long int length);

struct mixer_private_ioctl {
        __u32 cmd;
        __u32 val[90];
};


#define OUTPUT_BASE 0x20
#define CONSTANTS_BASE 0x40
#define GPR_BASE 0x100
#define TRAML_IDATA_BASE 0x200
#define TRAML_EDATA_BASE 0x280
#define TRAML_IADDR_BASE 0x300
#define TRAML_EADDR_BASE 0x380
#define DSP_CODE_BASE 0x400

#define CONSTANTS_SIZE 0x16
#define GPR_SIZE 0x100
#define TRAML_ISIZE 0x80
#define TRAML_ESIZE 0x20
#define TRAMB_ISIZE 0x2000
#define TRAMB_ESIZE 0x200000
#define DSP_CODE_SIZE 0x400

#define HW_REG_SIZE 6
#define HW_REG_BASE 0x057



#define CMD_READ 1
#define CMD_WRITE 2

//SOUND_MIXER_PRIVATE3:
/* bogus ioctls numbers to escape from OSS mixer limitations */
#define CMD_WRITEFN0            _IOW('D', 0, struct mixer_private_ioctl)
#define CMD_READFN0		_IOR('D', 1, struct mixer_private_ioctl)
#define CMD_WRITEPTR		_IOW('D', 2, struct mixer_private_ioctl) 
#define CMD_READPTR		_IOR('D', 3, struct mixer_private_ioctl) 
#define CMD_SETRECSRC		_IOW('D', 4, struct mixer_private_ioctl) 
#define CMD_GETRECSRC		_IOR('D', 5, struct mixer_private_ioctl) 
#define CMD_GETVOICEPARAM	_IOR('D', 6, struct mixer_private_ioctl) 
#define CMD_SETVOICEPARAM	_IOW('D', 7, struct mixer_private_ioctl) 
#define CMD_GETPATCH		_IOR('D', 8, struct mixer_private_ioctl) 
#define CMD_GETGPR		_IOR('D', 9, struct mixer_private_ioctl) 
#define CMD_GETCTLGPR           _IOR('D', 10, struct mixer_private_ioctl)
#define CMD_SETPATCH		_IOW('D', 11, struct mixer_private_ioctl) 
#define CMD_SETGPR		_IOW('D', 12, struct mixer_private_ioctl) 
#define CMD_SETCTLGPR		_IOW('D', 13, struct mixer_private_ioctl)


char pci_desc[][64]={
	"PTR         - Pointer access register",//0
	"DATA        - ptr access data",//4
	"IPR         - Interrupt pending register",//8
	"INTE        - Interupt enable register",//c
	"WC          - Wall clock register",//10
	"HCFG        - hardware config register",//14
	"MUDATA      - MPU401 data register (8 bits)",//18
	"MUCMD       - MPU401 command register(8 bits)",//19
	"TIMER       - Terminal count register",//1a-1b
	"AC97DATA    - AC97 data register (16 bit)",//1c-1d
	"AC97ADDRESS - AC97 address register (8bit)",//1e
	"Unknow      - (8bits)"//1f
};


char op_codes[16][16]={
	"MACS",
	"MACS1",
	"MACW",
	"MACW1",
	"MACINTS",
	"MACINTW",
	"ACC3",
	"MACMV",
	"ANDXOR",
	"TSTNEG",
	"LIMIT",
	"LIMIT1",
	"LOG",
	"EXP",
	"INTERP",
	"SKIP",
};

char *op_description[16]={
	"R = A + (X * Y >> 31) ; overflow: saturation",
	"R = A + (-X * Y >> 31); overflow: saturation",
	"R = A + (X * Y >> 31) ; overflow: wraparound",
	"R = A + (-X * Y >> 31); overflow: wraparound",
	"R = A + X * Y         ; overflow: saturation",
	"R = A + X * Y         ; overflow: wraparound (31-bit)",
	"R = A + X + Y         ; overflow: saturation",
	"R = A, acc += X * Y   ; overflow: ?,   67 bit accumulation",
	"R = (A & X) ^ Y",
	"R = (A >= Y) ? X : ~X",
	"LIMIT R = (A >= Y) ? X : Y",
	"LIMIT1 R = (A < Y) ? X : Y",
	"R=log(A), X controls max exponent(2-31), Y controls sign",
	"R=log(A), X controls max exponent(2-31), Y controls sign",
	"INTERP R = A + (X * (Y - A) >> 31); overflow: saturation",
	"SKIP R->GPR to store CCR, A -> CCR to use, X-> skip test equation,
 Y-> gpr containint number of instructions to skip"  
};


int fd,fd2;
char map[]="memory map:

GRPS:                  0x100-0x1FF
ITRAM Data Buffer:     0x200-0x27F
XTRAM Data Buffer:     0x280-0x29F
ITRAM Address Buffer:  0x300-0x37F
XTRAM Address Buffer:  0x380-0x39F
Program Memory:        0x400-0x600
";

char usage[]="
dbgemu [options]

All values in hex.

   -d  dumps registers, sub-options:

        -a <address>     
            dumps contents of the AC97 register at the given address

        -r <address>
            dumps contents of the DSP register at the given address.
            valid ranges of register:
              CONFIG REGS            0x000-0x063
              GRPS:                  0x100-0x1FF
              ITRAM Data Buffer:     0x200-0x27F
              XTRAM Data Buffer:     0x280-0x29F
              ITRAM Address Buffer:  0x300-0x37F
              XTRAM Address Buffer:  0x380-0x39F
              Program Memory:        0x400-0x600
        -f <address>
            dumps \"function0\" register space(includes the HCFG register)
        

   -l  loads values into register, sub-options:

        -a <address> <value>
             load value into given ac97 register

        -c <address> <opcode> <R> <A> 
             load formated dsp code
        
        -r <address> <value>
             load value into given dsp register
                
        -f <address> <value>
             load value into given fn0 register

   -D  [commands]
        If no command is specified, prints current dsp debug state.
       Debug mode commands:

        step_mode
             turns on step mode

        step [instruction #]
             executes given instruction, if no instruction is specified,
             esecutes next instruction

        clear_sat
             clears the \"saturation occured\" bit.

        step_mode_off
             resumes normal dsp operation.

   -h     Prints this help message.
   -v     Prints version number.

---

Examples:

To display 10 GPRs starting from 0x100:

dbgemu -d r  100 10

To load a value of 1 to 0x100:

dbgemu -l r 100 1

To change an instruction:

dbgemu -l c 400 macs 10 30 100 20


---
";
int foo=0;
char version[]="dbgemu, version 0.3";

char contact[]="Author: Daniel Bertrand <d.bertrand@ieee.org>";

char config_desc[][64]={
	"CPF",//0x00
	"PTRX",
	"CVCF",
	"VTCF",
	"Z2",
	"Z1",
	"PSST",
	"DSL",
	"CCCA",//0x08
	"CCR",
	"CLP",
	"FXRT",
	"MAPA",//0x0c
	"MAPB",
	"Unknown",
	"Unknown",
	"ENVVOL",//0x10
	"ATKHLDV",
	"DCYSUSV",
	"LFOVAL1",
	"ENVVAL",
	"ATKHLDM",
	"DCYSUSM",
	"LFOVAL2",
	"IP",//0x18
	"IFATN",
	"PEFE",
	"FMMOD",
	"TREMFRQ",
	"FM2FRQ2",
	" TEMPENV",
	"Unknown",//0x1f
	"CD0",//0x20
	"CD1",
	"CD2",
	"CD3",
	"CD4",
	"CD5",
	"CD6",
	"CD7",
	"CD8",
	"CD9",
	"CDA",
	"CDB",
	"CDC",
	"CDD",
	"CDE",
	"CDF",
	"Unknown",//0x30
	"Unknown",//0x31
	"Unknown",//0x32
	"Unknown",//0x33
	"Unknown",//0x34
	"Unknown",//0x35
	"Unknown",//0x36
	"Unknown",//0x37
	"Unknown",//0x38
	"Unknown",//0x39
	"Unknown",//0x3A
	"Unknown",//0x3B
	"Unknown",//0x3C
	"Unknown",//0x3D
	"Unknown",//0x3E
	"Unknown",//0x3F
	"PTB",//0X40
	"TCB",
	"ADCCR",
	"FXWC",
	"TCBS",
	"MICBA",
	"ADCBA",
	"FXBA",
	"unknown",//0x48
	"MICBS",
	"ADCBS",
	"FXBS",
	"unknown",//0x4C
	"unknown",//0x4D
	"unknown",//0x4E
	"unknown",//0x4F
	"CDCS",//0x50
	"GPSCS",
	"DBG   -- may blow up the planet if written to",
	"REG53 -- may cause the universe to implode on itself",
	"SPCS0",
	"SPCS1",
	"SPCS2",
	"Unknown",
	"CLIEL",//0x58
	"CLIEH",
	"CLIPL",
	"CLIPH",
	"SOLEL",
	"SOLEH",
	"SPBYPASS",
	"Unknown",
	"CDSRCS",//0x60
	"GPSRCS",
	"ZVSRCS",
	"MICIDX",
	"ADCIDX",
	"FXIDX",//0x65
	"Unknown",
};
__s32 constants[CONSTANTS_SIZE] = {
	0x0,
	0x1,
	0x2,
	0x3,
	0x4,
	0x8,
	0x10,
	0x20,
	0x100,
	0x10000,
	0x80000,
	0x10000000,
	0x20000000,
	0x40000000,
	0x80000000,
	0x7fffffff,
	0xffffffff,
	0xfffffffe,
	0xc0000000,
	0x4f1bbcdc,
	0x5a7ef9db,
	0x00100000
};

char *dsp_in_name[32] = {"Pcm L", "Pcm R", "Pcm1 L", "Pcm1 R", "fx4", "fx5", "fx6", "fx7", "fx8",
	"fx9", "fx10", "fx11", "fx12", "fx13", "fx14", "fx15",
	"Analog L", "Analog R", "CD-Spdif L", "CD-Spdif R", "in2l", "in2r", "Opt. Spdif L", "Opt. Spdif R",
	"RCA Aux L", "RCA Aux R", "RCA Spdif L", "RCA Spdif R", "Line2/Mic2 L", "Line2/Mic2 R", "in7l", "in7r"
};

char *dsp_out_name[32] =
    { "Front L", "Front R", "Digital L", "Digital R", "out2l", "out2r", "Phones L", "Phones R",
	"Rear L", "Rear R", "ADC Rec L", "ADC Rec R", "Mic Rec", "out6r", "out7l", "out7r",
	"out8l", "out8r", "out9l", "out9r", "out10l", "out10r", "out11l", "out11r",
	"out12l", "out12r", "out13l", "out13r", "out14l", "out14r", "out15l", "out15r"
};


char *dsp_hw_name[HW_REG_SIZE]={
	"Accumulator",
	"Condition Code Register",
	"Noise Source 1",
	"Noise Source 2",
	"Interrupt Reg",
	"Delay Base Address Counter"
};


		

#endif
