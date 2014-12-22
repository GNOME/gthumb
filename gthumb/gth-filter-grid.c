/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2001-2014 The Free Software Foundation, Inc.
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
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <config.h>
#include <stdlib.h>
#include <glib/gi18n.h>
#include "cairo-utils.h"
#include "cairo-scale.h"
#include "glib-utils.h"
#include "gth-filter-grid.h"
#include "gth-image-task.h"


#define MAX_N_COLUMNS 10
#define DEFAULT_N_COLUMNS 2
#define PREVIEW_SIZE 110
#define COLUMN_SPACING 20
#define ROW_SPACING 20
#define FILTER_ID_KEY "filter_id"


/* Properties */
enum {
        PROP_0,
        PROP_COLUMNS
};


/* Signals */
enum {
        ACTIVATED,
        LAST_SIGNAL
};


typedef struct {
	int	 filter_id;
	GthTask	*image_task;
} PreviewTask;


typedef struct {
	GthFilterGrid   *self;
	GList           *tasks;
	GList           *current_task;
	GCancellable    *cancellable;
	cairo_surface_t *original;
	GthTask         *resize_task;
} GeneratePreviewData;


struct _GthFilterGridPrivate {
	GtkWidget		*grid;
	int			 n_columns;
	int			 current_column;
	int			 current_row;
	GHashTable		*buttons;
	GHashTable		*previews;
	GtkWidget		*active_button;
	GeneratePreviewData	*gp_data;
};


static guint gth_filter_grid_signals[LAST_SIGNAL] = { 0 };


G_DEFINE_TYPE (GthFilterGrid, gth_filter_grid, GTK_TYPE_BOX)


static void
_gth_filter_grid_set_n_columns (GthFilterGrid *self,
				int            n_columns)
{
	self->priv->grid = gtk_grid_new ();
	gtk_grid_set_column_spacing (GTK_GRID (self->priv->grid), COLUMN_SPACING);
	gtk_grid_set_row_spacing (GTK_GRID (self->priv->grid), ROW_SPACING);
	gtk_widget_show (self->priv->grid);
	gtk_container_add (GTK_CONTAINER (self), self->priv->grid);

	self->priv->n_columns = n_columns;
	self->priv->current_column = 0;
	self->priv->current_row = 0;
}


static void
gth_filter_grid_set_property (GObject      *object,
			      guint         property_id,
			      const GValue *value,
			      GParamSpec   *pspec)
{
	GthFilterGrid *self;

        self = GTH_FILTER_GRID (object);

	switch (property_id) {
	case PROP_COLUMNS:
		_gth_filter_grid_set_n_columns (self, g_value_get_int (value));
		break;
	default:
		break;
	}
}


static void
gth_filter_grid_get_property (GObject    *object,
		              guint       property_id,
		              GValue     *value,
		              GParamSpec *pspec)
{
	GthFilterGrid *self;

        self = GTH_FILTER_GRID (object);

	switch (property_id) {
	case PROP_COLUMNS:
		g_value_set_int (value, self->priv->n_columns);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
		break;
	}
}


static void generate_preview_data_cancel (GeneratePreviewData *data);


static void
gth_filter_grid_finalize (GObject *obj)
{
	GthFilterGrid *self;

	self = GTH_FILTER_GRID (obj);
	if (self->priv->gp_data != NULL) {
		generate_preview_data_cancel (self->priv->gp_data);
		self->priv->gp_data = NULL;
	}
	g_hash_table_destroy (self->priv->previews);
	g_hash_table_destroy (self->priv->buttons);

	G_OBJECT_CLASS (gth_filter_grid_parent_class)->finalize (obj);
}


static void
gth_filter_grid_class_init (GthFilterGridClass *klass)
{
	GObjectClass   *object_class;

	g_type_class_add_private (klass, sizeof (GthFilterGridPrivate));

	object_class = (GObjectClass*) klass;
	object_class->set_property = gth_filter_grid_set_property;
	object_class->get_property = gth_filter_grid_get_property;
	object_class->finalize = gth_filter_grid_finalize;

	/* properties */

	g_object_class_install_property (object_class,
					 PROP_COLUMNS,
					 g_param_spec_int ("columns",
                                                           "Columns",
                                                           "Number of columns",
                                                           1,
                                                           MAX_N_COLUMNS,
                                                           DEFAULT_N_COLUMNS,
                                                           G_PARAM_WRITABLE | G_PARAM_READABLE | G_PARAM_CONSTRUCT_ONLY));

	/* signals */

	gth_filter_grid_signals[ACTIVATED] =
                g_signal_new ("activated",
                              G_TYPE_FROM_CLASS (klass),
                              G_SIGNAL_RUN_LAST,
                              G_STRUCT_OFFSET (GthFilterGridClass, activated),
                              NULL, NULL,
                              g_cclosure_marshal_VOID__INT,
                              G_TYPE_NONE,
                              1,
                              G_TYPE_INT);
}


