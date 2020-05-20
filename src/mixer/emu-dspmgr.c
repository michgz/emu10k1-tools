/*********************************************************************
 *     emu--dspmgr.c - text interface for the dsp program manager 
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

#include <fcntl.h>

#include <argp.h>
#include "include/dsp.h"

#include <stdlib.h>

#define MIXER_DEV_NAME "/dev/mixer"
#define DSP_DEV_NAME "/dev/dsp"

const char *argp_program_version = "emu-dspmgr 0.5";
const char *argp_program_bug_address = "<emu10k1-tools-devel@opensource.creative.com>";
const char doc[] = "emu-dspmgr - text interface for the dsp microcode patch manager for emu10k1 based sound cards";

const struct argp_option options[] = {

	{"add", 'a', "ROUTE", 0,
	 "adds a route"},
	
	{"control", 'c', "CONTROL", OPTION_ARG_OPTIONAL,
	 "prints control gprs/selects control gpr to be modified"},
	{"debug", 'd', 0, 0, "enables debug output"},
	{"dsp_device", 'D', "FILE", 0, "Use a device other than /dev/dsp"},
	{"filehead", 'f', "FILE", 0, "loads a dsp microcode program in front of other loaded patches"},
	{"filetail", 'F', "FILE", 0, "loads a dsp microcode program after other loaded patches"},
	{"inputs", 'i', 0, 0, "prints all the available inputs"},
	{"line", 'l', "LINE", 0,
	 "specifies the line to attatch the program"},
	{"mixer", 'm', "ELEMENT", OPTION_ARG_OPTIONAL,
	 "attaches specified control gpr to an OSS mixer element"},
	{"mixer_device", 'M', "FILE", 0, "Use a mixer other than /dev/mixer"},
	{"name", 'n', "NAME", 0, "changes name of the selected patch"},
	{"outputs", 'o', 0, 0, "prints all the available outputs"},
	{"patch", 'p', "PATCH", 0,
	 "selects patch to change name or control gpr"},
	{"query", 'q', 0 , 0, "Query driver's mixer interface version"},
	{"remove", 'r', "ROUTE", OPTION_ARG_OPTIONAL,
	 "prints existing routes/removes a route"},
	{"set", 's', "VALUE", 0,
	 "specifies a new value for the selected control gpr"},
	{"unload", 'u', "NAME", OPTION_ARG_OPTIONAL,
	 "lists loaded dsp programs/unloads a dsp microcode program"},
	{"volume", 'v', "ROUTE", 0,
	 "adds a route with volume control"},

	{"stop",'x', 0, 0,
	 "Stop the emu10k1 dsp processor"},
	{"start",'y', 0, 0,
	 "Start the emu10k1 dsp processor"},

	{"clear", 'z', 0, 0, "removes all routes and unloads all dsp microcode programs"},

	{0}
};

static error_t parser(int key, char *arg, struct argp_state *state)
{
	struct dsp_patch_manager *mgr = state->input;
	static char gpr[DSP_GPR_NAME_SIZE];
	static char patch[DSP_PATCH_NAME_SIZE];
	static char line[10];
	static int io=-1,input[DSP_NUM_INPUTS],output[DSP_NUM_OUTPUTS], num_in=0,num_out=0, placement=0;
	int ret;
	__s32 value;
	double fval;

	//As much sanity checking as possible should be done in here
	switch (key) {
	case 'a':
		if(dsp_add_route_name(mgr, arg))
			fprintf(stderr,"Error: Bad route specified\n");
		break;

	case 'c':
		if (arg == NULL){
			if(patch[0]=='\0')
				dsp_print_control_gpr_list(mgr);
			else
				if(dsp_print_control_gprs_patch(mgr, patch)<0)
					fprintf(stderr, "Error, bad patch name\n");;
			exit(0);
		}else {
			strncpy(gpr, arg, DSP_GPR_NAME_SIZE);
			gpr[DSP_GPR_NAME_SIZE - 1] = '\0';
		}
		break;

	case 'd':
		print_debug(mgr);
		exit(0);
		break;
	case 'D':
		break;
	case 'F':
		placement=1;
	case 'f':
		if( (num_in==0) && (num_out==0)){
			fprintf(stderr, "Error: You must specify a line to attach the patch to, use -l option\n");
			break;
		}
		
		if(dsp_read_patch(mgr, arg, input, output, num_in, num_out,io,patch, placement ))
			fprintf(stderr, "Error: Loading file %s\n",arg);
		
		placement=0;
		break;

	case 'i':
		dsp_print_inputs_name();
		break;
	case 'l':
		strncpy(line, arg, DSP_LINE_NAME_SIZE);
		line[DSP_LINE_NAME_SIZE - 1] = '\0';

		if      ( (ret = get_input_name(line)) >= 0){
			output[num_out]=input[num_in]=ret;
			if(io==0){	
				fprintf(stderr,"Error: Only input xor output lines should be specified\n");
				exit(-1);
			}
			io=1;
		}else if ( (ret = get_output_name(line)) >= 0 ){
			output[num_out]=input[num_in]=ret;
			if(io==1){	
				fprintf(stderr,"Error: Only input xor output lines should be specified\n");
				exit(-1);
			}
			io=0;
		}else if      ( (ret = get_stereo_input_name(line)) >= 0){
			output[num_out++]=input[num_in++]=ret++;
			output[num_out]=input[num_in]=ret;
			
			if(io==0){	
				fprintf(stderr,"Error: Only input xor output lines should be specified\n");
				exit(-1);
			}
			io=1;
		}else if ( (ret = get_stereo_output_name(line)) >= 0 ){
			output[num_out++]=input[num_in++]=ret++;
			output[num_out]=input[num_in]=ret;
			if(io==1){	
				fprintf(stderr,"Error: Only input xor output lines should be specified\n");
				exit(-1);
			}
			io=0;
		
		}else{
			fprintf(stderr,"Error:Bad Line name\n");
			exit(-1);
		}

		num_out++;
		num_in++;
		
		break;

	case 'm':
		if (arg == NULL)
			dsp_print_oss_mixers();
		else
			if((ret=dsp_set_oss_control_gpr(mgr, patch, gpr, arg))==-1 ){
				fprintf(stderr,"Error: Bad mixer contol\n");
				exit(-1);
			}else if(ret==-2){
				fprintf(stderr,"Error: bad patch or gpr name\n");
				exit(-1);
			}
		
		break;
	case 'M':
		break;
	
	case 'n':
		fprintf(stderr,"Function not yet implemented\n");
		dsp_set_patch_name(mgr, patch, arg);
		break;	
	case 'o':
		dsp_print_outputs_name();
		break;

	case 'p':
		strncpy(patch, arg, DSP_PATCH_NAME_SIZE);
		patch[DSP_PATCH_NAME_SIZE - 1] = '\0';
		break;
	case 'q':
		
		printf("%x\n",get_driver_version(mgr->mixer_fd));
		exit(0);
		break;
	case 'r':
		if (arg == NULL)
			dsp_print_route_list(mgr);
		else
			if((ret=dsp_del_route_name(mgr, arg))==-2)
				fprintf(stderr,"Error: Cannot remove route (check that no patches are attached)\n");
			else if(ret==-1)
				fprintf(stderr,"Error: Bad route name specified\n");
			else if(ret==-3)
				fprintf(stderr,"Error: Route is already non-active\n");
		break;

	case 's':

		if( arg[0] == '&' )  /* delay in seconds */
		{
			sscanf(arg+1, "%lf",&fval);
			value = (__s32)(fval*48000*0x800);
		}
		else if( arg[0] == '%' )  /* delay in samples */
		{
			sscanf(arg+1, "%d", &value);
			value *= 0x800;
		}
		else if( arg[0] == '#' )  /* fractional value */
		{
			sscanf(arg+1,"%lf",&fval);
			value = (__s32)(fval*0x7fffffff);
		}
		else   /* just a hex value */
			sscanf(arg, "%x", &value);

		if(dsp_set_control_gpr_value(mgr, patch, gpr, value))
			fprintf(stderr, "Error: Bad patch or gpr name\n");
		
		break;

	case 'u':
		if (arg == NULL)
			dsp_print_patch_list(mgr);
		else
			dsp_unload_patch(mgr, arg);

		break;
	case 'v':
		if(dsp_add_route_v_name(mgr, arg))
			fprintf(stderr,"Error: Bad route specified\n");
		strcpy(patch,"Routing");
		sprintf(gpr, "Vol %s",arg);
		break;
	case 'x':
		dsp_stop_proc(mgr);
		break;
	case 'y':
		dsp_start_proc(mgr);
		break;	
	case 'z':
		dsp_unload_all(mgr);
		break;

	default:
		return ARGP_ERR_UNKNOWN;

	}
	return 0;
}

