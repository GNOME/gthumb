/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2003, 2004, 2005 Free Software Foundation, Inc.
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

#include <glib/gi18n.h>
#include <gtk/gtk.h>
#include <libgnomevfs/gnome-vfs-mime.h>
#include <libgnomevfs/gnome-vfs-file-info.h>
#include <libgnomevfs/gnome-vfs-utils.h>

#include "file-utils.h"
#include "glib-utils.h"
#include "gth-exif-utils.h"
#include "gth-exif-data-viewer.h"
#include "image-viewer.h"
#include "gconf-utils.h"
#include "preferences.h"

#include <libexif/exif-data.h>
#include <libexif/exif-content.h>
#include <libexif/exif-entry.h>
#include <libexif/exif-tag.h>
#include <libexif/exif-ifd.h>
#include <libexif/exif-mnote-data.h>


enum {
	NAME_COLUMN,
	VALUE_COLUMN,
	POS_COLUMN,
	NUM_COLUMNS
};


struct _GthExifDataViewerPrivate
{
	char                *path;
	gboolean             view_file_info;
	GtkWidget           *scrolled_win;
	GtkWidget           *image_exif_view;
	GtkTreeStore        *image_exif_model;
	GHashTable	    *category_roots;
	ImageViewer         *viewer;
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
	int i;

	edv->priv = g_new0 (GthExifDataViewerPrivate, 1);
	edv->priv->view_file_info = TRUE;

	edv->priv->category_roots = g_hash_table_new_full (g_str_hash,
							   g_str_equal,
							   g_free,
							   NULL);
}


static void
gth_exif_data_viewer_construct (GthExifDataViewer *edv)
{
	GtkCellRenderer   *renderer;
	GtkTreeViewColumn *column;
	GValue             value = { 0, };

	edv->priv->scrolled_win = gtk_scrolled_window_new (NULL, NULL);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (edv->priv->scrolled_win), GTK_POLICY_AUTOMATIC, GTK_POLICY_ALWAYS);
	gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (edv->priv->scrolled_win), GTK_SHADOW_ETCHED_IN);
	gtk_container_add (GTK_CONTAINER (edv), edv->priv->scrolled_win);

	edv->priv->image_exif_view = gtk_tree_view_new ();
	gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (edv->priv->image_exif_view), FALSE);
	gtk_tree_view_set_rules_hint (GTK_TREE_VIEW (edv->priv->image_exif_view), TRUE);
	edv->priv->image_exif_model = gtk_tree_store_new (3,
							  G_TYPE_STRING,
							  G_TYPE_STRING,
							  G_TYPE_INT);
	gtk_tree_view_set_model (GTK_TREE_VIEW (edv->priv->image_exif_view),
				 GTK_TREE_MODEL (edv->priv->image_exif_model));
	g_object_unref (edv->priv->image_exif_model);
	gtk_container_add (GTK_CONTAINER (edv->priv->scrolled_win), edv->priv->image_exif_view);

	/**/

	renderer = gtk_cell_renderer_text_new ();
	column = gtk_tree_view_column_new_with_attributes ("",
							   renderer,
							   "text", NAME_COLUMN,
							   NULL);
	gtk_tree_view_column_set_expand (column, FALSE);
	gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_AUTOSIZE);
	gtk_tree_view_append_column (GTK_TREE_VIEW (edv->priv->image_exif_view),
				     column);

	/**/

	renderer = gtk_cell_renderer_text_new ();
	column = gtk_tree_view_column_new_with_attributes ("",
							   renderer,
							   "text", VALUE_COLUMN,
							   NULL);
	gtk_tree_view_column_set_expand (column, TRUE);
	gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_AUTOSIZE);

	g_value_init (&value, PANGO_TYPE_ELLIPSIZE_MODE);
	g_value_set_enum (&value, PANGO_ELLIPSIZE_END);
	g_object_set_property (G_OBJECT (renderer), "ellipsize", &value);
	g_value_unset (&value);

	gtk_tree_view_append_column (GTK_TREE_VIEW (edv->priv->image_exif_view),
				     column);

	gtk_tree_sortable_set_sort_column_id (GTK_TREE_SORTABLE (edv->priv->image_exif_model), POS_COLUMN, GTK_SORT_ASCENDING);
}


