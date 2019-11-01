/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2017 Free Software Foundation, Inc.
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
#include <glib/gi18n.h>
#include <glib.h>
#include <gtk/gtk.h>
#include "gth-accel-button.h"
#include "gtk-utils.h"


/* Properties */
enum {
	PROP_0,
	PROP_KEY,
	PROP_MODS
};

/* Signals */
enum {
	CHANGED,
	LAST_SIGNAL
};

struct _GthAccelButtonPrivate {
	guint            keyval;
	GdkModifierType  modifiers;
	gboolean         valid;
	GtkWidget       *label;
};


static guint gth_accel_button_signals[LAST_SIGNAL] = { 0 };


G_DEFINE_TYPE_WITH_CODE (GthAccelButton,
			 gth_accel_button,
			 GTK_TYPE_BUTTON,
			 G_ADD_PRIVATE (GthAccelButton))


static void
gth_accel_button_finalize (GObject *object)
{
	/*GthAccelButton *self;

	self = GTH_ACCEL_BUTTON (object);*/


	G_OBJECT_CLASS (gth_accel_button_parent_class)->finalize (object);
}


static void
gth_accel_button_set_property (GObject      *object,
			       guint         property_id,
			       const GValue *value,
			       GParamSpec   *pspec)
{
	GthAccelButton *self;

        self = GTH_ACCEL_BUTTON (object);

	switch (property_id) {
	case PROP_KEY:
		self->priv->keyval = g_value_get_uint (value);
		break;
	case PROP_MODS:
		self->priv->modifiers = g_value_get_flags (value);
		break;
	default:
		break;
	}
}


static void
gth_accel_button_get_property (GObject    *object,
			       guint       property_id,
			       GValue     *value,
			       GParamSpec *pspec)
{
	GthAccelButton *self;

        self = GTH_ACCEL_BUTTON (object);

	switch (property_id) {
	case PROP_KEY:
		g_value_set_uint (value, self->priv->keyval);
		break;
	case PROP_MODS:
		g_value_set_flags (value, self->priv->modifiers);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
		break;
	}
}


static void
gth_accel_button_class_init (GthAccelButtonClass *klass)
{
	GObjectClass *object_class;

	object_class = (GObjectClass*) klass;
	object_class->set_property = gth_accel_button_set_property;
	object_class->get_property = gth_accel_button_get_property;
	object_class->finalize = gth_accel_button_finalize;

	/* properties */

	g_object_class_install_property (object_class,
					 PROP_KEY,
					 g_param_spec_uint ("key",
							    "Key",
							    "The key value",
							    0,
							    G_MAXUINT,
							    0,
							    G_PARAM_READWRITE));
	g_object_class_install_property (object_class,
					 PROP_MODS,
					 g_param_spec_flags ("mods",
							     "Mods",
							     "The active modifiers",
							     GDK_TYPE_MODIFIER_TYPE,
							     0,
							     G_PARAM_READWRITE));

	/* signals */

	gth_accel_button_signals[CHANGED] =
                g_signal_new ("changed",
                              G_TYPE_FROM_CLASS (klass),
                              G_SIGNAL_RUN_LAST,
                              G_STRUCT_OFFSET (GthAccelButtonClass, changed),
                              NULL, NULL,
                              g_cclosure_marshal_VOID__VOID,
                              G_TYPE_NONE,
                              0);
}


static void
_update_label (GthAccelButton *self)
{
	self->priv->valid = gtk_accelerator_valid (self->priv->keyval, self->priv->modifiers);
	if (self->priv->valid) {
		char *label = gtk_accelerator_get_label (self->priv->keyval, self->priv->modifiers);
		gtk_label_set_text (GTK_LABEL (self->priv->label), label);
		g_free (label);
	}
	else
		gtk_label_set_text (GTK_LABEL (self->priv->label), _("None"));
}


#define _RESPONSE_RESET 10


static void
accel_dialog_response_cb (GtkDialog *dialog,
	                  gint       response_id,
	                  gpointer   user_data)
{
	GthAccelButton *accel_button = user_data;

	switch (response_id) {
	case GTK_RESPONSE_CANCEL:
		gtk_widget_destroy (GTK_WIDGET (dialog));
		break;
	case _RESPONSE_RESET:
		gth_accel_button_set_accelerator (accel_button, 0, 0);
		gtk_widget_destroy (GTK_WIDGET (dialog));
		break;
	}
}


