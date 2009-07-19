/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2009 Free Software Foundation, Inc.
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
#include <glib/gi18n.h>
#include <pango/pango.h>
#include "gth-progress-dialog.h"
#include "gtk-utils.h"

#define DIALOG_WIDTH 450


/* -- gth_task_progress -- */


#define GTH_TYPE_TASK_PROGRESS            (gth_task_progress_get_type ())
#define GTH_TASK_PROGRESS(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GTH_TYPE_TASK_PROGRESS, GthTaskProgress))
#define GTH_TASK_PROGRESS_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GTH_TYPE_TASK_PROGRESS, GthTaskProgressClass))
#define GTH_IS_TASK_PROGRESS(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GTH_TYPE_TASK_PROGRESS))
#define GTH_IS_TASK_PROGRESS_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GTH_TYPE_TASK_PROGRESS))
#define GTH_TASK_PROGRESS_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GTH_TYPE_TASK_PROGRESS, GthTaskProgressClass))

typedef struct _GthTaskProgress GthTaskProgress;
typedef struct _GthTaskProgressClass GthTaskProgressClass;

struct _GthTaskProgress {
	GtkHBox    parent_instance;
	GthTask   *task;
	GtkWidget *description_label;
	GtkWidget *details_label;
	GtkWidget *fraction_progressbar;
	GtkWidget *cancel_button;
	gulong     task_progress;
	gulong     task_completed;
};

struct _GthTaskProgressClass {
	GtkHBoxClass parent_class;
};

static gpointer gth_task_progress_parent_class = NULL;


static void
gth_task_progress_finalize (GObject *base)
{
	GthTaskProgress *self = (GthTaskProgress *) base;

	g_signal_handler_disconnect (self->task, self->task_progress);
	g_signal_handler_disconnect (self->task, self->task_completed);
	gth_task_cancel (self->task);
	g_object_unref (self->task);

	G_OBJECT_CLASS (gth_task_progress_parent_class)->finalize (base);
}


static void
gth_task_progress_class_init (GthTaskProgressClass *klass)
{
	gth_task_progress_parent_class = g_type_class_peek_parent (klass);
	G_OBJECT_CLASS (klass)->finalize = gth_task_progress_finalize;
}


static void
cancel_button_clicked_cb (GtkButton       *button,
			  GthTaskProgress *self)
{
	gth_task_cancel (self->task);
}


static void
gth_task_progress_init (GthTaskProgress *self)
{
	GtkWidget     *vbox;
	PangoAttrList *attr_list;
	GtkWidget     *image;

	self->task = NULL;

	vbox = gtk_vbox_new (FALSE, 3);
	gtk_container_set_border_width (GTK_CONTAINER (vbox), 5);
	gtk_widget_show (vbox);
	gtk_box_pack_start (GTK_BOX (self), vbox, TRUE, TRUE, 0);

	self->description_label = gtk_label_new ("");
	gtk_misc_set_alignment (GTK_MISC (self->description_label), 0.0, 0.5);
	gtk_widget_show (self->description_label);
	gtk_box_pack_start (GTK_BOX (vbox), self->description_label, FALSE, FALSE, 0);

	self->fraction_progressbar = gtk_progress_bar_new ();
	gtk_widget_set_size_request (self->fraction_progressbar, -1, 15);
	gtk_widget_show (self->fraction_progressbar);
	gtk_box_pack_start (GTK_BOX (vbox), self->fraction_progressbar, FALSE, FALSE, 0);

	self->details_label = gtk_label_new ("");
	attr_list = pango_attr_list_new ();
	pango_attr_list_insert (attr_list, pango_attr_size_new (8500));
	g_object_set (self->details_label, "attributes", attr_list, NULL);
	gtk_misc_set_alignment (GTK_MISC (self->details_label), 0.0, 0.5);
	gtk_widget_show (self->details_label);
	gtk_box_pack_start (GTK_BOX (vbox), self->details_label, FALSE, FALSE, 0);

	vbox = gtk_vbox_new (FALSE, 0);
	gtk_widget_show (vbox);
	gtk_box_pack_start (GTK_BOX (self), vbox, FALSE, FALSE, 0);

	self->cancel_button = gtk_button_new ();
	gtk_widget_show (self->cancel_button);
	g_signal_connect (self->cancel_button, "clicked", G_CALLBACK (cancel_button_clicked_cb), self);
	gtk_widget_set_tooltip_text (self->cancel_button, _("Cancel operation"));
	gtk_box_pack_start (GTK_BOX (vbox), self->cancel_button, TRUE, FALSE, 0);

	image = gtk_image_new_from_stock (GTK_STOCK_CANCEL, GTK_ICON_SIZE_MENU);
	gtk_widget_show (image);
	gtk_container_add (GTK_CONTAINER (self->cancel_button), image);

	pango_attr_list_unref (attr_list);
}


GType
gth_task_progress_get_type (void)
{
	static GType type = 0;

	if (type == 0) {
		static const GTypeInfo g_define_type_info = {
			sizeof (GthTaskProgressClass),
			(GBaseInitFunc) NULL,
			(GBaseFinalizeFunc) NULL,
			(GClassInitFunc) gth_task_progress_class_init,
			(GClassFinalizeFunc) NULL,
			NULL,
			sizeof (GthTaskProgress),
			0,
			(GInstanceInitFunc) gth_task_progress_init,
			NULL
		};
		type = g_type_register_static (GTK_TYPE_HBOX,
					       "GthTaskProgress",
					       &g_define_type_info,
					       0);
	}

	return type;
}