static void
gth_filter_grid_init (GthFilterGrid *self)
{
	self->priv = G_TYPE_INSTANCE_GET_PRIVATE (self, GTH_TYPE_FILTER_GRID, GthFilterGridPrivate);
	self->priv->n_columns = DEFAULT_N_COLUMNS;
	self->priv->previews = g_hash_table_new_full (g_direct_hash, g_direct_equal, NULL, NULL);
	self->priv->buttons = g_hash_table_new_full (g_direct_hash, g_direct_equal, NULL, NULL);
	self->priv->active_button = NULL;
	self->priv->gp_data = NULL;
}


GtkWidget *
gth_filter_grid_new (void)
{
	return (GtkWidget *) g_object_new (GTH_TYPE_FILTER_GRID, NULL);
}


static void
button_toggled_cb (GtkWidget *toggle_button,
	           gpointer   user_data)
{
	GthFilterGrid *self = user_data;
	int            filter_id = GTH_FILTER_GRID_NO_ACTIVE_FILTER;

	if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (toggle_button))) {
		if (self->priv->active_button != toggle_button) {
			if (self->priv->active_button != NULL) {
				g_signal_handlers_block_by_data (self->priv->active_button, self);
				gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (self->priv->active_button), FALSE);
				g_signal_handlers_unblock_by_data (self->priv->active_button, self);
			}
			self->priv->active_button = toggle_button;
		}
		filter_id = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (toggle_button), FILTER_ID_KEY));
	}
	else if (self->priv->active_button == toggle_button) {
		self->priv->active_button = NULL;
	}

	g_signal_emit (self,
		       gth_filter_grid_signals[ACTIVATED],
		       0,
		       filter_id);
}


static GtkWidget *
_gth_filter_grid_cell_new (GthFilterGrid	*self,
			   int			 filter_id,
			   cairo_surface_t	*preview,
			   const char		*label_text,
			   const char		*tooltip)
{
	GtkWidget *cell;
	GtkWidget *button;
	GtkWidget *image;
	GtkWidget *label;
	GtkWidget *button_content;

	cell = gtk_box_new (GTK_ORIENTATION_VERTICAL, 6);

	button = gtk_toggle_button_new ();
	g_object_set_data_full (G_OBJECT (button), FILTER_ID_KEY, GINT_TO_POINTER (filter_id), NULL);
	gtk_style_context_add_class (gtk_widget_get_style_context (button), "filter-preview");
	gtk_widget_set_tooltip_text (button, tooltip);
	g_signal_connect (button, "toggled", G_CALLBACK (button_toggled_cb), self);

	image = gtk_image_new_from_surface (preview);
	gtk_widget_set_size_request (image, PREVIEW_SIZE, PREVIEW_SIZE);

	label = gtk_label_new_with_mnemonic (label_text);
	gtk_label_set_ellipsize (GTK_LABEL (label), PANGO_ELLIPSIZE_END);

	button_content = gtk_box_new (GTK_ORIENTATION_VERTICAL, 6);

	gtk_box_pack_start (GTK_BOX (button_content), image, FALSE, FALSE, 0);
	gtk_box_pack_start (GTK_BOX (button_content), label, FALSE, FALSE, 0);
	gtk_container_add (GTK_CONTAINER (button), button_content);
	gtk_box_pack_start (GTK_BOX (cell), button, FALSE, FALSE, 0);
	gtk_widget_show_all (cell);

	g_hash_table_insert (self->priv->previews, GINT_TO_POINTER (filter_id), image);
	g_hash_table_insert (self->priv->buttons, GINT_TO_POINTER (filter_id), button);

	return cell;
}


void
gth_filter_grid_add_filter (GthFilterGrid	*self,
			    int			 filter_id,
			    cairo_surface_t	*preview,
			    const char		*label,
			    const char		*tooltip)
{
	GtkWidget *cell;

	cell = _gth_filter_grid_cell_new (self, filter_id, preview, label, tooltip);
	gtk_grid_attach (GTK_GRID (self->priv->grid),
			 cell,
			 self->priv->current_column,
			 self->priv->current_row,
			 1,
			 1);

	self->priv->current_column++;
	if (self->priv->current_column >= self->priv->n_columns) {
		self->priv->current_column = 0;
		self->priv->current_row++;
	}
}


void
gth_filter_grid_set_filter_preview (GthFilterGrid	*self,
				    int			 filter_id,
				    cairo_surface_t	*preview)
{
	GtkWidget *image;

	image = g_hash_table_lookup (self->priv->previews, GINT_TO_POINTER (filter_id));
	g_return_if_fail (image != NULL);

	gtk_image_set_from_surface (GTK_IMAGE (image), preview);
}


void
gth_filter_grid_activate (GthFilterGrid	*self,
			  int		 filter_id)
{
	GtkWidget *button;

	button = g_hash_table_lookup (self->priv->buttons, GINT_TO_POINTER (filter_id));
	g_return_if_fail (button != NULL);

	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button), TRUE);
}


/* -- gth_filter_grid_generate_previews -- */


static void
preview_task_free (PreviewTask *task)
{
	g_object_unref (task->image_task);
	g_free (task);
}


