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
 
#ifndef PICASA_ACCOUNT_CHOOSER_DIALOG_H
#define PICASA_ACCOUNT_CHOOSER_DIALOG_H

#include <gtk/gtk.h>
#include <gthumb.h>

G_BEGIN_DECLS

#define PICASA_ACCOUNT_CHOOSER_RESPONSE_NEW 1

#define PICASA_TYPE_ACCOUNT_CHOOSER_DIALOG            (picasa_account_chooser_dialog_get_type ())
#define PICASA_ACCOUNT_CHOOSER_DIALOG(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), PICASA_TYPE_ACCOUNT_CHOOSER_DIALOG, PicasaAccountChooserDialog))
#define PICASA_ACCOUNT_CHOOSER_DIALOG_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), PICASA_TYPE_ACCOUNT_CHOOSER_DIALOG, PicasaAccountChooserDialogClass))
#define PICASA_IS_ACCOUNT_CHOOSER_DIALOG(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), PICASA_TYPE_ACCOUNT_CHOOSER_DIALOG))
#define PICASA_IS_ACCOUNT_CHOOSER_DIALOG_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), PICASA_TYPE_ACCOUNT_CHOOSER_DIALOG))
#define PICASA_ACCOUNT_CHOOSER_DIALOG_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), PICASA_TYPE_ACCOUNT_CHOOSER_DIALOG, PicasaAccountChooserDialogClass))

typedef struct _PicasaAccountChooserDialog PicasaAccountChooserDialog;
typedef struct _PicasaAccountChooserDialogClass PicasaAccountChooserDialogClass;
typedef struct _PicasaAccountChooserDialogPrivate PicasaAccountChooserDialogPrivate;

struct _PicasaAccountChooserDialog {
	GtkDialog parent_instance;
	PicasaAccountChooserDialogPrivate *priv;
};

struct _PicasaAccountChooserDialogClass {
	GtkDialogClass parent_class;
};

GType          picasa_account_chooser_dialog_get_type    (void);
GtkWidget *    picasa_account_chooser_dialog_new         (GList      *accounts,
							  const char *_default);
char *         picasa_account_chooser_dialog_get_active  (PicasaAccountChooserDialog *self);

G_END_DECLS

#endif /* PICASA_ACCOUNT_CHOOSER_DIALOG_H */
