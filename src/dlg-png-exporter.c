/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2001, 2003 Free Software Foundation, Inc.
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
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <glib/gi18n.h>
#include <gtk/gtk.h>
#include <glade/glade.h>

#include "catalog-png-exporter.h"
#include "dlg-file-utils.h"
#include "file-utils.h"
#include "gtk-utils.h"
#include "gth-file-view.h"
#include "main.h"
#include "pixbuf-utils.h"
#include "gconf-utils.h"
#include "gth-window.h"
#include "gth-browser.h"
#include "gthumb-slide.h"
#include "glib-utils.h"
#include "gthumb-stock.h"


#define GLADE_EXPORTER_FILE "gthumb_png_exporter.glade"
#define ROW_SPACING         5
#define DEF_FILE_TYPE       "jpeg"
#define DEF_NAME_TEMPLATE   "###"
#define DEF_ROWS            3
#define DEF_COLS            4
#define DEF_PAGE_WIDTH      400
#define DEF_PAGE_HEIGHT     400
#define DEF_THUMB_SIZE      128

static int           sort_method_to_idx[] = { -1, 0, 1, 2, 3, 4, 5, 6 };
static GthSortMethod idx_to_sort_method[] = { GTH_SORT_METHOD_BY_NAME,
					      GTH_SORT_METHOD_BY_PATH,
					      GTH_SORT_METHOD_BY_SIZE,
					      GTH_SORT_METHOD_BY_TIME,
					      GTH_SORT_METHOD_BY_EXIF_DATE,
					      GTH_SORT_METHOD_BY_COMMENT,
					      GTH_SORT_METHOD_MANUAL};

typedef struct {
	GthBrowser         *browser;

	GladeXML           *gui;
	GtkWidget          *dialog;

	GtkWidget          *dest_filechooserbutton;
	GtkWidget          *template_entry;
	GtkWidget          *file_type_option_menu;
	GtkWidget          *image_map_checkbutton;
	GtkWidget          *start_at_spinbutton;

	GtkWidget          *header_entry;
	GtkWidget          *footer_entry;

	GtkWidget          *progress_dialog;
	GtkWidget          *progress_progressbar;
	GtkWidget          *progress_info;
	GtkWidget          *progress_cancel;

	GtkWidget          *btn_ok;

	CatalogPngExporter *exporter;
} DialogData;


/* called when the main dialog is closed. */
static void
destroy_cb (GtkWidget  *widget,
	    DialogData *data)
{
	g_object_unref (data->gui);
	if (data->exporter)
		g_object_unref (data->exporter);
	g_free (data);
}


static void
export (GtkWidget  *widget,
	DialogData *data)
{
	CatalogPngExporter *exporter = data->exporter;
	char               *dir;
	int                 type_id;
	guint32             bg_color;
	guint32             hgrad1;
	guint32             hgrad2;
	guint32             vgrad1;
	guint32             vgrad2;
	char               *template;
	char               *file_type;
	char               *color;
	char               *s;
	guint8              caption;
	char               *type_extension;

	eel_gconf_set_boolean (PREF_EXP_WRITE_IMAGE_MAP, gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (data->image_map_checkbutton)));

	eel_gconf_set_string (PREF_EXP_NAME_TEMPLATE, gtk_entry_get_text (GTK_ENTRY (data->template_entry)));

	eel_gconf_set_integer (PREF_EXP_START_FROM, gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (data->start_at_spinbutton)));

	type_id = gtk_option_menu_get_history (GTK_OPTION_MENU (data->file_type_option_menu));
	switch (type_id) {
	case 0:
		type_extension = "png"; break;
	case 1:
	default:
		type_extension = "jpeg"; break;
	}
	eel_gconf_set_string (PREF_EXP_FILE_TYPE, type_extension);

	eel_gconf_set_string (PREF_EXP_PAGE_HEADER_TEXT, gtk_entry_get_text (GTK_ENTRY (data->header_entry)));

	eel_gconf_set_string (PREF_EXP_PAGE_FOOTER_TEXT, gtk_entry_get_text (GTK_ENTRY (data->footer_entry)));

	/**/

	dir = gtk_file_chooser_get_uri (GTK_FILE_CHOOSER (data->dest_filechooserbutton));
	if (! dlg_check_folder (GTH_WINDOW (data->browser), dir)) {
		g_free (dir);
		return;
	}

	gtk_widget_hide (data->dialog);

	catalog_png_exporter_set_location (exporter, dir);
	g_free (dir);

	file_type = eel_gconf_get_string (PREF_EXP_FILE_TYPE, DEF_FILE_TYPE);
	catalog_png_exporter_set_file_type (exporter, file_type);
	g_free (file_type);

	template = eel_gconf_get_string (PREF_EXP_NAME_TEMPLATE, DEF_NAME_TEMPLATE);
        /* Bug fix: old default value was "###%e" for some reason.
           The "%e" wasn't even used or processed, and with the
           current code it actually causes a crash. Thus we 
           override anything with an "%" in it with the proper,
           safe default. */
        if (strstr (template, "%")) {
                g_free (template);
                template = g_strdup (DEF_NAME_TEMPLATE);
		eel_gconf_set_string (PREF_EXP_NAME_TEMPLATE, DEF_NAME_TEMPLATE);
        }
	catalog_png_exporter_set_name_template (exporter, template);

	catalog_png_exporter_set_start_at (exporter, eel_gconf_get_integer (PREF_EXP_START_FROM, 1));

	/* Page options. */

	if (eel_gconf_get_boolean (PREF_EXP_PAGE_SIZE_USE_RC, TRUE))
		catalog_png_exporter_set_page_size_row_col (exporter, eel_gconf_get_integer (PREF_EXP_PAGE_ROWS, DEF_ROWS), eel_gconf_get_integer (PREF_EXP_PAGE_COLS, DEF_COLS));
	else
		catalog_png_exporter_set_page_size (exporter, eel_gconf_get_integer (PREF_EXP_PAGE_WIDTH, DEF_PAGE_WIDTH), eel_gconf_get_integer (PREF_EXP_PAGE_HEIGHT, DEF_PAGE_HEIGHT));
	catalog_png_exporter_all_pages_same_size (exporter, eel_gconf_get_boolean (PREF_EXP_PAGE_SAME_SIZE, TRUE));

	color = eel_gconf_get_string (PREF_EXP_PAGE_BGCOLOR, "#62757b");
	bg_color = pref_util_get_int_value (color);
	g_free (color);

	color = eel_gconf_get_string (PREF_EXP_PAGE_HGRAD_COLOR1, "#e0d3c0");
	hgrad1 = pref_util_get_int_value (color);
	g_free (color);

	color = eel_gconf_get_string (PREF_EXP_PAGE_HGRAD_COLOR2, "#b1c3ad");
	hgrad2 = pref_util_get_int_value (color);
	g_free (color);

	color = eel_gconf_get_string (PREF_EXP_PAGE_VGRAD_COLOR1, "#e8e8ea");
	vgrad1 = pref_util_get_int_value (color);
	g_free (color);

	color = eel_gconf_get_string (PREF_EXP_PAGE_VGRAD_COLOR2, "#bad8d8");
	vgrad2 = pref_util_get_int_value (color);
	g_free (color);

	catalog_png_exporter_set_page_color (exporter,
					     eel_gconf_get_boolean (PREF_EXP_PAGE_USE_SOLID_COLOR, FALSE),
					     eel_gconf_get_boolean (PREF_EXP_PAGE_USE_HGRADIENT, TRUE),
					     eel_gconf_get_boolean (PREF_EXP_PAGE_USE_VGRADIENT, TRUE),
					     bg_color,
					     hgrad1, hgrad2,
					     vgrad1, vgrad2);

	catalog_png_exporter_set_sort_method (exporter, pref_get_exp_arrange_type ());
	catalog_png_exporter_set_sort_type (exporter, pref_get_exp_sort_order ());

	s = eel_gconf_get_string (PREF_EXP_PAGE_HEADER_TEXT, "");
	if (s != NULL && strcmp (s, "") == 0)
		catalog_png_exporter_set_header (exporter, NULL);
	else
		catalog_png_exporter_set_header (exporter, s);
	g_free (s);

	s = eel_gconf_get_string (PREF_EXP_PAGE_HEADER_FONT, "Arial 22");
	catalog_png_exporter_set_header_font (exporter, s);
	g_free (s);

	s = eel_gconf_get_string (PREF_EXP_PAGE_HEADER_COLOR, "#d5504a");
	catalog_png_exporter_set_header_color (exporter, s);
	g_free (s);

	s = eel_gconf_get_string (PREF_EXP_PAGE_FOOTER_TEXT, "");
	if (s != NULL && strcmp (s, "") == 0)
		catalog_png_exporter_set_footer (exporter, NULL);
	else
		catalog_png_exporter_set_footer (exporter, s);
	g_free (s);

	s = eel_gconf_get_string (PREF_EXP_PAGE_FOOTER_FONT, "Arial Bold Italic 12");
	catalog_png_exporter_set_footer_font (exporter, s);
	g_free (s);

	s = eel_gconf_get_string (PREF_EXP_PAGE_FOOTER_COLOR, "#394083");
	catalog_png_exporter_set_footer_color (exporter, s);
	g_free (s);

	/* Thumbnails options. */

	caption = 0;
	if (eel_gconf_get_boolean (PREF_EXP_SHOW_COMMENT, FALSE))
		caption |= GTH_CAPTION_COMMENT;
	if (eel_gconf_get_boolean (PREF_EXP_SHOW_PATH, FALSE))
		caption |= GTH_CAPTION_FILE_PATH;
	if (eel_gconf_get_boolean (PREF_EXP_SHOW_NAME, FALSE))
		caption |= GTH_CAPTION_FILE_NAME;
	if (eel_gconf_get_boolean (PREF_EXP_SHOW_SIZE, FALSE))
		caption |= GTH_CAPTION_FILE_SIZE;
	if (eel_gconf_get_boolean (PREF_EXP_SHOW_IMAGE_DIM, FALSE))
		caption |= GTH_CAPTION_IMAGE_DIM;

	catalog_png_exporter_set_caption (exporter, caption);
	catalog_png_exporter_set_frame_style (exporter, pref_get_exporter_frame_style ());

	color = eel_gconf_get_string (PREF_EXP_FRAME_COLOR, "#94d6cd");
	catalog_png_exporter_set_frame_color (exporter, color);
	g_free (color);

	catalog_png_exporter_set_thumb_size (exporter, eel_gconf_get_integer (PREF_EXP_THUMB_SIZE, DEF_THUMB_SIZE), eel_gconf_get_integer (PREF_EXP_THUMB_SIZE, DEF_THUMB_SIZE));

	color = eel_gconf_get_string (PREF_EXP_TEXT_COLOR, "#414141");
	catalog_png_exporter_set_caption_color (exporter, color);
	g_free (color);

	s = eel_gconf_get_string (PREF_EXP_TEXT_FONT, "Arial Bold 12");
	catalog_png_exporter_set_caption_font (exporter, s);
	g_free (s);

	catalog_png_exporter_write_image_map (exporter, eel_gconf_get_boolean (PREF_EXP_WRITE_IMAGE_MAP, FALSE));

	/* Export. */

	gtk_window_set_transient_for (GTK_WINDOW (data->progress_dialog),
				      GTK_WINDOW (data->browser));
	gtk_window_set_modal (GTK_WINDOW (data->progress_dialog), FALSE);
	gtk_widget_show_all (data->progress_dialog);

	catalog_png_exporter_export (exporter);
}


