/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2003 Free Software Foundation, Inc.
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
#include <gtk/gtk.h>
#include <libgnome/libgnome.h>
#include <libgnomevfs/gnome-vfs-file-info.h>
#include <libgnomevfs/gnome-vfs-utils.h>
#include "file-utils.h"
#include "glib-utils.h"
#include "gth-exif-data-viewer.h"
#include "image-viewer.h"

#ifdef HAVE_LIBEXIF
#include <exif-data.h>
#include <exif-content.h>
#include <exif-entry.h>
#endif /* HAVE_LIBEXIF */


enum {
	NAME_COLUMN,
	VALUE_COLUMN,
	POS_COLUMN,
	NUM_COLUMNS
};


struct _GthExifDataViewerPrivate
{
	char         *path;
	gboolean      view_file_info;
	GtkWidget    *scrolled_win;
	GtkWidget    *image_exif_view;
	GtkListStore *image_exif_model;
	ImageViewer  *viewer;
};


static GtkHBoxClass *parent_class = NULL;


static void
gth_exif_data_viewer_destroy (GtkObject *object)
{
	GthExifDataViewer *edv;

	edv = GTH_EXIF_DATA_VIEWER (object);

	if (edv->priv != NULL) {
		g_free (edv->priv->path);
		edv->priv->path = NULL;
		g_free (edv->priv);
		edv->priv = NULL;
	}

	GTK_OBJECT_CLASS (parent_class)->destroy (object);
}


static void
gth_exif_data_viewer_class_init (GthExifDataViewerClass *class)
{
	GtkObjectClass *object_class;

	parent_class = g_type_class_peek_parent (class);
	object_class = (GtkObjectClass*) class;

	object_class->destroy = gth_exif_data_viewer_destroy;
}


static void
gth_exif_data_viewer_init (GthExifDataViewer *edv)
{
	edv->priv = g_new0 (GthExifDataViewerPrivate, 1);
	edv->priv->view_file_info = TRUE;
}


static void
gth_exif_data_viewer_construct (GthExifDataViewer *edv)
{
	GtkCellRenderer   *renderer;
	GtkTreeViewColumn *column;

	edv->priv->scrolled_win = gtk_scrolled_window_new (NULL, NULL);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (edv->priv->scrolled_win), GTK_POLICY_AUTOMATIC, GTK_POLICY_ALWAYS);
	gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (edv->priv->scrolled_win), GTK_SHADOW_ETCHED_IN);
	gtk_container_add (GTK_CONTAINER (edv), edv->priv->scrolled_win);

	edv->priv->image_exif_view = gtk_tree_view_new ();
	gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (edv->priv->image_exif_view), FALSE);
	gtk_tree_view_set_rules_hint (GTK_TREE_VIEW (edv->priv->image_exif_view), TRUE);
	edv->priv->image_exif_model = gtk_list_store_new (3,
							  G_TYPE_STRING, 
							  G_TYPE_STRING,
							  G_TYPE_INT);
	gtk_tree_view_set_model (GTK_TREE_VIEW (edv->priv->image_exif_view),
				 GTK_TREE_MODEL (edv->priv->image_exif_model));
	g_object_unref (edv->priv->image_exif_model);
	gtk_container_add (GTK_CONTAINER (edv->priv->scrolled_win), edv->priv->image_exif_view);

	/**/

	renderer = gtk_cell_renderer_text_new ();
	column = gtk_tree_view_column_new_with_attributes (_("Field"),
							   renderer,
							   "text", NAME_COLUMN,
							   NULL);
	gtk_tree_view_append_column (GTK_TREE_VIEW (edv->priv->image_exif_view),
				     column);

	/**/

	renderer = gtk_cell_renderer_text_new ();
	column = gtk_tree_view_column_new_with_attributes (_("Value "),
							   renderer,
							   "text", VALUE_COLUMN,
							   NULL);
	gtk_tree_view_append_column (GTK_TREE_VIEW (edv->priv->image_exif_view),
				     column);

	gtk_tree_sortable_set_sort_column_id (GTK_TREE_SORTABLE (edv->priv->image_exif_model), POS_COLUMN, GTK_SORT_ASCENDING);
}


