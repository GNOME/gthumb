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

#ifndef FACEBOOK_ACCOUNT_CHOOSER_DIALOG_H
#define FACEBOOK_ACCOUNT_CHOOSER_DIALOG_H

#include <gtk/gtk.h>
#include <gthumb.h>
#include "facebook-account.h"

G_BEGIN_DECLS

#define FACEBOOK_ACCOUNT_CHOOSER_RESPONSE_NEW 1

#define FACEBOOK_TYPE_ACCOUNT_CHOOSER_DIALOG            (facebook_account_chooser_dialog_get_type ())
#define FACEBOOK_ACCOUNT_CHOOSER_DIALOG(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), FACEBOOK_TYPE_ACCOUNT_CHOOSER_DIALOG, FacebookAccountChooserDialog))
#define FACEBOOK_ACCOUNT_CHOOSER_DIALOG_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), FACEBOOK_TYPE_ACCOUNT_CHOOSER_DIALOG, FacebookAccountChooserDialogClass))
#define FACEBOOK_IS_ACCOUNT_CHOOSER_DIALOG(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), FACEBOOK_TYPE_ACCOUNT_CHOOSER_DIALOG))
#define FACEBOOK_IS_ACCOUNT_CHOOSER_DIALOG_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), FACEBOOK_TYPE_ACCOUNT_CHOOSER_DIALOG))
#define FACEBOOK_ACCOUNT_CHOOSER_DIALOG_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), FACEBOOK_TYPE_ACCOUNT_CHOOSER_DIALOG, FacebookAccountChooserDialogClass))

typedef struct _FacebookAccountChooserDialog FacebookAccountChooserDialog;
typedef struct _FacebookAccountChooserDialogClass FacebookAccountChooserDialogClass;
typedef struct _FacebookAccountChooserDialogPrivate FacebookAccountChooserDialogPrivate;

struct _FacebookAccountChooserDialog {
	GtkDialog parent_instance;
	FacebookAccountChooserDialogPrivate *priv;
};

struct _FacebookAccountChooserDialogClass {
	GtkDialogClass parent_class;
};

GType           facebook_account_chooser_dialog_get_type    (void);
GtkWidget *     facebook_account_chooser_dialog_new         (GList                      *accounts,
							   FacebookAccount              *default_account);
FacebookAccount * facebook_account_chooser_dialog_get_active  (FacebookAccountChooserDialog *self);

G_END_DECLS

#endif /* FACEBOOK_ACCOUNT_CHOOSER_DIALOG_H */