static void
export_done (GtkObject  *object,
	     DialogData *data)
{
	gtk_widget_destroy (data->progress_dialog);
	gtk_widget_destroy (data->dialog);
}


static void
export_progress (GtkObject  *object,
		 float       percent,
		 DialogData *data)
{
	gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (data->progress_progressbar), percent);
}


static void
export_info (GtkObject  *object,
	     const char *info,
	     DialogData *data)
{
	gtk_label_set_text (GTK_LABEL (data->progress_info), info);
}


static void dlg_png_exporter_pref (DialogData *data);


static void
popup_pref_dialog (GtkWidget  *widget,
		   DialogData *data)
{
	dlg_png_exporter_pref (data);
}


/* create the main dialog. */
void
dlg_exporter (GthBrowser *browser)
{
	DialogData   *data;
	GtkWidget    *btn_cancel;
	GtkWidget    *btn_pref;
	GList        *list;
	char         *template;
	char         *s;

	data = g_new (DialogData, 1);

	data->browser = browser;

	list = gth_window_get_file_list_selection_as_fd (GTH_WINDOW (browser));
	if (list == NULL) {
		g_warning ("No file selected.");
		g_free (data);
		return;
	}

	data->exporter = catalog_png_exporter_new (list);
	file_data_list_free (list);

	data->gui = glade_xml_new (GTHUMB_GLADEDIR "/" GLADE_EXPORTER_FILE, NULL, NULL);
        if (!data->gui) {
		g_object_unref (data->exporter);
		g_free (data);
                g_warning ("Could not find " GLADE_EXPORTER_FILE "\n");
                return;
        }

	/* Get the widgets. */

	data->dialog = glade_xml_get_widget (data->gui, "exporter_save_dialog");
	data->dest_filechooserbutton = glade_xml_get_widget (data->gui, "dest_filechooserbutton");
	data->template_entry = glade_xml_get_widget (data->gui, "template_entry");
	data->file_type_option_menu = glade_xml_get_widget (data->gui, "type_optionmenu");

	data->progress_dialog = glade_xml_get_widget (data->gui, "progress_dialog");
	data->progress_progressbar = glade_xml_get_widget (data->gui, "progress_progressbar");
	data->progress_info = glade_xml_get_widget (data->gui, "progress_info");
	data->progress_cancel = glade_xml_get_widget (data->gui, "progress_cancel");

	data->image_map_checkbutton = glade_xml_get_widget (data->gui, "image_map_checkbutton");
	data->start_at_spinbutton = glade_xml_get_widget (data->gui, "start_at_spinbutton");

	data->header_entry = glade_xml_get_widget (data->gui, "header_entry");
	data->footer_entry = glade_xml_get_widget (data->gui, "footer_entry");

        btn_cancel = glade_xml_get_widget (data->gui, "cancel_button");
	data->btn_ok = glade_xml_get_widget (data->gui, "ok_button");
        btn_pref = glade_xml_get_widget (data->gui, "pref_button");

	/* Signals. */

	g_signal_connect (G_OBJECT (data->dialog),
			  "destroy",
			  G_CALLBACK (destroy_cb),
			  data);
	g_signal_connect_swapped (G_OBJECT (btn_cancel),
				  "clicked",
				  G_CALLBACK (gtk_widget_destroy),
				  G_OBJECT (data->dialog));
	g_signal_connect (G_OBJECT (data->btn_ok),
			  "clicked",
			  G_CALLBACK (export),
			  data);
	g_signal_connect (G_OBJECT (btn_pref),
			  "clicked",
			  G_CALLBACK (popup_pref_dialog),
			  data);

	g_signal_connect (G_OBJECT (data->exporter),
			  "png_exporter_done",
			  G_CALLBACK (export_done),
			  data);
	g_signal_connect (G_OBJECT (data->exporter),
			  "png_exporter_progress",
			  G_CALLBACK (export_progress),
			  data);
	g_signal_connect (G_OBJECT (data->exporter),
			  "png_exporter_info",
			  G_CALLBACK (export_info),
			  data);

	g_signal_connect_swapped (G_OBJECT (data->progress_dialog),
				  "delete_event",
				  G_CALLBACK (catalog_png_exporter_interrupt),
				  data->exporter);
	g_signal_connect_swapped (G_OBJECT (data->progress_cancel),
				  "clicked",
				  G_CALLBACK (catalog_png_exporter_interrupt),
				  data->exporter);

	/* Set widgets data. */

	/**/

	gtk_file_chooser_set_current_folder_uri (GTK_FILE_CHOOSER (data->dest_filechooserbutton),
						 gth_browser_get_current_directory (browser));

	template = eel_gconf_get_string (PREF_EXP_NAME_TEMPLATE, DEF_NAME_TEMPLATE);
	if (template != NULL)
		gtk_entry_set_text (GTK_ENTRY (data->template_entry),
				    template);
	else
		_gtk_entry_set_locale_text (GTK_ENTRY (data->template_entry), "###");
	g_free (template);

	gtk_spin_button_set_value (GTK_SPIN_BUTTON (data->start_at_spinbutton),
				   eel_gconf_get_integer (PREF_EXP_START_FROM, 1));

	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (data->image_map_checkbutton), eel_gconf_get_boolean (PREF_EXP_WRITE_IMAGE_MAP, FALSE));

	s = eel_gconf_get_string (PREF_EXP_FILE_TYPE, DEF_FILE_TYPE);
	if (s != NULL) {
		if (strcmp (s, "png") == 0)
			gtk_option_menu_set_history (GTK_OPTION_MENU (data->file_type_option_menu), 0);
		else if (strcmp (s, "jpeg") == 0)
			gtk_option_menu_set_history (GTK_OPTION_MENU (data->file_type_option_menu), 1);
		g_free (s);
	}

	s = eel_gconf_get_string (PREF_EXP_PAGE_HEADER_TEXT, "");
	if (s != NULL)
		gtk_entry_set_text (GTK_ENTRY (data->header_entry), s);
	g_free (s);

	s = eel_gconf_get_string (PREF_EXP_PAGE_FOOTER_TEXT, "");
	if (s != NULL)
		gtk_entry_set_text (GTK_ENTRY (data->footer_entry), s);
	g_free (s);

	/* run dialog. */

	gtk_widget_grab_focus (data->template_entry);

	gtk_window_set_transient_for (GTK_WINDOW (data->dialog), GTK_WINDOW (browser));
	gtk_window_set_modal (GTK_WINDOW (data->dialog), TRUE);
	gtk_widget_show_all (data->dialog);
}


