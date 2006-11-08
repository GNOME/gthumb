/* Heavily modified by Marty Leisner from the file gredeye,
   in order to integrate gredeye into gthumb. The additions are under 
   the gpl. Here's the initial gredeye comment:

   --------------------------------------------

   A gtk+ program to interactively correct redeye in images.
   This program is mostly intended as a proof of concept for
   inclusion in more feature-full image viewing/organizing/editing
   packages.

   The criteria for deciding if a pixel is red and the formula
   for correcting red pixels came from the gimp redeye plugin by
   Robert Merkel, available at:
   http://registry.gimp.org/file/redeye.c?action=view&id=4215

   This program is

   Copyright (C) 2006, Carl Michal <michal at phas.ubc.ca>

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

*/

#include "config.h"
#include "gth-utils.h"
#include "gth-window.h"
#include <gtk/gtk.h>
#include <glade/glade.h>
#include <glib.h>
#include <glib/gprintf.h>
#include <glib/gi18n.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libgnome/gnome-help.h>

static  const double RED_FACTOR = 0.5133333;
static  const double GREEN_FACTOR  = 1.0;
static  const double BLUE_FACTOR =  0.1933333;

#define GLADE_FILE "gthumb_redeye.glade"


/* each  buffer maintains a red array */
typedef struct {
	char *isred;
	int  isred_size;	/* to efficiently copy array */
	GdkPixbuf *pixbuf;
} RedeyeBuf;


typedef struct {
        GthWindow 	*window;
	ImageViewer     *viewer;
        RedeyeBuf *original;	// original image
        RedeyeBuf *old;	// for potential undo operations
	RedeyeBuf *current; // if pixbuf has been modified
	GladeXML *gui;
	GtkWidget *dialog;
        GtkWidget *image_box;
	GtkWidget *image;
	gfloat scale_factor;
        int modified;
} DialogData;


static void initialize_redeye(DialogData *data, GdkPixbuf *pixbuf);
static RedeyeBuf *make_redeye_buf(char *isred, int isred_size, GdkPixbuf *pixbuf);
static RedeyeBuf *clone_redeye_buf(RedeyeBuf *p);
static void destroy_redeye_buf(RedeyeBuf *p);


static GtkWidget *get_widget(GladeXML *gui, const char *name)
{
	GtkWidget *widget;

	widget = glade_xml_get_widget(gui, name);
	g_assert(widget);
	return widget;
}


static void
destroy_cb (GtkWidget  *widget, 
	    DialogData *data)
{
	if(data->original)
		destroy_redeye_buf(data->original);
	if(data->old)
		destroy_redeye_buf(data->old);
	if(data->current)
		destroy_redeye_buf(data->current);
	g_object_unref (data->gui);
	g_free (data);
}

static void
cancel_cb (GtkWidget  *widget, 
	   DialogData *data)
{
	gtk_widget_destroy (data->dialog);
}


static gfloat compute_scale_factor(GdkPixbuf *pixbuf)
{
	int width, height;
	gfloat width_scale, height_scale;
	GdkScreen *screen;

	width = gdk_pixbuf_get_width(pixbuf);
	height = gdk_pixbuf_get_height(pixbuf);

	/* Figure out how big an image we can display on screen.
           Leave extra room in the height for the buttons. */

	screen = gdk_screen_get_default();
	width_scale = 1.1 * (gfloat) width / (gfloat) gdk_screen_get_width(screen);
	height_scale = 1.35 * (gfloat) height / (gfloat) gdk_screen_get_height(screen);

	return MAX(width_scale, height_scale);
}