static gboolean
accel_dialog_keypress_cb (GtkWidget    *widget,
			  GdkEventKey  *event,
	                  gpointer      user_data)
{
	GthAccelButton  *accel_button = user_data;
	GdkModifierType  modifiers;

	if (event->keyval == GDK_KEY_Escape)
		return FALSE;

	modifiers = event->state & gtk_accelerator_get_default_mod_mask ();
	if (gth_accel_button_set_accelerator (accel_button, event->keyval, modifiers))
		gtk_widget_destroy (widget);

	return TRUE;
}


static void
button_clicked_cb (GtkButton *button,
		   gpointer   user_data)
{
	GtkWidget *dialog, *box, *label, *secondary_label, *content_area;

	dialog = g_object_new (GTK_TYPE_DIALOG,
			       "use-header-bar", _gtk_settings_get_dialogs_use_header (),
			       "modal", TRUE,
			       "transient-for", _gtk_widget_get_toplevel_if_window (GTK_WIDGET (button)),
			       "resizable", FALSE,
			       "title", _("Shortcut"),
			       NULL);
	gtk_dialog_add_buttons (GTK_DIALOG (dialog),
			        _GTK_LABEL_CANCEL, GTK_RESPONSE_CANCEL,
				! gth_accel_button_get_valid (GTH_ACCEL_BUTTON (button)) ? NULL : _GTK_LABEL_DELETE,
				_RESPONSE_RESET,
				NULL);

	content_area = gtk_dialog_get_content_area (GTK_DIALOG (dialog));

	label = gtk_label_new (_("Press a combination of keys to use as shortcut."));
	secondary_label = gtk_label_new (_("Press Esc to cancel"));
	gtk_style_context_add_class (gtk_widget_get_style_context (secondary_label), "dim-label");
	box = gtk_box_new (GTK_ORIENTATION_VERTICAL, 50);
	gtk_widget_set_margin_top (box, 50);
	gtk_widget_set_margin_bottom (box, 50);
	gtk_widget_set_margin_start (box, 50);
	gtk_widget_set_margin_end (box, 50);
	gtk_box_pack_start (GTK_BOX (box), label, TRUE, TRUE, 0);
	gtk_box_pack_start (GTK_BOX (box), secondary_label, TRUE, TRUE, 0);

	g_signal_connect (dialog,
			  "response",
			  G_CALLBACK (accel_dialog_response_cb),
			  button);
	g_signal_connect (dialog,
			  "key-press-event",
			  G_CALLBACK (accel_dialog_keypress_cb),
			  button);

	gtk_container_add (GTK_CONTAINER (content_area), box);
	gtk_widget_show_all (dialog);
}


static void
gth_accel_button_init (GthAccelButton *self)
{
	self->priv = gth_accel_button_get_instance_private (self);
	self->priv->keyval = 0;
	self->priv->modifiers = 0;
	self->priv->valid = FALSE;

	self->priv->label = gtk_label_new ("");
	gtk_widget_show (self->priv->label);
	gtk_container_add (GTK_CONTAINER (self), self->priv->label);

	_update_label (self);

	g_signal_connect (self, "clicked", G_CALLBACK (button_clicked_cb), self);
}


GtkWidget *
gth_accel_button_new (void)
{
	return (GtkWidget*) g_object_new (GTH_TYPE_ACCEL_BUTTON, NULL);
}


gboolean
gth_accel_button_set_accelerator (GthAccelButton  *self,
		 	 	  guint            keyval,
				  GdkModifierType  modifiers)
{
	g_return_val_if_fail (GTH_IS_ACCEL_BUTTON (self), FALSE);

	self->priv->keyval = keyval;
	self->priv->modifiers = modifiers;

	_update_label (self);

	return self->priv->valid;
}


gboolean
gth_accel_button_get_accelerator (GthAccelButton  *self,
		 	 	  guint           *keyval,
				  GdkModifierType *modifiers)
{
	g_return_val_if_fail (GTH_IS_ACCEL_BUTTON (self), FALSE);

	if (! self->priv->valid) {
		if (keyval) *keyval = 0;
		if (modifiers) *modifiers = 0;
		return FALSE;
	}

	if (keyval) *keyval = self->priv->keyval;
	if (modifiers) *modifiers = self->priv->modifiers;

	return TRUE;
}


gboolean
gth_accel_button_get_valid (GthAccelButton *self)
{
	g_return_val_if_fail (GTH_IS_ACCEL_BUTTON (self), FALSE);
	return self->priv->valid;
}
