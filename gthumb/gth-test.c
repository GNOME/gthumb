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
#include <string.h>
#include <glib/gi18n.h>
#include <glib.h>
#include <gtk/gtk.h>
#include "gth-duplicable.h"
#include "gth-test.h"
#include "glib-utils.h"


/* Properties */
enum {
        PROP_0,
        PROP_ID,
        PROP_DISPLAY_NAME,
        PROP_VISIBLE
};

/* Signals */
enum {
        CHANGED,
        LAST_SIGNAL
};

struct _GthTestPrivate
{
	char      *id;
	char      *display_name;
	gboolean   visible;
};


static gpointer parent_class = NULL;
static GthDuplicableIface *gth_duplicable_parent_iface = NULL;
static guint gth_test_signals[LAST_SIGNAL] = { 0 };


GQuark
gth_test_error_quark (void)
{
	return g_quark_from_static_string ("gth-test-error-quark");
}


static void
gth_test_finalize (GObject *object)
{
	GthTest *test;

	test = GTH_TEST (object);

	g_free (test->priv->id);
	g_free (test->priv->display_name);
	g_free (test->files);

	G_OBJECT_CLASS (parent_class)->finalize (object);
}


static GtkWidget *
base_create_control (GthTest *test)
{
	return NULL;
}


static gboolean
base_update_from_control (GthTest   *test,
			  GError   **error)
{
	return TRUE;
}


static void
base_reset (GthTest *test)
{
}


static GthMatch
base_match (GthTest     *test,
	    GthFileData *fdata)
{
	return GTH_MATCH_YES;
}


void
base_set_file_list (GthTest *test,
	 	    GList   *files)
{
	GList *scan;
	int    i;

	test->n_files = g_list_length (files);

	g_free (test->files);
	test->files = g_malloc (sizeof (GthFileData*) * (test->n_files + 1));

	for (scan = files, i = 0; scan; scan = scan->next)
		test->files[i++] = scan->data;
	test->files[i++] = NULL;

	test->iterator = 0;
}


GthFileData *
base_get_next (GthTest *test)
{
	GthFileData *file = NULL;
	GthMatch     match = GTH_MATCH_NO;

	if (test->files == NULL)
		return NULL;

	while (match == GTH_MATCH_NO) {
		file = test->files[test->iterator];
		if (file != NULL) {
			match = gth_test_match (test, file);
			test->iterator++;
		}
		else
			match = GTH_MATCH_LIMIT_REACHED;
	}

	if (match != GTH_MATCH_YES)
		file = NULL;

	if (file == NULL) {
		g_free (test->files);
		test->files = NULL;
	}

	return file;
}


GObject *
gth_test_real_duplicate (GthDuplicable *duplicable)
{
	GthTest *test = GTH_TEST (duplicable);
	GthTest *new_test;

	new_test = g_object_new (GTH_TYPE_TEST,
				 "id", gth_test_get_id (test),
				 "display-name", gth_test_get_display_name (test),
				 "visible", gth_test_is_visible (test),
				 NULL);

	return (GObject *) new_test;
}


static void
gth_test_set_property (GObject      *object,
		       guint         property_id,
		       const GValue *value,
		       GParamSpec   *pspec)
{
	GthTest *test;

        test = GTH_TEST (object);

	switch (property_id) {
	case PROP_ID:
		g_free (test->priv->id);
		test->priv->id = g_value_dup_string (value);
		if (test->priv->id == NULL)
			test->priv->id = g_strdup ("");
		break;
	case PROP_DISPLAY_NAME:
		g_free (test->priv->display_name);
		test->priv->display_name = g_value_dup_string (value);
		if (test->priv->display_name == NULL)
			test->priv->display_name = g_strdup ("");
		break;
	case PROP_VISIBLE:
		test->priv->visible = g_value_get_boolean (value);
		break;
	default:
		break;
	}
}


static void
gth_test_get_property (GObject    *object,
		       guint       property_id,
		       GValue     *value,
		       GParamSpec *pspec)
{
	GthTest *test;

        test = GTH_TEST (object);

	switch (property_id) {
	case PROP_ID:
		g_value_set_string (value, test->priv->id);
		break;
	case PROP_DISPLAY_NAME:
		g_value_set_string (value, test->priv->display_name);
		break;
	case PROP_VISIBLE:
		g_value_set_boolean (value, test->priv->visible);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
		break;
	}
}


