/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2001-2005 Free Software Foundation, Inc.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Street #330, Boston, MA 02111-1307, USA.
 */

#include <config.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include <glib/gi18n.h>
#include <gtk/gtk.h>
#include <libgnome/gnome-help.h>
#include <libgnomevfs/gnome-vfs-utils.h>
#include <glade/glade.h>

#include "thumb-cache.h"
#include "comments.h"
#include "file-utils.h"
#include "gth-utils.h"
#include "gtk-utils.h"
#include "main.h"
#include "gconf-utils.h"
#include "typedefs.h"
#include "gth-browser.h"
#include "preferences.h"

#include "icons/layout1.xpm"
#include "icons/layout2.xpm"
#include "icons/layout3.xpm"
#include "icons/layout4.xpm"

static int thumb_size[] = { 48, 64, 85, 95, 112, 128, 164, 200, 256 };
static int thumb_sizes = sizeof (thumb_size) / sizeof (int);
static GthZoomChange zoom_change[] = { GTH_ZOOM_CHANGE_KEEP_PREV,
				       GTH_ZOOM_CHANGE_FIT_SIZE_IF_LARGER,
	 		               GTH_ZOOM_CHANGE_ACTUAL_SIZE,
	 		               GTH_ZOOM_CHANGE_FIT_WIDTH_IF_LARGER };
static int zoom_changes = sizeof (zoom_change) / sizeof (GthZoomChange);

#define GLADE_PREF_FILE "gthumb_preferences.glade"

typedef struct {
	GthBrowser *browser;

	GladeXML   *gui;
	GtkWidget  *dialog;

	GtkWidget  *radio_current_location;
	GtkWidget  *radio_last_location;
	GtkWidget  *radio_use_startup;
	GtkWidget  *startup_dir_filechooserbutton;
	GtkWidget  *btn_set_to_current;
	GtkWidget  *radio_layout1;
	GtkWidget  *radio_layout2;
	GtkWidget  *radio_layout3;
	GtkWidget  *radio_layout4;
	GtkWidget  *toolbar_style_optionmenu;
	GtkWidget  *radio_exif_use_orientation_tag;
	GtkWidget  *radio_exif_use_image_transform;

	GtkWidget  *toggle_show_filenames;
	GtkWidget  *toggle_show_comments;
	GtkWidget  *toggle_show_thumbs;
	GtkWidget  *toggle_file_type;
	GtkWidget  *opt_thumbs_size;
	GtkWidget  *toggle_confirm_del;
	GtkWidget  *toggle_ask_to_save;
	GtkWidget  *opt_click_policy;

	GtkWidget  *opt_zoom_quality_high;
	GtkWidget  *opt_zoom_quality_low;
	GtkWidget  *opt_zoom_change;
	GtkWidget  *opt_transparency;

	GtkWidget  *toggle_reset_scrollbars;

	GtkWidget  *radio_ss_direction_forward;
	GtkWidget  *radio_ss_direction_reverse;
	GtkWidget  *radio_ss_direction_random;
	GtkWidget  *spin_ss_delay;
	GtkWidget  *toggle_ss_wrap_around;
	GtkWidget  *toggle_ss_fading;

	GtkWidget  *hotkey0;
        GtkWidget  *hotkey1;
        GtkWidget  *hotkey2;
        GtkWidget  *hotkey3;
        GtkWidget  *hotkey4;
        GtkWidget  *hotkey5;
        GtkWidget  *hotkey6;
        GtkWidget  *hotkey7;
        GtkWidget  *hotkey8;
        GtkWidget  *hotkey9;

} DialogData;


/* called when the main dialog is closed. */
static void
destroy_cb (GtkWidget *widget,
	    DialogData *data)
{
	g_object_unref (G_OBJECT (data->gui));
	g_free (data);
}


