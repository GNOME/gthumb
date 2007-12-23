/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2006 Free Software Foundation, Inc.
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
#include <math.h>

#include <glib/gi18n.h>
#include <gtk/gtk.h>
#include <glade/glade.h>
#include <libgnomecanvas/libgnomecanvas.h>

#include "glib-utils.h"
#include "gtk-utils.h"
#include "gth-utils.h"
#include "gth-window.h"
#include "gconf-utils.h"
#include "preferences.h"
#include "pixbuf-utils.h"
#include "gth-iviewer.h"
#include "gth-image-history.h"
#include "gth-image-selector.h"
#include "image-viewer.h"
#include "gthumb-stock.h"
#include "typedefs.h"
#include "gth-nav-window.h"

#define REGION_SEARCH_SIZE 3
#define GLADE_FILE     "gthumb_redeye.glade"

static const double RED_FACTOR = 0.5133333;
static const double GREEN_FACTOR  = 1.0;
static const double BLUE_FACTOR =  0.1933333;

/**/

typedef struct {
	GthWindow       *window;
	GladeXML        *gui;

	GdkPixbuf       *pixbuf;
	int              image_width, image_height;
	char            *is_red;
	GthImageHistory *history;

	GtkWidget       *dialog;
	GtkWidget       *nav_win;
	GtkWidget       *nav_container;
	GtkWidget       *image_selector;
	GtkSpinButton   *redeye_x_spinbutton;
	GtkSpinButton   *redeye_y_spinbutton;
	GtkWidget       *undo_button;
	GtkWidget       *redo_button;
	GtkAdjustment   *hadj, *vadj;
} DialogData;


/* called when the main dialog is closed. */
static void
destroy_cb (GtkWidget  *widget,
	    DialogData *data)
{
	g_free (data->is_red);
	g_object_unref (data->history);
	g_object_unref (data->gui);
	g_free (data);
}



static void
apply_cb (DialogData *data,
	  gboolean    save)
{
	GdkPixbuf *old_pixbuf;
	GdkPixbuf *new_pixbuf;

	new_pixbuf = gth_image_selector_get_pixbuf (GTH_IMAGE_SELECTOR (data->image_selector));
	old_pixbuf = gth_window_get_image_pixbuf (data->window);
	if (new_pixbuf != old_pixbuf) {
		gth_window_set_image_pixbuf (data->window, new_pixbuf);
		if (save)
			gth_window_save_pixbuf (data->window, new_pixbuf, gth_window_get_image_data (data->window));
		else
			gth_window_set_image_modified (data->window, TRUE);
	}

	gtk_widget_destroy (data->dialog);
}


static void
ok_cb (GtkWidget  *widget,
       DialogData *data)
{
	apply_cb (data, FALSE);
}


static void
save_cb (GtkWidget  *widget,
	 DialogData *data)
{
	apply_cb (data, TRUE);
}


static void
help_cb (GtkWidget  *widget,
	 DialogData *data)
{
	gthumb_display_help (GTK_WINDOW (data->dialog), "gthumb-redeye");
}


static void
image_selector_motion_notify_cb (GthImageSelector *selector,
			   	 int               x,
			   	 int               y,
				    DialogData       *data)
{
	gtk_spin_button_set_value (GTK_SPIN_BUTTON (data->redeye_x_spinbutton), x);
	gtk_spin_button_set_value (GTK_SPIN_BUTTON (data->redeye_y_spinbutton), y);
}


