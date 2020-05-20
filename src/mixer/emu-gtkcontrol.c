#include <stdio.h>
#include <math.h>
#include <gtk/gtk.h>
#include "include/dsp.h"
#include "include/mix.h"

int WindowWidth = 640;
int WindowHeight = 480;
int LeftPaneHeight = 480;
int RightPaneHeight = 480;

enum {
	DSP_ROUTES,
	DSP_ROUTES_VOLUME,
	MONO_VOICE,
	STEREO_VOICE,
	REC_SETTINGS,
	HARDWARE
};

static struct mix_settings mix_set;
static struct dsp_patch_manager dsp_mgr;

#define NUM_CARDS 5
#define NUM_DRIVES 4

/*FIXME: can we get card/drive model info from driver?*/
static int card_selected=0;//default card model
static int drive_selected=0;//default live drive

const struct io_config cards[NUM_CARDS]={
      //out select ,# of out,in select, # in, "name"
	
	{0x00001F03 , 7   , 0x00030000 , 2  , "SBLive - Value (Original)"},
	{0x00001F0F , 10  , 0x00030000 , 2  , "SBLive - Value (w/digital out)"},
	{0x00001F0F , 10  , 0x000F0000 , 4  , "SBLive - Platnium"},
	{0x00061F3F , 13  , 0x000F0000 , 4   , "SBLive - 5.1"},		
	{0xFFFFFFFF , 32  , 0xFFFFFFFF , 32 , "SBLive - Secret Loony Dbg Model"}
};
const struct io_config live_drives[NUM_DRIVES]={
	{0x00000000 , 0   , 0x00000000 , 0 , "No Live Drive"},
	{0x000000C0 , 2   , 0x3FC00000 , 8 , "Live Drive I"},
	{0x000000C0 , 2   , 0x3FC00000 , 8 , "Live Drive II"},
	{0x000000C0 , 2   , 0x3FC00000 , 8 , "Live Drive IR"}
};
/* FIXME: make enable method for fxins and extra fxrec buses (0x30-0x3f) */
static struct io_config fx_cfg=
	{0x00000000, 0 , 0x00000033 , 4 ,"Fx buses config"};

static struct io_config io_cfg;


static gchar *voice_send[] = { "Send A", "Send B", "Send C", "Send D" };

static GtkWidget *notebook;

static GtkWidget *create_dsp_routes(void);


static GtkWidget *create_scrolled_window(GtkWidget * widget)
{
	GtkWidget *scrolled_win;

	scrolled_win = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_win), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_widget_show(scrolled_win);

	gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(scrolled_win), widget);

	return scrolled_win;
}

/* FIXME: Implement this.
static GtkWidget *create_control_slider(char *name, int addr)
{
	GtkWidget *frame;

	return frame;
}
*/

/* FIXME: Implement this.
static GtkWidget *create_controls()
{
	GtkWidget *scrolled_win;
	GtkWidget *vbox;

	vbox = gtk_vbox_new(FALSE, 1);
	gtk_widget_show(vbox);
	scrolled_win = create_scrolled_window(vbox);

	return scrolled_win;
}
*/

static void route_enable_callback(GtkWidget * widget, int route)
{
	int in, out;

	in = route & 0xff;
	out = (route >> 8) & 0xff;

	if (GTK_TOGGLE_BUTTON(widget)->active) {
		if (!dsp_check_route(&dsp_mgr, in, out)) {
			dsp_add_route(&dsp_mgr, in, out);
			dsp_load(&dsp_mgr);
		}
	} else {
		if (dsp_check_route(&dsp_mgr, in, out)) {
			if (dsp_del_route(&dsp_mgr, in, out) < 0)
				GTK_TOGGLE_BUTTON(widget)->active = 1;
			else
				dsp_load(&dsp_mgr);
		}
	}
}


