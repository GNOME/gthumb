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

#ifndef FLICKR_ACCOUNT_MANAGER_DIALOG_H
#define FLICKR_ACCOUNT_MANAGER_DIALOG_H

#include <gtk/gtk.h>
#include <gthumb.h>

G_BEGIN_DECLS

#define FLICKR_ACCOUNT_MANAGER_RESPONSE_NEW 1

#define FLICKR_TYPE_ACCOUNT_MANAGER_DIALOG            (flickr_account_manager_dialog_get_type ())
#define FLICKR_ACCOUNT_MANAGER_DIALOG(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), FLICKR_TYPE_ACCOUNT_MANAGER_DIALOG, FlickrAccountManagerDialog))
#define FLICKR_ACCOUNT_MANAGER_DIALOG_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), FLICKR_TYPE_ACCOUNT_MANAGER_DIALOG, FlickrAccountManagerDialogClass))
#define FLICKR_IS_ACCOUNT_MANAGER_DIALOG(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), FLICKR_TYPE_ACCOUNT_MANAGER_DIALOG))
#define FLICKR_IS_ACCOUNT_MANAGER_DIALOG_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), FLICKR_TYPE_ACCOUNT_MANAGER_DIALOG))
#define FLICKR_ACCOUNT_MANAGER_DIALOG_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), FLICKR_TYPE_ACCOUNT_MANAGER_DIALOG, FlickrAccountManagerDialogClass))

typedef struct _FlickrAccountManagerDialog FlickrAccountManagerDialog;
typedef struct _FlickrAccountManagerDialogClass FlickrAccountManagerDialogClass;
typedef struct _FlickrAccountManagerDialogPrivate FlickrAccountManagerDialogPrivate;

struct _FlickrAccountManagerDialog {
	GtkDialog parent_instance;
	FlickrAccountManagerDialogPrivate *priv;
};

struct _FlickrAccountManagerDialogClass {
	GtkDialogClass parent_class;
};

GType          flickr_account_manager_dialog_get_type     (void);
GtkWidget *    flickr_account_manager_dialog_new          (GList                      *accounts);
GList *        flickr_account_manager_dialog_get_accounts (FlickrAccountManagerDialog *dialog);

G_END_DECLS

#endif /* FLICKR_ACCOUNT_MANAGER_DIALOG_H */