static void
task_progress_cb (GthTask    *task,
		  const char *description,
		  const char *details,
		  gboolean    pulse,
		  double      fraction,
		  gpointer    user_data)
{
	GthTaskProgress *self = user_data;

	if (description == NULL)
		return;

	gtk_label_set_text (GTK_LABEL (self->description_label), description);
	gtk_label_set_text (GTK_LABEL (self->details_label), details);
	if (pulse)
		gtk_progress_bar_pulse (GTK_PROGRESS_BAR (self->fraction_progressbar));
	else
		gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (self->fraction_progressbar), fraction);
}


static void gth_progress_dialog_remove_child (GthProgressDialog *dialog, GtkWidget *child);


static void
task_completed_cb (GthTask  *task,
		   GError   *error,
		   gpointer  user_data)
{
	GthTaskProgress *self = user_data;

	gth_progress_dialog_remove_child (GTH_PROGRESS_DIALOG (gtk_widget_get_toplevel (GTK_WIDGET (self))), GTK_WIDGET (self));
}


GtkWidget *
gth_task_progress_new (GthTask *task)
{
	GthTaskProgress *self;

	self = g_object_new (GTH_TYPE_TASK_PROGRESS, NULL);
	self->task = g_object_ref (task);
	self->task_progress = g_signal_connect (self->task,
			  "progress",
			  G_CALLBACK (task_progress_cb),
			  self);
	self->task_completed = g_signal_connect (self->task,
			  "completed",
			  G_CALLBACK (task_completed_cb),
			  self);

	return (GtkWidget *) self;
}


/* -- gth_progress_dialog -- */


static gpointer gth_progress_dialog_parent_class = NULL;


struct _GthProgressDialogPrivate {
	GtkWidget *task_box;
};


static void
gth_progress_dialog_class_init (GthProgressDialogClass *klass)
{
	gth_progress_dialog_parent_class = g_type_class_peek_parent (klass);
	g_type_class_add_private (klass, sizeof (GthProgressDialogPrivate));
}


static void
progress_dialog_response_cb (GtkDialog *dialog,
			     int        response,
			     gpointer   user_data)
{
	if (response == GTK_RESPONSE_CLOSE)
		gtk_widget_hide (GTK_WIDGET (dialog));
}


static void
gth_progress_dialog_init (GthProgressDialog *self)
{
	self->priv = G_TYPE_INSTANCE_GET_PRIVATE (self, GTH_TYPE_PROGRESS_DIALOG, GthProgressDialogPrivate);

	gtk_widget_set_size_request (GTK_WIDGET (self), DIALOG_WIDTH, -1);
	gtk_window_set_title (GTK_WINDOW (self), "");
	gtk_window_set_resizable (GTK_WINDOW (self), TRUE);
	gtk_dialog_set_has_separator (GTK_DIALOG (self), FALSE);
	gtk_box_set_spacing (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (self))), 5);
	gtk_container_set_border_width (GTK_CONTAINER (self), 5);

	gtk_dialog_add_button (GTK_DIALOG (self), GTK_STOCK_CLOSE, GTK_RESPONSE_CLOSE);

	self->priv->task_box = gtk_vbox_new (FALSE, 6);
	gtk_widget_show (self->priv->task_box);
	gtk_container_set_border_width (GTK_CONTAINER (self->priv->task_box), 5);
	gtk_box_pack_start (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (self))), self->priv->task_box, FALSE, FALSE, 0);

	g_signal_connect (self, "response", G_CALLBACK (progress_dialog_response_cb), self);
	g_signal_connect (self, "delete-event", G_CALLBACK (gtk_widget_hide_on_delete), self);
}


GType
gth_progress_dialog_get_type (void)
{
	static GType type = 0;

	if (type == 0) {
		static const GTypeInfo g_define_type_info = {
			sizeof (GthProgressDialogClass),
			(GBaseInitFunc) NULL,
			(GBaseFinalizeFunc) NULL,
			(GClassInitFunc) gth_progress_dialog_class_init,
			(GClassFinalizeFunc) NULL,
			NULL,
			sizeof (GthProgressDialog),
			0,
			(GInstanceInitFunc) gth_progress_dialog_init,
			NULL
		};
		type = g_type_register_static (GTK_TYPE_DIALOG,
					       "GthProgressDialog",
					       &g_define_type_info,
					       0);
	}

	return type;
}


GtkWidget *
gth_progress_dialog_new (GtkWindow *parent)
{
	GthProgressDialog *self;

	self = g_object_new (GTH_TYPE_PROGRESS_DIALOG, NULL);
	gtk_window_set_transient_for (GTK_WINDOW (self), parent);

	return (GtkWidget *) self;
}


void
gth_progress_dialog_add_task (GthProgressDialog *self,
			      GthTask           *task)
{
	GtkWidget *child;

	child = gth_task_progress_new (task);
	gtk_widget_show (child);
	gtk_box_pack_start (GTK_BOX (self->priv->task_box), child, TRUE, TRUE, 0);
	gtk_window_present (GTK_WINDOW (self));

	gth_task_exec (task);
}


static void
gth_progress_dialog_remove_child (GthProgressDialog *self,
				  GtkWidget         *child)
{
	gtk_container_remove (GTK_CONTAINER (self->priv->task_box), child);
	if (_gtk_container_get_n_children (GTK_CONTAINER (self->priv->task_box)) == 0)
		gtk_widget_hide (GTK_WIDGET (self));
}