GType
gth_exif_data_viewer_get_type ()
{
        static guint type = 0;

        if (! type) {
                GTypeInfo type_info = {
			sizeof (GthExifDataViewerClass),
			NULL,
			NULL,
			(GClassInitFunc) gth_exif_data_viewer_class_init,
			NULL,
			NULL,
			sizeof (GthExifDataViewer),
			0,
			(GInstanceInitFunc) gth_exif_data_viewer_init
		};

		type = g_type_register_static (GTK_TYPE_HBOX,
					       "GthExifDataViewer",
					       &type_info,
					       0);
	}

        return type;
}


GtkWidget*     
gth_exif_data_viewer_new (gboolean view_file_info)
{
	GtkWidget *widget;

	widget = GTK_WIDGET (g_object_new (GTH_TYPE_EXIF_DATA_VIEWER, NULL));
	gth_exif_data_viewer_construct (GTH_EXIF_DATA_VIEWER (widget));
	gth_exif_data_viewer_view_file_info (GTH_EXIF_DATA_VIEWER (widget), view_file_info);

	return widget;
}


#ifdef HAVE_LIBEXIF

static ExifTag usefull_tags[] = {
	0, 

	EXIF_TAG_EXPOSURE_TIME,
	EXIF_TAG_EXPOSURE_PROGRAM,
	EXIF_TAG_EXPOSURE_MODE,
	EXIF_TAG_EXPOSURE_BIAS_VALUE,
	EXIF_TAG_FLASH,
	EXIF_TAG_FLASH_ENERGY,
	EXIF_TAG_SPECTRAL_SENSITIVITY,
	EXIF_TAG_SHUTTER_SPEED_VALUE,
	EXIF_TAG_APERTURE_VALUE,
	EXIF_TAG_FNUMBER,
	EXIF_TAG_BRIGHTNESS_VALUE,
	EXIF_TAG_LIGHT_SOURCE,
	EXIF_TAG_FOCAL_LENGTH,
	EXIF_TAG_WHITE_BALANCE,
	EXIF_TAG_WHITE_POINT,
	EXIF_TAG_DIGITAL_ZOOM_RATIO,
	EXIF_TAG_SUBJECT_DISTANCE,
	EXIF_TAG_SUBJECT_DISTANCE_RANGE,
	EXIF_TAG_METERING_MODE,
	EXIF_TAG_CONTRAST,
	EXIF_TAG_SATURATION,
	EXIF_TAG_SHARPNESS,
	EXIF_TAG_FOCAL_LENGTH_IN_35MM_FILM,
	EXIF_TAG_BATTERY_LEVEL,

	0,

	EXIF_TAG_DATE_TIME,
	EXIF_TAG_DATE_TIME_ORIGINAL,
	EXIF_TAG_DATE_TIME_DIGITIZED,
	EXIF_TAG_ORIENTATION,
	EXIF_TAG_BITS_PER_SAMPLE,
	EXIF_TAG_SAMPLES_PER_PIXEL,
	EXIF_TAG_COMPRESSION,
	EXIF_TAG_DOCUMENT_NAME,
	EXIF_TAG_IMAGE_DESCRIPTION,

	0,

	/*
	EXIF_TAG_RELATED_IMAGE_WIDTH,
	EXIF_TAG_RELATED_IMAGE_LENGTH,
	EXIF_TAG_IMAGE_WIDTH,
	EXIF_TAG_IMAGE_LENGTH,
	*/
	EXIF_TAG_PIXEL_X_DIMENSION,
	EXIF_TAG_PIXEL_Y_DIMENSION,
	EXIF_TAG_X_RESOLUTION,
	EXIF_TAG_Y_RESOLUTION,
	EXIF_TAG_RESOLUTION_UNIT,

	/*
	0,

	EXIF_TAG_ARTIST,
	EXIF_TAG_COPYRIGHT,
	EXIF_TAG_MAKER_NOTE,
	EXIF_TAG_USER_COMMENT,
	EXIF_TAG_SUBJECT_LOCATION,
	EXIF_TAG_SCENE_TYPE,
	*/

	0,

	EXIF_TAG_MAKE,
	EXIF_TAG_MODEL,
	EXIF_TAG_MAX_APERTURE_VALUE,
	EXIF_TAG_SENSING_METHOD,
	EXIF_TAG_FOCAL_PLANE_X_RESOLUTION,
	EXIF_TAG_FOCAL_PLANE_Y_RESOLUTION,
	EXIF_TAG_FOCAL_PLANE_RESOLUTION_UNIT,

	0,

	EXIF_TAG_EXIF_VERSION
};