GType
gth_exif_data_viewer_get_type ()
{
	static GType type = 0;

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


static void
add_to_exif_display_list (GthExifDataViewer *edv,
			  char              *category, 
			  const char 	    *utf8_name,
			  const char	    *utf8_value,
	 		  int		     category_position,
			  int		     tag_position)
{
	GtkTreeModel *model = GTK_TREE_MODEL (edv->priv->image_exif_model);
	GtkTreeIter   root_iter;
	GtkTreeIter   iter;

	if (g_hash_table_lookup (edv->priv->category_roots, category) == NULL) {
		GtkTreePath *path;
		gtk_tree_store_append (edv->priv->image_exif_model, &root_iter, NULL);
		gtk_tree_store_set (edv->priv->image_exif_model, &root_iter,
        		    	    NAME_COLUMN, category,
			    	    VALUE_COLUMN, "",
			    	    POS_COLUMN, category_position,
                            	    -1);
		path = gtk_tree_model_get_path (model, &root_iter);
		g_hash_table_insert (edv->priv->category_roots, 
				     g_strdup (category), 
				     gtk_tree_row_reference_new (model, path));
		gtk_tree_path_free (path);
	}
	else {
		GtkTreePath *path;
		path = gtk_tree_row_reference_get_path (
				g_hash_table_lookup (edv->priv->category_roots, category));
		gtk_tree_model_get_iter (model,
					 &root_iter,
					 path);
		gtk_tree_path_free (path);
	}

	gtk_tree_store_append (edv->priv->image_exif_model, &iter, &root_iter);
	gtk_tree_store_set (edv->priv->image_exif_model, &iter,
			    NAME_COLUMN, utf8_name,
			    VALUE_COLUMN, utf8_value,
			    POS_COLUMN, tag_position,
			    -1);
}


update_file_info (GthExifDataViewer *edv)
{
	char              *utf8_name;
	char		  *utf8_fullname;
	int                width, height;
	char              *size_txt;
	time_t             mtime;
	struct tm         *tm;
	char               time_txt[50], *utf8_time_txt;
	double             sec;
	GnomeVFSFileSize   file_size;
	char              *file_size_txt;
	const char	  *mime_type;

	if (edv->priv->viewer == NULL)
		return;

	utf8_name = basename_for_display (edv->priv->path);
	utf8_fullname = gnome_vfs_unescape_string_for_display (edv->priv->path);

	if (!image_viewer_is_void (IMAGE_VIEWER (edv->priv->viewer))) {
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

	mime_type = get_file_mime_type (edv->priv->path,
					eel_gconf_get_boolean (PREF_FAST_FILE_TYPE, TRUE));
	/**/

	add_to_exif_display_list (edv, _("Filesystem Data"), _("Name"), utf8_name, 0, 0);
	add_to_exif_display_list (edv, _("Filesystem Data"), _("Path"), utf8_fullname, 0, 1);
	add_to_exif_display_list (edv, _("Filesystem Data"), _("Dimensions"), size_txt, 0, 2);
	add_to_exif_display_list (edv, _("Filesystem Data"), _("Size"), file_size_txt, 0, 3);
	add_to_exif_display_list (edv, _("Filesystem Data"), _("Modified"), utf8_time_txt, 0, 4);
	add_to_exif_display_list (edv, _("Filesystem Data"), _("Type"), mime_type, 0, 5);

	/**/

	g_free (utf8_time_txt);
	g_free (utf8_name);
	g_free (utf8_fullname);
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
update_metadata_display (gpointer key, gpointer value, gpointer edv)
{
	char group_name[256];
	char key_name[256];
	char value_string[65536];
	int category_position, tag_position;

	if (sscanf (key, "%255[^:]:%255[^\n]", group_name, key_name) != 2)
		return;
	if (sscanf (value, "%d:%d:%65535[^\n]", &category_position, &tag_position, value_string) != 3)
		return;

	/* Don't use exiftool's reading of filesystem data - we can do 
	   that better. */
	if (!strcmp(group_name,"File"))
		return;
	if (!strcmp(group_name,"ExifTool"))
		return;

	add_to_exif_display_list (edv, group_name, key_name, value_string, category_position, tag_position);
}


void
free_roots (gpointer key, gpointer value, gpointer edv)
{
	gtk_tree_row_reference_free (value);
}


void
gth_exif_data_viewer_update (GthExifDataViewer *edv,
			     ImageViewer       *viewer,
			     const char        *path)
{
	GHashTable *metadata_hash;

	set_path (edv, path);

	if (viewer != NULL)
		edv->priv->viewer = viewer;

	g_hash_table_foreach (edv->priv->category_roots, free_roots, NULL);
	g_hash_table_remove_all (edv->priv->category_roots);
	gtk_tree_store_clear (edv->priv->image_exif_model);

	if (path == NULL)
		return;

	if (edv->priv->view_file_info)
		update_file_info (edv);

	metadata_hash = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, g_free);

	get_metadata_for_file (edv->priv->path, metadata_hash);
        g_hash_table_foreach (metadata_hash, update_metadata_display, edv);

	gtk_tree_view_expand_all (GTK_TREE_VIEW (edv->priv->image_exif_view));

	g_hash_table_destroy (metadata_hash);
}


GtkWidget *
gth_exif_data_viewer_get_view (GthExifDataViewer *edv)
{
	return edv->priv->image_exif_view;
}