/* -- Exporter Preferences -- */

typedef struct {
	GladeXML  *gui;
	GtkWidget *dialog;

	GtkWidget *solid_color_radiobutton;
	GtkWidget *gradient_radiobutton;

	GtkWidget *hgrad_checkbutton;
	GtkWidget *vgrad_checkbutton;

	GtkWidget *page_bg_colorpicker;
	GtkWidget *hgrad1_colorpicker;
	GtkWidget *hgrad2_colorpicker;
	GtkWidget *vgrad1_colorpicker;
	GtkWidget *vgrad2_colorpicker;

	GtkWidget *hgrad_swap_button;
	GtkWidget *vgrad_swap_button;

	GtkWidget *sort_method_combobox;
	GtkWidget *sort_type_checkbutton;
	GtkWidget *width_entry;
	GtkWidget *height_entry;
	GtkWidget *rows_spinbutton;
	GtkWidget *cols_spinbutton;
	GtkWidget *same_size_checkbutton;
	GtkWidget *pixel_size_radiobutton;
	GtkWidget *row_col_size_radiobutton;
	GtkWidget *rows_cols_table;
	GtkWidget *pixel_hbox;

	GtkWidget *comment_checkbutton;
	GtkWidget *filepath_checkbutton;
	GtkWidget *filename_checkbutton;
	GtkWidget *filesize_checkbutton;
	GtkWidget *image_dim_checkbutton;

	GtkWidget *frame_style_optionmenu;
	GtkWidget *frame_colorpicker;
	GtkWidget *draw_frame_checkbutton;
	GtkWidget *prop_frame_table;
	GtkWidget *prop_thumb_preview;

	GtkWidget *header_fontpicker;
	GtkWidget *header_colorpicker;
	GtkWidget *footer_fontpicker;
	GtkWidget *footer_colorpicker;

	GtkWidget *thumb_size_optionmenu;
	GtkWidget *text_colorpicker;
	GtkWidget *text_fontpicker;

	GdkPixmap *pixmap;
} PrefDialogData;


/* called when the pref dialog is closed. */
static void
pref_destroy_cb (GtkWidget *widget,
		 PrefDialogData *data)
{
	if (data->pixmap != NULL)
		g_object_unref (data->pixmap);
        g_object_unref (G_OBJECT (data->gui));
	g_free (data);
}


/* get the option menu index from the size value. */
static gint
get_idx_from_size (gint size)
{
	switch (size) {
	case 48:  return 0;
	case 64:  return 1;
	case 75:  return 2;
	case 85:  return 3;
	case 95:  return 4;
	case 112: return 5;
	case 128: return 6;
	case 164: return 7;
	case 200: return 8;
	case 256: return 9;
	}

	return -1;
}


static gint
get_size_from_idx (gint idx)
{
	switch (idx) {
	case 0: return 48;
	case 1: return 64;
	case 2: return 75;
	case 3: return 85;
	case 4: return 95;
	case 5: return 112;
	case 6: return 128;
	case 7: return 164;
	case 8: return 200;
	case 9: return 256;
	}

	return 0;
}


/* get the option menu index from the frame style. */
static gint
get_idx_from_style (GthFrameStyle style)
{
	switch (style) {
	case GTH_FRAME_STYLE_NONE: return 0;
	case GTH_FRAME_STYLE_SIMPLE: return 0;
	case GTH_FRAME_STYLE_SIMPLE_WITH_SHADOW: return 1;
	case GTH_FRAME_STYLE_SHADOW: return 2;
	case GTH_FRAME_STYLE_SLIDE: return 3;
	case GTH_FRAME_STYLE_SHADOW_IN: return 4;
	case GTH_FRAME_STYLE_SHADOW_OUT: return 5;
	}

	return -1;
}


static GthFrameStyle
get_style_from_idx (gint idx)
{
	switch (idx) {
	case 0: return GTH_FRAME_STYLE_SIMPLE;
	case 1: return GTH_FRAME_STYLE_SIMPLE_WITH_SHADOW;
	case 2: return GTH_FRAME_STYLE_SHADOW;
	case 3: return GTH_FRAME_STYLE_SLIDE;
	case 4: return GTH_FRAME_STYLE_SHADOW_IN;
	case 5: return GTH_FRAME_STYLE_SHADOW_OUT;
	}

	return GTH_FRAME_STYLE_NONE;
}


static void
paint_sample_text (GtkWidget   *drawable,
		   GdkPixmap   *pixmap,
		   GdkGC       *gc,
		   const char  *text,
		   int          x,
		   int          y,
		   int          width,
		   const char  *text_font,
		   GdkColor    *text_color)
{
	PangoFontDescription *font_desc;
	PangoLayout          *layout;

	layout = gtk_widget_create_pango_layout (drawable, text);

	pango_layout_set_width (PANGO_LAYOUT (layout), width * PANGO_SCALE);
	pango_layout_set_alignment (layout, PANGO_ALIGN_CENTER);
	pango_layout_set_wrap (layout, PANGO_WRAP_CHAR);

	font_desc = pango_font_description_from_string (text_font);
	pango_layout_set_font_description (layout, font_desc);
	pango_font_description_free (font_desc);

	gdk_gc_set_rgb_fg_color (gc, text_color);
	gdk_draw_layout (pixmap, gc, x, y, layout);

	g_object_unref (layout);
}


static int
get_text_height (GtkWidget   *drawable,
		 const char  *text,
		 const char  *font_name,
		 int          width)
{
	PangoFontDescription *font_desc;
	PangoRectangle        bounds;
	char                 *utf8_text;
	PangoLayout          *layout;

	layout = gtk_widget_create_pango_layout (drawable, text);

	font_desc = pango_font_description_from_string (font_name);
	pango_layout_set_font_description (layout, font_desc);

	utf8_text = g_locale_to_utf8 (text, -1, NULL, NULL, NULL);
	pango_layout_set_text (layout, utf8_text, strlen (utf8_text));
	g_free (utf8_text);

	pango_layout_set_width (layout, width * PANGO_SCALE);
	pango_layout_get_pixel_extents (layout, NULL, &bounds);

	if (font_desc)
		pango_font_description_free (font_desc);
	g_object_unref (layout);

	return bounds.height;
}


