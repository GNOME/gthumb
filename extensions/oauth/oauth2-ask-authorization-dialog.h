/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2012 Free Software Foundation, Inc.
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

#ifndef OAUTH2_ASK_AUTHORIZATION_DIALOG_H
#define OAUTH2_ASK_AUTHORIZATION_DIALOG_H

#include <gtk/gtk.h>
#include <gthumb.h>

G_BEGIN_DECLS

#define OAUTH2_TYPE_ASK_AUTHORIZATION_DIALOG            (oauth2_ask_authorization_dialog_get_type ())
#define OAUTH2_ASK_AUTHORIZATION_DIALOG(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), OAUTH2_TYPE_ASK_AUTHORIZATION_DIALOG, OAuth2AskAuthorizationDialog))
#define OAUTH2_ASK_AUTHORIZATION_DIALOG_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), OAUTH2_TYPE_ASK_AUTHORIZATION_DIALOG, OAuth2AskAuthorizationDialogClass))
#define OAUTH2_IS_ASK_AUTHORIZATION_DIALOG(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), OAUTH2_TYPE_ASK_AUTHORIZATION_DIALOG))
#define OAUTH2_IS_ASK_AUTHORIZATION_DIALOG_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), OAUTH2_TYPE_ASK_AUTHORIZATION_DIALOG))
#define OAUTH2_ASK_AUTHORIZATION_DIALOG_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), OAUTH2_TYPE_ASK_AUTHORIZATION_DIALOG, OAuth2AskAuthorizationDialogClass))

typedef struct _OAuth2AskAuthorizationDialog OAuth2AskAuthorizationDialog;
typedef struct _OAuth2AskAuthorizationDialogClass OAuth2AskAuthorizationDialogClass;
typedef struct _OAuth2AskAuthorizationDialogPrivate OAuth2AskAuthorizationDialogPrivate;

struct _OAuth2AskAuthorizationDialog {
	GtkDialog parent_instance;
	OAuth2AskAuthorizationDialogPrivate *priv;
};

struct _OAuth2AskAuthorizationDialogClass {
	GtkDialogClass parent_class;

	/*< signals >*/

	void  (*redirected)  (OAuth2AskAuthorizationDialog *self);
};

GType          oauth2_ask_authorization_dialog_get_type     (void);
GtkWidget *    oauth2_ask_authorization_dialog_new          (const char *title,
							     const char *url);
GtkWidget *    oauth2_ask_authorization_dialog_get_view     (OAuth2AskAuthorizationDialog *self);
const char *   oauth2_ask_authorization_dialog_get_uri      (OAuth2AskAuthorizationDialog *self);

G_END_DECLS

#endif /* OAUTH2_ASK_AUTHORIZATION_DIALOG_H */