static GtkWidget *create_routing_table(int in, int out)
{
	GtkWidget *frame;
	GtkWidget *vbox;
	GtkWidget *table;
	GtkWidget *label;
	GtkWidget *button;
	int i, j;

	frame = gtk_frame_new(NULL);
	gtk_widget_show(frame);
	gtk_frame_set_label(GTK_FRAME(frame), "Routing Table");

	vbox = gtk_vbox_new(FALSE, 5);
	gtk_widget_show(vbox);
	gtk_container_add(GTK_CONTAINER(frame), vbox);

	table = gtk_table_new(in + 1, out + 1, TRUE);
	gtk_widget_show(table);
	gtk_box_pack_start(GTK_BOX(vbox), table, FALSE, FALSE, 5);

	in = 1;
	for (i = 0; i < DSP_NUM_INPUTS; i++)
		if (test_bit(i, &(io_cfg.input) )){
			label = gtk_label_new(dsp_in_name[i]);
			gtk_table_attach(GTK_TABLE(table), label, 0, 1, in, in + 1, 0, 0, 4, 4);
			gtk_widget_show(label);
			in++;
		}

	out = 1;
	for (i = 0; i < DSP_NUM_OUTPUTS; i++)
		if ( test_bit(i, &(io_cfg.output)) ) {
			label = gtk_label_new(dsp_out_name[i]);
			gtk_table_attach(GTK_TABLE(table), label, out, out + 1, 0, 1, 0, 0, 4, 4);
			gtk_widget_show(label);
			out++;
		}

	in = 1;
	for (i = 0; i < DSP_NUM_INPUTS; i++)
		if (test_bit(i, &(io_cfg.input))) {
			out = 1;
			for (j = 0; j < DSP_NUM_OUTPUTS; j++)
				if (test_bit(j, &(io_cfg.output))) {
					button = gtk_toggle_button_new();
					gtk_table_attach(GTK_TABLE(table), button, out, out + 1, in, in + 1, GTK_FILL,
							 GTK_FILL, 20, 6);

					gtk_widget_show(button);

					if (dsp_check_route(&dsp_mgr, i, j))
						gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), TRUE);

					gtk_signal_connect(GTK_OBJECT(button), "toggled",
							   GTK_SIGNAL_FUNC(route_enable_callback),
							   GINT_TO_POINTER(i | (j << 8)));


					out++;
				}
			in++;
		}

	return frame;
}


static GtkWidget *create_dsp_routes()
{
	GtkWidget *scrolled_win;
	GtkWidget *vbox;

	vbox = gtk_vbox_new(FALSE, 1);
	
	scrolled_win = create_scrolled_window(vbox);
	

	/* Matrix routing */
	gtk_box_pack_start(GTK_BOX(vbox), create_routing_table(io_cfg.num_in,io_cfg.num_out), TRUE, FALSE, 0);
	gtk_widget_show(vbox);

	return scrolled_win;
}

static void route_volume_callback(GtkAdjustment * adj)
{
	int in, out;
	char control_name[DSP_GPR_NAME_SIZE];

	in = (int) gtk_object_get_data(GTK_OBJECT(adj), "in");
	out = (int) gtk_object_get_data(GTK_OBJECT(adj), "out");

	sprintf(control_name, "vol %s:%s", dsp_in_name[in], dsp_out_name[out]);

	if (adj->value != 0x7fffffff) {
		if (!dsp_check_route_volume(&dsp_mgr, in, out)) {
			dsp_add_route_v(&dsp_mgr, in, out);
			dsp_load(&dsp_mgr);
		}
		dsp_set_control_gpr_value(&dsp_mgr, "Routing", control_name, adj->value);
	} else {
		if (dsp_check_route_volume(&dsp_mgr, in, out)) {
			dsp_add_route(&dsp_mgr, in, out);
			dsp_load(&dsp_mgr);
		}
	}
}

/* FIXME invert sliders, use logarithmic scale */
static GtkWidget *create_dsp_routes_volume()
{
	GtkWidget *scrolled_win;
	GtkWidget *frame;
	GtkWidget *vbox;
	GtkWidget *hbox;
	GtkWidget *scale;
	GtkObject *adj;
	int count;
	int i, j;
	__s32 vol, min, max;
	char control_name[DSP_GPR_NAME_SIZE];
	struct dsp_gpr control_gpr;

	vbox = gtk_vbox_new(FALSE, 1);
	gtk_widget_show(vbox);
	scrolled_win = create_scrolled_window(vbox);

	hbox = gtk_hbox_new(FALSE, 1);
	gtk_widget_show(hbox);

	count = 4;
	for (i = 0; i < DSP_NUM_INPUTS; i++)
		if (test_bit(i, &(io_cfg.input) )) {
			for (j = 0; j < DSP_NUM_OUTPUTS; j++)
				if (dsp_check_route(&dsp_mgr, i, j)) {

					count++;

					if (count == 5) {
						hbox = gtk_hbox_new(FALSE, 1);
						gtk_widget_show(hbox);
						gtk_box_pack_start(GTK_BOX(vbox), hbox, TRUE, FALSE, 0);
						count = 0;
					}

					sprintf(control_name, "%s:%s", dsp_in_name[i], dsp_out_name[j]);

					frame = gtk_frame_new(NULL);
					gtk_widget_show(frame);
					gtk_frame_set_label(GTK_FRAME(frame), control_name);
					gtk_box_pack_start(GTK_BOX(hbox), frame, TRUE, FALSE, 0);

					if (dsp_check_route_volume(&dsp_mgr, i, j)) {
						sprintf(control_name, "vol %s:%s", dsp_in_name[i], dsp_out_name[j]);
						get_control_gpr_info(&dsp_mgr, "Routing", control_name, &control_gpr);
						vol = control_gpr.value;
						min = control_gpr.min_val;
						max = control_gpr.max_val;
					} else {
						vol = 0x7fffffff;
						min = 0;
						max = 0x7fffffff;
					}

					adj = gtk_adjustment_new(vol, min, max, 1, 5, 0);

					gtk_object_set_data(GTK_OBJECT(adj), "in", GINT_TO_POINTER(i));
					gtk_object_set_data(GTK_OBJECT(adj), "out", GINT_TO_POINTER(j));

					gtk_signal_connect(GTK_OBJECT(adj), "value_changed",
							   GTK_SIGNAL_FUNC(route_volume_callback), NULL);

					scale = gtk_vscale_new(GTK_ADJUSTMENT(adj));
					gtk_scale_set_draw_value(GTK_SCALE(scale), FALSE);
					gtk_range_set_update_policy(GTK_RANGE(scale), GTK_UPDATE_DELAYED);

					gtk_widget_show(scale);
					gtk_container_add(GTK_CONTAINER(frame), scale);
				}
		}

	return scrolled_win;
}

