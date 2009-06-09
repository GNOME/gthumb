/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2008 Free Software Foundation, Inc.
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
#include "glib-utils.h"
#include "gth-extensions.h"

#define EXTENSION_SUFFIX ".extension"


/* -- gth_extension --  */


static gpointer gth_extension_parent_class = NULL;


static gboolean
gth_extension_base_open (GthExtension *self)
{
	g_return_val_if_fail (GTH_IS_EXTENSION (self), FALSE);
	return FALSE;
}


static void
gth_extension_base_close (GthExtension *self)
{
	g_return_if_fail (GTH_IS_EXTENSION (self));
}


static void
gth_extension_base_activate (GthExtension *self)
{
	g_return_if_fail (GTH_IS_EXTENSION (self));
	self->active = TRUE;
}


static void
gth_extension_base_deactivate (GthExtension *self)
{
	g_return_if_fail (GTH_IS_EXTENSION (self));
	self->active = FALSE;
}


static gboolean
gth_extension_base_is_configurable (GthExtension *self)
{
	g_return_val_if_fail (GTH_IS_EXTENSION (self), FALSE);
	return FALSE;
}


static void
gth_extension_base_configure (GthExtension *self,
			      GtkWindow    *parent)
{
	g_return_if_fail (GTH_IS_EXTENSION (self));
	g_return_if_fail (GTK_IS_WINDOW (parent));
}


static void
gth_extension_class_init (GthExtensionClass *klass)
{
	gth_extension_parent_class = g_type_class_peek_parent (klass);

	klass->open = gth_extension_base_open;
	klass->close = gth_extension_base_close;
	klass->activate = gth_extension_base_activate;
	klass->deactivate = gth_extension_base_deactivate;
	klass->is_configurable = gth_extension_base_is_configurable;
	klass->configure = gth_extension_base_configure;
}


static void
gth_extension_instance_init (GthExtension *self)
{
	self->initialized = FALSE;
	self->active = FALSE;
}


GType
gth_extension_get_type (void)
{
	static GType gth_extension_type_id = 0;
	if (gth_extension_type_id == 0) {
		static const GTypeInfo g_define_type_info = {
			sizeof (GthExtensionClass),
			(GBaseInitFunc) NULL,
			(GBaseFinalizeFunc) NULL,
			(GClassInitFunc) gth_extension_class_init,
			(GClassFinalizeFunc) NULL,
			NULL,
			sizeof (GthExtension),
			0,
			(GInstanceInitFunc) gth_extension_instance_init,
			NULL
		};
		gth_extension_type_id = g_type_register_static (G_TYPE_OBJECT, "GthExtension", &g_define_type_info, 0);
	}
	return gth_extension_type_id;
}


gboolean
gth_extension_open (GthExtension *self)
{
	return GTH_EXTENSION_GET_CLASS (self)->open (self);
}


void
gth_extension_close (GthExtension *self)
{
	GTH_EXTENSION_GET_CLASS (self)->close (self);
}


gboolean
gth_extension_is_active (GthExtension *self)
{
	g_return_val_if_fail (GTH_IS_EXTENSION (self), FALSE);
	return self->active;
}


void
gth_extension_activate (GthExtension *self)
{
	GTH_EXTENSION_GET_CLASS (self)->activate (self);
}


void
gth_extension_deactivate (GthExtension *self)
{
	GTH_EXTENSION_GET_CLASS (self)->deactivate (self);
}


gboolean
gth_extension_is_configurable (GthExtension *self)
{
	return GTH_EXTENSION_GET_CLASS (self)->is_configurable (self);
}


void
gth_extension_configure (GthExtension *self,
				GtkWindow          *parent)
{
	GTH_EXTENSION_GET_CLASS (self)->configure (self, parent);
}


/* -- gth_extension_module -- */


static gpointer gth_extension_module_parent_class = NULL;


struct _GthExtensionModulePrivate {
	char    *module_name;
	GModule *module;
};