static void
paint_background (PrefDialogData *data,
		  GtkWidget      *drawable,
		  GdkPixmap      *pixmap,
		  int             width,
		  int             height)
{
	GdkPixbuf  *pixbuf;
	GdkColor    color;
	gboolean    use_solid_color;
	gboolean    use_hgradient, use_vgradient;
	guint32     bg_color;
	guint32     hgrad1, hgrad2;
	guint32     vgrad1, vgrad2;

	use_solid_color = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (data->solid_color_radiobutton));
	use_hgradient = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (data->hgrad_checkbutton));
	use_vgradient = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (data->vgrad_checkbutton));

	gtk_color_button_get_color (GTK_COLOR_BUTTON (data->page_bg_colorpicker),
				    &color);
	bg_color = pref_util_get_ui32_from_color (&color);

	gtk_color_button_get_color (GTK_COLOR_BUTTON (data->hgrad1_colorpicker),
				    &color);
	hgrad1 = pref_util_get_ui32_from_color (&color);

	gtk_color_button_get_color (GTK_COLOR_BUTTON (data->hgrad2_colorpicker),
				    &color);
	hgrad2 = pref_util_get_ui32_from_color (&color);

	gtk_color_button_get_color (GTK_COLOR_BUTTON (data->vgrad1_colorpicker),
				    &color);
	vgrad1 = pref_util_get_ui32_from_color (&color);

	gtk_color_button_get_color (GTK_COLOR_BUTTON (data->vgrad2_colorpicker),
				    &color);
	vgrad2 = pref_util_get_ui32_from_color (&color);

	/**/

	pixbuf = gdk_pixbuf_new (GDK_COLORSPACE_RGB, TRUE, 8, width, height);

	if (use_solid_color)
		gdk_pixbuf_fill (pixbuf, bg_color);
	else {
		GdkPixbuf *tmp;

		tmp = gdk_pixbuf_new (GDK_COLORSPACE_RGB, 1, 8, width, height);
		gdk_pixbuf_fill (tmp, 0xFFFFFFFF);

		if (use_hgradient && use_vgradient)
			_gdk_pixbuf_hv_gradient (tmp, hgrad1, hgrad2, vgrad1, vgrad2);

		else if (use_hgradient)
			_gdk_pixbuf_horizontal_gradient (tmp, hgrad1, hgrad2);

		else if (use_vgradient)
			_gdk_pixbuf_vertical_gradient (tmp, vgrad1, vgrad2);

		gdk_pixbuf_composite (tmp, pixbuf,
				      0, 0, width, height,
				      0, 0,
				      1.0, 1.0,
				      GDK_INTERP_NEAREST, 255);

		g_object_unref (tmp);
	}

	gdk_draw_rgb_32_image_dithalign (pixmap,
                                         drawable->style->black_gc,
                                         0, 0, width, height,
                                         GDK_RGB_DITHER_MAX,
                                         gdk_pixbuf_get_pixels (pixbuf),
                                         gdk_pixbuf_get_rowstride (pixbuf),
                                         0, 0);
        g_object_unref (pixbuf);
}


static void
update_thumb_preview (PrefDialogData *data,
		      gboolean        recalc)
{
	GtkWidget            *drawable = data->prop_thumb_preview;
	GthFrameStyle         frame_style;
	GdkColor              frame_color;
	GdkColor              bg_color;
	GdkColor              text_color;
	int                   thumb_size;
	int                   x, y, width, height;
	GdkGC                *gc;
	GdkColor              white, black, gray, dark_gray;
	int                   frame_x, frame_y, frame_size;
	int                   image_x, image_y, image_width, image_height;
	int                   border;
	const char           *text_font;

	if (! GTK_WIDGET_REALIZED (drawable))
		return;

	if ((data->pixmap != NULL) && ! recalc) {
		gdk_draw_drawable (drawable->window,
				   drawable->style->black_gc,
				   data->pixmap,
				   0, 0,
				   0, 0,
				   drawable->allocation.width,
				   drawable->allocation.height);
		return;
	}

	if (data->pixmap != NULL)
		g_object_unref (data->pixmap);

	data->pixmap = gdk_pixmap_new (drawable->window,
				       drawable->allocation.width,
				       drawable->allocation.height,
				       -1);

	/* Get current values. */

	frame_style = get_style_from_idx (gtk_option_menu_get_history (GTK_OPTION_MENU (data->frame_style_optionmenu)));
	if (! gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (data->draw_frame_checkbutton)))
		frame_style = GTH_FRAME_STYLE_NONE;

	gtk_color_button_get_color (GTK_COLOR_BUTTON (data->frame_colorpicker),
				    &frame_color);

	gtk_color_button_get_color (GTK_COLOR_BUTTON (data->page_bg_colorpicker),
				    &bg_color);

	thumb_size = get_size_from_idx (gtk_option_menu_get_history (GTK_OPTION_MENU (data->thumb_size_optionmenu)));

	/* Init colors. */

	gc = gdk_gc_new (drawable->window);

	gdk_color_parse ("#777777", &black);
	gdk_color_parse ("#AAAAAA", &dark_gray);
	gdk_color_parse ("#CCCCCC", &gray);
	gdk_color_parse ("#FFFFFF", &white);

	/* Clear. */

	paint_background (data,
			  drawable,
			  data->pixmap,
			  drawable->allocation.width,
			  drawable->allocation.height);

	/* Paint the frame. */

	border = 8;
	frame_size = thumb_size + (border * 2);
	image_width = thumb_size - (thumb_size / 3);
	image_height = thumb_size;

	frame_x = (drawable->allocation.width - frame_size) / 2;
	frame_y = (drawable->allocation.height - frame_size) / 2;

	image_x = frame_x + (frame_size - image_width) / 2 + 1;
	image_y = frame_y + (frame_size - image_height) / 2 + 1;

	switch (frame_style) {
	case GTH_FRAME_STYLE_NONE:
		break;

	case GTH_FRAME_STYLE_SLIDE:
		gdk_gc_set_rgb_fg_color (gc, &frame_color);
		gthumb_draw_slide_with_colors (frame_x, frame_y,
					       frame_size, frame_size,
					       image_width, image_height,
					       data->pixmap,
					       &frame_color,
					       &black,
					       &dark_gray,
					       &gray,
					       &white);
		break;

	case GTH_FRAME_STYLE_SIMPLE:
	case GTH_FRAME_STYLE_SIMPLE_WITH_SHADOW:
	case GTH_FRAME_STYLE_SHADOW:
		if (frame_style == GTH_FRAME_STYLE_SHADOW)
			gthumb_draw_image_shadow (image_x, image_y,
						  image_width, image_height,
						  data->pixmap);

		if (frame_style == GTH_FRAME_STYLE_SIMPLE_WITH_SHADOW)
			gthumb_draw_frame_shadow (image_x, image_y,
						  image_width, image_height,
						  data->pixmap);

		if ((frame_style == GTH_FRAME_STYLE_SIMPLE)
		    || (frame_style == GTH_FRAME_STYLE_SIMPLE_WITH_SHADOW)) {
			gthumb_draw_frame (image_x, image_y,
					   image_width, image_height,
					   data->pixmap,
					   &frame_color);
		}
		break;

	case GTH_FRAME_STYLE_SHADOW_IN:
		gthumb_draw_image_shadow_in (image_x,
					     image_y,
					     image_width,
					     image_height,
					     data->pixmap);
		break;

	case GTH_FRAME_STYLE_SHADOW_OUT:
		gthumb_draw_image_shadow_out (image_x,
					      image_y,
					      image_width,
					      image_height,
					      data->pixmap);
		break;
	}

	/* Paint Caption. */
	gtk_color_button_get_color (GTK_COLOR_BUTTON (data->text_colorpicker),
				    &text_color);
	text_font = gtk_font_button_get_font_name (GTK_FONT_BUTTON (data->text_fontpicker));

	paint_sample_text (drawable,
			   data->pixmap,
			   gc,
			   _("Image Caption"),
			   frame_x,
			   frame_y + frame_size + 5,
			   frame_size,
			   text_font,
			   &text_color);

	/* Paint 'image'. */

	x = image_x;
	y = image_y;
	width = image_width;
	height = image_height;

	gdk_gc_set_rgb_fg_color (gc, &white);
	gdk_draw_rectangle (data->pixmap,
			    gc,
			    TRUE,
			    x,
			    y,
			    width,
			    height);

	gdk_gc_set_rgb_fg_color (gc, &text_color);
	if (frame_style == GTH_FRAME_STYLE_NONE)
		gdk_draw_rectangle (data->pixmap,
				    gc,
				    FALSE,
				    x,
				    y,
				    width,
				    height);
	gdk_draw_line (data->pixmap,
		       gc,
		       x,
		       y,
		       x + width - 1,
		       y + height - 1);
	gdk_draw_line (data->pixmap,
		       gc,
		       x + width - 1,
		       y,
		       x,
		       y + height - 1);

	/* Paint Header. */

	gtk_color_button_get_color (GTK_COLOR_BUTTON (data->header_colorpicker),
				    &text_color);
	text_font = gtk_font_button_get_font_name (GTK_FONT_BUTTON (data->header_fontpicker));

	paint_sample_text (drawable,
			   data->pixmap,
			   gc,
			   _("Header"),
			   0,
			   ROW_SPACING,
			   drawable->allocation.width,
			   text_font,
			   &text_color);

	/* Paint Footer. */
	gtk_color_button_get_color (GTK_COLOR_BUTTON (data->footer_colorpicker),
				    &text_color);
	text_font = gtk_font_button_get_font_name (GTK_FONT_BUTTON (data->footer_fontpicker));

	y = (drawable->allocation.height - get_text_height (drawable,
							    _("Footer"),
							    text_font,
							    drawable->allocation.width));
	paint_sample_text (drawable,
			   data->pixmap,
			   gc,
			   _("Footer"),
			   0,
			   y - ROW_SPACING,
			   drawable->allocation.width,
			   text_font,
			   &text_color);

	gdk_draw_drawable (drawable->window,
			   gc,
			   data->pixmap,
			   0, 0,
			   0, 0,
			   drawable->allocation.width,
			   drawable->allocation.height);

	/* Release data. */

	g_object_unref (gc);
}