static void voice_set_send_amount_callback(GtkAdjustment * adj, unsigned int send)
{
	int type;

	type = (int) gtk_object_get_data(GTK_OBJECT(adj), "type");

	mix_set_voice_send_amount(&mix_set, type, send, adj->value);
	mix_hw_voice_settings_set(&mix_set);
}

static void voice_set_send_callback(GtkWidget * widget, unsigned int bus)
{
	int type;
	unsigned int send;

	type = (int) gtk_object_get_data(GTK_OBJECT(widget), "type");
	send = (unsigned int) gtk_object_get_data(GTK_OBJECT(widget), "send");

	if (GTK_TOGGLE_BUTTON(widget)->active) {
		mix_set_voice_send_bus(&mix_set, type, send, bus);
		mix_hw_voice_settings_set(&mix_set);
	}
}

static GtkWidget *create_voice_send_amount(unsigned int type)
{
	GtkWidget *frame;
	GtkWidget *vbox, *vbox1;
	GtkWidget *label;
	GtkWidget *scale;
	GtkObject *adj;
	int i;

	frame = gtk_frame_new(NULL);
	gtk_widget_show(frame);
	gtk_frame_set_label(GTK_FRAME(frame), "Voice Send Amount");

	vbox = gtk_vbox_new(TRUE, 5);
	gtk_widget_show(vbox);
	gtk_container_add(GTK_CONTAINER(frame), vbox);

	for (i = 0; i < 4; i++) {
		vbox1 = gtk_vbox_new(TRUE, 5);
		gtk_widget_show(vbox1);
		gtk_box_pack_start(GTK_BOX(vbox), vbox1, FALSE, FALSE, 10);

		label = gtk_label_new(voice_send[i]);
		gtk_box_pack_start(GTK_BOX(vbox1), label, FALSE, FALSE, 2);
		gtk_widget_show(label);

		adj = gtk_adjustment_new(mix_get_voice_send_amount(&mix_set, type, i), 0, 0xff, 1, 5, 0);

		gtk_object_set_data(GTK_OBJECT(adj), "type", GINT_TO_POINTER(type));

		gtk_signal_connect(GTK_OBJECT(adj), "value_changed",
				   GTK_SIGNAL_FUNC(voice_set_send_amount_callback), GINT_TO_POINTER(i));

		scale = gtk_hscale_new(GTK_ADJUSTMENT(adj));
		gtk_scale_set_draw_value(GTK_SCALE(scale), FALSE);
		gtk_box_pack_start(GTK_BOX(vbox1), scale, FALSE, FALSE, 2);

		gtk_range_set_update_policy(GTK_RANGE(scale), GTK_UPDATE_DELAYED);

		gtk_widget_show(scale);
	}

	return frame;
}