static gboolean
gth_extension_module_real_open (GthExtension *base)
{
	GthExtensionModule *self;
	char *file_name;

	self = GTH_EXTENSION_MODULE (base);

	if (self->priv->module != NULL)
		g_module_close (self->priv->module);

#ifdef RUN_IN_PLACE
	{
		char *extension_dir;

		extension_dir = g_build_filename (GTHUMB_EXTENSIONS_DIR, self->priv->module_name, ".libs", NULL);
		file_name = g_module_build_path (extension_dir, self->priv->module_name);
		g_free (extension_dir);
	}
#else
	file_name = g_module_build_path (GTHUMB_EXTENSIONS_DIR, self->priv->module_name);
#endif
	self->priv->module = g_module_open (file_name, G_MODULE_BIND_LAZY);
	g_free (file_name);

	if (self->priv->module == NULL) {
		/*g_warning ("could not open the module `%s`: %s\n", self->priv->module_name, g_module_error ());*/
	}

	return self->priv->module != NULL;
}


static void
gth_extension_module_real_close (GthExtension *base)
{
	GthExtensionModule *self;

	self = GTH_EXTENSION_MODULE (base);

	if (self->priv->module != NULL) {
		g_module_close (self->priv->module);
		self->priv->module = NULL;
	}
}


static char *
get_module_function_name (GthExtensionModule *self,
			  const char         *function_name)
{
	return g_strconcat ("gthumb_extension_", function_name, NULL);
}


static void
gth_extension_module_exec_generic_func (GthExtensionModule *self,
					const char         *name)
{
	char *function_name;
	void (*func) (void);

	g_return_if_fail (GTH_IS_EXTENSION_MODULE (self));
	g_return_if_fail (name != NULL);

	function_name = get_module_function_name (self, name);
	if (g_module_symbol (self->priv->module, function_name, (gpointer *)&func))
		func();
	else
		g_warning ("could not exec module function: %s\n", g_module_error ());

	g_free (function_name);
}


static void
gth_extension_module_real_activate (GthExtension *base)
{
	GthExtensionModule *self;

	self = GTH_EXTENSION_MODULE (base);

	if (base->active)
		return;

	gth_extension_module_exec_generic_func (self, "activate");
	base->active = TRUE;
}


static void
gth_extension_module_real_deactivate (GthExtension *base)
{
	GthExtensionModule *self;

	self = GTH_EXTENSION_MODULE (base);

	gth_extension_module_exec_generic_func (self, "deactivate");
}


static gboolean
gth_extension_module_real_is_configurable (GthExtension *base)
{
	GthExtensionModule *self;
	char               *function_name;
	gboolean            result = FALSE;
	gboolean (*is_configurable_func) ();

	g_return_val_if_fail (GTH_IS_EXTENSION_MODULE (base), FALSE);

	self = GTH_EXTENSION_MODULE (base);

	function_name = get_module_function_name (self, "is_configurable");
	if (g_module_symbol (self->priv->module, function_name, (gpointer *)&is_configurable_func))
		result = is_configurable_func();
	g_free (function_name);

	return result;
}


static void
gth_extension_module_real_configure (GthExtension *base,
				     GtkWindow    *parent)
{
	GthExtensionModule *self;
	char               *function_name;
	void (*configure_func) (GtkWindow *);

	g_return_if_fail (GTK_IS_WINDOW (parent));

	self = GTH_EXTENSION_MODULE (base);

	function_name = get_module_function_name (self, "configure");
	if (g_module_symbol (self->priv->module, function_name, (gpointer *)&configure_func))
		configure_func (parent);

	g_free (function_name);
}


static void
gth_extension_module_finalize (GObject *obj)
{
	GthExtensionModule *self;

	self = GTH_EXTENSION_MODULE (obj);

	if (self->priv != NULL) {
		if (self->priv->module != NULL)
			g_module_close (self->priv->module);
		g_free (self->priv->module_name);
		g_free (self->priv);
	}

	G_OBJECT_CLASS (gth_extension_module_parent_class)->finalize (obj);
}


static void
gth_extension_module_class_init (GthExtensionModuleClass *klass)
{
	GthExtensionClass *elc;

	gth_extension_module_parent_class = g_type_class_peek_parent (klass);

	G_OBJECT_CLASS (klass)->finalize = gth_extension_module_finalize;

	elc = GTH_EXTENSION_CLASS (klass);
	elc->open = gth_extension_module_real_open;
	elc->close = gth_extension_module_real_close;
	elc->activate = gth_extension_module_real_activate;
	elc->deactivate = gth_extension_module_real_deactivate;
	elc->is_configurable = gth_extension_module_real_is_configurable;
	elc->configure = gth_extension_module_real_configure;
}