static int
find_region (int   row,
 	     int   col,
 	     int  *rtop,
 	     int  *rbot,
	     int  *rleft,
	     int  *rright,
	     char *isred,
	     int   width,
	     int   height)
{
	int *rows, *cols, list_length = 0;
	int  mydir;
	int  total = 1;

	/* a relatively efficient way to find all connected points in a
	 * region.  It considers points that have isred == 1, sets them to 2
	 * if they are connected to the starting point.
	 * row and col are the starting point of the region,
	 * the next four params define a rectangle our region fits into.
	 * isred is an array that tells us if pixels are red or not.
	 */

	*rtop = row;
	*rbot = row;
	*rleft = col;
	*rright = col;

	rows = g_malloc (width * height * sizeof(int));
	cols = g_malloc (width * height * sizeof(int));

	rows[0] = row;
	cols[0] = col;
	list_length = 1;

	do {
		list_length -= 1;
		row = rows[list_length];
		col = cols[list_length];
		for (mydir = 0; mydir < 8 ; mydir++){
			switch (mydir) {
			case 0:
				/*  going left */
				if (col - 1 < 0) break;
				if (isred[col-1+row*width] == 1) {
					isred[col-1+row*width] = 2;
					if (*rleft > col-1) *rleft = col-1;
					rows[list_length] = row;
					cols[list_length] = col-1;
					list_length+=1;
					total += 1;
				}
				break;
			case 1:
				/* up and left */
				if (col - 1 < 0 || row -1 < 0 ) break;
				if (isred[col-1+(row-1)*width] == 1 ) {
					isred[col-1+(row-1)*width] = 2;
					if (*rleft > col -1) *rleft = col-1;
					if (*rtop > row -1) *rtop = row-1;
					rows[list_length] = row-1;
					cols[list_length] = col-1;
					list_length += 1;
					total += 1;
				}
				break;
			case 2:
				/* up */
				if (row -1 < 0 ) break;
				if (isred[col + (row-1)*width] == 1) {
					isred[col + (row-1)*width] = 2;
					if (*rtop > row-1) *rtop=row-1;
					rows[list_length] = row-1;
					cols[list_length] = col;
					list_length +=1;
					total += 1;
				}
				break;
			case 3:
				/*  up and right */
				if (col + 1 >= width || row -1 < 0 ) break;
				if (isred[col+1+(row-1)*width] == 1) {
					isred[col+1+(row-1)*width] = 2;
					if (*rright < col +1) *rright = col+1;
					if (*rtop > row -1) *rtop = row-1;
					rows[list_length] = row-1;
					cols[list_length] = col+1;
					list_length += 1;
					total +=1;
				}
				break;
			case 4:
				/* going right */
				if (col + 1 >= width) break;
				if (isred[col+1+row*width] == 1) {
					isred[col+1+row*width] = 2;
					if (*rright < col+1) *rright = col+1;
					rows[list_length] = row;
					cols[list_length] = col+1;
					list_length += 1;
					total += 1;
				}
				break;
			case 5:
				/* down and right */
				if (col + 1 >= width || row +1 >= height ) break;
				if (isred[col+1+(row+1)*width] ==1) {
					isred[col+1+(row+1)*width] = 2;
					if (*rright < col +1) *rright = col+1;
					if (*rbot < row +1) *rbot = row+1;
					rows[list_length] = row+1;
					cols[list_length] = col+1;
					list_length += 1;
					total += 1;
				}
				break;
			case 6:
				/* down */
				if (row +1 >=  height ) break;
				if (isred[col + (row+1)*width] == 1) {
					isred[col + (row+1)*width] = 2;
					if (*rbot < row+1) *rbot=row+1;
					rows[list_length] = row+1;
					cols[list_length] = col;
					list_length += 1;
					total += 1;
				}
				break;
			case 7:
				/* down and left */
				if (col - 1 < 0  || row +1 >= height ) break;
				if (isred[col-1+(row+1)*width] == 1) {
					isred[col-1+(row+1)*width] = 2;
					if (*rleft  > col -1) *rleft = col-1;
					if (*rbot < row +1) *rbot = row+1;
					rows[list_length] = row+1;
					cols[list_length] = col-1;
					list_length += 1;
					total += 1;
				}
				break;
			default:
				break;
			}
		}

	} while (list_length > 0);  /* stop when we add no more */

	g_free (rows);
	g_free (cols);

	return total;
}