static GtkWidget *create_voice_routing(unsigned int type)
{
	GtkWidget *frame;
	GtkWidget *vbox;
	GtkWidget *table;
	GtkWidget *label;
	GtkWidget *button;
	GSList *group;

	static gchar bus[3];
	int i, j;

	frame = gtk_frame_new(NULL);
	gtk_widget_show(frame);
	gtk_frame_set_label(GTK_FRAME(frame), "Voice Send Routing");

	vbox = gtk_vbox_new(TRUE, 5);
	gtk_widget_show(vbox);
	gtk_container_add(GTK_CONTAINER(frame), vbox);

	table = gtk_table_new(6, 17, FALSE);
	gtk_widget_show(table);
	gtk_box_pack_start(GTK_BOX(vbox), table, FALSE, FALSE, 5);

	label = gtk_label_new("FX bus");
	gtk_table_attach(GTK_TABLE(table), label, 1, 17, 0, 1, 0, 0, 2, 2);
	gtk_widget_show(label);

	for (i = 0; i < 0x10; i++) {
		sprintf(bus, "%d", i);
		label = gtk_label_new(bus);
		gtk_table_attach(GTK_TABLE(table), label, i + 1, i + 2, 1, 2, 0, 0, 2, 2);
		gtk_widget_show(label);
	}

	for (i = 0; i < 4; i++) {
		label = gtk_label_new(voice_send[i]);
		gtk_table_attach(GTK_TABLE(table), label, 0, 1, i + 2, i + 3, 0, 0, 2, 2);
		gtk_widget_show(label);

		button = gtk_radio_button_new(NULL);

		if (mix_check_voice_send_bus(&mix_set, type, i, 0))
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), TRUE);

		gtk_table_attach(GTK_TABLE(table), button, 1, 2, i + 2, i + 3, 0, 0, 2, 2);
		gtk_signal_connect(GTK_OBJECT(button), "toggled",
				   GTK_SIGNAL_FUNC(voice_set_send_callback), GINT_TO_POINTER(0));
		gtk_object_set_data(GTK_OBJECT(button), "type", GINT_TO_POINTER(type));
		gtk_object_set_data(GTK_OBJECT(button), "send", GINT_TO_POINTER(i));
		gtk_widget_show(button);

		for (j = 1; j < 0x10; j++) {
			group = gtk_radio_button_group(GTK_RADIO_BUTTON(button));
			button = gtk_radio_button_new(group);

			if (mix_check_voice_send_bus(&mix_set, type, i, j))
				gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), TRUE);

			gtk_table_attach(GTK_TABLE(table), button, j + 1, j + 2, i + 2, i + 3, 0, 0, 2, 2);
			gtk_signal_connect(GTK_OBJECT(button), "toggled",
					   GTK_SIGNAL_FUNC(voice_set_send_callback), GINT_TO_POINTER(j));
			gtk_object_set_data(GTK_OBJECT(button), "type", GINT_TO_POINTER(type));
			gtk_object_set_data(GTK_OBJECT(button), "send", GINT_TO_POINTER(i));
			gtk_widget_show(button);
		}
	}

	return frame;
}

static GtkWidget *create_voice_settings(int type)
{
	GtkWidget *scrolled_win;
	GtkWidget *vbox;

	vbox = gtk_vbox_new(TRUE, 5);
	gtk_widget_show(vbox);

	scrolled_win = create_scrolled_window(vbox);

	gtk_box_pack_start(GTK_BOX(vbox), create_voice_routing(type), TRUE, FALSE, 5);
	gtk_box_pack_start(GTK_BOX(vbox), create_voice_send_amount(type), TRUE, FALSE, 5);

	return scrolled_win;
}

static void rec_fx_buffer_callback(GtkWidget * widget, unsigned int data)
{
	if (GTK_TOGGLE_BUTTON(widget)->active)
		mix_set_rec_fx_bus(&mix_set, data);
	else
		mix_reset_rec_fx_bus(&mix_set, data);

	mix_hw_recording_set(&mix_set);
}

static void rec_buffer_callback(GtkWidget * widget, gchar * data)
{
	if (GTK_TOGGLE_BUTTON(widget)->active) {
		mix_set_rec_source(&mix_set, data);
		mix_hw_recording_set(&mix_set);
	}
}


