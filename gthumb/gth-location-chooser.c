/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2005-2009 Free Software Foundation, Inc.
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
#include <glib.h>
#include <glib/gprintf.h>
#include <gtk/gtk.h>
#include "glib-utils.h"
#include "gth-file-source.h"
#include "gth-icon-cache.h"
#include "gth-location-chooser.h"
#include "gth-main.h"
#include "gtk-utils.h"
#include "pixbuf-utils.h"


#define BUFFER_SIZE 4096


enum {
	ITEM_TYPE_NONE,
	ITEM_TYPE_SEPARATOR,
	ITEM_TYPE_LOCATION,
	ITEM_TYPE_BOOKMARK,
	ITEM_TYPE_ENTRY_POINT
};

enum {
	ICON_COLUMN,
	NAME_COLUMN,
	URI_COLUMN,
	TYPE_COLUMN,
	ELLIPSIZE_COLUMN,
	N_COLUMNS
};

enum {
	CHANGED,
	LAST_SIGNAL
};

struct _GthLocationChooserPrivate
{
	GtkWidget     *combo;
	GtkTreeStore  *model;
	GFile         *location;
	GthIconCache  *icon_cache;
	GthFileSource *file_source;
	gulong         entry_points_changed_id;
};


static GtkHBoxClass *parent_class = NULL;
static guint gth_location_chooser_signals[LAST_SIGNAL] = { 0 };


static void
gth_location_chooser_finalize (GObject *object)
{
	GthLocationChooser *chooser;

	chooser = GTH_LOCATION_CHOOSER (object);

	if (chooser->priv != NULL) {
		g_signal_handler_disconnect (gth_main_get_default_monitor (),
					     chooser->priv->entry_points_changed_id);
		if (chooser->priv->file_source != NULL)
			g_object_unref (chooser->priv->file_source);
		gth_icon_cache_free (chooser->priv->icon_cache);
		if (chooser->priv->location != NULL)
			g_object_unref (chooser->priv->location);
		g_free (chooser->priv);
	}

	G_OBJECT_CLASS (parent_class)->finalize (object);
}


static void
gth_location_chooser_class_init (GthLocationChooserClass *class)
{
	GObjectClass *object_class;

	parent_class = g_type_class_peek_parent (class);
	object_class = (GObjectClass*) class;

	object_class->finalize = gth_location_chooser_finalize;

	gth_location_chooser_signals[CHANGED] =
		g_signal_new ("changed",
			      G_TYPE_FROM_CLASS (class),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (GthLocationChooserClass, changed),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__VOID,
			      G_TYPE_NONE,
			      0);
}


static void
gth_location_chooser_init (GthLocationChooser *chooser)
{
	chooser->priv = g_new0 (GthLocationChooserPrivate, 1);
}


static void
combo_changed_cb (GtkComboBox *widget,
		  gpointer     user_data)
{
	GthLocationChooser *chooser = user_data;
	GtkTreeIter         iter;
	char               *uri = NULL;
	int                 item_type = ITEM_TYPE_NONE;

	if (! gtk_combo_box_get_active_iter (GTK_COMBO_BOX (chooser->priv->combo), &iter))
		return;

	gtk_tree_model_get (GTK_TREE_MODEL (chooser->priv->model),
			    &iter,
			    TYPE_COLUMN, &item_type,
			    URI_COLUMN, &uri,
			    -1);

	if (uri != NULL) {
		GFile *file;

		file = g_file_new_for_uri (uri);
		gth_location_chooser_set_current (chooser, file);

		g_object_unref (file);
	}
}


static gboolean
row_separator_func (GtkTreeModel *model,
		    GtkTreeIter  *iter,
		    gpointer      data)
{
	int item_type = ITEM_TYPE_NONE;

	gtk_tree_model_get (model,
			    iter,
			    TYPE_COLUMN, &item_type,
			    -1);

	return (item_type == ITEM_TYPE_SEPARATOR);
}