/* returns TRUE if the pixbuf has been modified */
static gboolean
fix_redeye (GdkPixbuf *pixbuf,
	    char      *isred,
	    int        x,
	    int        y)
{
	gboolean  region_fixed = FALSE;
	int       width;
	int       height;
	int       rowstride;
	int       channels;
	guchar   *pixels;
	int       search, i, j, ii, jj;
	int       ad_red, ad_blue, ad_green;
	int       rtop, rbot, rleft, rright; /* edges of region */
	int       num_pix;

	width = gdk_pixbuf_get_width (pixbuf);
	height = gdk_pixbuf_get_height (pixbuf);
	rowstride = gdk_pixbuf_get_rowstride (pixbuf);
	channels = gdk_pixbuf_get_n_channels (pixbuf);
	pixels = gdk_pixbuf_get_pixels (pixbuf);

	/*
   	 * if isred is 0, we don't think the point is red, 1 means red, 2 means
   	 * part of our region already.
   	 */

	for (search = 0; ! region_fixed && (search < REGION_SEARCH_SIZE); search++)
    		for (i = MAX (0, y - search); ! region_fixed && (i <= MIN (height - 1, y + search)); i++ )
      			for (j = MAX (0, x - search); ! region_fixed && (j <= MIN (width - 1, x + search)); j++) {
				if (isred[j + i * width] == 0)
					continue;

				isred[j + i * width] = 2;

				num_pix = find_region (i, j, &rtop, &rbot, &rleft, &rright, isred, width, height);

				/* Fix the region. */
				for (ii = rtop; ii <= rbot; ii++)
					for (jj = rleft; jj <= rright; jj++)
						if (isred[jj + ii * width] == 2) { /* Fix the pixel. */
							int ofs;

							ofs = channels*jj + ii*rowstride;
							ad_red = pixels[ofs] * RED_FACTOR;
							ad_green = pixels[ofs + 1] * GREEN_FACTOR;
							ad_blue = pixels[ofs + 2] * BLUE_FACTOR;

							pixels[ofs] = ((float) (ad_green + ad_blue)) / (2.0 * RED_FACTOR);

							isred[jj + ii * width] = 0;
						}

				region_fixed = TRUE;
      			}

	return region_fixed;
}


static void
init_is_red (DialogData *data)
{
	GdkPixbuf *pixbuf;
	int        width, height;
	int        rowstride, channels;
	guchar    *pixels;
	int        i, j;
	int        ad_red, ad_green, ad_blue;
	const int  THRESHOLD = 0;

	pixbuf = gth_image_selector_get_pixbuf (GTH_IMAGE_SELECTOR (data->image_selector));

	width = gdk_pixbuf_get_width (pixbuf);
	height = gdk_pixbuf_get_height (pixbuf);
	rowstride = gdk_pixbuf_get_rowstride (pixbuf);
	channels = gdk_pixbuf_get_n_channels (pixbuf);
	pixels = gdk_pixbuf_get_pixels(pixbuf);

	if (data->is_red != NULL)
		g_free (data->is_red);
	data->is_red = g_new0 (char, width * height);

	for (i = 0; i < height; i++)  {
		for (j = 0; j < width; j++) {
			int ofs = channels * j + i * rowstride;

      			ad_red = pixels[ofs] * RED_FACTOR;
			ad_green = pixels[ofs + 1] * GREEN_FACTOR;
			ad_blue = pixels[ofs + 2] * BLUE_FACTOR;

			//  This test from the gimp redeye plugin.

			if ((ad_red >= ad_green - THRESHOLD) && (ad_red >= ad_blue - THRESHOLD))
				data->is_red[j + i * width] = 1;
     		}
	}
}


static void
image_selector_clicked_cb (GthImageSelector *selector,
			   int               x,
			   int               y,
			   DialogData       *data)
{
	GdkPixbuf *old_pixbuf;
	GdkPixbuf *new_pixbuf;

	old_pixbuf = gth_image_selector_get_pixbuf (GTH_IMAGE_SELECTOR (data->image_selector));
	new_pixbuf = gdk_pixbuf_copy (old_pixbuf);

	if (fix_redeye (new_pixbuf, data->is_red, x, y)) {
		gth_image_history_add_image (data->history, old_pixbuf, FALSE);
		gth_image_selector_set_pixbuf (GTH_IMAGE_SELECTOR (data->image_selector), new_pixbuf);
		init_is_red (data);
	}

	g_object_unref (new_pixbuf);
}