static GtkWidget *create_rec_settings()
{
	GtkWidget *scrolled_win;
	GtkWidget *frame;
	GtkWidget *hbox, *vbox;
	GtkWidget *vbbox;
	GtkWidget *button;
	static GtkWidget *fx_button[0x10];
	GSList *group;
	int i;

	vbox = gtk_vbox_new(TRUE, 5);
	gtk_widget_show(vbox);
	scrolled_win = create_scrolled_window(vbox);

	hbox = gtk_hbox_new(FALSE, 5);
	gtk_widget_show(hbox);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 5);

	/* Recording Buffer */
	frame = gtk_frame_new(NULL);
	gtk_widget_show(frame);
	gtk_frame_set_label(GTK_FRAME(frame), "Recording Buffer");
	gtk_box_pack_start(GTK_BOX(hbox), frame, TRUE, FALSE, 5);

	vbbox = gtk_vbutton_box_new();
	gtk_widget_show(vbbox);
	gtk_container_add(GTK_CONTAINER(frame), vbbox);

	button = gtk_radio_button_new_with_label(NULL, mix_recsrc[0]);
	gtk_signal_connect(GTK_OBJECT(button), "toggled",
			   GTK_SIGNAL_FUNC(rec_buffer_callback), (gpointer) mix_recsrc[0]);

	if (mix_check_rec_source(&mix_set, mix_recsrc[0]))
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), TRUE);

	gtk_box_pack_start(GTK_BOX(vbbox), button, FALSE, FALSE, 0);
	gtk_widget_show(button);

	for (i = 1; i < 3; i++) {
		group = gtk_radio_button_group(GTK_RADIO_BUTTON(button));
		button = gtk_radio_button_new_with_label(group, mix_recsrc[i]);

		gtk_signal_connect(GTK_OBJECT(button), "toggled",
				   GTK_SIGNAL_FUNC(rec_buffer_callback), (gpointer) mix_recsrc[i]);

		if (mix_check_rec_source(&mix_set, mix_recsrc[i]))
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), TRUE);

		gtk_box_pack_start(GTK_BOX(vbbox), button, FALSE, FALSE, 0);
		gtk_widget_show(button);
	}

	/* Selected FX buses */
	frame = gtk_frame_new(NULL);
	gtk_frame_set_label(GTK_FRAME(frame), "Selected FX buses");
	gtk_box_pack_start(GTK_BOX(hbox), frame, TRUE, FALSE, 0);
	gtk_widget_show(frame);

	vbbox = gtk_vbutton_box_new();
	gtk_widget_show(vbbox);
	gtk_container_add(GTK_CONTAINER(frame), vbbox);

	for (i = 0; i < 0x10; i++) {
		button = gtk_toggle_button_new_with_label(dsp_out_name[i]);
		gtk_widget_show(button);
		fx_button[i] = button;

		gtk_signal_connect(GTK_OBJECT(button), "toggled",
				   GTK_SIGNAL_FUNC(rec_fx_buffer_callback), GINT_TO_POINTER(i));

		if (mix_check_rec_fx_bus(&mix_set, i))
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), TRUE);

		gtk_box_pack_start(GTK_BOX(vbbox), button, TRUE, FALSE, 0);
	}

	return scrolled_win;
}
void update_io_enable()
{
	io_cfg.output  =  cards[card_selected].output  |  live_drives[drive_selected].output | fx_cfg.output;
	io_cfg.num_out =  cards[card_selected].num_out + live_drives[drive_selected].num_out + fx_cfg.num_out ;
	io_cfg.input   =  cards[card_selected].input   | live_drives[drive_selected].input   | fx_cfg.input;
	io_cfg.num_in  =  cards[card_selected].num_in  + live_drives[drive_selected].num_in  + fx_cfg.num_in;
	
}
void  card_model_callback (GtkWidget *widget, gint val)
{
	card_selected=val;
	update_io_enable();
	return ;
	
}void  drive_model_callback (GtkWidget *widget, gint val)
{
	drive_selected=val;
	update_io_enable();
	return ;
	
}
static GtkWidget *create_hardware_config()
{
	int i;
	GtkWidget *vbox,*option,*menu,*item,*hbox,*label,*scrolled_win;

	vbox = gtk_vbox_new(FALSE, 5);
	scrolled_win =  create_scrolled_window(vbox);

	//card model
	hbox = gtk_hbox_new(FALSE, 0);
	gtk_box_pack_start (GTK_BOX (vbox),hbox,FALSE,FALSE,25);
	gtk_widget_show(hbox);

	label= gtk_label_new( "Card Model" );
	gtk_box_pack_start (GTK_BOX (hbox),label,FALSE,FALSE,50);
	gtk_widget_show(label);

	option=gtk_option_menu_new();
	gtk_box_pack_start (GTK_BOX (hbox),option,FALSE,FALSE,50);
	gtk_widget_show(option);
	gtk_widget_ref (option);

	menu=gtk_menu_new();
	gtk_widget_show(menu);
	for(i=0;i< NUM_CARDS;i++){
		item = gtk_menu_item_new_with_label( cards[i].name);
		gtk_menu_append(GTK_MENU(menu),item);
		gtk_widget_show (item); 
		gtk_signal_connect (GTK_OBJECT (item), "select",
                      GTK_SIGNAL_FUNC (card_model_callback),
                      GINT_TO_POINTER(i));

		gtk_menu_item_activate (GTK_MENU_ITEM(item));
	}

	gtk_option_menu_set_menu (GTK_OPTION_MENU (option),menu);
	gtk_option_menu_set_history (GTK_OPTION_MENU (option), card_selected);

	//Live Drive

	hbox = gtk_hbox_new(FALSE, 0);
	gtk_box_pack_start (GTK_BOX (vbox),hbox,FALSE,FALSE,25);
	gtk_widget_show(hbox);

	label= gtk_label_new( "Live Drive Model" );
	gtk_box_pack_start (GTK_BOX (hbox),label,FALSE,FALSE,50);
	gtk_widget_show(label);

	option=gtk_option_menu_new();	
	gtk_box_pack_start (GTK_BOX (hbox),option,FALSE,FALSE,50);
	gtk_widget_show(option);
	gtk_widget_ref (option);
	
	menu=gtk_menu_new();
	gtk_widget_show(menu);
	for(i=0;i< NUM_DRIVES;i++){
		item = gtk_menu_item_new_with_label( live_drives[i].name);
		gtk_menu_append(GTK_MENU(menu),item);
		gtk_signal_connect (GTK_OBJECT (item), "select",
                      GTK_SIGNAL_FUNC (drive_model_callback),
                      GINT_TO_POINTER(i));
		
		gtk_widget_show (item);
	}
	gtk_option_menu_set_menu (GTK_OPTION_MENU (option),menu);
	gtk_option_menu_set_history (GTK_OPTION_MENU (option), drive_selected);


	gtk_widget_show(vbox);
	return scrolled_win;
}

	
static GtkWidget *create_right_pane()
{
	notebook = gtk_notebook_new();
//        gtk_notebook_set_tab_pos (GTK_NOTEBOOK (notebook), GTK_POS_TOP);
	gtk_widget_set_usize(notebook, (WindowWidth / 3) * 2, RightPaneHeight);
	gtk_container_border_width(GTK_CONTAINER(notebook), 10);
	gtk_widget_show(notebook);
	return notebook;
}