/* called when the "apply" button is clicked. */
static void
apply_cb (GtkWidget  *widget,
	  DialogData *data)
{
	GthDirectionType direction = GTH_DIRECTION_FORWARD;

	/* Startup dir. */

	eel_gconf_set_boolean (PREF_GO_TO_LAST_LOCATION, gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (data->radio_last_location)));
	eel_gconf_set_boolean (PREF_USE_STARTUP_LOCATION, gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (data->radio_use_startup)));

	if (eel_gconf_get_boolean (PREF_USE_STARTUP_LOCATION, FALSE)) {
		char *location;

		location = gtk_file_chooser_get_uri (GTK_FILE_CHOOSER (data->startup_dir_filechooserbutton));
		eel_gconf_set_path (PREF_STARTUP_LOCATION, location);
		preferences_set_startup_location (location);
		g_free (location);
	}

	/* Slide Show. */

	eel_gconf_set_boolean (PREF_SLIDESHOW_WRAP_AROUND, gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (data->toggle_ss_wrap_around)));
	eel_gconf_set_boolean (PREF_SLIDESHOW_FADING, gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (data->toggle_ss_fading)));

	eel_gconf_set_float (PREF_SLIDESHOW_DELAY, gtk_spin_button_get_value (GTK_SPIN_BUTTON (data->spin_ss_delay)));

	/**/

	if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (data->radio_ss_direction_forward)))
		direction = GTH_DIRECTION_FORWARD;
	else if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (data->radio_ss_direction_reverse)))
		direction = GTH_DIRECTION_REVERSE;
	else if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (data->radio_ss_direction_random)))
		direction = GTH_DIRECTION_RANDOM;

	pref_set_slideshow_direction (direction);

	eel_gconf_set_string (PREF_HOTKEY0, gtk_entry_get_text (GTK_ENTRY (data->hotkey0)));
        eel_gconf_set_string (PREF_HOTKEY1, gtk_entry_get_text (GTK_ENTRY (data->hotkey1)));
        eel_gconf_set_string (PREF_HOTKEY2, gtk_entry_get_text (GTK_ENTRY (data->hotkey2)));
        eel_gconf_set_string (PREF_HOTKEY3, gtk_entry_get_text (GTK_ENTRY (data->hotkey3)));
        eel_gconf_set_string (PREF_HOTKEY4, gtk_entry_get_text (GTK_ENTRY (data->hotkey4)));
        eel_gconf_set_string (PREF_HOTKEY5, gtk_entry_get_text (GTK_ENTRY (data->hotkey5)));
        eel_gconf_set_string (PREF_HOTKEY6, gtk_entry_get_text (GTK_ENTRY (data->hotkey6)));
        eel_gconf_set_string (PREF_HOTKEY7, gtk_entry_get_text (GTK_ENTRY (data->hotkey7)));
        eel_gconf_set_string (PREF_HOTKEY8, gtk_entry_get_text (GTK_ENTRY (data->hotkey8)));
        eel_gconf_set_string (PREF_HOTKEY9, gtk_entry_get_text (GTK_ENTRY (data->hotkey9)));	
}


/* called when the "close" button is clicked. */
static void
close_cb (GtkWidget  *widget,
	  DialogData *data)
{
	apply_cb (widget, data);
	gtk_widget_destroy (data->dialog);
}


/* called when the "help" button is clicked. */
static void
help_cb (GtkWidget  *widget,
	 DialogData *data)
{
	gthumb_display_help (GTK_WINDOW (data->dialog), "preferences");
}


/* called when the "use startup" is clicked. */
static void
use_startup_toggled_cb (GtkWidget *widget,
			DialogData *data)
{
	gtk_widget_set_sensitive (data->startup_dir_filechooserbutton,
				  GTK_TOGGLE_BUTTON (widget)->active);
	gtk_widget_set_sensitive (data->btn_set_to_current,
				  GTK_TOGGLE_BUTTON (widget)->active);
}


/* called when the "set to current" button is clicked. */
static void
set_to_current_cb (GtkWidget  *widget,
		   DialogData *data)
{
	gtk_file_chooser_set_current_folder_uri (GTK_FILE_CHOOSER (data->startup_dir_filechooserbutton),
						 gth_browser_get_current_directory (data->browser));
}


/* get the option menu index from the size value. */
static int
get_idx_from_size (gint size)
{
	int i;

	for (i = 0; i < thumb_sizes; i++)
		if (size == thumb_size[i])
			return i;
	return -1;
}