static void
preview_expose_cb (GtkWidget      *widget,
		   GdkEventExpose *event,
		   PrefDialogData *data)
{
	update_thumb_preview (data, FALSE);
}


static void
color_button_color_set_cb (GtkColorButton *color_button,
			   PrefDialogData *data)
{
	update_thumb_preview (data, TRUE);
}


static void
optionmenu_changed_cb (GtkWidget      *widget,
		       PrefDialogData *data)
{
	update_thumb_preview (data, TRUE);
}


static void
font_button_font_set_cb (GtkFontButton   *font_button,
			 PrefDialogData  *data)
{
	update_thumb_preview (data, TRUE);
}


static void
toggled_cb (GtkWidget      *widget,
	    PrefDialogData *data)
{
	update_thumb_preview (data, TRUE);
}


static void
hgrad_toggled_cb (GtkWidget      *widget,
		  PrefDialogData *data)
{
	gboolean state;

	state = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (widget));

	gtk_widget_set_sensitive (data->hgrad1_colorpicker, state);
	gtk_widget_set_sensitive (data->hgrad2_colorpicker, state);
	gtk_widget_set_sensitive (data->hgrad_swap_button, state);

	update_thumb_preview (data, TRUE);
}


static void
vgrad_toggled_cb (GtkWidget      *widget,
		  PrefDialogData *data)
{
	gboolean state;

	state = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (widget));

	gtk_widget_set_sensitive (data->vgrad1_colorpicker, state);
	gtk_widget_set_sensitive (data->vgrad2_colorpicker, state);
	gtk_widget_set_sensitive (data->vgrad_swap_button, state);

	update_thumb_preview (data, TRUE);
}


/* called when the "ok" button is clicked. */
static void
ok_cb (GtkWidget *widget,
       PrefDialogData *data)
{
	GdkColor color;
	char    *s;

	/* Page */

	s = _gtk_entry_get_locale_text (GTK_ENTRY (data->width_entry));
	eel_gconf_set_integer (PREF_EXP_PAGE_WIDTH, atoi (s));
	g_free (s);

	s = _gtk_entry_get_locale_text (GTK_ENTRY (data->height_entry));
	eel_gconf_set_integer (PREF_EXP_PAGE_HEIGHT, atoi (s));
	g_free (s);

	eel_gconf_set_boolean (PREF_EXP_PAGE_SIZE_USE_RC, gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (data->row_col_size_radiobutton)));
	eel_gconf_set_boolean (PREF_EXP_PAGE_SAME_SIZE, gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (data->same_size_checkbutton)));
	eel_gconf_set_integer (PREF_EXP_PAGE_ROWS, gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (data->rows_spinbutton)));
	eel_gconf_set_integer (PREF_EXP_PAGE_COLS, gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (data->cols_spinbutton)));

	gtk_color_button_get_color (GTK_COLOR_BUTTON (data->page_bg_colorpicker),
				    &color);
	eel_gconf_set_string (PREF_EXP_PAGE_BGCOLOR, pref_util_get_hex_value (color.red, color.green, color.blue));

	/**/

	eel_gconf_set_boolean (PREF_EXP_PAGE_USE_SOLID_COLOR, gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (data->solid_color_radiobutton)));
	eel_gconf_set_boolean (PREF_EXP_PAGE_USE_HGRADIENT, gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (data->hgrad_checkbutton)));
	eel_gconf_set_boolean (PREF_EXP_PAGE_USE_VGRADIENT, gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (data->vgrad_checkbutton)));

	/* gradients  */

	gtk_color_button_get_color (GTK_COLOR_BUTTON (data->hgrad1_colorpicker),
				    &color);
	eel_gconf_set_string (PREF_EXP_PAGE_HGRAD_COLOR1, pref_util_get_hex_value (color.red, color.green, color.blue));

	gtk_color_button_get_color (GTK_COLOR_BUTTON (data->hgrad2_colorpicker),
				    &color);
	eel_gconf_set_string (PREF_EXP_PAGE_HGRAD_COLOR2, pref_util_get_hex_value (color.red, color.green, color.blue));

	gtk_color_button_get_color (GTK_COLOR_BUTTON (data->vgrad1_colorpicker),
				    &color);
	eel_gconf_set_string (PREF_EXP_PAGE_VGRAD_COLOR1, pref_util_get_hex_value (color.red, color.green, color.blue));

	gtk_color_button_get_color (GTK_COLOR_BUTTON (data->vgrad2_colorpicker),
				    &color);
	eel_gconf_set_string (PREF_EXP_PAGE_VGRAD_COLOR2, pref_util_get_hex_value (color.red, color.green, color.blue));

	if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (data->sort_type_checkbutton)))
		pref_set_exp_sort_order (GTK_SORT_DESCENDING);
	else
		pref_set_exp_sort_order (GTK_SORT_ASCENDING);
	pref_set_exp_arrange_type (idx_to_sort_method [gtk_combo_box_get_active (GTK_COMBO_BOX (data->sort_method_combobox))]);

	/* Thumbnails */

	/* ** Caption */

	eel_gconf_set_boolean (PREF_EXP_SHOW_COMMENT, gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (data->comment_checkbutton)));
	eel_gconf_set_boolean (PREF_EXP_SHOW_PATH, gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (data->filepath_checkbutton)));
	eel_gconf_set_boolean (PREF_EXP_SHOW_NAME, gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (data->filename_checkbutton)));
	eel_gconf_set_boolean (PREF_EXP_SHOW_SIZE, gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (data->filesize_checkbutton)));
	eel_gconf_set_boolean (PREF_EXP_SHOW_IMAGE_DIM, gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (data->image_dim_checkbutton)));

	/* ** Frame */

	pref_set_exporter_frame_style (get_style_from_idx (gtk_option_menu_get_history (GTK_OPTION_MENU (data->frame_style_optionmenu))));

	gtk_color_button_get_color (GTK_COLOR_BUTTON (data->frame_colorpicker),
				    &color);
	eel_gconf_set_string (PREF_EXP_FRAME_COLOR, pref_util_get_hex_value (color.red, color.green, color.blue));

	if (! gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (data->draw_frame_checkbutton)))
		pref_set_exporter_frame_style (GTH_FRAME_STYLE_NONE);

	/* ** Others */

	eel_gconf_set_integer (PREF_EXP_THUMB_SIZE, get_size_from_idx (gtk_option_menu_get_history (GTK_OPTION_MENU (data->thumb_size_optionmenu))));

	gtk_color_button_get_color (GTK_COLOR_BUTTON (data->text_colorpicker),
				    &color);
	eel_gconf_set_string (PREF_EXP_TEXT_COLOR, pref_util_get_hex_value (color.red, color.green, color.blue));

	eel_gconf_set_string (PREF_EXP_TEXT_FONT, gtk_font_button_get_font_name (GTK_FONT_BUTTON (data->text_fontpicker)));

	/* Header / Footer */

	eel_gconf_set_string (PREF_EXP_PAGE_HEADER_FONT, gtk_font_button_get_font_name (GTK_FONT_BUTTON (data->header_fontpicker)));

	gtk_color_button_get_color (GTK_COLOR_BUTTON (data->header_colorpicker),
				    &color);
	eel_gconf_set_string (PREF_EXP_PAGE_HEADER_COLOR, pref_util_get_hex_value (color.red, color.green, color.blue));

	eel_gconf_set_string (PREF_EXP_PAGE_FOOTER_FONT, gtk_font_button_get_font_name (GTK_FONT_BUTTON (data->footer_fontpicker)));

	gtk_color_button_get_color (GTK_COLOR_BUTTON (data->footer_colorpicker),
				    &color);
	eel_gconf_set_string (PREF_EXP_PAGE_FOOTER_COLOR, pref_util_get_hex_value (color.red, color.green, color.blue));

	/* Close */

	gtk_widget_destroy (data->dialog);
}


