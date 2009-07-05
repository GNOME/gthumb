/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2009 Free Software Foundation, Inc.
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
#include <gthumb.h>
#include "gth-script.h"


enum {
        PROP_0,
        PROP_ID,
        PROP_DISPLAY_NAME,
        PROP_COMMAND,
        PROP_VISIBLE,
        PROP_FOR_EACH_FILE,
        PROP_WAIT_COMMAND
};


struct _GthScriptPrivate {
	char     *id;
	char     *display_name;
	char     *command;
	gboolean  visible;
	gboolean  for_each_file;
	gboolean  wait_command;
};


static gpointer *parent_class = NULL;


static DomElement*
gth_script_real_create_element (DomDomizable *base,
				DomDocument  *doc)
{
	GthScript  *self;
	DomElement *element;

	g_return_val_if_fail (DOM_IS_DOCUMENT (doc), NULL);

	self = GTH_SCRIPT (base);
	element = dom_document_create_element (doc, "script",
					       "id", self->priv->id,
					       "display-name", self->priv->display_name,
					       "command", self->priv->command,
					       "for-each-file", self->priv->for_each_file ? "true" : "false",
					       "wait-command", self->priv->wait_command ? "true" : "false",
					       NULL);
	if (! self->priv->visible)
		dom_element_set_attribute (element, "display", "none");

	return element;
}


static void
gth_script_real_load_from_element (DomDomizable *base,
				   DomElement   *element)
{
	GthScript *self;

	g_return_if_fail (DOM_IS_ELEMENT (element));

	self = GTH_SCRIPT (base);
	g_object_set (self,
		      "id", dom_element_get_attribute (element, "id"),
		      "display-name", dom_element_get_attribute (element, "display-name"),
		      "command", dom_element_get_attribute (element, "command"),
		      "visible", (g_strcmp0 (dom_element_get_attribute (element, "display"), "none") != 0),
		      "for-each-file", (g_strcmp0 (dom_element_get_attribute (element, "for-each-file"), "true") == 0),
		      "wait-command", (g_strcmp0 (dom_element_get_attribute (element, "wait-command"), "true") == 0),
		      NULL);
}


GObject *
gth_script_real_duplicate (GthDuplicable *duplicable)
{
	GthScript *script = GTH_SCRIPT (duplicable);
	GthScript *new_script;

	new_script = gth_script_new ();
	g_object_set (new_script,
		      "id", script->priv->id,
		      "display-name", script->priv->display_name,
		      "command", script->priv->command,
		      "visible", script->priv->visible,
		      "for-each-file", script->priv->for_each_file,
		      "wait-command", script->priv->wait_command,
		      NULL);

	return (GObject *) new_script;
}


static void
gth_script_finalize (GObject *base)
{
	GthScript *self;

	self = GTH_SCRIPT (base);

	g_free (self->priv->id);
	g_free (self->priv->display_name);
	g_free (self->priv->command);

	G_OBJECT_CLASS (parent_class)->finalize (base);
}


static void
gth_script_set_property (GObject      *object,
		         guint         property_id,
		         const GValue *value,
		         GParamSpec   *pspec)
{
	GthScript *self;

        self = GTH_SCRIPT (object);

	switch (property_id) {
	case PROP_ID:
		g_free (self->priv->id);
		self->priv->id = g_value_dup_string (value);
		if (self->priv->id == NULL)
			self->priv->id = g_strdup ("");
		break;
	case PROP_DISPLAY_NAME:
		g_free (self->priv->display_name);
		self->priv->display_name = g_value_dup_string (value);
		if (self->priv->display_name == NULL)
			self->priv->display_name = g_strdup ("");
		break;
	case PROP_COMMAND:
		g_free (self->priv->command);
		self->priv->command = g_value_dup_string (value);
		if (self->priv->command == NULL)
			self->priv->command = g_strdup ("");
		break;
	case PROP_VISIBLE:
		self->priv->visible = g_value_get_boolean (value);
		break;
	case PROP_FOR_EACH_FILE:
		self->priv->for_each_file = g_value_get_boolean (value);
		break;
	case PROP_WAIT_COMMAND:
		self->priv->wait_command = g_value_get_boolean (value);
		break;
	default:
		break;
	}
}


static void
gth_script_get_property (GObject    *object,
		         guint       property_id,
		         GValue     *value,
		         GParamSpec *pspec)
{
	GthScript *self;

        self = GTH_SCRIPT (object);

	switch (property_id) {
	case PROP_ID:
		g_value_set_string (value, self->priv->id);
		break;
	case PROP_DISPLAY_NAME:
		g_value_set_string (value, self->priv->display_name);
		break;
	case PROP_COMMAND:
		g_value_set_string (value, self->priv->command);
		break;
	case PROP_VISIBLE:
		g_value_set_boolean (value, self->priv->visible);
		break;
	case PROP_FOR_EACH_FILE:
		g_value_set_boolean (value, self->priv->for_each_file);
		break;
	case PROP_WAIT_COMMAND:
		g_value_set_boolean (value, self->priv->wait_command);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
		break;
	}
}