static void tree_item_callback(GtkWidget * item, int item_n)
{
	GtkWidget *label;
	gint page_number;

	while ((page_number = gtk_notebook_get_current_page(GTK_NOTEBOOK(notebook))) >= 0)
		gtk_notebook_remove_page(GTK_NOTEBOOK(notebook), page_number);

	switch (item_n) {
	case DSP_ROUTES:
		label = gtk_label_new("DSP Routes");
		gtk_notebook_append_page(GTK_NOTEBOOK(notebook), create_dsp_routes(), label);
		break;

	case DSP_ROUTES_VOLUME:
		label = gtk_label_new("DSP Routes Volume");
		gtk_notebook_append_page(GTK_NOTEBOOK(notebook), create_dsp_routes_volume(), label);
		break;

	case MONO_VOICE:
		label = gtk_label_new("Mono Voice Settings");
		gtk_notebook_append_page(GTK_NOTEBOOK(notebook), create_voice_settings(MONO), label);
		break;

	case STEREO_VOICE:
		label = gtk_label_new("Left Voice Settings");
		gtk_notebook_append_page(GTK_NOTEBOOK(notebook), create_voice_settings(LEFT), label);
		label = gtk_label_new("Right Voice Settings");
		gtk_notebook_append_page(GTK_NOTEBOOK(notebook), create_voice_settings(RIGHT), label);
		break;

	case REC_SETTINGS:
		label = gtk_label_new("Recording Settings");
		gtk_notebook_append_page(GTK_NOTEBOOK(notebook), create_rec_settings(), label);
		break;
	case HARDWARE:
		label = gtk_label_new("Hardware Configurations");
		gtk_notebook_append_page(GTK_NOTEBOOK(notebook), create_hardware_config(), label);
		break;
	default:
		break;
	}
}

static GtkWidget *create_voice_tree()
{
	GtkWidget *tree;
	GtkWidget *item;

	tree = gtk_tree_new();
	gtk_widget_show(tree);

	item = gtk_tree_item_new_with_label("Mono");
	gtk_tree_append(GTK_TREE(tree), item);
	gtk_widget_show(item);
	gtk_signal_connect(GTK_OBJECT(item), "select", GTK_SIGNAL_FUNC(tree_item_callback), (gpointer) MONO_VOICE);


	item = gtk_tree_item_new_with_label("Stereo");
	gtk_tree_append(GTK_TREE(tree), item);
	gtk_widget_show(item);
	gtk_signal_connect(GTK_OBJECT(item), "select", GTK_SIGNAL_FUNC(tree_item_callback), (gpointer) STEREO_VOICE);

	return tree;
}