static void
use_pixel_cb (GtkWidget *widget,
	      PrefDialogData *data)
{
	if (! GTK_TOGGLE_BUTTON (widget)->active)
		return;

	gtk_widget_set_sensitive (data->pixel_hbox, TRUE);
	gtk_widget_set_sensitive (data->rows_cols_table, FALSE);
}


static void
use_row_col_cb (GtkWidget *widget,
		PrefDialogData *data)
{
	if (! GTK_TOGGLE_BUTTON (widget)->active)
		return;

	gtk_widget_set_sensitive (data->rows_cols_table, TRUE);
	gtk_widget_set_sensitive (data->pixel_hbox, FALSE);
}


static void
use_solid_color_cb (GtkWidget *widget,
		    PrefDialogData *data)
{
	if (! GTK_TOGGLE_BUTTON (widget)->active)
		return;

	gtk_widget_set_sensitive (data->page_bg_colorpicker, TRUE);
	gtk_widget_set_sensitive (data->hgrad_checkbutton, FALSE);
	gtk_widget_set_sensitive (data->vgrad_checkbutton, FALSE);

	gtk_widget_set_sensitive (data->hgrad1_colorpicker, FALSE);
	gtk_widget_set_sensitive (data->hgrad2_colorpicker, FALSE);
	gtk_widget_set_sensitive (data->vgrad1_colorpicker, FALSE);
	gtk_widget_set_sensitive (data->vgrad2_colorpicker, FALSE);

	gtk_widget_set_sensitive (data->hgrad_swap_button, FALSE);
	gtk_widget_set_sensitive (data->vgrad_swap_button, FALSE);
}


static void
use_gradient_cb (GtkWidget *widget,
		 PrefDialogData *data)
{
	gboolean state;

	if (! GTK_TOGGLE_BUTTON (widget)->active)
		return;

	gtk_widget_set_sensitive (data->page_bg_colorpicker, FALSE);
	gtk_widget_set_sensitive (data->hgrad_checkbutton, TRUE);
	gtk_widget_set_sensitive (data->vgrad_checkbutton, TRUE);

	state = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (data->hgrad_checkbutton));

	gtk_widget_set_sensitive (data->hgrad1_colorpicker, state);
	gtk_widget_set_sensitive (data->hgrad2_colorpicker, state);
	gtk_widget_set_sensitive (data->hgrad_swap_button, state);

	state = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (data->vgrad_checkbutton));

	gtk_widget_set_sensitive (data->vgrad1_colorpicker, state);
	gtk_widget_set_sensitive (data->vgrad2_colorpicker, state);
	gtk_widget_set_sensitive (data->vgrad_swap_button, state);
}


static void
radio_button_clicked_cb (GtkWidget *widget,
			 PrefDialogData *data)
{
	if (! GTK_TOGGLE_BUTTON (widget)->active)
		return;
	update_thumb_preview (data, TRUE);
}


static void
draw_frame_toggled_cb (GtkWidget *button,
		       PrefDialogData *data)
{
	gtk_widget_set_sensitive (data->prop_frame_table, gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (data->draw_frame_checkbutton)));
}


static void
hgrad_swap_cb (GtkWidget      *button,
	       PrefDialogData *data)
{
	GdkColor color1, color2;

	gtk_color_button_get_color (GTK_COLOR_BUTTON (data->hgrad1_colorpicker),
				    &color1);
	gtk_color_button_get_color (GTK_COLOR_BUTTON (data->hgrad2_colorpicker),
				    &color2);

	gtk_color_button_set_color (GTK_COLOR_BUTTON (data->hgrad1_colorpicker),
				    &color2);
	gtk_color_button_set_color (GTK_COLOR_BUTTON (data->hgrad2_colorpicker),
				    &color1);

	update_thumb_preview (data, TRUE);
}


static void
vgrad_swap_cb (GtkWidget      *button,
	       PrefDialogData *data)
{
	GdkColor color1, color2;

	gtk_color_button_get_color (GTK_COLOR_BUTTON (data->vgrad1_colorpicker),
				    &color1);
	gtk_color_button_get_color (GTK_COLOR_BUTTON (data->vgrad2_colorpicker),
				    &color2);

	gtk_color_button_set_color (GTK_COLOR_BUTTON (data->vgrad1_colorpicker),
				    &color2);
	gtk_color_button_set_color (GTK_COLOR_BUTTON (data->vgrad2_colorpicker),
				    &color1);

	update_thumb_preview (data, TRUE);
}


