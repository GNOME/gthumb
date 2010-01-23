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
#include "gth-account-chooser-dialog.h"


static gpointer parent_class = NULL;


struct _GthAccountChooserDialogPrivate {
	GList *accounts;
};


static void
gth_account_chooser_dialog_finalize (GObject *object)
{
	GthAccountChooserDialog *self;

	self = GTH_ACCOUNT_CHOOSER_DIALOG (object);
	_g_string_list_free (self->priv->accounts);

	G_OBJECT_CLASS (parent_class)->finalize (object);
}


static void
gth_account_chooser_dialog_class_init (GthAccountChooserDialogClass *klass)
{
	GObjectClass *object_class;

	parent_class = g_type_class_peek_parent (klass);
	g_type_class_add_private (klass, sizeof (GthAccountChooserDialogPrivate));

	object_class = (GObjectClass*) klass;
	object_class->finalize = gth_account_chooser_dialog_finalize;
}


static void
gth_account_chooser_dialog_init (GthAccountChooserDialog *self)
{
	self->priv = G_TYPE_INSTANCE_GET_PRIVATE (self, GTH_TYPE_ACCOUNT_CHOOSER_DIALOG, GthAccountChooserDialogPrivate);
	self->priv->accounts = NULL;
}


GType
gth_account_chooser_dialog_get_type (void)
{
	static GType type = 0;

	if (type == 0) {
		static const GTypeInfo g_define_type_info = {
			sizeof (GthAccountChooserDialogClass),
			(GBaseInitFunc) NULL,
			(GBaseFinalizeFunc) NULL,
			(GClassInitFunc) gth_account_chooser_dialog_class_init,
			(GClassFinalizeFunc) NULL,
			NULL,
			sizeof (GthAccountChooserDialog),
			0,
			(GInstanceInitFunc) gth_account_chooser_dialog_init,
			NULL
		};
		type = g_type_register_static (GTK_TYPE_DIALOG,
					       "GthAccountChooserDialog",
					       &g_define_type_info,
					       0);
	}

	return type;
}


GtkWidget *
gth_account_chooser_dialog_new (GList *accounts)
{
	GthAccountChooserDialog *self;

	self = g_object_new (GTH_TYPE_ACCOUNT_CHOOSER_DIALOG, NULL);
	self->priv->accounts = _g_string_list_dup (accounts);

	return (GtkWidget *) self;
}


const char *
gth_account_chooser_dialog_get_active (GthAccountChooserDialog *self)
{
	/* FIXME */
	return NULL;
}
