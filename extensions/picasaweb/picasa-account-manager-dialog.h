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
 
#ifndef PICASA_ACCOUNT_MANAGER_DIALOG_H
#define PICASA_ACCOUNT_MANAGER_DIALOG_H

#include <gtk/gtk.h>
#include <gthumb.h>

G_BEGIN_DECLS

#define PICASA_ACCOUNT_MANAGER_RESPONSE_NEW 1

#define PICASA_TYPE_ACCOUNT_MANAGER_DIALOG            (picasa_account_manager_dialog_get_type ())
#define PICASA_ACCOUNT_MANAGER_DIALOG(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), PICASA_TYPE_ACCOUNT_MANAGER_DIALOG, PicasaAccountManagerDialog))
#define PICASA_ACCOUNT_MANAGER_DIALOG_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), PICASA_TYPE_ACCOUNT_MANAGER_DIALOG, PicasaAccountManagerDialogClass))
#define PICASA_IS_ACCOUNT_MANAGER_DIALOG(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), PICASA_TYPE_ACCOUNT_MANAGER_DIALOG))
#define PICASA_IS_ACCOUNT_MANAGER_DIALOG_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), PICASA_TYPE_ACCOUNT_MANAGER_DIALOG))
#define PICASA_ACCOUNT_MANAGER_DIALOG_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), PICASA_TYPE_ACCOUNT_MANAGER_DIALOG, PicasaAccountManagerDialogClass))

typedef struct _PicasaAccountManagerDialog PicasaAccountManagerDialog;
typedef struct _PicasaAccountManagerDialogClass PicasaAccountManagerDialogClass;
typedef struct _PicasaAccountManagerDialogPrivate PicasaAccountManagerDialogPrivate;

struct _PicasaAccountManagerDialog {
	GtkDialog parent_instance;
	PicasaAccountManagerDialogPrivate *priv;
};

struct _PicasaAccountManagerDialogClass {
	GtkDialogClass parent_class;
};

GType          picasa_account_manager_dialog_get_type     (void);
GtkWidget *    picasa_account_manager_dialog_new          (GList                      *accounts);
GList *        picasa_account_manager_dialog_get_accounts (PicasaAccountManagerDialog *dialog);

G_END_DECLS

#endif /* PICASA_ACCOUNT_MANAGER_DIALOG_H */