static int find_region( int row, int col, int *rtop, int *rbot,
		 int *rleft, int *rright, char *isred,
		 int width, int height)
{
	int *rows,*cols,list_length=0;
	int mydir;
	int total=1;

	/* a relatively efficient way to find all connected points in a 
	 * region.  It considers points that have isred == 1, sets them to 2 
	 * if they are connected to the starting point.
	 * row and col are the starting point of the region,
	 * the next four params define a rectangle our region fits into.
	 * isred is an array that tells us if pixels are red or not.
	 */

	*rtop=row;
	*rbot=row;
	*rleft=col;
	*rright=col;
	
	rows = g_malloc(width*height*sizeof(int));
	cols = g_malloc(width*height*sizeof(int));

	rows[0] = row;
	cols[0] = col;
	list_length = 1;

	do{
		list_length -= 1;
		row = rows[list_length];
		col = cols[list_length];
		for (mydir = 0;mydir < 8 ; mydir++){
			switch (mydir){
			case 0:
				/*  going left */
				if (col - 1 < 0) break;
				if (isred[col-1+row*width] == 1){
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
				if (isred[col-1+(row-1)*width] == 1 ){
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
				if (isred[col + (row-1)*width] == 1){
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
				if (isred[col+1+(row-1)*width] == 1){
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
				if (isred[col+1+row*width] == 1){
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
				if (isred[col+1+(row+1)*width] ==1){
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
				if (isred[col + (row+1)*width] == 1){
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
				if (isred[col-1+(row+1)*width] == 1){
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
					
	}while (list_length > 0);  /*  stop when we add no more */

	g_free(rows);
	g_free(cols);
	return total;
}


/* returns the number of modifications made */
static gint fix_redeye(gint x, gint y, int search_region, char *isred, GdkPixbuf *pixbufp) 
{
	int width = gdk_pixbuf_get_width(pixbufp);
	int height = gdk_pixbuf_get_height(pixbufp);
	int rowstride = gdk_pixbuf_get_rowstride(pixbufp);
	int channels = gdk_pixbuf_get_n_channels (pixbufp);
	guchar *pixels = gdk_pixbuf_get_pixels(pixbufp);
	int i,j,search,ii,jj;
	int mods = 0;
  
	int ad_red,ad_blue,ad_green;
  
	/* edges of region */
	int rtop,rbot,rleft,rright; 
    
	int num_pix;
       
	/* 
   	* if isred is 0, we don't think the point is red, 1 means red, 2 means
   	* part of our region already.
   	*/
	
	for ( search = 0 ; search < search_region ; search++ )    
    		for( i = MAX( 0, y - search ) ; i <= MIN( height-1, y + search ) ; i++ )
      			for( j = MAX( 0, x - search ) ; j <= MIN( width-1, x + search ) ; j++ ) {
				if ( isred[j+i*width] ) {  
	  
					isred[j+i*width] = 2;
	  				mods += 1;
	  
					num_pix = find_region(i,j,&rtop,&rbot,&rleft,&rright,isred,width,height);
					
					// now fix the region.
					for (ii=rtop;ii<=rbot;ii++)
						for(jj=rleft;jj<=rright;jj++)
							if (isred[jj+ii*width] == 2) {
		
								isred[jj+ii*width] = 0;

								// actually fix the pixel
								ad_red = pixels[channels*jj + ii*rowstride] * RED_FACTOR;
								ad_green = pixels[channels*jj+ii*rowstride + 1] * GREEN_FACTOR;
								ad_blue = pixels[channels*jj+ii*rowstride + 2] * BLUE_FACTOR;
								pixels[channels*jj + ii*rowstride] = 
									(((float) (ad_green + ad_blue))/(2.0 * RED_FACTOR));
								mods++;
							}
					i=height;
					j=width; // get out of loop so we only fix once per call.
					search = search_region;
				}
			}	
	return mods;
}


/* scales pixbuf and updates image */
static void update_image(DialogData *data, GdkPixbuf *pixbuf)
{
	GdkPixbuf *scaled_pixbuf;
	int height, width;
	gfloat scale_factor;


	width = gdk_pixbuf_get_width(pixbuf);
        height = gdk_pixbuf_get_height(pixbuf);
	scale_factor = data->scale_factor;

        scaled_pixbuf = gdk_pixbuf_scale_simple(pixbuf, width/scale_factor,
				height/scale_factor, GDK_INTERP_NEAREST);

        gtk_image_set_from_pixbuf(GTK_IMAGE(data->image), scaled_pixbuf);
        g_object_unref(scaled_pixbuf);

}


static gint press_in_win_event(GtkWidget *widget, GdkEventButton *event, DialogData *pdata)
{
	gfloat scale_factor = pdata->scale_factor;
	const int REGION_SEARCH_SIZE = 3;
	int mods;
	char *new_red;
	RedeyeBuf *redeye_to_use;
	RedeyeBuf *new_buffer;
  
  	/* get either the editing pixbuf or the original pixbuf */
	if(pdata->current) 
		redeye_to_use = pdata->current;
	else redeye_to_use = pdata->original;

	new_buffer = clone_redeye_buf(redeye_to_use);
   
	mods = fix_redeye(((int) event->x)*scale_factor,
		((int) event->y)*scale_factor,
		REGION_SEARCH_SIZE*scale_factor, new_buffer->isred, new_buffer->pixbuf);

	if(mods > 0) {
		/* something was changed */
        	update_image(pdata, new_buffer->pixbuf);
		if(pdata->old) {
			destroy_redeye_buf(pdata->old);
			pdata->old = NULL;
		}

		if(pdata->current) {
			pdata->old = pdata->current;
		}
        	
		pdata->current = new_buffer;
	} 
	else destroy_redeye_buf(new_buffer); 	// nothing was done
	
  	return TRUE;
}


static gboolean undo_cb(GtkWidget *widget, DialogData *data)
{
        if(data->old) {
		// replace current with old
		destroy_redeye_buf(data->current);
		data->current = data->old;
		data->old = NULL;
		update_image(data, data->current->pixbuf);
	} else  if(data->current) {
		// no edits -- use original and remove current
		destroy_redeye_buf(data->current);
		data->current = NULL;
		update_image(data, data->original->pixbuf);
	}
		
	return TRUE;
}


/* called when the "help" button is clicked. */
static void
help_cb (GtkWidget  *widget,
         DialogData *data)
{
        gthumb_display_help (GTK_WINDOW (data->dialog), "gthumb-redeye");
}



static void ok_cb(GtkWidget *widget, DialogData *data)
{
	if(data->current) {
     		gth_window_set_image_pixbuf(data->window, data->current->pixbuf);
		gth_window_set_image_modified(data->window, TRUE);
	}
	gtk_widget_destroy(data->dialog);
}


/* allocate a Redeye_buf and copy the pixbuf into it */
static RedeyeBuf *make_redeye_buf(char *isred, int isred_size, GdkPixbuf *pixbuf)
{
	RedeyeBuf *pbuf;

	pbuf = g_new(RedeyeBuf, 1);
	pbuf->isred = isred;
	pbuf->isred_size = isred_size;
	pbuf->pixbuf =  gdk_pixbuf_copy(pixbuf);
	
	return pbuf;
}


static RedeyeBuf *clone_redeye_buf(RedeyeBuf *p)
{
	char *isred;

	isred = g_malloc(p->isred_size);
        memcpy(isred, p->isred, p->isred_size);
        return make_redeye_buf(isred, p->isred_size, p->pixbuf);
}



static void destroy_redeye_buf(RedeyeBuf *p)
{
	g_object_unref(p->pixbuf);
	g_free(p->isred);
	g_free(p);
}


/* calculate and return the isred array */
static  void initialize_redeye(DialogData *data, GdkPixbuf *pixbuf)
{
	int ad_red, ad_green, ad_blue;
	char *isred;
	int width, height, red_size;
	int i, j;
	int rowstride, channels;
	guchar *pixels;
	const int THRESHOLD = 0;

	width = gdk_pixbuf_get_width(pixbuf);
	height = gdk_pixbuf_get_height(pixbuf);
	rowstride = gdk_pixbuf_get_rowstride(pixbuf);
	channels = gdk_pixbuf_get_n_channels (pixbuf);
	pixels = gdk_pixbuf_get_pixels(pixbuf);

	red_size = width * height;
	isred = g_malloc0(red_size);

	for (i = 0; i < height; i++)  {
		for (j = 0; j < width; j++) {
      			ad_red = pixels[channels*j + i*rowstride] * RED_FACTOR;
			ad_green = pixels[channels*j+i*rowstride + 1] * GREEN_FACTOR;
			ad_blue = pixels[channels*j+i*rowstride + 2] * BLUE_FACTOR;

			//  This test from the gimp redeye plugin. 
      
			if ( (ad_red >= ad_green-THRESHOLD) && (ad_red >= ad_blue- THRESHOLD ) ) 
				isred[j+i*width] = 1;            
     		}
	}

	data->original = make_redeye_buf(isred, red_size, pixbuf);
}


void  dlg_redeye_removal(GthWindow *window)
{
	GladeXML *gui;
	DialogData *data;
	GtkWidget *cancel_button;
	GtkWidget *undo_button;
  	GtkWidget *ok_button;
	GtkWidget *help_button;
        GdkPixbuf  *orig_pixbuf;

	gui = glade_xml_new(GTHUMB_GLADEDIR "/" GLADE_FILE, NULL, NULL);
        if(!gui) {
		g_warning("Could not find " GLADE_FILE "\n");	
		return;
	}

	data = g_new0(DialogData, 1);
	data->gui = gui;
	data->viewer = gth_window_get_image_viewer(window);
	data->window = window;	
	data->dialog = get_widget(gui, "redeye-removal");
	data->image_box = get_widget(gui, "image-eventbox");
	data->image = get_widget(gui, "redeye-image");
	cancel_button = get_widget(gui, "redeye_cancel_button");
        help_button = get_widget(gui, "redeye_help_button");
	undo_button = get_widget(gui, "redeye_undo_button");
	ok_button = get_widget(gui, "redeye_ok_button");

	orig_pixbuf = image_viewer_get_current_pixbuf (data->viewer);
        initialize_redeye(data, orig_pixbuf);
        data->scale_factor = compute_scale_factor(orig_pixbuf);
        update_image(data, orig_pixbuf);

        gtk_widget_set_size_request(data->image,  gdk_pixbuf_get_width(orig_pixbuf)/data->scale_factor,
					          gdk_pixbuf_get_height(orig_pixbuf)/data->scale_factor);
        gtk_window_set_resizable(GTK_WINDOW(data->dialog), FALSE);

	g_signal_connect (G_OBJECT (data->dialog),
			  "destroy",
			  G_CALLBACK (destroy_cb),
			  data);

	g_signal_connect (G_OBJECT (cancel_button), 
			  "clicked",
			  G_CALLBACK (cancel_cb),
			  data);

	g_signal_connect (G_OBJECT (ok_button), 
			  "clicked",
			  G_CALLBACK (ok_cb),
			  data);
    
        g_signal_connect (G_OBJECT(help_button), "clicked",
				G_CALLBACK(help_cb), data);

        g_signal_connect (G_OBJECT(undo_button), "clicked",
				G_CALLBACK(undo_cb), data);

	initialize_redeye(data, orig_pixbuf);

        g_signal_connect(G_OBJECT(data->image_box), "button_press_event", 
	        G_CALLBACK(press_in_win_event), data);

	gtk_widget_show(data->dialog);
}