static struct argp argp = { options, parser, 0, doc, };

int open_dev(struct dsp_patch_manager *mgr, char *dsp_name, char *mixer_name )
{
	if ( ( mgr->mixer_fd = open(mixer_name, O_RDWR, 0)) < 0) {
		perror(mixer_name);
		return -1;
	}
	
	if ((mgr->dsp_fd = open(dsp_name, O_RDWR, 0)) < 0) {
		perror(dsp_name);
		return -1;
	}
	return 0;
}

int main(int argc, char **argv)
{
	struct dsp_patch_manager mgr;
	int ret,i;
	char dsp_name[64]=DSP_DEV_NAME, mixer_name[64]=MIXER_DEV_NAME;

	mgr.init = 0;
	mgr.debug = 0;

	//We handle any -M and -D options first, regardless of were they are

	for(i=0;i<argc;i++){
		if(strcmp(argv[i],"-M")==0 ||strcmp(argv[i],"--mixer")==0  ){
			if((argc>(i+1)) && (argv[i+1][0]!='-')){
				strncpy(mixer_name, argv[i+1], 64);
				mixer_name[63]=0;
			}
			i++;

		}
		else if(strcmp(argv[i],"-D")==0 ||strcmp(argv[i],"--dsp")==0  ){
			if((argc>(i+1)) && (argv[i+1][0]!='-')){
				strncpy(dsp_name, argv[i+1], 64);
				dsp_name[63]=0;
			}
			i++;

		}
		
	}

	if( (ret=open_dev(&mgr, dsp_name, mixer_name)) )
		exit (ret);
	
	if( ( ret=dsp_init(&mgr) ) <0 )
		exit(ret); 

	argp_parse(&argp, argc, argv, 0, 0, &mgr);

	ret = dsp_load(&mgr);
	if (ret < 0)
		return ret;

	return 0;
}