static gboolean
get_nth_separator_pos (GthLocationChooser *chooser,
		       int                 pos,
		       int                *idx)
{
	GtkTreeIter iter;
	gboolean    n_found = 0;

	if (idx != NULL)
		*idx = 0;

	if (! gtk_tree_model_get_iter_first (GTK_TREE_MODEL (chooser->priv->model), &iter))
		return FALSE;

	do {
		int item_type = ITEM_TYPE_NONE;

		gtk_tree_model_get (GTK_TREE_MODEL (chooser->priv->model),
				    &iter,
				    TYPE_COLUMN, &item_type,
				    -1);
		if (item_type == ITEM_TYPE_SEPARATOR) {
			n_found++;
			if (n_found == pos)
				break;
		}

		if (idx != NULL)
			*idx = *idx + 1;
	}
	while (gtk_tree_model_iter_next (GTK_TREE_MODEL (chooser->priv->model), &iter));

	return n_found == pos;
}


static void
add_file_source_entries (GthLocationChooser *chooser,
			 GFile              *file,
			 const char         *name,
			 GIcon              *icon,
			 int                 position,
			 gboolean            update_active_iter,
			 int                 iter_type)
{
	GtkTreeIter  iter;
	GdkPixbuf   *pixbuf;
	char        *uri;

	pixbuf = gth_icon_cache_get_pixbuf (chooser->priv->icon_cache, icon);
	uri = g_file_get_uri (file);

	gtk_tree_store_insert (chooser->priv->model, &iter, NULL, position);
	gtk_tree_store_set (chooser->priv->model, &iter,
			    TYPE_COLUMN, iter_type,
			    ICON_COLUMN, pixbuf,
			    NAME_COLUMN, name,
			    URI_COLUMN, uri,
			    ELLIPSIZE_COLUMN, PANGO_ELLIPSIZE_END,
			    -1);

	g_free (uri);
	g_object_unref (pixbuf);

	if (update_active_iter && g_file_equal (chooser->priv->location, file)) {
		g_signal_handlers_block_by_func (chooser->priv->combo, combo_changed_cb, chooser);
		gtk_combo_box_set_active_iter (GTK_COMBO_BOX (chooser->priv->combo), &iter);
		g_signal_handlers_unblock_by_func (chooser->priv->combo, combo_changed_cb, chooser);
	}
}


static void
update_entry_point_list (GthLocationChooser *chooser)
{
	int    first_position;
	int    last_position;
	int    i;
	int    position;
	GList *scan;

	if (! get_nth_separator_pos (chooser, 1, &first_position))
		return;
	if (! get_nth_separator_pos (chooser, 2, &last_position))
		return;

	for (i = first_position + 1; i < last_position; i++) {
		GtkTreePath *path;
		GtkTreeIter  iter;

		path = gtk_tree_path_new_from_indices (first_position + 1, -1);
		if (gtk_tree_model_get_iter (GTK_TREE_MODEL (chooser->priv->model), &iter, path))
			gtk_tree_store_remove (chooser->priv->model, &iter);

		gtk_tree_path_free (path);
	}

	position = first_position + 1;
	for (scan = gth_main_get_all_file_sources (); scan; scan = scan->next) {
		GthFileSource *file_source = scan->data;
		GList         *entry_points;
		GList         *scan_entry;

		entry_points = gth_file_source_get_entry_points (file_source);
		for (scan_entry = entry_points; scan_entry; scan_entry = scan_entry->next) {
			GthFileData *file_data = scan_entry->data;

			add_file_source_entries (chooser,
						 file_data->file,
						 g_file_info_get_display_name (file_data->info),
						 g_file_info_get_icon (file_data->info),
						 position++,
						 FALSE,
						 ITEM_TYPE_ENTRY_POINT);
		}

		_g_object_list_unref (entry_points);
	}
}


typedef struct {
	GthLocationChooser *chooser;
	GInputStream       *stream;
	char                buffer[BUFFER_SIZE];
	GString            *file_content;
} UpdateBookmarksData;


static gboolean
get_bookmark_iter (GthLocationChooser *chooser,
		   GtkTreeIter        *iter)
{
	gboolean found = FALSE;

	if (! gtk_tree_model_get_iter_first (GTK_TREE_MODEL (chooser->priv->model), iter))
		return FALSE;

	do {
		int item_type = ITEM_TYPE_NONE;

		gtk_tree_model_get (GTK_TREE_MODEL (chooser->priv->model),
				    iter,
				    TYPE_COLUMN, &item_type,
				    -1);
		if (item_type == ITEM_TYPE_BOOKMARK) {
			found = TRUE;
			break;
		}
	}
	while (gtk_tree_model_iter_next (GTK_TREE_MODEL (chooser->priv->model), iter));

	return found;
}