static gboolean
tag_is_present (GtkTreeModel *model,
		const char   *tag_name)
{
	GtkTreeIter  iter;

	if (tag_name == NULL)
		return FALSE;

	if (! gtk_tree_model_get_iter_first (model, &iter))
		return FALSE;

	do {
		char *tag_name2;

		gtk_tree_model_get (model,
				    &iter,
				    NAME_COLUMN, &tag_name2,
				    -1);
		if ((tag_name2 != NULL) 
		    && (strcmp (tag_name, tag_name2) == 0)) {
			g_free (tag_name2);
			return TRUE;
		}

		g_free (tag_name2);
	} while (gtk_tree_model_iter_next (model, &iter));

	return FALSE;
}


static ExifEntry *
get_entry_from_tag (ExifData *edata,
		    int       tag)
{
	int i, j;

	for (i = 0; i < EXIF_IFD_COUNT; i++) {
		ExifContent *content = edata->ifd[i];

		if ((content == NULL) || (content->count == 0)) 
			continue;

		for (j = 0; j < content->count; j++) {
			ExifEntry *e = content->entries[j];

			if (! content->entries[j]) 
				continue;

			if (e->tag == tag)
				return e;
		}
	}

	return NULL;
}


static void
update_exif_data (GthExifDataViewer *edv)
{
	ExifData     *edata;
	unsigned int  i;
	gboolean      date_added = FALSE;
	gboolean      aperture_added = FALSE;
	gboolean      last_entry_is_void = ! edv->priv->view_file_info;
	gboolean      list_is_empty = TRUE;

	if (edv->priv->path == NULL)
		return;

	edata = exif_data_new_from_file (edv->priv->path);

	if (edata == NULL) 
                return;

	for (i = 0; i < G_N_ELEMENTS (usefull_tags); i++) {
		ExifEntry   *e;
		GtkTreeIter  iter;
		char        *utf8_name;
		char        *utf8_value;
		
		if ((usefull_tags[i] == 0) && ! last_entry_is_void) {
			gtk_list_store_append (edv->priv->image_exif_model, &iter);
			gtk_list_store_set (edv->priv->image_exif_model, &iter,
					    NAME_COLUMN, "",
					    VALUE_COLUMN, "",
					    POS_COLUMN, i,
					    -1);
			last_entry_is_void = TRUE;
			continue;
		}

		e = get_entry_from_tag (edata, usefull_tags[i]);
		if (e == NULL)
			continue;

		utf8_name = g_locale_to_utf8 (exif_tag_get_name (e->tag), -1, 0, 0, 0);
		if (tag_is_present (GTK_TREE_MODEL (edv->priv->image_exif_model), utf8_name)) {
			g_free (utf8_name);
			continue;
		}

		utf8_value = g_locale_to_utf8 (exif_entry_get_value (e), -1, 0, 0, 0);
		if ((utf8_value == NULL) 
		    || (*utf8_value == 0) 
		    || _g_utf8_all_spaces (utf8_value)) {
			g_free (utf8_name);
			g_free (utf8_value);
			continue;
		}

		if ((e->tag == EXIF_TAG_DATE_TIME)
		    || (e->tag == EXIF_TAG_DATE_TIME_ORIGINAL)
		    || (e->tag == EXIF_TAG_DATE_TIME_DIGITIZED)) {
			if (date_added) {
				g_free (utf8_name);
				g_free (utf8_value);
				continue;
			} else
				date_added = TRUE;
		}

		if ((e->tag == EXIF_TAG_APERTURE_VALUE)
		    || (e->tag == EXIF_TAG_FNUMBER)) {
			if (aperture_added) {
				g_free (utf8_name);
				g_free (utf8_value);
				continue;
			} else 
				aperture_added = TRUE;
		}

		gtk_list_store_append (edv->priv->image_exif_model, &iter);
		gtk_list_store_set (edv->priv->image_exif_model, &iter,
				    NAME_COLUMN, utf8_name,
				    VALUE_COLUMN, utf8_value,
				    POS_COLUMN, i,
				    -1);
		g_free (utf8_name);
		g_free (utf8_value);

		last_entry_is_void = FALSE;
		list_is_empty = FALSE;
	}

	exif_data_unref (edata);
}

