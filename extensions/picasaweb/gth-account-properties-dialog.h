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
 
#ifndef GTH_ACCOUNT_PROPERTIES_DIALOG_H
#define GTH_ACCOUNT_PROPERTIES_DIALOG_H

#include <gtk/gtk.h>
#include <gthumb.h>

G_BEGIN_DECLS

#define GTH_TYPE_ACCOUNT_PROPERTIES_DIALOG            (gth_account_properties_dialog_get_type ())
#define GTH_ACCOUNT_PROPERTIES_DIALOG(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GTH_TYPE_ACCOUNT_PROPERTIES_DIALOG, GthAccountPropertiesDialog))
#define GTH_ACCOUNT_PROPERTIES_DIALOG_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GTH_TYPE_ACCOUNT_PROPERTIES_DIALOG, GthAccountPropertiesDialogClass))
#define GTH_IS_ACCOUNT_PROPERTIES_DIALOG(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GTH_TYPE_ACCOUNT_PROPERTIES_DIALOG))
#define GTH_IS_ACCOUNT_PROPERTIES_DIALOG_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GTH_TYPE_ACCOUNT_PROPERTIES_DIALOG))
#define GTH_ACCOUNT_PROPERTIES_DIALOG_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GTH_TYPE_ACCOUNT_PROPERTIES_DIALOG, GthAccountPropertiesDialogClass))

typedef struct _GthAccountPropertiesDialog GthAccountPropertiesDialog;
typedef struct _GthAccountPropertiesDialogClass GthAccountPropertiesDialogClass;
typedef struct _GthAccountPropertiesDialogPrivate GthAccountPropertiesDialogPrivate;

struct _GthAccountPropertiesDialog {
	GtkDialog parent_instance;
	GthAccountPropertiesDialogPrivate *priv;
};

struct _GthAccountPropertiesDialogClass {
	GtkDialogClass parent_class;
};

GType          gth_account_properties_dialog_get_type      (void);
GtkWidget *    gth_account_properties_dialog_new           (const char *email,
							    const char *password);
const char *   gth_account_properties_dialog_get_email     (GthAccountPropertiesDialog *self);
const char *   gth_account_properties_dialog_get_password  (GthAccountPropertiesDialog *self);
const char *   gth_account_properties_dialog_get_challange (GthAccountPropertiesDialog *self);

G_END_DECLS

#endif /* GTH_ACCOUNT_PROPERTIES_DIALOG_H */