static void
gth_script_class_init (GthScriptClass *klass)
{
	GObjectClass *object_class;

	parent_class = g_type_class_peek_parent (klass);
	g_type_class_add_private (klass, sizeof (GthScriptPrivate));

	object_class = G_OBJECT_CLASS (klass);
	object_class->set_property = gth_script_set_property;
	object_class->get_property = gth_script_get_property;
	object_class->finalize = gth_script_finalize;

	/* properties */

	g_object_class_install_property (object_class,
					 PROP_ID,
					 g_param_spec_string ("id",
                                                              "ID",
                                                              "The object id",
                                                              NULL,
                                                              G_PARAM_READWRITE));
	g_object_class_install_property (object_class,
					 PROP_DISPLAY_NAME,
					 g_param_spec_string ("display-name",
                                                              "Display name",
                                                              "The user visible name",
                                                              NULL,
                                                              G_PARAM_READWRITE));
	g_object_class_install_property (object_class,
					 PROP_COMMAND,
					 g_param_spec_string ("command",
                                                              "Command",
                                                              "The command to execute",
                                                              NULL,
                                                              G_PARAM_READWRITE));
	g_object_class_install_property (object_class,
					 PROP_VISIBLE,
					 g_param_spec_boolean ("visible",
							       "Visible",
							       "Whether this script should be visible in the script list",
							       FALSE,
							       G_PARAM_READWRITE));
	g_object_class_install_property (object_class,
					 PROP_FOR_EACH_FILE,
					 g_param_spec_boolean ("for-each-file",
							       "Each File",
							       "Whether to execute the command on file at a time",
							       FALSE,
							       G_PARAM_READWRITE));
	g_object_class_install_property (object_class,
					 PROP_WAIT_COMMAND,
					 g_param_spec_boolean ("wait-command",
							       "Wait command",
							       "Whether to wait command to finish",
							       FALSE,
							       G_PARAM_READWRITE));
}


static void
gth_script_dom_domizable_interface_init (DomDomizableIface *iface)
{
	iface->create_element = gth_script_real_create_element;
	iface->load_from_element = gth_script_real_load_from_element;
}


static void
gth_script_gth_duplicable_interface_init (GthDuplicableIface *iface)
{
	iface->duplicate = gth_script_real_duplicate;
}


static void
gth_script_init (GthScript *self)
{
	self->priv = G_TYPE_INSTANCE_GET_PRIVATE (self, GTH_TYPE_SCRIPT, GthScriptPrivate);
	self->priv->id = NULL;
	self->priv->display_name = NULL;
	self->priv->command = NULL;
}


GType
gth_script_get_type (void)
{
        static GType type = 0;

        if (! type) {
                GTypeInfo type_info = {
			sizeof (GthScriptClass),
			NULL,
			NULL,
			(GClassInitFunc) gth_script_class_init,
			NULL,
			NULL,
			sizeof (GthScript),
			0,
			(GInstanceInitFunc) gth_script_init
		};
		static const GInterfaceInfo dom_domizable_info = {
			(GInterfaceInitFunc) gth_script_dom_domizable_interface_init,
			(GInterfaceFinalizeFunc) NULL,
			NULL
		};
		static const GInterfaceInfo gth_duplicable_info = {
			(GInterfaceInitFunc) gth_script_gth_duplicable_interface_init,
			(GInterfaceFinalizeFunc) NULL,
			NULL
		};

		type = g_type_register_static (G_TYPE_OBJECT,
					       "GthScript",
					       &type_info,
					       0);
		g_type_add_interface_static (type, DOM_TYPE_DOMIZABLE, &dom_domizable_info);
		g_type_add_interface_static (type, GTH_TYPE_DUPLICABLE, &gth_duplicable_info);
	}

        return type;
}


GthScript*
gth_script_new (void)
{
	GthScript *script;
	char      *id;

	id = _g_rand_string (ID_LENGTH);
	script = (GthScript *) g_object_new (GTH_TYPE_SCRIPT, "id", id, NULL);
	g_free (id);

	return script;
}


const char *
gth_script_get_id (GthScript *script)
{
	return script->priv->id;
}


const char *
gth_script_get_display_name (GthScript *script)
{
	return script->priv->display_name;
}


const char *
gth_script_get_command (GthScript *script)
{
	return script->priv->command;
}


gboolean
gth_script_is_visible (GthScript *script)
{
	return script->priv->visible;
}


gboolean
gth_script_for_each_file (GthScript *script)
{
	return script->priv->for_each_file;
}


gboolean
gth_script_wait_command (GthScript *script)
{
	return script->priv->wait_command;
}