static void
update_bookmark_list_from_content (GthLocationChooser *chooser,
				   const char         *content)
{
	GtkTreeIter    bookmark_iter;
	GtkTreeIter    iter;
	char         **lines;
	int            i;

	if (! get_bookmark_iter (chooser, &bookmark_iter))
		return;

	if (gtk_tree_model_iter_children (GTK_TREE_MODEL (chooser->priv->model), &iter, &bookmark_iter))
		while (gtk_tree_store_remove (chooser->priv->model, &iter)) /* void */;

	lines = g_strsplit (content, "\n", -1);
	for (i = 0; lines[i] != NULL; i++) {
		char      **line;
		char       *uri;
		GFile      *file;
		GIcon      *icon;
		char       *name;
		GdkPixbuf  *pixbuf;

		line = g_strsplit (lines[i], " ", 2);
		uri = line[0];
		if (uri == NULL)
			continue;

		file = g_file_new_for_uri (uri);
		icon = _g_file_get_icon (file);
		if (line[1] != NULL)
			name = g_strdup (line[1]);
		else
			name = _g_file_get_display_name (file);
		pixbuf = gth_icon_cache_get_pixbuf (chooser->priv->icon_cache, icon);

		gtk_tree_store_append (chooser->priv->model, &iter, &bookmark_iter);
		gtk_tree_store_set (chooser->priv->model, &iter,
				    TYPE_COLUMN, ITEM_TYPE_LOCATION,
				    ICON_COLUMN, pixbuf,
				    NAME_COLUMN, name,
				    URI_COLUMN, uri,
				    ELLIPSIZE_COLUMN, PANGO_ELLIPSIZE_NONE,
				    -1);

		g_object_unref (pixbuf);
		g_free (name);
		g_object_unref (icon);
		g_object_unref (file);
		g_strfreev (line);
	}
	g_strfreev (lines);
}


static void
update_bookmark_list_ready (GObject      *source_object,
			    GAsyncResult *result,
			    gpointer      user_data)
{
	UpdateBookmarksData *data = user_data;
	gssize               size;

	size = g_input_stream_read_finish (data->stream, result, NULL);
	if (size < 0) {
		g_input_stream_close (data->stream, NULL, NULL);
		g_object_unref (data->stream);
		g_free (data);
		return;
	}

	if (size > 0) {
		data->buffer[size + 1] = '\0';
		g_string_append (data->file_content, data->buffer);

		g_input_stream_read_async (data->stream,
				   	   data->buffer,
				   	   BUFFER_SIZE - 1,
					   G_PRIORITY_DEFAULT,
					   NULL,
					   update_bookmark_list_ready,
					   data);
		return;
	}

	update_bookmark_list_from_content (data->chooser, data->file_content->str);

	g_input_stream_close (data->stream, NULL, NULL);
	g_object_unref (data->stream);
	g_string_free (data->file_content, TRUE);
	g_free (data);
}


static void
update_bookmark_list (GthLocationChooser *chooser)
{
	char                *bookmark_file_path;
	GFile               *bookmark_file;
	GFileInputStream    *input_stream;
	UpdateBookmarksData *data;

	bookmark_file_path = g_build_filename (g_get_home_dir (), ".gtk-bookmarks", NULL);
	bookmark_file = g_file_new_for_path (bookmark_file_path);
	g_free (bookmark_file_path);

	input_stream = g_file_read (bookmark_file, NULL, NULL);
	g_object_unref (bookmark_file);

	if (input_stream == NULL)
		return;

	data = g_new0 (UpdateBookmarksData, 1);
	data->chooser = chooser;
	data->stream = (GInputStream*) input_stream;
	data->file_content = g_string_new ("");

	g_input_stream_read_async (data->stream,
				   data->buffer,
				   BUFFER_SIZE - 1,
				   G_PRIORITY_DEFAULT,
				   NULL,
				   update_bookmark_list_ready,
				   data);
}


static void
entry_points_changed_cb (GthMonitor         *monitor,
			 GthLocationChooser *chooser)
{
	update_entry_point_list (chooser);
}