/* create the exporter preferences dialog. */
static void
dlg_png_exporter_pref (DialogData *ddata)
{
	PrefDialogData *data;
	GtkWidget      *btn_ok;
	GtkWidget      *btn_cancel;
	GtkWidget      *image;
	char            s[10];
	char           *v;
	gboolean        use_rc, active;
	GdkColor        color;
	gboolean        reorderable;
	int             idx;

	data = g_new0 (PrefDialogData, 1);

	data->gui = glade_xml_new (GTHUMB_GLADEDIR "/" GLADE_EXPORTER_FILE, NULL, NULL);
        if (!data->gui) {
                g_warning ("Could not find " GLADE_EXPORTER_FILE "\n");
		g_free (data);
                return;
        }

	eel_gconf_preload_cache ("/apps/gthumb/exporter", GCONF_CLIENT_PRELOAD_RECURSIVE);

	/* Get the widgets. */

	data->dialog = glade_xml_get_widget (data->gui, "exporter_property_dialog");

	data->solid_color_radiobutton = glade_xml_get_widget (data->gui, "solid_color_radiobutton");
	data->gradient_radiobutton = glade_xml_get_widget (data->gui, "gradient_radiobutton");

	data->hgrad_checkbutton = glade_xml_get_widget (data->gui, "hgrad_checkbutton");
	data->vgrad_checkbutton = glade_xml_get_widget (data->gui, "vgrad_checkbutton");

	data->hgrad_swap_button = glade_xml_get_widget (data->gui, "hgrad_swap_button");
	data->vgrad_swap_button = glade_xml_get_widget (data->gui, "vgrad_swap_button");

	data->page_bg_colorpicker = glade_xml_get_widget (data->gui, "page_bg_colorpicker");
	data->hgrad1_colorpicker = glade_xml_get_widget (data->gui, "hgrad1_colorpicker");
	data->hgrad2_colorpicker = glade_xml_get_widget (data->gui, "hgrad2_colorpicker");
	data->vgrad1_colorpicker = glade_xml_get_widget (data->gui, "vgrad1_colorpicker");
	data->vgrad2_colorpicker = glade_xml_get_widget (data->gui, "vgrad2_colorpicker");
	data->sort_method_combobox = glade_xml_get_widget (data->gui, "sort_method_combobox");
	data->sort_type_checkbutton = glade_xml_get_widget (data->gui, "sort_type_checkbutton");
	data->width_entry = glade_xml_get_widget (data->gui, "width_entry");
	data->height_entry = glade_xml_get_widget (data->gui, "height_entry");
	data->rows_spinbutton = glade_xml_get_widget (data->gui, "rows_spinbutton");
	data->cols_spinbutton = glade_xml_get_widget (data->gui, "cols_spinbutton");
	data->same_size_checkbutton = glade_xml_get_widget (data->gui, "same_size_checkbutton");
	data->pixel_size_radiobutton = glade_xml_get_widget (data->gui, "pixel_size_radiobutton");
	data->row_col_size_radiobutton = glade_xml_get_widget (data->gui, "row_col_size_radiobutton");
	data->rows_cols_table = glade_xml_get_widget (data->gui, "rows_cols_table");
	data->pixel_hbox = glade_xml_get_widget (data->gui, "pixel_hbox");
	data->comment_checkbutton = glade_xml_get_widget (data->gui, "comment_checkbutton");
	data->filepath_checkbutton = glade_xml_get_widget (data->gui, "filepath_checkbutton");
	data->filename_checkbutton = glade_xml_get_widget (data->gui, "filename_checkbutton");
	data->filesize_checkbutton = glade_xml_get_widget (data->gui, "filesize_checkbutton");
	data->image_dim_checkbutton = glade_xml_get_widget (data->gui, "image_dim_checkbutton");

	data->frame_style_optionmenu = glade_xml_get_widget (data->gui, "frame_style_optionmenu");
	data->frame_colorpicker = glade_xml_get_widget (data->gui, "frame_colorpicker");
	data->draw_frame_checkbutton = glade_xml_get_widget (data->gui, "draw_frame_checkbutton");
	data->prop_frame_table = glade_xml_get_widget (data->gui, "prop_frame_table");
	data->prop_thumb_preview = glade_xml_get_widget (data->gui, "prop_thumb_preview");

	data->thumb_size_optionmenu = glade_xml_get_widget (data->gui, "thumb_size_optionmenu");
	data->text_colorpicker = glade_xml_get_widget (data->gui, "text_colorpicker");
	data->text_fontpicker = glade_xml_get_widget (data->gui, "text_fontpicker");

	data->header_fontpicker = glade_xml_get_widget (data->gui, "header_fontpicker");
	data->header_colorpicker = glade_xml_get_widget (data->gui, "header_colorpicker");

	data->footer_fontpicker = glade_xml_get_widget (data->gui, "footer_fontpicker");
	data->footer_colorpicker = glade_xml_get_widget (data->gui, "footer_colorpicker");

	btn_ok = glade_xml_get_widget (data->gui, "prop_ok_button");
        btn_cancel = glade_xml_get_widget (data->gui, "prop_cancel_button");

	/**/

	image = glade_xml_get_widget (data->gui, "hgrad_swap_image");
	gtk_image_set_from_stock (GTK_IMAGE (image), GTHUMB_STOCK_SWAP, GTK_ICON_SIZE_MENU);

	image = glade_xml_get_widget (data->gui, "vgrad_swap_image");
	gtk_image_set_from_stock (GTK_IMAGE (image), GTHUMB_STOCK_SWAP, GTK_ICON_SIZE_MENU);

	/* Signals. */

	g_signal_connect (G_OBJECT (data->dialog),
			  "destroy",
			  G_CALLBACK (pref_destroy_cb),
			  data);
	g_signal_connect_swapped (G_OBJECT (btn_cancel),
				  "clicked",
				  G_CALLBACK (gtk_widget_destroy),
				  G_OBJECT (data->dialog));
	g_signal_connect (G_OBJECT (btn_ok),
			  "clicked",
			  G_CALLBACK (ok_cb),
			  data);
	g_signal_connect (G_OBJECT (data->pixel_size_radiobutton),
			  "toggled",
			  G_CALLBACK (use_pixel_cb),
			  data);
	g_signal_connect (G_OBJECT (data->row_col_size_radiobutton),
			  "toggled",
			  G_CALLBACK (use_row_col_cb),
			  data);
	g_signal_connect (G_OBJECT (data->solid_color_radiobutton),
			  "toggled",
			  G_CALLBACK (use_solid_color_cb),
			  data);
	g_signal_connect (G_OBJECT (data->gradient_radiobutton),
			  "toggled",
			  G_CALLBACK (use_gradient_cb),
			  data);
	g_signal_connect (G_OBJECT (data->prop_thumb_preview),
			  "expose_event",
			  G_CALLBACK (preview_expose_cb),
			  data);

	g_signal_connect (G_OBJECT (data->page_bg_colorpicker),
			  "color_set",
			  G_CALLBACK (color_button_color_set_cb),
			  data);
	g_signal_connect (G_OBJECT (data->hgrad1_colorpicker),
			  "color_set",
			  G_CALLBACK (color_button_color_set_cb),
			  data);
	g_signal_connect (G_OBJECT (data->hgrad2_colorpicker),
			  "color_set",
			  G_CALLBACK (color_button_color_set_cb),
			  data);
	g_signal_connect (G_OBJECT (data->vgrad1_colorpicker),
			  "color_set",
			  G_CALLBACK (color_button_color_set_cb),
			  data);
	g_signal_connect (G_OBJECT (data->vgrad2_colorpicker),
			  "color_set",
			  G_CALLBACK (color_button_color_set_cb),
			  data);

	g_signal_connect (G_OBJECT (data->frame_colorpicker),
			  "color_set",
			  G_CALLBACK (color_button_color_set_cb),
			  data);
	g_signal_connect (G_OBJECT (data->text_colorpicker),
			  "color_set",
			  G_CALLBACK (color_button_color_set_cb),
			  data);
	g_signal_connect (G_OBJECT (data->header_colorpicker),
			  "color_set",
			  G_CALLBACK (color_button_color_set_cb),
			  data);
	g_signal_connect (G_OBJECT (data->footer_colorpicker),
			  "color_set",
			  G_CALLBACK (color_button_color_set_cb),
			  data);

	g_signal_connect (G_OBJECT (data->frame_style_optionmenu),
			  "changed",
			  G_CALLBACK (optionmenu_changed_cb),
			  data);
	g_signal_connect (G_OBJECT (data->thumb_size_optionmenu),
			  "changed",
			  G_CALLBACK (optionmenu_changed_cb),
			  data);

	g_signal_connect (G_OBJECT (data->text_fontpicker),
			  "font_set",
			  G_CALLBACK (font_button_font_set_cb),
			  data);
	g_signal_connect (G_OBJECT (data->header_fontpicker),
			  "font_set",
			  G_CALLBACK (font_button_font_set_cb),
			  data);
	g_signal_connect (G_OBJECT (data->footer_fontpicker),
			  "font_set",
			  G_CALLBACK (font_button_font_set_cb),
			  data);

	g_signal_connect (G_OBJECT (data->draw_frame_checkbutton),
			  "toggled",
			  G_CALLBACK (toggled_cb),
			  data);

	g_signal_connect (G_OBJECT (data->hgrad_checkbutton),
			  "toggled",
			  G_CALLBACK (hgrad_toggled_cb),
			  data);
	g_signal_connect (G_OBJECT (data->vgrad_checkbutton),
			  "toggled",
			  G_CALLBACK (vgrad_toggled_cb),
			  data);

	g_signal_connect (G_OBJECT (data->solid_color_radiobutton),
			  "toggled",
			  G_CALLBACK (radio_button_clicked_cb),
			  data);
	g_signal_connect (G_OBJECT (data->gradient_radiobutton),
			  "toggled",
			  G_CALLBACK (radio_button_clicked_cb),
			  data);

	g_signal_connect (G_OBJECT (data->hgrad_swap_button),
			  "clicked",
			  G_CALLBACK (hgrad_swap_cb),
			  data);
	g_signal_connect (G_OBJECT (data->vgrad_swap_button),
			  "clicked",
			  G_CALLBACK (vgrad_swap_cb),
			  data);


	/* Set widget data. */

	/* * Page */

	/* background color */

	v = eel_gconf_get_string (PREF_EXP_PAGE_BGCOLOR, "#62757b");
	pref_util_get_color_from_hex (v, &color);
	gtk_color_button_set_color (GTK_COLOR_BUTTON (data->page_bg_colorpicker),
				    &color);
	g_free (v);

	v = eel_gconf_get_string (PREF_EXP_PAGE_HGRAD_COLOR1, "#e0d3c0");
	pref_util_get_color_from_hex (v, &color);
	gtk_color_button_set_color (GTK_COLOR_BUTTON (data->hgrad1_colorpicker),
				    &color);
	g_free (v);

	v = eel_gconf_get_string (PREF_EXP_PAGE_HGRAD_COLOR2, "#b1c3ad");
	pref_util_get_color_from_hex (v, &color);
	gtk_color_button_set_color (GTK_COLOR_BUTTON (data->hgrad2_colorpicker),
				    &color);
	g_free (v);

	v = eel_gconf_get_string (PREF_EXP_PAGE_VGRAD_COLOR1, "#e8e8ea");
	pref_util_get_color_from_hex (v, &color);
	gtk_color_button_set_color (GTK_COLOR_BUTTON (data->vgrad1_colorpicker),
				    &color);
	g_free (v);

	v = eel_gconf_get_string (PREF_EXP_PAGE_VGRAD_COLOR2, "#bad8d8");
	pref_util_get_color_from_hex (v, &color);
	gtk_color_button_set_color (GTK_COLOR_BUTTON (data->vgrad2_colorpicker),
				    &color);
	g_free (v);

	/* sort type */

	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (data->sort_type_checkbutton), pref_get_exp_sort_order () == GTK_SORT_DESCENDING);

	gtk_combo_box_append_text (GTK_COMBO_BOX (data->sort_method_combobox),
				   _("by path"));
	gtk_combo_box_append_text (GTK_COMBO_BOX (data->sort_method_combobox),
				   _("by size"));
	gtk_combo_box_append_text (GTK_COMBO_BOX (data->sort_method_combobox),
				   _("by file modified time"));
	gtk_combo_box_append_text (GTK_COMBO_BOX (data->sort_method_combobox),
				   _("by Exif DateTime tag"));
	gtk_combo_box_append_text (GTK_COMBO_BOX (data->sort_method_combobox),
                                   _("by comment"));

	reorderable = gth_file_view_get_reorderable (gth_browser_get_file_view (ddata->browser));
	if (reorderable)
		gtk_combo_box_append_text (GTK_COMBO_BOX (data->sort_method_combobox),
					   _("manual order"));

	idx = sort_method_to_idx [pref_get_exp_arrange_type ()];
	if (!reorderable && (sort_method_to_idx[GTH_SORT_METHOD_MANUAL] == idx))
		idx = sort_method_to_idx[GTH_SORT_METHOD_BY_NAME];
	gtk_combo_box_set_active (GTK_COMBO_BOX (data->sort_method_combobox), idx);

	/* page size */

	sprintf (s, "%d", eel_gconf_get_integer (PREF_EXP_PAGE_WIDTH, DEF_PAGE_WIDTH));
	_gtk_entry_set_locale_text (GTK_ENTRY (data->width_entry), s);

	sprintf (s, "%d", eel_gconf_get_integer (PREF_EXP_PAGE_HEIGHT, DEF_PAGE_HEIGHT));
	_gtk_entry_set_locale_text (GTK_ENTRY (data->height_entry), s);

	gtk_spin_button_set_value (GTK_SPIN_BUTTON (data->rows_spinbutton),
				   eel_gconf_get_integer (PREF_EXP_PAGE_ROWS, DEF_ROWS));
	gtk_spin_button_set_value (GTK_SPIN_BUTTON (data->cols_spinbutton),
				   eel_gconf_get_integer (PREF_EXP_PAGE_COLS, DEF_COLS));

	use_rc = eel_gconf_get_boolean (PREF_EXP_PAGE_SIZE_USE_RC, TRUE);
	if (use_rc)
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (data->row_col_size_radiobutton), TRUE);
	else
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (data->pixel_size_radiobutton), TRUE);
	gtk_widget_set_sensitive (data->rows_cols_table, use_rc);
	gtk_widget_set_sensitive (data->pixel_hbox, ! use_rc);

	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (data->same_size_checkbutton), eel_gconf_get_boolean (PREF_EXP_PAGE_SAME_SIZE, TRUE));

	/**/

	if (eel_gconf_get_boolean (PREF_EXP_PAGE_USE_SOLID_COLOR, FALSE)) {
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (data->solid_color_radiobutton), TRUE);
		use_solid_color_cb (data->solid_color_radiobutton, data);
	} else {
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (data->gradient_radiobutton), TRUE);
		use_gradient_cb (data->gradient_radiobutton, data);
	}

	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (data->hgrad_checkbutton), eel_gconf_get_boolean (PREF_EXP_PAGE_USE_HGRADIENT, TRUE));
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (data->vgrad_checkbutton), eel_gconf_get_boolean (PREF_EXP_PAGE_USE_VGRADIENT, TRUE));

	/* * Thumbnails */

	/* ** Caption */

	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (data->comment_checkbutton), eel_gconf_get_boolean (PREF_EXP_SHOW_COMMENT, FALSE));

	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (data->filepath_checkbutton), eel_gconf_get_boolean (PREF_EXP_SHOW_PATH, FALSE));

	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (data->filename_checkbutton), eel_gconf_get_boolean (PREF_EXP_SHOW_NAME, FALSE));

	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (data->filesize_checkbutton), eel_gconf_get_boolean (PREF_EXP_SHOW_SIZE, FALSE));

	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (data->image_dim_checkbutton), eel_gconf_get_boolean (PREF_EXP_SHOW_IMAGE_DIM, FALSE));

	/* ** Frame */

	gtk_option_menu_set_history (GTK_OPTION_MENU (data->frame_style_optionmenu), get_idx_from_style (pref_get_exporter_frame_style ()));

	v = eel_gconf_get_string (PREF_EXP_FRAME_COLOR, "#94d6cd");
	pref_util_get_color_from_hex (v, &color);
	gtk_color_button_set_color (GTK_COLOR_BUTTON (data->frame_colorpicker),
				    &color);
	g_free (v);

	active = pref_get_exporter_frame_style () != GTH_FRAME_STYLE_NONE;
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (data->draw_frame_checkbutton), active);
	gtk_widget_set_sensitive (data->prop_frame_table, active);

	/* ** Others */

	gtk_option_menu_set_history (GTK_OPTION_MENU (data->thumb_size_optionmenu), get_idx_from_size (eel_gconf_get_integer (PREF_EXP_THUMB_SIZE, DEF_THUMB_SIZE)));

	v = eel_gconf_get_string (PREF_EXP_TEXT_COLOR, "#414141");
	pref_util_get_color_from_hex (v, &color);
	gtk_color_button_set_color (GTK_COLOR_BUTTON (data->text_colorpicker),
				    &color);
	g_free (v);

	v = eel_gconf_get_string (PREF_EXP_TEXT_FONT, "Arial Bold 12");
	gtk_font_button_set_font_name (GTK_FONT_BUTTON (data->text_fontpicker), v);
	g_free (v);

	/* * Header/Footer */

	v = eel_gconf_get_string (PREF_EXP_PAGE_HEADER_FONT, "Arial 22");
	gtk_font_button_set_font_name (GTK_FONT_BUTTON (data->header_fontpicker), v);
	g_free (v);

	v = eel_gconf_get_string (PREF_EXP_PAGE_HEADER_COLOR, "#d5504a");
	pref_util_get_color_from_hex (v, &color);
	gtk_color_button_set_color (GTK_COLOR_BUTTON (data->header_colorpicker),
				    &color);
	g_free (v);

	v = eel_gconf_get_string (PREF_EXP_PAGE_FOOTER_FONT, "Arial Bold Italic 12");
	gtk_font_button_set_font_name (GTK_FONT_BUTTON (data->footer_fontpicker), v);
	g_free (v);

	v = eel_gconf_get_string (PREF_EXP_PAGE_FOOTER_COLOR, "#394083");
	pref_util_get_color_from_hex (v, &color);
	gtk_color_button_set_color (GTK_COLOR_BUTTON (data->footer_colorpicker),
				    &color);
	g_free (v);

	/* Signals */

	g_signal_connect (G_OBJECT (data->draw_frame_checkbutton),
			  "toggled",
			  G_CALLBACK (draw_frame_toggled_cb),
			  data);

	/* run dialog. */

	gtk_window_set_transient_for (GTK_WINDOW (data->dialog),
				      GTK_WINDOW (ddata->dialog));
	gtk_window_set_modal (GTK_WINDOW (data->dialog), TRUE);
	gtk_widget_show_all (data->dialog);
}

