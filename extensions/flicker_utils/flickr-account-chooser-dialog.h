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

#ifndef FLICKR_ACCOUNT_CHOOSER_DIALOG_H
#define FLICKR_ACCOUNT_CHOOSER_DIALOG_H

#include <gtk/gtk.h>
#include <gthumb.h>
#include "flickr-account.h"

G_BEGIN_DECLS

#define FLICKR_ACCOUNT_CHOOSER_RESPONSE_NEW 1

#define FLICKR_TYPE_ACCOUNT_CHOOSER_DIALOG            (flickr_account_chooser_dialog_get_type ())
#define FLICKR_ACCOUNT_CHOOSER_DIALOG(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), FLICKR_TYPE_ACCOUNT_CHOOSER_DIALOG, FlickrAccountChooserDialog))
#define FLICKR_ACCOUNT_CHOOSER_DIALOG_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), FLICKR_TYPE_ACCOUNT_CHOOSER_DIALOG, FlickrAccountChooserDialogClass))
#define FLICKR_IS_ACCOUNT_CHOOSER_DIALOG(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), FLICKR_TYPE_ACCOUNT_CHOOSER_DIALOG))
#define FLICKR_IS_ACCOUNT_CHOOSER_DIALOG_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), FLICKR_TYPE_ACCOUNT_CHOOSER_DIALOG))
#define FLICKR_ACCOUNT_CHOOSER_DIALOG_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), FLICKR_TYPE_ACCOUNT_CHOOSER_DIALOG, FlickrAccountChooserDialogClass))

typedef struct _FlickrAccountChooserDialog FlickrAccountChooserDialog;
typedef struct _FlickrAccountChooserDialogClass FlickrAccountChooserDialogClass;
typedef struct _FlickrAccountChooserDialogPrivate FlickrAccountChooserDialogPrivate;

struct _FlickrAccountChooserDialog {
	GtkDialog parent_instance;
	FlickrAccountChooserDialogPrivate *priv;
};

struct _FlickrAccountChooserDialogClass {
	GtkDialogClass parent_class;
};

GType           flickr_account_chooser_dialog_get_type    (void);
GtkWidget *     flickr_account_chooser_dialog_new         (GList                      *accounts,
							   FlickrAccount              *default_account);
FlickrAccount * flickr_account_chooser_dialog_get_active  (FlickrAccountChooserDialog *self);

G_END_DECLS

#endif /* FLICKR_ACCOUNT_CHOOSER_DIALOG_H */
