/*********************************************************************
 *     cmd_line.c - a command line interface for the mixer library
 * 
 *      Copyright (C) 2000 Rui Sousa 
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

#include <argp.h>
#include <stdlib.h>
#include <stdio.h>
#include "include/mix.h"
#include <string.h>
const char *argp_program_version = "mix 0.1";
const char *argp_program_bug_address = "<emu10k1-tools-devel@opensource.creative.com>";
const char doc[] = "mix - a program to control special emu10k1 mixer settings";

const struct argp_option options[] = {
	{"analog",'a', 0, 0,
	 "Set output in analog mode (5.1 cards only)"},
	{"bus", 'b', "BUS", OPTION_ARG_OPTIONAL,
	 "sets/prints the FX recording bus/es"},
	{"boost",'B',"on/off",0,
	 "Turn on or off a 12dB boost on analog front output (headphone mode)"},
	{"digital",'d', 0, 0,
	 "Set output in digital mode (5.1 cards only)"},
	{"spdifrate", 'f', "FREQ", 0,
	 "sets the output frequency in kHz of the S/PDIF port (use 44, 48 or 96)"},
	{"ir",'i', 0, 0,
	 "turn on IR remote control and midi port on the Live Drive"},
	{"info",'I', 0,0,"print card indo"},
	{"inputfx", 'm', "FXNUM", 0, 
	 "sets multichannel input fx"},
	{"mixer", 'M', "DEVICE", 0,
	 "use an alternate mixer device to /dev/mixer"},	
	{"print", 'p', 0, 0, "prints mixer settings"},
	{"reset", 'r', "BUS", OPTION_ARG_OPTIONAL,
	 "resets/prints the FX recording bus/es"},
	{"source", 's', "SOURCE", OPTION_ARG_OPTIONAL,
	 "sets/prints the recording source"},
	{"passthrough", 't', "ACTIVE", 0,
	 "sets digital pass-through mode and selects which S/PDIF ports to use"},
	{0}	
};

static error_t parser(int key, char *arg, struct argp_state *state)
{
	struct mix_settings *set = state->input;

	switch (key) {

	case 'a':
		set_51(set,0);
		break;
	case 'b':
		if (set->recsrc != mix_rec_source_number("FX"))
			argp_failure(state, EXIT_FAILURE, 0, "First select FX as the recording source");


		{
			int bus;
			if (arg) {
				sscanf(arg, "%d\n", &bus);
				if (mix_set_rec_fx_bus(set, bus) < 0)
					argp_failure(state, EXIT_FAILURE, 0, "Invalid recording FX bus");
			} else {
				printf("FX bus[es] selected for recording:");
				for (bus = 0; bus < 16; bus++)
					if (mix_check_rec_fx_bus(set, bus))
						printf(" 0x%1x", bus);
				printf("\n");
			}
		}
		break;
	case 'B':
		if(strncasecmp("on",arg, 2)==0)
			mix_ac97_boost(set,1);
		else
			mix_ac97_boost(set,0);
		break;
	case 'd':
		set_51(set,1);
		break;
	case 'f':
		mix_set_spdif_freq(set,atoi(arg));
		break;
	case 'i':
		enable_ir(set);
		break;
	case 'm':
		set_mch_fx(set, atoi(arg));
		break;
	case 'M':
		if(set->dev_name[0]!=0)
			break;
		strncpy(set->dev_name, arg, 64);
		set->dev_name[63]=0;
		break;
	case 't':
		set_passthrough(set, atoi(arg));
		break;
	case 'p':
		mix_print_settings(set);
		break;

	case 'r':
		if (set->recsrc != mix_rec_source_number("FX"))
			argp_failure(state, EXIT_FAILURE, 0, "First select FX as the recording source");


		{
			int bus;
			if (arg) {
				sscanf(arg, "%d\n", &bus);
				if (mix_reset_rec_fx_bus(set, bus) < 0)
					argp_failure(state, EXIT_FAILURE, 0, "Invalid recording FX bus");
			} else {
				printf("FX bus[es] not selected for recording:");
				for (bus = 0; bus < 0x10; bus++)
					if (!mix_check_rec_fx_bus(set, bus))
						printf(" 0x%1x", bus);
				printf("\n");
			}
		}
		break;

	case 's':
		if (arg) {
			if (mix_set_rec_source(set, arg) < 0)
				argp_failure(state, EXIT_FAILURE, 0, "Invalid recording source");
		} else {
			printf("Recording Source: %s\n", mix_get_rec_source(set));
		}
		break;
	case 'I':
		mix_print_info(set);
		break;
	default:
		return ARGP_ERR_UNKNOWN;
	}

	return 0;
}

static struct argp argp = { options, parser, 0, doc, };

int main(int argc, char **argv)
{
	struct mix_settings set;
	int i;
	int ret;
	//We handle any -M option first, regardless of were it is
	set.dev_name[0]=0;
	for(i=0;i<argc;i++){
		if(strcmp(argv[i],"-M")==0 ||strcmp(argv[i],"--mixer")==0  ){
			if((argc>(i+1)) && (argv[i+1][0]!='-')){
				strncpy(set.dev_name, argv[i+1], 64);
				set.dev_name[63]=0;
			}
			goto match;
		}
	}

match:
	ret=mix_init(&set);
	if(ret)
		return ret;

	ret=argp_parse(&argp, argc, argv, 0, 0, &set);
	if(ret)
		return ret;
	
	ret=mix_commit(&set);
	
	return ret;
}
