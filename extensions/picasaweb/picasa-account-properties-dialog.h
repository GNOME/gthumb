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
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
 
#ifndef PICASA_ACCOUNT_PROPERTIES_DIALOG_H
#define PICASA_ACCOUNT_PROPERTIES_DIALOG_H

#include <gtk/gtk.h>
#include <gthumb.h>

G_BEGIN_DECLS

#define PICASA_ACCOUNT_PROPERTIES_RESPONSE_CHOOSE 1

#define PICASA_TYPE_ACCOUNT_PROPERTIES_DIALOG            (picasa_account_properties_dialog_get_type ())
#define PICASA_ACCOUNT_PROPERTIES_DIALOG(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), PICASA_TYPE_ACCOUNT_PROPERTIES_DIALOG, PicasaAccountPropertiesDialog))
#define PICASA_ACCOUNT_PROPERTIES_DIALOG_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), PICASA_TYPE_ACCOUNT_PROPERTIES_DIALOG, PicasaAccountPropertiesDialogClass))
#define PICASA_IS_ACCOUNT_PROPERTIES_DIALOG(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), PICASA_TYPE_ACCOUNT_PROPERTIES_DIALOG))
#define PICASA_IS_ACCOUNT_PROPERTIES_DIALOG_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), PICASA_TYPE_ACCOUNT_PROPERTIES_DIALOG))
#define PICASA_ACCOUNT_PROPERTIES_DIALOG_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), PICASA_TYPE_ACCOUNT_PROPERTIES_DIALOG, PicasaAccountPropertiesDialogClass))

typedef struct _PicasaAccountPropertiesDialog PicasaAccountPropertiesDialog;
typedef struct _PicasaAccountPropertiesDialogClass PicasaAccountPropertiesDialogClass;
typedef struct _PicasaAccountPropertiesDialogPrivate PicasaAccountPropertiesDialogPrivate;

struct _PicasaAccountPropertiesDialog {
	GtkDialog parent_instance;
	PicasaAccountPropertiesDialogPrivate *priv;
};

struct _PicasaAccountPropertiesDialogClass {
	GtkDialogClass parent_class;
};

GType          picasa_account_properties_dialog_get_type      (void);
GtkWidget *    picasa_account_properties_dialog_new           (const char *email,
							       const char *password,
							       const char *challange);
void           picasa_account_properties_dialog_can_choose    (PicasaAccountPropertiesDialog *self,
		       	       	       	       	       	       gboolean                       can_choose);
void           picasa_account_properties_dialog_set_error     (PicasaAccountPropertiesDialog *self,
							       GError                        *error);
const char *   picasa_account_properties_dialog_get_email     (PicasaAccountPropertiesDialog *self);
const char *   picasa_account_properties_dialog_get_password  (PicasaAccountPropertiesDialog *self);
const char *   picasa_account_properties_dialog_get_challange (PicasaAccountPropertiesDialog *self);

G_END_DECLS

#endif /* PICASA_ACCOUNT_PROPERTIES_DIALOG_H */