static int
get_idx_from_zoom_change (GthZoomChange change_mode)
{
	int i;

	for (i = 0; i < zoom_changes; i++)
		if (change_mode == zoom_change[i])
			return i;
	return -1;
}


static void
layout_toggled_cb (GtkWidget *widget,
		   DialogData *data)
{
	int layout_type;

	if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (data->radio_layout1)))
		layout_type = 0;
	else if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (data->radio_layout2)))
		layout_type = 1;
	else if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (data->radio_layout3)))
		layout_type = 2;
	else
		layout_type = 3;

	eel_gconf_set_integer (PREF_UI_LAYOUT, layout_type);
}

static void
toolbar_style_changed_cb (GtkOptionMenu *option_menu,
			  DialogData    *data)
{
	pref_set_toolbar_style (gtk_option_menu_get_history (GTK_OPTION_MENU (data->toolbar_style_optionmenu)));
}


static void
show_thumbs_toggled_cb (GtkToggleButton *button,
			DialogData      *data)
{
	eel_gconf_set_boolean (PREF_SHOW_THUMBNAILS, gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (data->toggle_show_thumbs)));
}


static void
show_filenames_toggled_cb (GtkToggleButton *button,
			   DialogData      *data)
{
	eel_gconf_set_boolean (PREF_SHOW_FILENAMES, gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (data->toggle_show_filenames)));
}


static void
show_comments_toggled_cb (GtkToggleButton *button,
			  DialogData      *data)
{
	eel_gconf_set_boolean (PREF_SHOW_COMMENTS, gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (data->toggle_show_comments)));
}


static void
thumbs_size_changed_cb (GtkOptionMenu *option_menu,
			DialogData    *data)
{
	int i;

	i = gtk_option_menu_get_history (GTK_OPTION_MENU (data->opt_thumbs_size));
	eel_gconf_set_integer (PREF_THUMBNAIL_SIZE, thumb_size[i]);
}


static void
click_policy_changed_cb (GtkOptionMenu *option_menu,
			 DialogData    *data)
{
	pref_set_click_policy (gtk_option_menu_get_history (GTK_OPTION_MENU (data->opt_click_policy)));
}


static void
confirm_del_toggled_cb (GtkToggleButton *button,
			DialogData      *data)
{
	eel_gconf_set_boolean (PREF_CONFIRM_DELETION, gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (data->toggle_confirm_del)));
}


static void
ask_to_save_toggled_cb (GtkToggleButton *button,
			DialogData      *data)
{
	eel_gconf_set_boolean (PREF_MSG_SAVE_MODIFIED_IMAGE, gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (data->toggle_ask_to_save)));
}


static void
fast_file_type_toggled_cb (GtkToggleButton *button,
			   DialogData      *data)
{
	eel_gconf_set_boolean (PREF_FAST_FILE_TYPE, ! gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (data->toggle_file_type)));
}


static void
zoom_quality_high_cb (GtkToggleButton *button,
		      DialogData      *data)
{
	if (! gtk_toggle_button_get_active (button))
		return;
	pref_set_zoom_quality (GTH_ZOOM_QUALITY_HIGH);
}


static void
zoom_quality_low_cb (GtkToggleButton *button,
		     DialogData      *data)
{
	if (! gtk_toggle_button_get_active (button))
		return;
	pref_set_zoom_quality (GTH_ZOOM_QUALITY_LOW);
}


static void
zoom_change_changed_cb (GtkOptionMenu *option_menu,
			DialogData    *data)
{
	int idx = gtk_option_menu_get_history (GTK_OPTION_MENU (data->opt_zoom_change));
	pref_set_zoom_change (zoom_change[idx]);
}


static void
reset_scrollbars_toggled_cb (GtkToggleButton *button,
			     DialogData      *data)
{
	eel_gconf_set_boolean (PREF_RESET_SCROLLBARS, gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (data->toggle_reset_scrollbars)));
}


static void
transp_type_changed_cb (GtkOptionMenu *option_menu,
			DialogData    *data)
{
	pref_set_transp_type(gtk_option_menu_get_history (GTK_OPTION_MENU (data->opt_transparency)));
}


