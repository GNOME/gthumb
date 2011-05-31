/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2011 Free Software Foundation, Inc.
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

#include <config.h>
#include "gth-screensaver.h"
#include <gdk/gdkx.h>


#define GNOME_SESSION_MANAGER_INHIBIT_IDLE 8


/* Properties */
enum {
        PROP_0,
        PROP_APP_ID
};


struct _GthScreensaverPrivate {
	char       *app_id;
	guint       cookie;
	GDBusProxy *proxy;
};


static gpointer parent_class = NULL;


static void
gth_screensaver_finalize (GObject *object)
{
	GthScreensaver *self;

	self = GTH_SCREENSAVER (object);

	gth_screensaver_uninhibit (self);

	g_free (self->priv->app_id);
	if (self->priv->proxy != NULL)
		g_object_unref (self->priv->proxy);

	G_OBJECT_CLASS (parent_class)->finalize (object);
}


static void
gth_screensaver_set_property (GObject      *object,
			      guint         property_id,
			      const GValue *value,
			      GParamSpec   *pspec)
{
	GthScreensaver *self;

	self = GTH_SCREENSAVER (object);

	switch (property_id) {
	case PROP_APP_ID:
		g_free (self->priv->app_id);
		self->priv->app_id = g_value_dup_string (value);
		break;
	default:
		break;
	}
}


static void
gth_screensaver_get_property (GObject    *object,
			      guint       property_id,
			      GValue     *value,
			      GParamSpec *pspec)
{
	GthScreensaver *self;

	self = GTH_SCREENSAVER (object);

	switch (property_id) {
	case PROP_APP_ID:
		g_value_set_string (value, self->priv->app_id);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
		break;
	}
}


static void
gth_screensaver_class_init (GthScreensaverClass *klass)
{
	GObjectClass *object_class;

	parent_class = g_type_class_peek_parent (klass);
	g_type_class_add_private (klass, sizeof (GthScreensaverPrivate));

	object_class = (GObjectClass*) klass;
	object_class->set_property = gth_screensaver_set_property;
	object_class->get_property = gth_screensaver_get_property;
	object_class->finalize = gth_screensaver_finalize;

	/* properties */

	g_object_class_install_property (object_class,
					 PROP_APP_ID,
					 g_param_spec_string ("app-id",
                                                              "Application ID",
                                                              "The application identifier",
                                                              NULL,
                                                              G_PARAM_READWRITE));
}


static void
gth_screensaver_init (GthScreensaver *self)
{
	self->priv = G_TYPE_INSTANCE_GET_PRIVATE (self, GTH_TYPE_SCREENSAVER, GthScreensaverPrivate);
	self->priv->app_id = NULL;
	self->priv->cookie = 0;
	self->priv->proxy = NULL;
}


GType
gth_screensaver_get_type (void)
{
        static GType type = 0;

        if (! type) {
                GTypeInfo type_info = {
			sizeof (GthScreensaverClass),
			NULL,
			NULL,
			(GClassInitFunc) gth_screensaver_class_init,
			NULL,
			NULL,
			sizeof (GthScreensaver),
			0,
			(GInstanceInitFunc) gth_screensaver_init
		};

		type = g_type_register_static (G_TYPE_OBJECT,
					       "GthScreensaver",
					       &type_info,
					       0);
	}

        return type;
}


GthScreensaver *
gth_screensaver_new (const char *application_id)
{
	if (application_id == NULL)
		application_id = g_get_application_name ();

	return (GthScreensaver*) g_object_new (GTH_TYPE_SCREENSAVER,
					       "app-id", application_id,
					       NULL);
}


static void
_gth_screensaver_create_sm_proxy (GthScreensaver  *self,
			          GError         **error)
{
	GDBusConnection *connection;

	if (self->priv->proxy != NULL)
		return;

	connection = g_bus_get_sync (G_BUS_TYPE_SESSION, NULL, error);
	if (connection == NULL)
		return;

	self->priv->proxy = g_dbus_proxy_new_sync (connection,
						   G_DBUS_PROXY_FLAGS_NONE,
						   NULL,
						   "org.gnome.SessionManager",
						   "/org/gnome/SessionManager",
						   "org.gnome.SessionManager",
						   NULL,
						   error);
}


static void
org_gnome_session_manager_inhibit_ready_cb (GObject      *source_object,
					    GAsyncResult *res,
					    gpointer      user_data)
{
	GthScreensaver  *self = user_data;
	GVariant        *value;
	GError          *error = NULL;

	value = g_dbus_proxy_call_finish (self->priv->proxy, res, &error);
	if (value == NULL) {
		g_warning ("%s\n", error->message);
		g_clear_error (&error);
		return;
	}

	g_print ("idle inhibited\n");

	g_variant_get (value, "(u)", &self->priv->cookie);

	g_variant_unref (value);
}


void
gth_screensaver_inhibit (GthScreensaver *self,
			 GtkWidget      *widget,
			 const char     *reason)
{
	GError    *error = NULL;
	guint      xid;
	GtkWidget *toplevel_window;

	if (self->priv->cookie != 0)
		return;

	_gth_screensaver_create_sm_proxy (self, &error);

	if (error != NULL) {
		g_warning ("%s\n", error->message);
		g_clear_error (&error);
		return;
	}

	xid = 0;
	toplevel_window = gtk_widget_get_toplevel (widget);
	if (gtk_widget_is_toplevel (toplevel_window))
		xid = GDK_WINDOW_XID (gtk_widget_get_window (toplevel_window));

	g_dbus_proxy_call (self->priv->proxy,
			   "Inhibit",
			   g_variant_new ("(susu)",
					  self->priv->app_id,
					  xid,
					  reason,
					  GNOME_SESSION_MANAGER_INHIBIT_IDLE),
			   G_DBUS_CALL_FLAGS_NONE,
			   G_MAXINT,
			   NULL,
			   org_gnome_session_manager_inhibit_ready_cb,
			   self);
}


static void
org_gnome_session_manager_uninhibit_ready_cb (GObject      *source_object,
					      GAsyncResult *res,
					      gpointer      user_data)
{
	GthScreensaver  *self = user_data;
	GVariant        *value;
	GError          *error = NULL;

	value = g_dbus_proxy_call_finish (self->priv->proxy, res, &error);
	if (value == NULL) {
		g_warning ("%s\n", error->message);
		g_clear_error (&error);
	}

	g_print ("idle uninhibited\n");

	self->priv->cookie = 0;

	if (value != NULL)
		g_variant_unref (value);
}


void
gth_screensaver_uninhibit (GthScreensaver *self)
{
	GError *error = NULL;

	if (self->priv->cookie == 0)
		return;

	_gth_screensaver_create_sm_proxy (self, &error);

	if (error != NULL) {
		g_warning ("%s\n", error->message);
		g_clear_error (&error);
		return;
	}

	g_dbus_proxy_call (self->priv->proxy,
			   "Uninhibit",
			   g_variant_new ("(u)", self->priv->cookie),
			   G_DBUS_CALL_FLAGS_NONE,
			   G_MAXINT,
			   NULL,
			   org_gnome_session_manager_uninhibit_ready_cb,
			   self);
}