static void
zoom_in_button_clicked_cb (GtkButton  *button,
			   DialogData *data)
{
	gth_iviewer_zoom_in (GTH_IVIEWER (data->image_selector));
}


static void
zoom_out_button_clicked_cb (GtkButton  *button,
			    DialogData *data)
{
	gth_iviewer_zoom_out (GTH_IVIEWER (data->image_selector));
}


static void
zoom_100_button_clicked_cb (GtkButton  *button,
			    DialogData *data)
{
	gth_iviewer_set_zoom (GTH_IVIEWER (data->image_selector), 1.0);
}


static void
zoom_fit_button_clicked_cb (GtkButton  *button,
			    DialogData *data)
{
	gth_iviewer_zoom_to_fit (GTH_IVIEWER (data->image_selector));
}


static void
undo_button_clicked_cb (GtkButton  *button,
	   		DialogData *data)
{
	GthImageData *idata;

	idata = gth_image_history_undo (data->history,
					gth_image_selector_get_pixbuf (GTH_IMAGE_SELECTOR (data->image_selector)),
					FALSE);
	if (idata == NULL)
		return;
	gth_image_selector_set_pixbuf (GTH_IMAGE_SELECTOR (data->image_selector), idata->image);
	init_is_red (data);

	gth_image_data_unref (idata);
}


static void
redo_button_clicked_cb (GtkButton  *button,
	   		DialogData *data)
{
	GthImageData *idata;

	idata = gth_image_history_redo (data->history,
					gth_image_selector_get_pixbuf (GTH_IMAGE_SELECTOR (data->image_selector)),
					FALSE);
	if (idata == NULL)
		return;
	gth_image_selector_set_pixbuf (GTH_IMAGE_SELECTOR (data->image_selector), idata->image);
	init_is_red (data);

	gth_image_data_unref (idata);
}


static void
history_changed_cb (GthImageHistory *history,
		    DialogData      *data)
{
	gtk_widget_set_sensitive (data->undo_button, gth_image_history_can_undo (history));
	gtk_widget_set_sensitive (data->redo_button, gth_image_history_can_redo (history));
}