static void
gth_extension_module_instance_init (GthExtensionModule *self)
{
	self->priv = g_new0 (GthExtensionModulePrivate, 1);
}


GType
gth_extension_module_get_type (void)
{
	static GType gth_extension_module_type_id = 0;
	if (gth_extension_module_type_id == 0) {
		static const GTypeInfo g_define_type_info = {
			sizeof (GthExtensionModuleClass),
			(GBaseInitFunc) NULL,
			(GBaseFinalizeFunc) NULL,
			(GClassInitFunc) gth_extension_module_class_init,
			(GClassFinalizeFunc) NULL,
			NULL,
			sizeof (GthExtensionModule),
			0,
			(GInstanceInitFunc) gth_extension_module_instance_init,
			NULL
		};
		gth_extension_module_type_id = g_type_register_static (GTH_TYPE_EXTENSION, "GthExtensionModule", &g_define_type_info, 0);
	}
	return gth_extension_module_type_id;
}


GthExtension *
gth_extension_module_new (const char *module_name)
{
	GthExtension *loader;

	loader = g_object_new (GTH_TYPE_EXTENSION_MODULE, NULL);
	GTH_EXTENSION_MODULE (loader)->priv->module_name = g_strdup (module_name);

	return loader;
}


/* -- gth_extension_description -- */


static gpointer gth_extension_description_parent_class = NULL;


static void
gth_extension_description_finalize (GObject *obj)
{
	GthExtensionDescription *self;

	self = GTH_EXTENSION_DESCRIPTION (obj);

	g_free (self->name);
	g_free (self->description);
	g_strfreev (self->authors);
	g_free (self->copyright);
	g_free (self->version);
	g_free (self->url);
	g_free (self->loader_type);
	g_free (self->loader_file);

	G_OBJECT_CLASS (gth_extension_description_parent_class)->finalize (obj);
}


static void
gth_extension_description_class_init (GthExtensionDescriptionClass *klass)
{
	gth_extension_description_parent_class = g_type_class_peek_parent (klass);
	G_OBJECT_CLASS (klass)->finalize = gth_extension_description_finalize;
}


static void
gth_extension_description_instance_init (GthExtensionDescription *self)
{
}


GType
gth_extension_description_get_type (void)
{
	static GType gth_extension_description_type_id = 0;
	if (gth_extension_description_type_id == 0) {
		static const GTypeInfo g_define_type_info = {
			sizeof (GthExtensionDescriptionClass),
			(GBaseInitFunc) NULL,
			(GBaseFinalizeFunc) NULL,
			(GClassInitFunc) gth_extension_description_class_init,
			(GClassFinalizeFunc) NULL,
			NULL,
			sizeof (GthExtensionDescription),
			0,
			(GInstanceInitFunc) gth_extension_description_instance_init,
			NULL
		};
		gth_extension_description_type_id = g_type_register_static (G_TYPE_OBJECT, "GthExtensionDescription", &g_define_type_info, 0);
	}
	return gth_extension_description_type_id;
}


static void
gth_extension_description_load_from_file (GthExtensionDescription *desc,
					  const char              *file_name)
{
	GKeyFile *key_file;

	key_file = g_key_file_new ();
	g_key_file_load_from_file (key_file, file_name, G_KEY_FILE_NONE, NULL);
	desc->name = g_key_file_get_string (key_file, "Extension", "Name", NULL);
	desc->description = g_key_file_get_string (key_file, "Extension", "Description", NULL);
	desc->version = g_key_file_get_string (key_file, "Extension", "Version", NULL);
	desc->authors = g_key_file_get_string_list (key_file, "Extension", "Authors", NULL, NULL);
	desc->copyright = g_key_file_get_string (key_file, "Extension", "Copyright", NULL);
	desc->url = g_key_file_get_string (key_file, "Extension", "URL", NULL);
	desc->loader_type = g_key_file_get_string (key_file, "Loader", "Type", NULL);
	desc->loader_file = g_key_file_get_string (key_file, "Loader", "File", NULL);
	g_key_file_free (key_file);
}