static GtkWidget *create_patches_tree()
{
	GtkWidget *tree, *tree1;
	GtkWidget *item, *item1;
	struct dsp_patch *patch;
	struct list_head *entry;
	char patch_name[DSP_PATCH_NAME_SIZE + 4];
	int i;

	tree = gtk_tree_new();
	gtk_widget_show(tree);

	item = gtk_tree_item_new_with_label("Input");
	gtk_tree_append(GTK_TREE(tree), item);
	gtk_widget_show(item);

	tree1 = gtk_tree_new();
	gtk_widget_show(tree1);

	for (i = 0; i < DSP_NUM_INPUTS; i++)
		list_for_each(entry, &dsp_mgr.ipatch_list[i]) {
		patch = list_entry(entry, struct dsp_patch, list);
		if (patch->id)
			sprintf(patch_name, "%s %d", patch->name, patch->id);
		else
			sprintf(patch_name, "%s", patch->name);

		item1 = gtk_tree_item_new_with_label(patch_name);
		gtk_tree_append(GTK_TREE(tree1), item1);
		gtk_widget_show(item1);
		}

	gtk_tree_item_set_subtree(GTK_TREE_ITEM(item), tree1);

//      gtk_signal_connect (GTK_OBJECT (item), "select", GTK_SIGNAL_FUNC (tree_item_callback), pages.patches_input);

	item = gtk_tree_item_new_with_label("Output");
	gtk_tree_append(GTK_TREE(tree), item);
	gtk_widget_show(item);

	tree1 = gtk_tree_new();
	gtk_widget_show(tree1);

	for (i = 0; i < DSP_NUM_OUTPUTS; i++)
		list_for_each(entry, &dsp_mgr.opatch_list[i]) {
		patch = list_entry(entry, struct dsp_patch, list);
		if (patch->id)
			sprintf(patch_name, "%s %d", patch->name, patch->id);
		else
			sprintf(patch_name, "%s", patch->name);

		item1 = gtk_tree_item_new_with_label(patch_name);
		gtk_tree_append(GTK_TREE(tree1), item1);
		gtk_widget_show(item1);
		}

	gtk_tree_item_set_subtree(GTK_TREE_ITEM(item), tree1);

//      gtk_signal_connect (GTK_OBJECT (item), "select", GTK_SIGNAL_FUNC (tree_item_callback), pages.patches_output);

	return tree;
}

static GtkWidget *create_dsp_routing_tree()
{
	GtkWidget *tree, *item;

	tree = gtk_tree_new();
	gtk_widget_show(tree);

	item = gtk_tree_item_new_with_label("Routes");
	gtk_tree_append(GTK_TREE(tree), item);
	gtk_widget_show(item);
	gtk_signal_connect(GTK_OBJECT(item), "select", GTK_SIGNAL_FUNC(tree_item_callback), (gpointer) DSP_ROUTES);

	item = gtk_tree_item_new_with_label("Routes Volume");
	gtk_tree_append(GTK_TREE(tree), item);
	gtk_widget_show(item);
	gtk_signal_connect(GTK_OBJECT(item), "select", GTK_SIGNAL_FUNC(tree_item_callback),
			   (gpointer) DSP_ROUTES_VOLUME);

	return tree;
}


static GtkWidget *create_left_pane()
{
	GtkWidget *tree, *item;

	tree = gtk_tree_new();

	gtk_widget_show(tree);

	item = gtk_tree_item_new_with_label("Hardware Config");
	gtk_tree_append(GTK_TREE(tree), item);
	gtk_widget_show(item);
	gtk_signal_connect(GTK_OBJECT(item), "select", GTK_SIGNAL_FUNC(tree_item_callback), (gpointer) HARDWARE);

	item = gtk_tree_item_new_with_label("Controls");
	gtk_tree_append(GTK_TREE(tree), item);
	gtk_widget_show(item);
//      gtk_signal_connect (GTK_OBJECT (item), "select", GTK_SIGNAL_FUNC (tree_item_callback), pages.dsp_controls);

	item = gtk_tree_item_new_with_label("Patches");
	gtk_tree_append(GTK_TREE(tree), item);
	gtk_widget_show(item);

	gtk_tree_item_set_subtree(GTK_TREE_ITEM(item), create_patches_tree());

	item = gtk_tree_item_new_with_label("DSP Routing");
	gtk_tree_append(GTK_TREE(tree), item);
	gtk_widget_show(item);

	gtk_tree_item_set_subtree(GTK_TREE_ITEM(item), create_dsp_routing_tree());

	item = gtk_tree_item_new_with_label("Voice settings");
	gtk_tree_append(GTK_TREE(tree), item);
	gtk_widget_show(item);

	gtk_tree_item_set_subtree(GTK_TREE_ITEM(item), create_voice_tree());

	item = gtk_tree_item_new_with_label("Recording Settings");
	gtk_widget_show(item);
	gtk_tree_append(GTK_TREE(tree), item);
	gtk_signal_connect(GTK_OBJECT(item), "select", GTK_SIGNAL_FUNC(tree_item_callback), (gpointer) REC_SETTINGS);

	return tree;
}


static void QuitApp(gpointer callback_data, guint callback_action, GtkWidget * widget)
{
	gtk_main_quit();
}

static void ShowMenu(gpointer callback_data, guint callback_action, GtkWidget * widget)
{
}

static void LoadPatch(gpointer callback_data, guint callback_action, GtkWidget * widget)
{
}

static void ShowAbout(gpointer callback_data, guint callback_action, GtkWidget * widget)
{
}

