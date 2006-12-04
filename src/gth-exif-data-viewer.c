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

#include <libexif/exif-data.h>
#include <libexif/exif-content.h>
#include <libexif/exif-entry.h>
#include <libexif/exif-tag.h>
#include <libexif/exif-mnote-data.h>


typedef enum {
	GTH_METADATA_CATEGORY_FILE = 0,
	GTH_METADATA_CATEGORY_EXIF,
	GTH_METADATA_CATEGORY_NOTE,
	GTH_METADATA_CATEGORIES
} GthMetadataCategory;


static char *metadata_category_name[GTH_METADATA_CATEGORIES] = { N_("Filesystem Data"), N_("Standard Exif Tags"), N_("MakerNote Exif Tags") };

enum {
	NAME_COLUMN,
	VALUE_COLUMN,
	POS_COLUMN,
	NUM_COLUMNS
};


struct _GthExifDataViewerPrivate
{
	char          *path;
	gboolean       view_file_info;
	GtkWidget     *scrolled_win;
	GtkWidget     *image_exif_view;
	GtkTreeStore  *image_exif_model;
	GtkTreePath   *category_root_path[GTH_METADATA_CATEGORIES];
	ImageViewer   *viewer;
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

	for (i = 0; i < GTH_METADATA_CATEGORIES; i++)
		edv->priv->category_root_path[i] = NULL;
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
	gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (edv->priv->image_exif_view), TRUE);
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
	column = gtk_tree_view_column_new_with_attributes (_("Field"),
							   renderer,
							   "text", NAME_COLUMN,
							   NULL);
	gtk_tree_view_column_set_expand (column, FALSE);
	gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_AUTOSIZE);
	gtk_tree_view_append_column (GTK_TREE_VIEW (edv->priv->image_exif_view),
				     column);

	/**/

	renderer = gtk_cell_renderer_text_new ();
	column = gtk_tree_view_column_new_with_attributes (_("Value "),
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


static gboolean
tag_is_present_in_category (GtkTreeModel *model,
			    GtkTreeIter  *category_root_iter,
			    const char   *tag_name)
{
	GtkTreeIter iter;

	if (tag_name == NULL)
		return FALSE;

	if (! gtk_tree_model_iter_children  (model, &iter, category_root_iter))
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




static gboolean
tag_is_present (GthExifDataViewer *edv,
		const char        *tag_name)
{
	GtkTreeModel *model;
	int           i;

	model = GTK_TREE_MODEL (edv->priv->image_exif_model);
	for (i = 0; i < GTH_METADATA_CATEGORIES; i++) {
		GtkTreeIter iter;
		if (edv->priv->category_root_path[i] == NULL)
			continue;
		if (! gtk_tree_model_get_iter (model, &iter, edv->priv->category_root_path[i]))
			continue;
		if (tag_is_present_in_category (model, &iter, tag_name))
			return TRUE;
	}

	return FALSE;
}


static void
add_to_exif_display_list (GthExifDataViewer   *edv,
			  GthMetadataCategory  category,
			  const char 	      *utf8_name,
			  const char	      *utf8_value,
	 		  int		       position)
{
	GtkTreeIter root_iter;
	GtkTreeIter iter;

	if (edv->priv->category_root_path[category] == NULL) {
		gtk_tree_store_append (edv->priv->image_exif_model, &root_iter, NULL);
		gtk_tree_store_set (edv->priv->image_exif_model, &root_iter,
        		    	    NAME_COLUMN, _(metadata_category_name[category]),
			    	    VALUE_COLUMN, "",
			    	    POS_COLUMN, category,
                            	    -1);
		edv->priv->category_root_path[category] = gtk_tree_model_get_path (GTK_TREE_MODEL (edv->priv->image_exif_model), &root_iter);
	}
	else
		gtk_tree_model_get_iter (GTK_TREE_MODEL (edv->priv->image_exif_model),
					 &root_iter,
					 edv->priv->category_root_path[category]);

	gtk_tree_store_append (edv->priv->image_exif_model, &iter, &root_iter);
        gtk_tree_store_set (edv->priv->image_exif_model, &iter,
        		    NAME_COLUMN, utf8_name,
			    VALUE_COLUMN, utf8_value,
			    POS_COLUMN, position,
                            -1);
}


static void
update_exif_data (GthExifDataViewer *edv,
		  ExifData          *edata)
{
	const char   *path;
	unsigned int  i,j;
	int	      pos_shift=0;
	gboolean      list_is_empty = TRUE;

	path = get_file_path_from_uri (edv->priv->path);
	if (path == NULL)
		return;

	if (edata == NULL)
		edata = exif_data_new_from_file (path);
	else
		exif_data_ref (edata);

	if (edata == NULL)
                return;

	/* Iterate through every IFD in the Exif data, checking for tags. The GPS tags are
	   stored in their own private IFD. */

        for (i = 0; i < EXIF_IFD_COUNT; i++) {
                ExifContent *content = edata->ifd[i];
		const char  *value;
		char        *utf8_name;
		char        *utf8_value;

                if (! edata->ifd[i] || ! edata->ifd[i]->count)
                        continue;

                for (j = 0; j < content->count; j++) {
                        ExifEntry *e = content->entries[j];

                        if (! content->entries[j])
                                continue;

			/* Accept all tags, but handle "maker notes" separately below */

			if (e->tag != EXIF_TAG_MAKER_NOTE) {

				/* The tag IDs for the GPS and non-GPS IFDs overlap slightly,
				   so it is important to use the exif_tag_get_name_in_ifd
				   function, and not the older exif_tag_get_name function. */

				value = exif_tag_get_name_in_ifd (e->tag, i);

				if (value == NULL)
					continue;

				utf8_name = g_strdup (value);
				if (tag_is_present (edv, value)) {
					g_free (utf8_name);
					continue;
				}

				value = get_exif_entry_value (e);
				if (value == NULL) {
					g_free (utf8_name);
					continue;
				}

				utf8_value = g_strdup (value);
				if ((utf8_value == NULL)
				    || (*utf8_value == 0)
				    || _g_utf8_all_spaces (utf8_value)) {
					g_free (utf8_name);
					g_free (utf8_value);
					continue;
				}

				add_to_exif_display_list (edv, GTH_METADATA_CATEGORY_EXIF, utf8_name, utf8_value, i+pos_shift);

				g_free (utf8_name);
				g_free (utf8_value);
			}
			else {
				/* Maker Notes have their own sub-structure which must be
				   iterated through. libexif only knows how to handle certain
				   types of manufacturer note styles. */

				ExifMnoteData *mnote;
				unsigned int k;
				unsigned int subnote_count;
				char	     mnote_buf[1024];

				mnote = exif_data_get_mnote_data (edata);
				if (mnote == NULL)
					continue;

				/* Supported MakerNote Found */

				subnote_count = exif_mnote_data_count (mnote);

				/* Iterate through each "sub-note" */
				for (k = 0; k < subnote_count; k++) {
	                               	value = exif_mnote_data_get_title (mnote, k);

        	                       	if (value == NULL)
                	                	continue;

					++pos_shift;

	      	                       	utf8_name = g_strdup (value);
                	               	if (tag_is_present (edv, utf8_name)) {
                        	               	g_free (utf8_name);
                                	       	continue;
	                               	}

        	                       	exif_mnote_data_get_value (mnote, k, mnote_buf, sizeof (mnote_buf));

					utf8_value = g_strdup (mnote_buf);
	                               	if ((utf8_value == NULL)
        	                           || (*utf8_value == 0)
                	                   || _g_utf8_all_spaces (utf8_value)) {
                        	              	g_free (utf8_name);
                                	      	g_free (utf8_value);
                                        	continue;
	                        	}

					add_to_exif_display_list (edv, GTH_METADATA_CATEGORY_NOTE, utf8_name, utf8_value, i+pos_shift);

	   	                        g_free (utf8_name);
		                        g_free (utf8_value);
				}
			}

			list_is_empty = FALSE;
		}
	}

	exif_data_unref (edata);
}


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

	if (edv->priv->viewer == NULL)
		return;

	utf8_name = g_filename_display_basename (edv->priv->path);

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

	/**/

	add_to_exif_display_list (edv, GTH_METADATA_CATEGORY_FILE, _("Name"), utf8_name, -6);
	add_to_exif_display_list (edv, GTH_METADATA_CATEGORY_FILE, _("Dimensions"), size_txt, -5);
	add_to_exif_display_list (edv, GTH_METADATA_CATEGORY_FILE, _("Size"), file_size_txt, -4);
	add_to_exif_display_list (edv, GTH_METADATA_CATEGORY_FILE, _("Modified"), utf8_time_txt, -3);
	add_to_exif_display_list (edv, GTH_METADATA_CATEGORY_FILE, _("Type"), gnome_vfs_mime_get_description (get_mime_type (edv->priv->path)), -2);

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
			     const char        *path,
			     gpointer           exif_data)
{
	int i;

	set_path (edv, path);

	if (viewer != NULL)
		edv->priv->viewer = viewer;

	for (i = 0; i < GTH_METADATA_CATEGORIES; i++)
		if (edv->priv->category_root_path[i] != NULL) {
			gtk_tree_path_free (edv->priv->category_root_path[i]);
			edv->priv->category_root_path[i] = NULL;
		}
	gtk_tree_store_clear (edv->priv->image_exif_model);

	if (path == NULL)
		return;

	if (edv->priv->view_file_info)
		update_file_info (edv);

	update_exif_data (edv, exif_data);

	gtk_tree_view_expand_all (GTK_TREE_VIEW (edv->priv->image_exif_view));
}


GtkWidget *
gth_exif_data_viewer_get_view (GthExifDataViewer *edv)
{
	return edv->priv->image_exif_view;
}