GthExtensionDescription *
gth_extension_description_new (GFile *file)
{
	GthExtensionDescription *desc;

	desc = g_object_new (GTH_TYPE_EXTENSION_DESCRIPTION, NULL);
	if (file != NULL) {
		char *file_name;
		file_name = g_file_get_path (file);
		gth_extension_description_load_from_file (desc, file_name);
		g_free (file_name);
	}

	return desc;
}


/* -- gth_extension_manager --  */


static gpointer gth_extension_manager_parent_class = NULL;


struct _GthExtensionManagerPrivate {
	GList *extensions;
};


static void
gth_extension_manager_finalize (GObject *obj)
{
	GthExtensionManager *self;

	self = GTH_EXTENSION_MANAGER (obj);

	if (self->priv != NULL) {
		_g_object_list_unref (self->priv->extensions);
		g_free (self->priv);
	}

	G_OBJECT_CLASS (gth_extension_manager_parent_class)->finalize (obj);
}


static void
gth_extension_manager_class_init (GthExtensionManagerClass *klass)
{
	gth_extension_manager_parent_class = g_type_class_peek_parent (klass);

	G_OBJECT_CLASS (klass)->finalize = gth_extension_manager_finalize;
}


static void
gth_extension_manager_instance_init (GthExtensionManager *self)
{
	self->priv = g_new0 (GthExtensionManagerPrivate, 1);
}


GType
gth_extension_manager_get_type (void) {
	static GType gth_extension_manager_type_id = 0;
	if (gth_extension_manager_type_id == 0) {
		static const GTypeInfo g_define_type_info = {
			sizeof (GthExtensionManagerClass),
			(GBaseInitFunc) NULL,
			(GBaseFinalizeFunc) NULL,
			(GClassInitFunc) gth_extension_manager_class_init,
			(GClassFinalizeFunc) NULL,
			NULL,
			sizeof (GthExtensionManager),
			0,
			(GInstanceInitFunc) gth_extension_manager_instance_init,
			NULL
		};
		gth_extension_manager_type_id = g_type_register_static (G_TYPE_OBJECT, "GthExtensionManager", &g_define_type_info, 0);
	}
	return gth_extension_manager_type_id;
}


static GthExtensionManager *
gth_extension_manager_construct (GType object_type)
{
	GthExtensionManager *self;
	self = g_object_newv (object_type, 0, NULL);
	return self;
}


GthExtensionManager *
gth_extension_manager_new (void)
{
	return gth_extension_manager_construct (GTH_TYPE_EXTENSION_MANAGER);
}


void
gth_extension_manager_load_extensions (GthExtensionManager *self)
{
	GFile           *extensions_dir;
	GFileEnumerator *enumerator;
	GFileInfo       *info;

	g_return_if_fail (GTH_IS_EXTENSION_MANAGER (self));

	_g_object_list_unref (self->priv->extensions);
	self->priv->extensions = NULL;

	extensions_dir = g_file_new_for_path (GTHUMB_EXTENSIONS_DIR);
	enumerator = g_file_enumerate_children (extensions_dir, G_FILE_ATTRIBUTE_STANDARD_NAME, 0, NULL, NULL);
	while ((info = g_file_enumerator_next_file (enumerator, NULL, NULL)) != NULL) {
		const char *name;

		name = g_file_info_get_name (info);
		if ((name != NULL) && g_str_has_suffix (name, EXTENSION_SUFFIX)) {
			GFile                   *ext_file;
			GthExtensionDescription *ext_desc;

			ext_file = g_file_get_child (extensions_dir, name);
			ext_desc = gth_extension_description_new (ext_file);
			if (ext_desc != NULL)
				self->priv->extensions = g_list_prepend (self->priv->extensions, ext_desc);

			g_object_unref (ext_file);
		}

		g_object_unref (info);
	}
	g_object_unref (enumerator);
	g_object_unref (extensions_dir);
}


GthExtension *
gth_extension_manager_get_extension (GthExtensionManager     *self,
				     GthExtensionDescription *desc)
{
	GthExtension *loader = NULL;
	return loader;
}
