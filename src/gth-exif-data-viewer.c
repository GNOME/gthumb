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


char *metadata_category_name[GTH_METADATA_CATEGORIES] =  
{  
        N_("Filesystem"),  
        N_("Exif General"),  
        N_("Exif Conditions"),  
        N_("Exif Structure"),  
        N_("Exif Thumbnail"),  
        N_("Exif GPS"),  
	N_("Exif Maker Notes"),  
        N_("Exif Versions"),  
	N_("IPTC"),
        N_("XMP Embedded"),  
	N_("XMP Sidecar"),
	N_("Audio / Video"),
        N_("Other")  
};  


enum {
	FULL_NAME_COLUMN,
	DISPLAY_NAME_COLUMN,
	VALUE_COLUMN,
	POS_COLUMN,
	WRITEABLE_COLUMN,
	NUM_COLUMNS
};


struct _GthExifDataViewerPrivate
{
	FileData            *file;
	gboolean             view_file_info;
	GtkWidget           *scrolled_win;
	GtkWidget           *image_exif_view;
	GtkTreeStore        *image_exif_model;
	GtkTreeRowReference *category_root[GTH_METADATA_CATEGORIES];
	ImageViewer         *viewer;
};


static GtkHBoxClass *parent_class = NULL;


static void
gth_exif_data_viewer_destroy (GtkObject *object)
{
	GthExifDataViewer *edv;

	edv = GTH_EXIF_DATA_VIEWER (object);

	if (edv->priv != NULL) {
		file_data_unref (edv->priv->file);
		edv->priv->file = NULL;
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

	for (i = 0; i < GTH_METADATA_CATEGORIES; i++)
		edv->priv->category_root[i] = NULL;
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
	edv->priv->image_exif_model = gtk_tree_store_new (5,
							  G_TYPE_STRING,
							  G_TYPE_STRING,
							  G_TYPE_STRING,
							  G_TYPE_INT,
							  G_TYPE_BOOLEAN);
	gtk_tree_view_set_model (GTK_TREE_VIEW (edv->priv->image_exif_view),
				 GTK_TREE_MODEL (edv->priv->image_exif_model));
	g_object_unref (edv->priv->image_exif_model);
	gtk_container_add (GTK_CONTAINER (edv->priv->scrolled_win), edv->priv->image_exif_view);

	/**/

	renderer = gtk_cell_renderer_text_new ();
	column = gtk_tree_view_column_new_with_attributes ("",
							   renderer,
							   "text", DISPLAY_NAME_COLUMN,
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
gth_exif_data_viewer_get_type (void)
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


static gboolean
tag_is_present_in_category (GthExifDataViewer   *edv,
			    GthMetadataCategory  category,
			    const char          *tag_name)
{
	GtkTreeModel *model = GTK_TREE_MODEL (edv->priv->image_exif_model);
	GtkTreePath  *category_path;
	GtkTreeIter   category_iter, iter;

	if (tag_name == NULL)
		return FALSE;

	if (edv->priv->category_root[category] == NULL)
		return FALSE;

	category_path = gtk_tree_row_reference_get_path (edv->priv->category_root[category]);
	if (category_path == NULL)
		return FALSE;

	if (! gtk_tree_model_get_iter (model,
				       &category_iter,
				       category_path))
		return FALSE;

	gtk_tree_path_free (category_path);

	if (! gtk_tree_model_iter_children  (model, &iter, &category_iter))
		return FALSE;

	do {
		char *tag_name2;

		gtk_tree_model_get (model,
				    &iter,
				    DISPLAY_NAME_COLUMN, &tag_name2,
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


static void
add_to_exif_display_list (GthExifDataViewer   *edv,
			  GthMetadataCategory  category,
			  const char	      *full_name,
			  const char 	      *display_name,
			  const char	      *utf8_value,
	 		  int		       position,
			  gboolean	       writeable)
{
	GtkTreeModel *model = GTK_TREE_MODEL (edv->priv->image_exif_model);
	GtkTreeIter   root_iter;
	GtkTreeIter   iter;

	if (edv->priv->category_root[category] == NULL) {
		GtkTreePath *path;
		gtk_tree_store_append (edv->priv->image_exif_model, &root_iter, NULL);
		gtk_tree_store_set (edv->priv->image_exif_model, &root_iter,
				    FULL_NAME_COLUMN, NULL,
				    DISPLAY_NAME_COLUMN, _(metadata_category_name[category]),
				    VALUE_COLUMN, "",
				    POS_COLUMN, category,
				    WRITEABLE_COLUMN, writeable,
				    -1);
		path = gtk_tree_model_get_path (model, &root_iter);
		edv->priv->category_root[category] = gtk_tree_row_reference_new (model, path);
		gtk_tree_path_free (path);
	}
	else {
		GtkTreePath *path;
		path = gtk_tree_row_reference_get_path (edv->priv->category_root[category]);
		gtk_tree_model_get_iter (model,
					 &root_iter,
					 path);
		gtk_tree_path_free (path);
	}

	gtk_tree_store_append (edv->priv->image_exif_model, &iter, &root_iter);
	gtk_tree_store_set (edv->priv->image_exif_model, &iter,
			    FULL_NAME_COLUMN, full_name,
			    DISPLAY_NAME_COLUMN, display_name,
			    VALUE_COLUMN, utf8_value,
			    POS_COLUMN, position,
			    WRITEABLE_COLUMN, writeable,
			    -1);
}


static void
add_to_display (GthMetadata       *entry,
		GthExifDataViewer *edv)
{
	add_to_exif_display_list (edv,
		       		  entry->category,
				  entry->full_name,
			  	  entry->display_name,
				  entry->value,
				  entry->position,
				  entry->writeable);
}


static void
update_file_info (GthExifDataViewer *edv)
{
	char              *utf8_name;
	char		  *utf8_fullname;
	int                width, height;
	char              *size_txt;
	time_t             mtime;
	struct tm         *tm;
	char               time_txt[50], *utf8_time_txt;
	char              *file_size_txt;
	const char	  *mime_type;

	if (edv->priv->viewer == NULL)
		return;

	utf8_name = basename_for_display (edv->priv->file->path);
	utf8_fullname = gnome_vfs_unescape_string_for_display (edv->priv->file->path);

	if (!image_viewer_is_void (IMAGE_VIEWER (edv->priv->viewer))) {
		width = image_viewer_get_image_width (edv->priv->viewer);
		height = image_viewer_get_image_height (edv->priv->viewer);
	} 
	else {
		width = 0;
		height = 0;
	}
	size_txt = g_strdup_printf (_("%d x %d pixels"), width, height);

	mtime = edv->priv->file->mtime;
	tm = localtime (&mtime);
	strftime (time_txt, 50, _("%d %B %Y, %H:%M"), tm);
	utf8_time_txt = g_locale_to_utf8 (time_txt, -1, 0, 0, 0);

	file_size_txt = gnome_vfs_format_file_size_for_display (edv->priv->file->size);

	mime_type = edv->priv->file->mime_type;
	
	/**/

	add_to_exif_display_list (edv, GTH_METADATA_CATEGORY_FILE, NULL, _("Name"), utf8_name, -7, FALSE);
	add_to_exif_display_list (edv, GTH_METADATA_CATEGORY_FILE, NULL, _("Path"), utf8_fullname, -6, FALSE);

	if (mime_type_is_image (mime_type))
		add_to_exif_display_list (edv, GTH_METADATA_CATEGORY_FILE, NULL, _("Dimensions"), size_txt, -5, FALSE);

	add_to_exif_display_list (edv, GTH_METADATA_CATEGORY_FILE, NULL, _("Size"), file_size_txt, -4, FALSE);
	add_to_exif_display_list (edv, GTH_METADATA_CATEGORY_FILE, NULL, _("Modified"), utf8_time_txt, -3, FALSE);
	add_to_exif_display_list (edv, GTH_METADATA_CATEGORY_FILE, NULL, _("Type"), mime_type, -2, FALSE);

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
set_file_data (GthExifDataViewer *edv,
	       FileData          *fd)
{
	g_return_if_fail (edv != NULL);

	file_data_unref (edv->priv->file);
	edv->priv->file = NULL;
	if (fd != NULL)
		edv->priv->file = file_data_ref (fd);
}


void
gth_exif_data_viewer_update (GthExifDataViewer *edv,
			     ImageViewer       *viewer,
			     FileData          *file_data)
{
	int i;

	set_file_data (edv, file_data);

	if (viewer != NULL)
		edv->priv->viewer = viewer;

	for (i = 0; i < GTH_METADATA_CATEGORIES; i++)
		if (edv->priv->category_root[i] != NULL) {
			gtk_tree_row_reference_free (edv->priv->category_root[i]);
			edv->priv->category_root[i] = NULL;
		}
	gtk_tree_store_clear (edv->priv->image_exif_model);

	if (file_data == NULL)
		return;

	if (edv->priv->view_file_info)
		update_file_info (edv);

        /* Now read metadata, if it isn't already loaded */
	file_data_insert_metadata (file_data);

	/* Display the data */
        g_list_foreach (file_data->metadata, (GFunc) add_to_display, edv);

	gtk_tree_view_expand_all (GTK_TREE_VIEW (edv->priv->image_exif_view));
}


GtkWidget *
gth_exif_data_viewer_get_view (GthExifDataViewer *edv)
{
	return edv->priv->image_exif_view;
}
