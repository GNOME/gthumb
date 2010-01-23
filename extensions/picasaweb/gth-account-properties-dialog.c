/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2010 Free Software Foundation, Inc.
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
#include "gth-account-properties-dialog.h"


static gpointer parent_class = NULL;


struct _GthAccountPropertiesDialogPrivate {
	char *email;
	char *password;
	char *challange;
};


static void
gth_account_properties_dialog_finalize (GObject *object)
{
	GthAccountPropertiesDialog *self;

	self = GTH_ACCOUNT_PROPERTIES_DIALOG (object);
	g_free (self->priv->email);
	g_free (self->priv->password);
	g_free (self->priv->challange);

	G_OBJECT_CLASS (parent_class)->finalize (object);
}


static void
gth_account_properties_dialog_class_init (GthAccountPropertiesDialogClass *klass)
{
	GObjectClass *object_class;

	parent_class = g_type_class_peek_parent (klass);
	g_type_class_add_private (klass, sizeof (GthAccountPropertiesDialogPrivate));

	object_class = (GObjectClass*) klass;
	object_class->finalize = gth_account_properties_dialog_finalize;
}


static void
gth_account_properties_dialog_init (GthAccountPropertiesDialog *self)
{
	self->priv = G_TYPE_INSTANCE_GET_PRIVATE (self, GTH_TYPE_ACCOUNT_PROPERTIES_DIALOG, GthAccountPropertiesDialogPrivate);
	self->priv->email = NULL;
	self->priv->password = NULL;
	self->priv->challange = NULL;
}


GType
gth_account_properties_dialog_get_type (void)
{
	static GType type = 0;

	if (type == 0) {
		static const GTypeInfo g_define_type_info = {
			sizeof (GthAccountPropertiesDialogClass),
			(GBaseInitFunc) NULL,
			(GBaseFinalizeFunc) NULL,
			(GClassInitFunc) gth_account_properties_dialog_class_init,
			(GClassFinalizeFunc) NULL,
			NULL,
			sizeof (GthAccountPropertiesDialog),
			0,
			(GInstanceInitFunc) gth_account_properties_dialog_init,
			NULL
		};
		type = g_type_register_static (GTK_TYPE_DIALOG,
					       "GthAccountPropertiesDialog",
					       &g_define_type_info,
					       0);
	}

	return type;
}


GtkWidget *
gth_account_properties_dialog_new (const char *email,
			           const char *password)
{
	GthAccountPropertiesDialog *self;

	self = g_object_new (GTH_TYPE_ACCOUNT_PROPERTIES_DIALOG, NULL);
	self->priv->email = g_strdup (email);
	self->priv->password = g_strdup (password);

	return (GtkWidget *) self;
}


const char *
gth_account_properties_dialog_get_email (GthAccountPropertiesDialog *self)
{
	return self->priv->email;
}


const char *
gth_account_properties_dialog_get_password (GthAccountPropertiesDialog *self)
{
	return self->priv->password;
}


const char *
gth_account_properties_dialog_get_challange (GthAccountPropertiesDialog *self)
{
	return self->priv->challange;
}