static void
gth_location_chooser_construct (GthLocationChooser *chooser)
{
	GtkCellRenderer *renderer;
	GtkTreeIter      iter;
	GIcon           *icon;
	GdkPixbuf       *pixbuf;

	chooser->priv->icon_cache = gth_icon_cache_new (gtk_icon_theme_get_for_screen (gtk_widget_get_screen (GTK_WIDGET (chooser))),
							 _gtk_icon_get_pixel_size (GTK_WIDGET (chooser), GTK_ICON_SIZE_MENU));

	chooser->priv->model = gtk_tree_store_new (N_COLUMNS,
						    GDK_TYPE_PIXBUF,
						    G_TYPE_STRING,
						    G_TYPE_STRING,
						    G_TYPE_INT,
						    PANGO_TYPE_ELLIPSIZE_MODE);
	chooser->priv->combo = gtk_combo_box_new_with_model (GTK_TREE_MODEL (chooser->priv->model));
	g_object_unref (chooser->priv->model);
	g_signal_connect (chooser->priv->combo,
			  "changed",
			  G_CALLBACK (combo_changed_cb),
			  chooser);
	gtk_combo_box_set_row_separator_func (GTK_COMBO_BOX (chooser->priv->combo),
					      row_separator_func,
					      chooser,
					      NULL);

	/* icon column */

	renderer = gtk_cell_renderer_pixbuf_new ();
	gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (chooser->priv->combo),
				    renderer,
				    FALSE);
	gtk_cell_layout_set_attributes  (GTK_CELL_LAYOUT (chooser->priv->combo),
					 renderer,
					 "pixbuf", ICON_COLUMN,
					 NULL);

	/* path column */

	renderer = gtk_cell_renderer_text_new ();
	gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (chooser->priv->combo),
				    renderer,
				    TRUE);
	gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (chooser->priv->combo),
					renderer,
					"text", NAME_COLUMN,
					"ellipsize", ELLIPSIZE_COLUMN,
					NULL);

	/**/

	gtk_widget_show (chooser->priv->combo);
	gtk_container_add (GTK_CONTAINER (chooser), chooser->priv->combo);

	/* Add standard items. */

	/* separator #1 */

	gtk_tree_store_append (chooser->priv->model, &iter, NULL);
	gtk_tree_store_set (chooser->priv->model, &iter,
			    TYPE_COLUMN, ITEM_TYPE_SEPARATOR,
			    -1);

	/* separator #2 */

	gtk_tree_store_append (chooser->priv->model, &iter, NULL);
	gtk_tree_store_set (chooser->priv->model, &iter,
			    TYPE_COLUMN, ITEM_TYPE_SEPARATOR,
			    -1);

	icon = g_themed_icon_new ("bookmark-view");
	pixbuf = gth_icon_cache_get_pixbuf (chooser->priv->icon_cache, icon);

	gtk_tree_store_append (chooser->priv->model, &iter, NULL);
	gtk_tree_store_set (chooser->priv->model, &iter,
			    TYPE_COLUMN, ITEM_TYPE_BOOKMARK,
			    ICON_COLUMN, pixbuf,
			    NAME_COLUMN, _("System Bookmarks"),
			    ELLIPSIZE_COLUMN, PANGO_ELLIPSIZE_END,
			    -1);

	g_object_unref (pixbuf);
	g_object_unref (icon);

	/**/

	performance (DEBUG_INFO, "update_entry_point_list");

	update_entry_point_list (chooser);

	performance (DEBUG_INFO, "update_bookmark_list");

	update_bookmark_list (chooser);

	performance (DEBUG_INFO, "location constructed");

	/**/

	chooser->priv->entry_points_changed_id =
			g_signal_connect (gth_main_get_default_monitor (),
					  "entry-points-changed",
					  G_CALLBACK (entry_points_changed_cb),
					  chooser);
}


GType
gth_location_chooser_get_type (void)
{
	static GType type = 0;

	if (! type) {
		GTypeInfo type_info = {
			sizeof (GthLocationChooserClass),
			NULL,
			NULL,
			(GClassInitFunc) gth_location_chooser_class_init,
			NULL,
			NULL,
			sizeof (GthLocationChooser),
			0,
			(GInstanceInitFunc) gth_location_chooser_init
		};

		type = g_type_register_static (GTK_TYPE_HBOX,
					       "GthLocationChooser",
					       &type_info,
					       0);
	}

	return type;
}