GtkWidget *create_menus(GtkWidget * window)
{
	GtkAccelGroup *accel_group;
	GtkItemFactory *item_factory;

	static GtkItemFactoryEntry menu_items[] = {
		{"/_File", NULL, 0, 0, "<Branch>"},
		{"/File/_Load", NULL, LoadPatch, 0, 0},
		{"/File/tearoff1", NULL, ShowMenu, 0, "<Tearoff>"},
		{"/File/E_xit", "<control>Q", QuitApp, 0, 0},

		{"/_Help", NULL, 0, 0, "<Branch>"},
		{"/Help/_About", NULL, ShowAbout, 0, 0},
	};

	static int nmenu_items = sizeof(menu_items) / sizeof(menu_items[0]);

	accel_group = gtk_accel_group_new();
	item_factory = gtk_item_factory_new(GTK_TYPE_MENU_BAR, "<menu>", accel_group);
	gtk_item_factory_create_items(item_factory, nmenu_items, menu_items, NULL);
	gtk_accel_group_attach(accel_group, GTK_OBJECT(window));
	return (gtk_item_factory_get_widget(item_factory, "<menu>"));
}

int main(int argc, char *argv[])
{
	GtkWidget *vbox;
	GtkWidget *window;
	GtkWidget *paned;
	GtkWidget *lframe;
	GtkWidget *scrolled_win;
	GtkWidget *MenuBar;
	GtkWidget *separator;

	gtk_init(&argc, &argv);

	mix_init(&mix_set);
	update_io_enable();
	dsp_mgr.debug = 0;
	dsp_init(&dsp_mgr);

	/* Adjust window to resolution. */
	WindowWidth = gdk_screen_width() * 0.8;
	if (WindowWidth > 750)
		WindowWidth = 750;

	WindowHeight = gdk_screen_height() * 0.8;
	if (WindowHeight > 590)
		WindowHeight = 590;

	LeftPaneHeight = WindowHeight - 100;
	RightPaneHeight = WindowHeight - 70;

	window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_title(GTK_WINDOW(window), "Emu10k1 mixer");
	gtk_signal_connect(GTK_OBJECT(window), "destroy", GTK_SIGNAL_FUNC(gtk_main_quit), NULL);
	gtk_widget_set_usize(GTK_WIDGET(window), WindowWidth, WindowHeight);

	gtk_vbutton_box_set_layout_default(GTK_BUTTONBOX_SPREAD);
	gtk_hbutton_box_set_layout_default(GTK_BUTTONBOX_SPREAD);
	gtk_button_box_set_child_size_default(5, 5);
	gtk_button_box_set_child_ipadding_default(1, 1);
	gtk_hbutton_box_set_spacing_default(5);
	gtk_vbutton_box_set_spacing_default(5);

	vbox = gtk_vbox_new(FALSE, 0);
	MenuBar = create_menus(window);

	/* create a vpaned widget and add it to our toplevel window */
	paned = gtk_hpaned_new();
	gtk_paned_set_handle_size(GTK_PANED(paned), 10);
	gtk_paned_set_gutter_size(GTK_PANED(paned), 15);

	/* Left hand side. */
	scrolled_win = gtk_scrolled_window_new(NULL, NULL);
	gtk_widget_set_usize(scrolled_win, (WindowWidth / 4), LeftPaneHeight);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_win), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(scrolled_win), create_left_pane());
	gtk_widget_show(scrolled_win);

	lframe = gtk_frame_new("Emu10k1");
	gtk_container_set_border_width(GTK_CONTAINER(lframe), 10);
	gtk_frame_set_shadow_type(GTK_FRAME(lframe), GTK_SHADOW_IN);
	gtk_frame_set_label_align(GTK_FRAME(lframe), 0, 0);
	gtk_widget_show(lframe);
	gtk_container_add(GTK_CONTAINER(lframe), scrolled_win);
	gtk_paned_add1(GTK_PANED(paned), lframe);

	/* Right hand side (Notebook page(s), containing scrolled window containing widgets */
	gtk_paned_add2(GTK_PANED(paned), create_right_pane());

	/* Bottom half. */
	separator = gtk_hseparator_new();

	/* Now pack everything into the vbox */
	gtk_box_pack_start(GTK_BOX(vbox), MenuBar, FALSE, FALSE, 0);
	gtk_widget_show(MenuBar);
	gtk_box_pack_start(GTK_BOX(vbox), paned, TRUE, TRUE, 0);
	gtk_widget_show(paned);
	gtk_box_pack_start(GTK_BOX(vbox), separator, FALSE, FALSE, 0);
	gtk_widget_show(separator);


	gtk_container_add(GTK_CONTAINER(window), vbox);
	gtk_widget_show(vbox);
	gtk_widget_show(window);
	gtk_main();

	return 0;
}