#endif /* HAVE_LIBEXIF */


static void
update_file_info (GthExifDataViewer *edv)
{
	char              *utf8_name;
	int                width, height;
	char              *size_txt;
	time_t             mtime;
	struct tm         *tm;
	char               time_txt[50], *utf8_time_txt;
	double             sec;
	GnomeVFSFileSize   file_size;
	char              *file_size_txt;
	GtkTreeIter        iter;

	if (edv->priv->viewer == NULL)
		return;

	utf8_name = g_filename_to_utf8 (file_name_from_path (edv->priv->path), -1, 
					NULL, NULL, NULL);

	if (!image_viewer_is_void(IMAGE_VIEWER (edv->priv->viewer))) {
		width = image_viewer_get_image_width (edv->priv->viewer);
		height = image_viewer_get_image_height (edv->priv->viewer);
	} else {
		width = 0;
		height = 0;
	}
	size_txt = g_strdup_printf (_("%d x %d pixels"), width, height);
	
	mtime = get_file_mtime (edv->priv->path);
	tm = localtime (&mtime);
	strftime (time_txt, 50, _("%d %B %Y, %H:%M"), tm);
	utf8_time_txt = g_locale_to_utf8 (time_txt, -1, 0, 0, 0);
	sec = g_timer_elapsed (image_loader_get_timer (edv->priv->viewer->loader),  NULL);
		
	file_size = get_file_size (edv->priv->path);
	file_size_txt = gnome_vfs_format_file_size_for_display (file_size);

	/**/

	gtk_list_store_append (edv->priv->image_exif_model, &iter);
	gtk_list_store_set (edv->priv->image_exif_model, &iter,
			    NAME_COLUMN, _("Name"), 
			    VALUE_COLUMN, utf8_name,
			    POS_COLUMN, -4,
			    -1);
	
	gtk_list_store_append (edv->priv->image_exif_model, &iter);
	gtk_list_store_set (edv->priv->image_exif_model, &iter,
			    NAME_COLUMN, _("Size"), 
			    VALUE_COLUMN, size_txt,
			    POS_COLUMN, -3,
			    -1);
	
	gtk_list_store_append (edv->priv->image_exif_model, &iter);
	gtk_list_store_set (edv->priv->image_exif_model, &iter,
			    NAME_COLUMN, _("Bytes"), 
			    VALUE_COLUMN, file_size_txt,
			    POS_COLUMN, -2,
			    -1);
	
	gtk_list_store_append (edv->priv->image_exif_model, &iter);
	gtk_list_store_set (edv->priv->image_exif_model, &iter,
			    NAME_COLUMN, _("Modified"),
			    VALUE_COLUMN, utf8_time_txt,
			    POS_COLUMN, -1,
			    -1);

	/**/

	g_free (utf8_time_txt);
	g_free (utf8_name);
	g_free (size_txt);
	g_free (file_size_txt);
}


void
gth_exif_data_viewer_view_file_info (GthExifDataViewer *edv,
				     gboolean           view_file_info)
{
	edv->priv->view_file_info = view_file_info;
}


static void
set_path (GthExifDataViewer *edv,
	  const char        *path)
{
	g_return_if_fail (edv != NULL);

	g_free (edv->priv->path);
	edv->priv->path = NULL;
	if (path != NULL)
		edv->priv->path = g_strdup (path);
}


void
gth_exif_data_viewer_update (GthExifDataViewer *edv,
			     ImageViewer       *viewer,
			     const char        *path)
{

	set_path (edv, path);
	if (viewer != NULL)
		edv->priv->viewer = viewer;

	gtk_list_store_clear (edv->priv->image_exif_model);
	
	if (path == NULL) 
		return;

	if (edv->priv->view_file_info)
		update_file_info (edv);

#ifdef HAVE_LIBEXIF
	update_exif_data (edv);
#endif /* HAVE_LIBEXIF */
}


GtkWidget *
gth_exif_data_viewer_get_view (GthExifDataViewer *edv)
{
	return edv->priv->image_exif_view;
}