void
dlg_redeye_removal (GthWindow *window)
{
	DialogData   *data;
	GtkWidget    *ok_button;
	GtkWidget    *save_button;
	GtkWidget    *cancel_button;
	GtkWidget    *help_button;
	GtkWidget    *zoom_in_button, *zoom_out_button, *zoom_100_button, *zoom_fit_button;
	int	      width, height;

	data = g_new0 (DialogData, 1);
	data->window = window;
	data->gui = glade_xml_new (GTHUMB_GLADEDIR "/" GLADE_FILE, NULL, NULL);

	if (! data->gui) {
		g_warning ("Could not find " GLADE_FILE "\n");
		g_free (data);
		return;
	}

	data->history = gth_image_history_new ();

	/* Get the widgets. */

	data->dialog = glade_xml_get_widget (data->gui, "redeye_dialog");
	data->nav_container = glade_xml_get_widget (data->gui, "nav_container");

	data->redeye_x_spinbutton = (GtkSpinButton*) glade_xml_get_widget (data->gui, "redeye_x_spinbutton");
	data->redeye_y_spinbutton = (GtkSpinButton*) glade_xml_get_widget (data->gui, "redeye_y_spinbutton");

	data->undo_button = glade_xml_get_widget (data->gui, "redeye_undo_button");
	data->redo_button = glade_xml_get_widget (data->gui, "redeye_redo_button");
	zoom_in_button = glade_xml_get_widget (data->gui, "redeye_zoom_in_button");
	zoom_out_button = glade_xml_get_widget (data->gui, "redeye_zoom_out_button");
	zoom_100_button = glade_xml_get_widget (data->gui, "redeye_zoom_100_button");
	zoom_fit_button = glade_xml_get_widget (data->gui, "redeye_zoom_fit_button");

	ok_button = glade_xml_get_widget (data->gui, "redeye_okbutton");
	save_button = glade_xml_get_widget (data->gui, "redeye_savebutton");
	cancel_button = glade_xml_get_widget (data->gui, "redeye_cancelbutton");
	help_button = glade_xml_get_widget (data->gui, "redeye_helpbutton");

	data->image_selector = gth_image_selector_new (GTH_SELECTOR_TYPE_POINT, NULL);

	/* Set widgets data. */

	data->pixbuf = image_viewer_get_current_pixbuf (gth_window_get_image_viewer (data->window));
	data->image_width = gdk_pixbuf_get_width (data->pixbuf);
	data->image_height = gdk_pixbuf_get_height (data->pixbuf);
	gth_image_selector_set_pixbuf (GTH_IMAGE_SELECTOR (data->image_selector), data->pixbuf);

	data->nav_win = gth_nav_window_new (GTH_IVIEWER (data->image_selector));
	gtk_container_add (GTK_CONTAINER (data->nav_container), data->nav_win);

	gtk_widget_show_all (data->nav_container);

	/**/

	init_is_red (data);
	gtk_spin_button_set_range (GTK_SPIN_BUTTON (data->redeye_x_spinbutton), 0, data->image_width);
	gtk_spin_button_set_range (GTK_SPIN_BUTTON (data->redeye_y_spinbutton), 0, data->image_height);

	/* Set the signals handlers. */

	g_signal_connect (G_OBJECT (data->dialog),
			  "destroy",
			  G_CALLBACK (destroy_cb),
			  data);
	g_signal_connect_swapped (G_OBJECT (cancel_button),
				  "clicked",
				  G_CALLBACK (gtk_widget_destroy),
				  data->dialog);
	g_signal_connect (G_OBJECT (ok_button),
			  "clicked",
			  G_CALLBACK (ok_cb),
			  data);
	g_signal_connect (G_OBJECT (save_button),
			  "clicked",
			  G_CALLBACK (save_cb),
			  data);
	g_signal_connect (G_OBJECT (help_button),
			  "clicked",
			  G_CALLBACK (help_cb),
			  data);
	g_signal_connect (G_OBJECT (data->image_selector),
			  "motion_notify",
			  G_CALLBACK (image_selector_motion_notify_cb),
			  data);
	g_signal_connect (G_OBJECT (data->image_selector),
			  "clicked",
			  G_CALLBACK (image_selector_clicked_cb),
			  data);

	g_signal_connect (G_OBJECT (data->undo_button),
			  "clicked",
			  G_CALLBACK (undo_button_clicked_cb),
			  data);
	g_signal_connect (G_OBJECT (data->redo_button),
			  "clicked",
			  G_CALLBACK (redo_button_clicked_cb),
			  data);
	g_signal_connect (G_OBJECT (zoom_in_button),
			  "clicked",
			  G_CALLBACK (zoom_in_button_clicked_cb),
			  data);
	g_signal_connect (G_OBJECT (zoom_out_button),
			  "clicked",
			  G_CALLBACK (zoom_out_button_clicked_cb),
			  data);
	g_signal_connect (G_OBJECT (zoom_100_button),
			  "clicked",
			  G_CALLBACK (zoom_100_button_clicked_cb),
			  data);
	g_signal_connect (G_OBJECT (zoom_fit_button),
			  "clicked",
			  G_CALLBACK (zoom_fit_button_clicked_cb),
			  data);
	g_signal_connect (G_OBJECT (data->history),
			  "changed",
			  G_CALLBACK (history_changed_cb),
			  data);

	/* Run dialog. */

	gtk_widget_realize (data->dialog);

	get_screen_size (GTK_WINDOW (window), &width, &height);

	gtk_window_set_default_size (GTK_WINDOW (data->dialog),
				     width * 7 / 10,
				     height * 6 / 10);

	gth_iviewer_zoom_to_fit (GTH_IVIEWER (data->image_selector));
	gtk_widget_grab_focus (data->image_selector);

	gtk_window_set_transient_for (GTK_WINDOW (data->dialog),
				      GTK_WINDOW (window));
	gtk_window_set_modal (GTK_WINDOW (data->dialog), TRUE);
	gtk_widget_show (data->dialog);
}