/* create the main dialog. */
void
dlg_preferences (GthBrowser *browser)
{
	DialogData       *data;
	GtkWidget        *btn_close;
	GtkWidget        *btn_help;
	int               layout_type;
	char             *startup_location;
	GthDirectionType  direction;

	data = g_new0 (DialogData, 1);
	data->browser = browser;
	data->gui = glade_xml_new (GTHUMB_GLADEDIR "/" GLADE_PREF_FILE, NULL, NULL);
        if (!data->gui) {
                g_warning ("Could not find " GLADE_PREF_FILE "\n");
		g_free (data);
                return;
        }

	eel_gconf_preload_cache ("/apps/gthumb/browser", GCONF_CLIENT_PRELOAD_ONELEVEL);
	eel_gconf_preload_cache ("/apps/gthumb/viewer", GCONF_CLIENT_PRELOAD_ONELEVEL);
	eel_gconf_preload_cache ("/apps/gthumb/ui", GCONF_CLIENT_PRELOAD_ONELEVEL);

	/* Get the widgets. */

	data->dialog = glade_xml_get_widget (data->gui, "preferences_dialog");

        data->radio_current_location = glade_xml_get_widget (data->gui, "radio_current_location");
        data->radio_last_location = glade_xml_get_widget (data->gui, "radio_last_location");
        data->radio_use_startup = glade_xml_get_widget (data->gui, "radio_use_startup");
        data->startup_dir_filechooserbutton = glade_xml_get_widget (data->gui, "startup_dir_filechooserbutton");
	data->btn_set_to_current = glade_xml_get_widget (data->gui, "btn_set_to_current");
        data->toggle_confirm_del = glade_xml_get_widget (data->gui, "toggle_confirm_del");
        data->toggle_ask_to_save = glade_xml_get_widget (data->gui, "toggle_ask_to_save");
	data->radio_layout1 = glade_xml_get_widget (data->gui, "radio_layout1");
	data->radio_layout2 = glade_xml_get_widget (data->gui, "radio_layout2");
	data->radio_layout3 = glade_xml_get_widget (data->gui, "radio_layout3");
	data->radio_layout4 = glade_xml_get_widget (data->gui, "radio_layout4");
	data->toolbar_style_optionmenu = glade_xml_get_widget (data->gui, "toolbar_style_optionmenu");
	data->radio_exif_use_orientation_tag = glade_xml_get_widget (data->gui, "radio_exif_use_orientation_tag");
	data->radio_exif_use_image_transform = glade_xml_get_widget (data->gui, "radio_exif_use_image_transform");

        data->toggle_show_filenames = glade_xml_get_widget (data->gui, "toggle_show_filenames");
        data->toggle_show_comments = glade_xml_get_widget (data->gui, "toggle_show_comments");

        data->toggle_show_thumbs = glade_xml_get_widget (data->gui, "toggle_show_thumbs");
        data->toggle_file_type = glade_xml_get_widget (data->gui, "toggle_file_type");

        data->opt_thumbs_size = glade_xml_get_widget (data->gui, "opt_thumbs_size");
        data->opt_click_policy = glade_xml_get_widget (data->gui, "opt_click_policy");

	data->opt_zoom_quality_high = glade_xml_get_widget (data->gui, "opt_zoom_quality_high");
	data->opt_zoom_quality_low = glade_xml_get_widget (data->gui, "opt_zoom_quality_low");
        data->opt_zoom_change = glade_xml_get_widget (data->gui, "opt_zoom_change");
	data->opt_transparency = glade_xml_get_widget (data->gui, "opt_transparency");

	data->toggle_reset_scrollbars = glade_xml_get_widget (data->gui, "toggle_reset_scrollbars");

        data->radio_ss_direction_forward = glade_xml_get_widget (data->gui, "radio_ss_direction_forward");
        data->radio_ss_direction_reverse = glade_xml_get_widget (data->gui, "radio_ss_direction_reverse");
        data->radio_ss_direction_random = glade_xml_get_widget (data->gui, "radio_ss_direction_random");
        data->spin_ss_delay = glade_xml_get_widget (data->gui, "spin_ss_delay");
        data->toggle_ss_wrap_around = glade_xml_get_widget (data->gui, "toggle_ss_wrap_around");
        data->toggle_ss_fading = glade_xml_get_widget (data->gui, "toggle_ss_fading");

	data->hotkey0 = glade_xml_get_widget (data->gui, "hotkey0");
        data->hotkey1 = glade_xml_get_widget (data->gui, "hotkey1");
        data->hotkey2 = glade_xml_get_widget (data->gui, "hotkey2");
        data->hotkey3 = glade_xml_get_widget (data->gui, "hotkey3");
        data->hotkey4 = glade_xml_get_widget (data->gui, "hotkey4");
        data->hotkey5 = glade_xml_get_widget (data->gui, "hotkey5");
        data->hotkey6 = glade_xml_get_widget (data->gui, "hotkey6");
        data->hotkey7 = glade_xml_get_widget (data->gui, "hotkey7");
        data->hotkey8 = glade_xml_get_widget (data->gui, "hotkey8");
        data->hotkey9 = glade_xml_get_widget (data->gui, "hotkey9");

	btn_close  = glade_xml_get_widget (data->gui, "p_close_button");
	btn_help   = glade_xml_get_widget (data->gui, "p_help_button");

	/* Set widgets data. */

	/* * general */

	if (eel_gconf_get_boolean (PREF_USE_STARTUP_LOCATION, FALSE))
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (data->radio_use_startup), TRUE);
	else if (eel_gconf_get_boolean (PREF_GO_TO_LAST_LOCATION, TRUE))
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (data->radio_last_location), TRUE);
	else
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (data->radio_current_location), TRUE);

	if (! eel_gconf_get_boolean (PREF_USE_STARTUP_LOCATION, FALSE)) {
		gtk_widget_set_sensitive (data->startup_dir_filechooserbutton, FALSE);
		gtk_widget_set_sensitive (data->btn_set_to_current, FALSE);
	}

	startup_location = eel_gconf_get_path (PREF_STARTUP_LOCATION, NULL);

	if (uri_scheme_is_file (startup_location))
		gtk_file_chooser_set_current_folder_uri (GTK_FILE_CHOOSER (data->startup_dir_filechooserbutton), startup_location);

	g_free (startup_location);

	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (data->toggle_confirm_del), eel_gconf_get_boolean (PREF_CONFIRM_DELETION, TRUE));
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (data->toggle_ask_to_save), eel_gconf_get_boolean (PREF_MSG_SAVE_MODIFIED_IMAGE, TRUE));

	gtk_container_add (GTK_CONTAINER (data->radio_layout1),
			   _gtk_image_new_from_xpm_data (layout1_xpm));
	gtk_container_add (GTK_CONTAINER (data->radio_layout2),
			   _gtk_image_new_from_xpm_data (layout2_xpm));
	gtk_container_add (GTK_CONTAINER (data->radio_layout3),
			   _gtk_image_new_from_xpm_data (layout3_xpm));
	gtk_container_add (GTK_CONTAINER (data->radio_layout4),
			   _gtk_image_new_from_xpm_data (layout4_xpm));
	gtk_option_menu_set_history (GTK_OPTION_MENU (data->toolbar_style_optionmenu), pref_get_toolbar_style ());

	/* ** layout */

	layout_type = eel_gconf_get_integer (PREF_UI_LAYOUT, 2);

	if (layout_type == 0)
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (data->radio_layout1), TRUE);
	else if (layout_type == 1)
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (data->radio_layout2), TRUE);
	else if (layout_type == 2)
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (data->radio_layout3), TRUE);
	else
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (data->radio_layout4), TRUE);

	/* * browser */

	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (data->toggle_file_type), ! eel_gconf_get_boolean (PREF_FAST_FILE_TYPE, TRUE));
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (data->toggle_show_filenames), eel_gconf_get_boolean (PREF_SHOW_FILENAMES, FALSE));
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (data->toggle_show_comments), eel_gconf_get_boolean (PREF_SHOW_COMMENTS, TRUE));
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (data->toggle_show_thumbs), eel_gconf_get_boolean (PREF_SHOW_THUMBNAILS, TRUE));
	gtk_option_menu_set_history (GTK_OPTION_MENU (data->opt_thumbs_size), get_idx_from_size (eel_gconf_get_integer (PREF_THUMBNAIL_SIZE, 95)));
	gtk_option_menu_set_history (GTK_OPTION_MENU (data->opt_click_policy), pref_get_click_policy ());

	/* * viewer */

	if (pref_get_zoom_quality () == GTH_ZOOM_QUALITY_HIGH)
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (data->opt_zoom_quality_high), TRUE);
	else
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (data->opt_zoom_quality_low), TRUE);

	gtk_option_menu_set_history (GTK_OPTION_MENU (data->opt_zoom_change), get_idx_from_zoom_change (pref_get_zoom_change ()));
	gtk_option_menu_set_history (GTK_OPTION_MENU (data->opt_transparency), image_viewer_get_transp_type (gth_window_get_image_viewer (GTH_WINDOW (browser))));

	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (data->toggle_reset_scrollbars), eel_gconf_get_boolean (PREF_RESET_SCROLLBARS, TRUE));

	/* * slide show */

	direction = pref_get_slideshow_direction ();
	if (direction == GTH_DIRECTION_FORWARD)
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (data->radio_ss_direction_forward), TRUE);
	else if (direction == GTH_DIRECTION_REVERSE)
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (data->radio_ss_direction_reverse), TRUE);
	else
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (data->radio_ss_direction_random), TRUE);

	gtk_spin_button_set_value (GTK_SPIN_BUTTON (data->spin_ss_delay),
				   eel_gconf_get_float (PREF_SLIDESHOW_DELAY, 4.0));
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (data->toggle_ss_wrap_around), eel_gconf_get_boolean (PREF_SLIDESHOW_WRAP_AROUND, FALSE));
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (data->toggle_ss_fading), eel_gconf_get_boolean (PREF_SLIDESHOW_FADING, TRUE));

	gtk_entry_set_text (GTK_ENTRY (data->hotkey0), 
			    eel_gconf_get_string (PREF_HOTKEY0, 
			    "gimp %F"));

	gtk_entry_set_text (GTK_ENTRY (data->hotkey1), 
			    eel_gconf_get_string (PREF_HOTKEY1, 
			    "convert %f -font Helvetica -pointsize 20 -fill white  -box '#00000080'  -gravity South -annotate +0+5 ' Copyright 2007, Your Name Here ' %n-copyright%e"));

        gtk_entry_set_text (GTK_ENTRY (data->hotkey2), 
			    eel_gconf_get_string (PREF_HOTKEY2, 
			    "mkdir -p %p/approved ; cp %f %p/approved/"));

        gtk_entry_set_text (GTK_ENTRY (data->hotkey3), 
			    eel_gconf_get_string (PREF_HOTKEY3, 
			    "uuencode %f %f | mail -s Photos your@emailaddress.com"));

        gtk_entry_set_text (GTK_ENTRY (data->hotkey4), 
			    eel_gconf_get_string (PREF_HOTKEY4, 
			    "rm ~/myarchive.zip; zip -j ~/myarchive %F"));

        gtk_entry_set_text (GTK_ENTRY (data->hotkey5), 
			    eel_gconf_get_string (PREF_HOTKEY5, 
		 	    "rm ~/myarchive.zip; zip -j ~/myarchive %F; uuencode ~/myarchive.zip ~/myarchive.zip | mail -s Photos your@emailaddress.com"));

        gtk_entry_set_text (GTK_ENTRY (data->hotkey6), eel_gconf_get_string (PREF_HOTKEY6, ""));
        gtk_entry_set_text (GTK_ENTRY (data->hotkey7), eel_gconf_get_string (PREF_HOTKEY7, ""));
        gtk_entry_set_text (GTK_ENTRY (data->hotkey8), eel_gconf_get_string (PREF_HOTKEY8, ""));
        gtk_entry_set_text (GTK_ENTRY (data->hotkey9), eel_gconf_get_string (PREF_HOTKEY9, ""));

	/* Set the signals handlers. */

	g_signal_connect (G_OBJECT (data->dialog),
			  "destroy",
			  G_CALLBACK (destroy_cb),
			  data);

	g_signal_connect (G_OBJECT (btn_close),
			  "clicked",
			  G_CALLBACK (close_cb),
			  data);
	g_signal_connect (G_OBJECT (btn_help),
			  "clicked",
			  G_CALLBACK (help_cb),
			  data);

	g_signal_connect (G_OBJECT (data->radio_use_startup),
			  "toggled",
			  G_CALLBACK (use_startup_toggled_cb),
			  data);
	g_signal_connect (G_OBJECT (data->btn_set_to_current),
			  "clicked",
			  G_CALLBACK (set_to_current_cb),
			  data);

	/**/

	g_signal_connect (G_OBJECT (data->radio_layout1),
			  "toggled",
			  G_CALLBACK (layout_toggled_cb),
			  data);
	g_signal_connect (G_OBJECT (data->radio_layout2),
			  "toggled",
			  G_CALLBACK (layout_toggled_cb),
			  data);
	g_signal_connect (G_OBJECT (data->radio_layout3),
			  "toggled",
			  G_CALLBACK (layout_toggled_cb),
			  data);
	g_signal_connect (G_OBJECT (data->radio_layout4),
			  "toggled",
			  G_CALLBACK (layout_toggled_cb),
			  data);

	g_signal_connect (G_OBJECT (data->toolbar_style_optionmenu),
			  "changed",
			  G_CALLBACK (toolbar_style_changed_cb),
			  data);
	g_signal_connect (G_OBJECT (data->toggle_show_thumbs),
			  "toggled",
			  G_CALLBACK (show_thumbs_toggled_cb),
			  data);
	g_signal_connect (G_OBJECT (data->toggle_show_filenames),
			  "toggled",
			  G_CALLBACK (show_filenames_toggled_cb),
			  data);
	g_signal_connect (G_OBJECT (data->toggle_show_comments),
			  "toggled",
			  G_CALLBACK (show_comments_toggled_cb),
			  data);

	g_signal_connect (G_OBJECT (data->opt_thumbs_size),
			  "changed",
			  G_CALLBACK (thumbs_size_changed_cb),
			  data);

	g_signal_connect (G_OBJECT (data->opt_click_policy),
			  "changed",
			  G_CALLBACK (click_policy_changed_cb),
			  data);
	g_signal_connect (G_OBJECT (data->toggle_confirm_del),
			  "toggled",
			  G_CALLBACK (confirm_del_toggled_cb),
			  data);
	g_signal_connect (G_OBJECT (data->toggle_ask_to_save),
			  "toggled",
			  G_CALLBACK (ask_to_save_toggled_cb),
			  data);
	g_signal_connect (G_OBJECT (data->toggle_file_type),
			  "toggled",
			  G_CALLBACK (fast_file_type_toggled_cb),
			  data);

	g_signal_connect (G_OBJECT (data->opt_zoom_quality_high),
			  "toggled",
			  G_CALLBACK (zoom_quality_high_cb),
			  data);
	g_signal_connect (G_OBJECT (data->opt_zoom_quality_low),
			  "toggled",
			  G_CALLBACK (zoom_quality_low_cb),
			  data);
	g_signal_connect (G_OBJECT (data->opt_zoom_change),
			  "changed",
			  G_CALLBACK (zoom_change_changed_cb),
			  data);
	g_signal_connect (G_OBJECT (data->opt_transparency),
			  "changed",
			  G_CALLBACK (transp_type_changed_cb),
			  data);
        g_signal_connect (G_OBJECT (data->toggle_reset_scrollbars),
                          "toggled",
                          G_CALLBACK (reset_scrollbars_toggled_cb),
                          data);

	/* run dialog. */

	gtk_window_set_transient_for (GTK_WINDOW (data->dialog),
				      GTK_WINDOW (browser));
	gtk_window_set_modal (GTK_WINDOW (data->dialog), FALSE);
	gtk_widget_show_all (data->dialog);
}