static void
gth_test_class_init (GthTestClass *klass)
{
	GObjectClass *object_class;

	parent_class = g_type_class_peek_parent (klass);
	g_type_class_add_private (klass, sizeof (GthTestPrivate));

	object_class = (GObjectClass*) klass;
	object_class->set_property = gth_test_set_property;
	object_class->get_property = gth_test_get_property;
	object_class->finalize = gth_test_finalize;

	klass->create_control = base_create_control;
	klass->update_from_control = base_update_from_control;
	klass->reset = base_reset;
	klass->match = base_match;
	klass->set_file_list = base_set_file_list;
	klass->get_next = base_get_next;

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
					 PROP_VISIBLE,
					 g_param_spec_boolean ("visible",
							       "Visible",
							       "Whether this test should be visible in the filter bar",
							       FALSE,
							       G_PARAM_READWRITE));

	/* signals */

	gth_test_signals[CHANGED] =
                g_signal_new ("changed",
                              G_TYPE_FROM_CLASS (klass),
                              G_SIGNAL_RUN_LAST,
                              G_STRUCT_OFFSET (GthTestClass, changed),
                              NULL, NULL,
                              g_cclosure_marshal_VOID__VOID,
                              G_TYPE_NONE,
                              0);
}


static void
gth_test_gth_duplicable_interface_init (GthDuplicableIface *iface)
{
	gth_duplicable_parent_iface = g_type_interface_peek_parent (iface);
	iface->duplicate = gth_test_real_duplicate;
}


static void
gth_test_init (GthTest *test)
{
	test->priv = G_TYPE_INSTANCE_GET_PRIVATE (test, GTH_TYPE_TEST, GthTestPrivate);
	test->priv->id = g_strdup ("");
	test->priv->display_name = g_strdup ("");
	test->priv->visible = FALSE;
}


GType
gth_test_get_type (void)
{
        static GType type = 0;

        if (! type) {
                GTypeInfo type_info = {
			sizeof (GthTestClass),
			NULL,
			NULL,
			(GClassInitFunc) gth_test_class_init,
			NULL,
			NULL,
			sizeof (GthTest),
			0,
			(GInstanceInitFunc) gth_test_init
		};
		static const GInterfaceInfo gth_duplicable_info = {
			(GInterfaceInitFunc) gth_test_gth_duplicable_interface_init,
			(GInterfaceFinalizeFunc) NULL,
			NULL
		};

		type = g_type_register_static (G_TYPE_OBJECT,
					       "GthTest",
					       &type_info,
					       0);
		g_type_add_interface_static (type, GTH_TYPE_DUPLICABLE, &gth_duplicable_info);
	}

        return type;
}


GthTest *
gth_test_new (void)
{
	return (GthTest*) g_object_new (GTH_TYPE_TEST, NULL);
}


const char *
gth_test_get_id (GthTest *test)
{
	return test->priv->id;
}


const char *
gth_test_get_display_name (GthTest *test)
{
	return test->priv->display_name;
}


gboolean
gth_test_is_visible (GthTest *test)
{
	return test->priv->visible;
}


GtkWidget *
gth_test_create_control (GthTest *test)
{
	return GTH_TEST_GET_CLASS (test)->create_control (test);
}


gboolean
gth_test_update_from_control (GthTest   *test,
			      GError   **error)
{
	return GTH_TEST_GET_CLASS (test)->update_from_control (test, error);
}


void
gth_test_changed (GthTest *test)
{
	g_signal_emit (test, gth_test_signals[CHANGED], 0);
}


void
gth_test_reset (GthTest *test)
{
	return GTH_TEST_GET_CLASS (test)->reset (test);
}


GthMatch
gth_test_match (GthTest     *test,
		GthFileData *fdata)
{
	return GTH_TEST_GET_CLASS (test)->match (test, fdata);
}

void
gth_test_set_file_list (GthTest *test,
		        GList   *files)
{
	GTH_TEST_GET_CLASS (test)->set_file_list (test, files);
}


GthFileData *
gth_test_get_next (GthTest *test)
{
	return GTH_TEST_GET_CLASS (test)->get_next (test);
}