GtkWidget*
gth_location_chooser_new (void)
{
	GtkWidget *widget;

	widget = GTK_WIDGET (g_object_new (GTH_TYPE_LOCATION_CHOOSER, NULL));
	gth_location_chooser_construct (GTH_LOCATION_CHOOSER (widget));

	return widget;
}


static gboolean
delete_current_file_entries (GthLocationChooser *chooser)
{
	gboolean    found = FALSE;
	GtkTreeIter iter;

	if (! gtk_tree_model_get_iter_first (GTK_TREE_MODEL (chooser->priv->model), &iter))
		return FALSE;

	do {
		int item_type = ITEM_TYPE_NONE;

		gtk_tree_model_get (GTK_TREE_MODEL (chooser->priv->model),
				    &iter,
				    TYPE_COLUMN, &item_type,
				    -1);
		if (item_type == ITEM_TYPE_SEPARATOR)
			break;
	}
	while (gtk_tree_store_remove (chooser->priv->model, &iter));

	return found;
}


static gboolean
get_iter_from_current_file_entries (GthLocationChooser *chooser,
				    GFile              *file,
				    GtkTreeIter        *iter)
{
	gboolean  found = FALSE;
	char     *uri;

	if (! gtk_tree_model_get_iter_first (GTK_TREE_MODEL (chooser->priv->model), iter))
		return FALSE;

	uri = g_file_get_uri (file);
	do {
		int   item_type = ITEM_TYPE_NONE;
		char *list_uri;

		gtk_tree_model_get (GTK_TREE_MODEL (chooser->priv->model),
				    iter,
				    TYPE_COLUMN, &item_type,
				    URI_COLUMN, &list_uri,
				    -1);
		if (item_type == ITEM_TYPE_SEPARATOR)
			break;
		if (same_uri (uri, list_uri)) {
			found = TRUE;
			g_free (list_uri);
			break;
		}
		g_free (list_uri);
	}
	while (gtk_tree_model_iter_next (GTK_TREE_MODEL (chooser->priv->model), iter));

	g_free (uri);

	return found;
}


void
gth_location_chooser_set_current (GthLocationChooser *chooser,
				  GFile              *file)
{
	GtkTreeIter iter;

	if (chooser->priv->file_source != NULL)
		g_object_unref (chooser->priv->file_source);
	chooser->priv->file_source = gth_main_get_file_source (file);

	if (chooser->priv->file_source == NULL)
		return;

	if ((chooser->priv->location != NULL) && g_file_equal (file, chooser->priv->location))
		return;

	if (chooser->priv->location != NULL)
		g_object_unref (chooser->priv->location);
	chooser->priv->location = g_file_dup (file);

	if (get_iter_from_current_file_entries (chooser, chooser->priv->location, &iter)) {
		g_signal_handlers_block_by_func (chooser->priv->combo, combo_changed_cb, chooser);
		gtk_combo_box_set_active_iter (GTK_COMBO_BOX (chooser->priv->combo), &iter);
		g_signal_handlers_unblock_by_func (chooser->priv->combo, combo_changed_cb, chooser);
	}
	else {
		GList *list;
		GList *scan;
		int    position = 0;

		delete_current_file_entries (chooser);

		list = gth_file_source_get_current_list (chooser->priv->file_source, chooser->priv->location);
		for (scan = list; scan; scan = scan->next) {
			GFile     *file = scan->data;
			GFileInfo *info;

			info = gth_file_source_get_file_info (chooser->priv->file_source, file);
			if (info == NULL)
				continue;
			add_file_source_entries (chooser,
						 file,
						 g_file_info_get_display_name (info),
						 g_file_info_get_icon (info),
						 position++,
						 TRUE,
						 ITEM_TYPE_LOCATION);

			g_object_unref (info);
		}
	}

	g_signal_emit (G_OBJECT (chooser), gth_location_chooser_signals[CHANGED], 0);
}


GFile *
gth_location_chooser_get_current (GthLocationChooser *chooser)
{
	return chooser->priv->location;
}