static void
generate_preview_data_free (GeneratePreviewData *data)
{
	cairo_surface_destroy (data->original);
	if (data->self != NULL) {
		if (data->self->priv->gp_data == data)
			data->self->priv->gp_data = NULL;
		g_object_remove_weak_pointer (G_OBJECT (data->self), (gpointer *) &data->self);
	}
	g_list_free_full (data->tasks, (GDestroyNotify) preview_task_free);
	_g_object_unref (data->cancellable);
	_g_object_unref (data->resize_task);
	g_free (data);
}


static void
generate_preview_data_cancel (GeneratePreviewData *data)
{
	if (data->cancellable != NULL)
		g_cancellable_cancel (data->cancellable);
}


static void generate_preview (GeneratePreviewData *data);


static void
image_preview_completed_cb (GthTask    *task,
		       	    GError     *error,
		       	    gpointer    user_data)
{
	GeneratePreviewData *data = user_data;
	cairo_surface_t     *preview;

	if ((error != NULL) || (data->self == NULL)) {
		generate_preview_data_free (data);
		return;
	}

	preview = gth_image_task_get_destination_surface (GTH_IMAGE_TASK (task));
	if (preview != NULL) {
		PreviewTask *task = (PreviewTask *) data->current_task->data;
		gth_filter_grid_set_filter_preview (data->self, task->filter_id, preview);
	}

	data->current_task = g_list_next (data->current_task);
	generate_preview (data);
}


static void
generate_preview (GeneratePreviewData *data)
{
	PreviewTask *task;

	if (data->current_task == NULL) {
		generate_preview_data_free (data);
		return;
	}

	task = (PreviewTask *) data->current_task->data;

	g_signal_connect (task->image_task,
			  "completed",
			  G_CALLBACK (image_preview_completed_cb),
			  data);
	gth_image_task_set_source_surface (GTH_IMAGE_TASK (task->image_task), data->original);

	_g_object_unref (data->cancellable);
	data->cancellable = g_cancellable_new ();
	gth_task_exec (task->image_task, data->cancellable);
}


static void
resize_task_completed_cb (GthTask  *task,
			  GError   *error,
			  gpointer  user_data)
{
	GeneratePreviewData *data = user_data;

	_g_object_unref (data->resize_task);
	data->resize_task = NULL;

	if ((error != NULL) || (data->self == NULL)) {
		generate_preview_data_free (data);
		return;
	}

	if (data->original != NULL)
		cairo_surface_destroy (data->original);
	data->original = gth_image_task_get_destination_surface (GTH_IMAGE_TASK (task));
	if (data->original == NULL) {
		generate_preview_data_free (data);
		return;
	}

	data->current_task = data->tasks;
	generate_preview (data);
}


static gpointer
resize_task_exec (GthAsyncTask *task,
		  gpointer      user_data)
{
	cairo_surface_t *source;
	cairo_surface_t *destination;

	source = gth_image_task_get_source_surface (GTH_IMAGE_TASK (task));
	destination = _cairo_image_surface_scale_squared (source,
						          PREVIEW_SIZE,
						          SCALE_FILTER_GOOD,
						          task);
	_cairo_image_surface_clear_metadata (destination);
	gth_image_task_set_destination_surface (GTH_IMAGE_TASK (task), destination);

	cairo_surface_destroy (destination);
	cairo_surface_destroy (source);

	return NULL;
}


void
gth_filter_grid_generate_previews (GthFilterGrid	*self,
				   cairo_surface_t	*image,
				   int                   filter_id,
				   ...
				   /* series of:
				   int			 filter_id,
				   GthTask		*image_task,
				   */)
{
	GeneratePreviewData	*data;
	GthTask			*image_task;
	va_list			 args;

	if (self->priv->gp_data != NULL)
		generate_preview_data_cancel (self->priv->gp_data);

	data = g_new (GeneratePreviewData, 1);
	data->self = self;
	data->tasks = NULL;
	data->cancellable = NULL;
	data->original = NULL;

	g_object_add_weak_pointer (G_OBJECT (self), (gpointer *) &data->self);
	self->priv->gp_data = data;

	/* collect the (filter, task) pairs */

	va_start (args, filter_id);
	image_task = va_arg (args, GthTask *);
	while ((filter_id >= 0) && (image_task != NULL)) {
		PreviewTask *task;

		task = g_new0 (PreviewTask, 1);
		task->filter_id = filter_id;
		task->image_task = image_task;
		data->tasks = g_list_prepend (data->tasks, task);

		filter_id = va_arg (args, int);
		if (filter_id < 0)
			break;
		image_task = va_arg (args, GthTask *);
	}
	va_end (args);
	data->tasks = g_list_reverse (data->tasks);

	/* resize the original image */

	data->resize_task = gth_image_task_new (_("Resizing images"),
						NULL,
						resize_task_exec,
						NULL,
						data,
						NULL);
	gth_image_task_set_source_surface (GTH_IMAGE_TASK (data->resize_task), image);
	g_signal_connect (data->resize_task,
			  "completed",
			  G_CALLBACK (resize_task_completed_cb),
			  data);

	data->cancellable = g_cancellable_new ();
	gth_task_exec (data->resize_task, data->cancellable);
}
