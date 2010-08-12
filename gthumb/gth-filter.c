/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2006-2008 Free Software Foundation, Inc.
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
#include <stdlib.h>
#include <string.h>
#include <glib/gi18n.h>
#include <glib.h>
#include "dom.h"
#include "gth-file-data.h"
#include "glib-utils.h"
#include "gth-duplicable.h"
#include "gth-enum-types.h"
#include "gth-filter.h"
#include "gth-main.h"


typedef struct {
	char    *name;
	goffset  size;
} GthSizeData;


static GthSizeData size_data[] = {
	{ N_("kB"), 1024 },
	{ N_("MB"), 1024*1024 },
	{ N_("GB"), 1024*1024*1024 }
};


struct _GthFilterPrivate {
	GthTestChain *test;
	GthLimitType  limit_type;
	goffset       limit;
	const char   *sort_name;
	GtkSortType   sort_direction;
	int           current_images;
	goffset       current_size;
	GtkWidget    *limit_entry;
	GtkWidget    *size_combo_box;
};


static gpointer parent_class = NULL;


static DomElement*
gth_filter_real_create_element (DomDomizable *base,
				DomDocument  *doc)
{
	GthFilter  *self;
	DomElement *element;

	g_return_val_if_fail (DOM_IS_DOCUMENT (doc), NULL);

	self = GTH_FILTER (base);
	element = dom_document_create_element (doc, "filter",
					       "id", gth_test_get_id (GTH_TEST (self)),
					       "name", gth_test_get_display_name (GTH_TEST (self)),
					       NULL);

	if (! gth_test_is_visible (GTH_TEST (self)))
		dom_element_set_attribute (element, "display", "none");

	if ((self->priv->test != NULL) && (gth_test_chain_get_match_type (self->priv->test) != GTH_MATCH_TYPE_NONE))
		dom_element_append_child (element, dom_domizable_create_element (DOM_DOMIZABLE (self->priv->test), doc));

	if (self->priv->limit_type != GTH_LIMIT_TYPE_NONE) {
		DomElement *limit;
		char       *value;

		value = g_strdup_printf ("%" G_GOFFSET_FORMAT, self->priv->limit);
		limit = dom_document_create_element (doc, "limit",
						     "value", value,
						     "type", _g_enum_type_get_value (GTH_TYPE_LIMIT_TYPE, self->priv->limit_type)->value_nick,
						     "selected_by", self->priv->sort_name,
						     "direction", (self->priv->sort_direction == GTK_SORT_ASCENDING ? "ascending" : "descending"),
						     NULL);
		g_free (value);

		dom_element_append_child (element, limit);
	}

	return element;
}


static void
gth_filter_real_load_from_element (DomDomizable *base,
				   DomElement   *element)
{
	GthFilter  *self;
	DomElement *node;

	g_return_if_fail (DOM_IS_ELEMENT (element));

	self = GTH_FILTER (base);
	g_object_set (self,
		      "id", dom_element_get_attribute (element, "id"),
		      "display-name", dom_element_get_attribute (element, "name"),
		      "visible", (g_strcmp0 (dom_element_get_attribute (element, "display"), "none") != 0),
		      NULL);

	gth_filter_set_test (self, NULL);
	for (node = element->first_child; node; node = node->next_sibling) {
		if (g_strcmp0 (node->tag_name, "tests") == 0) {
			GthTest *test;

			test = gth_test_chain_new (GTH_MATCH_TYPE_NONE, NULL);
			dom_domizable_load_from_element (DOM_DOMIZABLE (test), node);
			gth_filter_set_test (self, GTH_TEST_CHAIN (test));
		}
		else if (g_strcmp0 (node->tag_name, "limit") == 0) {
			gth_filter_set_limit (self,
					      _g_enum_type_get_value_by_nick (GTH_TYPE_LIMIT_TYPE, dom_element_get_attribute (node, "type"))->value,
					      atol (dom_element_get_attribute (node, "value")),
					      dom_element_get_attribute (node, "selected_by"),
					      (g_strcmp0 (dom_element_get_attribute (node, "direction"), "ascending") == 0) ? GTK_SORT_ASCENDING : GTK_SORT_DESCENDING);
		}
	}
}


static const char *
gth_filter_get_attributes (GthTest *test)
{
	GthFilter *filter = GTH_FILTER (test);

	if (filter->priv->test != NULL)
		return gth_test_get_attributes (GTH_TEST (filter->priv->test));
	else
		return "";
}


static int
qsort_campare_func (gconstpointer a,
		    gconstpointer b,
		    gpointer      user_data)
{
	GthFileDataSort *sort_type = user_data;
	GthFileData     *file_a = *((GthFileData **) a);
	GthFileData     *file_b = *((GthFileData **) b);

	return sort_type->cmp_func (file_a, file_b);
}


static void
gth_filter_set_file_list (GthTest *test,
			  GList   *files)
{
	GthFilter *filter = GTH_FILTER (test);

	GTH_TEST_CLASS (parent_class)->set_file_list (test, files);

	if ((filter->priv->limit_type != GTH_LIMIT_TYPE_NONE)
	    && (filter->priv->sort_name != NULL)
	    && (strcmp (filter->priv->sort_name, "") != 0))
	{
		GthFileDataSort *sort_type;

		sort_type = gth_main_get_sort_type (filter->priv->sort_name);
		if ((sort_type != NULL) && (sort_type->cmp_func != NULL)) {
			g_qsort_with_data (test->files, test->n_files, (gsize) sizeof (GthFileData *), qsort_campare_func, sort_type);
			if (filter->priv->sort_direction == GTK_SORT_DESCENDING) {
				int i;

				for (i = 0; i < test->n_files / 2; i++) {
					GthFileData *tmp;

					tmp = test->files[i];
					test->files[i] = test->files[test->n_files - 1 - i];
					test->files[test->n_files - 1 - i] = tmp;
				}
			}
		}
	}

	filter->priv->current_images = 0;
	filter->priv->current_size = 0;
}


static GthMatch
gth_filter_match (GthTest     *test,
	          GthFileData *file)
{
	GthFilter *filter = GTH_FILTER (test);
	GthMatch   match = GTH_MATCH_NO;

	if (filter->priv->test != NULL)
		match = gth_test_match (GTH_TEST (filter->priv->test), file);
	else
		match = GTH_MATCH_YES;

	if (match == GTH_MATCH_YES) {
		filter->priv->current_images++;
		filter->priv->current_size += g_file_info_get_size (file->info);
	}

	switch (filter->priv->limit_type) {
	case GTH_LIMIT_TYPE_NONE:
		break;
	case GTH_LIMIT_TYPE_FILES:
		if (filter->priv->current_images > filter->priv->limit)
			match = GTH_MATCH_LIMIT_REACHED;
		break;
	case GTH_LIMIT_TYPE_SIZE:
		if (filter->priv->current_size > filter->priv->limit)
			match = GTH_MATCH_LIMIT_REACHED;
		break;
	}

	return match;
}


static void
file_limit_entry_activate_cb (GtkEntry  *entry,
                              GthFilter *filter)
{
	filter->priv->limit = atol (gtk_entry_get_text (GTK_ENTRY (filter->priv->limit_entry)));
	gth_test_changed (GTH_TEST (filter));
}


static GtkWidget *
create_control_for_files (GthFilter *filter)
{
	GtkWidget *control;
	GtkWidget *limit_label;
	GtkWidget *label;
	char      *value;

	control = gtk_hbox_new (FALSE, 6);

	/* limit label */

	limit_label = gtk_label_new_with_mnemonic (_("_Limit to"));
	gtk_widget_show (limit_label);

	/* limit entry */

	filter->priv->limit_entry = gtk_entry_new ();
	gtk_entry_set_width_chars (GTK_ENTRY (filter->priv->limit_entry), 6);
	value = g_strdup_printf ("%" G_GOFFSET_FORMAT, filter->priv->limit);
	gtk_entry_set_text (GTK_ENTRY (filter->priv->limit_entry), value);
	g_free (value);
	gtk_widget_show (filter->priv->limit_entry);

	g_signal_connect (G_OBJECT (filter->priv->limit_entry),
			  "activate",
			  G_CALLBACK (file_limit_entry_activate_cb),
			  filter);

	gtk_label_set_mnemonic_widget (GTK_LABEL (limit_label), filter->priv->limit_entry);

	/* "files" label */

	label = gtk_label_new (_("files"));
	gtk_widget_show (label);

	/**/

	gtk_box_pack_start (GTK_BOX (control), limit_label, FALSE, FALSE, 0);
	gtk_box_pack_start (GTK_BOX (control), filter->priv->limit_entry, FALSE, FALSE, 0);
	gtk_box_pack_start (GTK_BOX (control), label, FALSE, FALSE, 0);

	return control;
}


static void
size_limit_entry_activate_cb (GtkEntry  *entry,
                              GthFilter *filter)
{
	GthSizeData size;

	size = size_data[gtk_combo_box_get_active (GTK_COMBO_BOX (filter->priv->size_combo_box))];
	filter->priv->limit = size.size * atol (gtk_entry_get_text (GTK_ENTRY (filter->priv->limit_entry)));

	gth_test_changed (GTH_TEST (filter));
}


static void
size_combo_box_changed_cb (GtkComboBox *combo_box,
                           GthFilter   *filter)
{
	GthSizeData size;

	size = size_data[gtk_combo_box_get_active (GTK_COMBO_BOX (filter->priv->size_combo_box))];
	filter->priv->limit = size.size * atol (gtk_entry_get_text (GTK_ENTRY (filter->priv->limit_entry)));

	gth_test_changed (GTH_TEST (filter));
}


static GtkWidget *
create_control_for_size (GthFilter *filter)
{
	GtkWidget *control;
	GtkWidget *limit_label;
	int        i, size_idx;
	gboolean   size_set = FALSE;

	control = gtk_hbox_new (FALSE, 6);

	/* limit label */

	limit_label = gtk_label_new_with_mnemonic (_("_Limit to"));
	gtk_widget_show (limit_label);

	/* limit entry */

	filter->priv->limit_entry = gtk_entry_new ();
	gtk_entry_set_width_chars (GTK_ENTRY (filter->priv->limit_entry), 6);
	gtk_widget_show (filter->priv->limit_entry);

	g_signal_connect (G_OBJECT (filter->priv->limit_entry),
			  "activate",
			  G_CALLBACK (size_limit_entry_activate_cb),
			  filter);

	gtk_label_set_mnemonic_widget (GTK_LABEL (limit_label), filter->priv->limit_entry);

	/* size combo box */

	filter->priv->size_combo_box = gtk_combo_box_new_text ();
	gtk_widget_show (filter->priv->size_combo_box);

	size_idx = 0;
	for (i = 0; i < G_N_ELEMENTS (size_data); i++) {
		gtk_combo_box_append_text (GTK_COMBO_BOX (filter->priv->size_combo_box), _(size_data[i].name));
		if (! size_set && ((i == G_N_ELEMENTS (size_data) - 1) || (filter->priv->limit < size_data[i + 1].size))) {
			char *value;

			size_idx = i;
			value = g_strdup_printf ("%.2f", (double) filter->priv->limit / size_data[i].size);
			gtk_entry_set_text (GTK_ENTRY (filter->priv->limit_entry), value);
			g_free (value);
			size_set = TRUE;
		}
	}
	gtk_combo_box_set_active (GTK_COMBO_BOX (filter->priv->size_combo_box), size_idx);

	g_signal_connect (G_OBJECT (filter->priv->size_combo_box),
			  "changed",
			  G_CALLBACK (size_combo_box_changed_cb),
			  filter);

	/**/

	gtk_box_pack_start (GTK_BOX (control), limit_label, FALSE, FALSE, 0);
	gtk_box_pack_start (GTK_BOX (control), filter->priv->limit_entry, FALSE, FALSE, 0);
	gtk_box_pack_start (GTK_BOX (control), filter->priv->size_combo_box, FALSE, FALSE, 0);

	return control;
}


static GtkWidget *
gth_filter_real_create_control (GthTest *test)
{
	GthFilter *filter = (GthFilter *) test;
	GtkWidget *control = NULL;

	switch (filter->priv->limit_type) {
	case GTH_LIMIT_TYPE_NONE:
		break;
	case GTH_LIMIT_TYPE_FILES:
		control = create_control_for_files (filter);
		break;
	case GTH_LIMIT_TYPE_SIZE:
		control = create_control_for_size (filter);
		break;
	}

	return control;
}


GObject *
gth_filter_real_duplicate (GthDuplicable *duplicable)
{
	GthFilter *filter = GTH_FILTER (duplicable);
	GthFilter *new_filter;

	new_filter = gth_filter_new ();
	g_object_set (new_filter,
		      "id", gth_test_get_id (GTH_TEST (filter)),
		      "attributes", gth_test_get_attributes (GTH_TEST (filter)),
		      "display-name", gth_test_get_display_name (GTH_TEST (filter)),
		      "visible", gth_test_is_visible (GTH_TEST (filter)),
		      NULL);
	if (filter->priv->test != NULL)
		new_filter->priv->test = (GthTestChain*) gth_duplicable_duplicate (GTH_DUPLICABLE (filter->priv->test));
	new_filter->priv->limit = filter->priv->limit;
	new_filter->priv->limit_type = filter->priv->limit_type;
	new_filter->priv->sort_name = filter->priv->sort_name;
	new_filter->priv->sort_direction = filter->priv->sort_direction;

	return (GObject *) new_filter;
}


static void
gth_filter_finalize (GObject *object)
{
	GthFilter *filter;

	filter = GTH_FILTER (object);

	_g_object_unref (filter->priv->test);

	G_OBJECT_CLASS (parent_class)->finalize (object);
}


static void
gth_filter_class_init (GthFilterClass *klass)
{
	GObjectClass *object_class;
	GthTestClass *test_class;

	parent_class = g_type_class_peek_parent (klass);
	g_type_class_add_private (klass, sizeof (GthFilterPrivate));

	object_class = G_OBJECT_CLASS (klass);
	object_class->finalize = gth_filter_finalize;

	test_class = GTH_TEST_CLASS (klass);
	test_class->get_attributes = gth_filter_get_attributes;
	test_class->set_file_list = gth_filter_set_file_list;
	test_class->match = gth_filter_match;
	test_class->create_control = gth_filter_real_create_control;
}


static void
gth_filter_dom_domizable_interface_init (DomDomizableIface *iface)
{
	iface->create_element = gth_filter_real_create_element;
	iface->load_from_element = gth_filter_real_load_from_element;
}


static void
gth_filter_gth_duplicable_interface_init (GthDuplicableIface *iface)
{
	iface->duplicate = gth_filter_real_duplicate;
}


static void
gth_filter_init (GthFilter *filter)
{
	filter->priv = G_TYPE_INSTANCE_GET_PRIVATE (filter, GTH_TYPE_FILTER, GthFilterPrivate);
	filter->priv->test = NULL;
	filter->priv->limit_type = GTH_LIMIT_TYPE_NONE;
	filter->priv->limit = 0;
	filter->priv->sort_name = NULL;
	filter->priv->sort_direction = GTK_SORT_ASCENDING;
}


GType
gth_filter_get_type (void)
{
        static GType type = 0;

        if (! type) {
                GTypeInfo type_info = {
			sizeof (GthFilterClass),
			NULL,
			NULL,
			(GClassInitFunc) gth_filter_class_init,
			NULL,
			NULL,
			sizeof (GthFilter),
			0,
			(GInstanceInitFunc) gth_filter_init
		};
		static const GInterfaceInfo dom_domizable_info = {
			(GInterfaceInitFunc) gth_filter_dom_domizable_interface_init,
			(GInterfaceFinalizeFunc) NULL,
			NULL
		};
		static const GInterfaceInfo gth_duplicable_info = {
			(GInterfaceInitFunc) gth_filter_gth_duplicable_interface_init,
			(GInterfaceFinalizeFunc) NULL,
			NULL
		};

		type = g_type_register_static (GTH_TYPE_TEST,
					       "GthFilter",
					       &type_info,
					       0);
		g_type_add_interface_static (type, DOM_TYPE_DOMIZABLE, &dom_domizable_info);
		g_type_add_interface_static (type, GTH_TYPE_DUPLICABLE, &gth_duplicable_info);
	}

        return type;
}


GthFilter*
gth_filter_new (void)
{
	GthFilter *filter;
	char      *id;

	id = _g_rand_string (ID_LENGTH);
	filter = (GthFilter *) g_object_new (GTH_TYPE_FILTER, "id", id, NULL);
	g_free (id);

	return filter;
}


void
gth_filter_set_limit (GthFilter    *filter,
		      GthLimitType  type,
		      goffset       value,
		      const char   *sort_name,
		      GtkSortType   sort_direction)
{
	filter->priv->limit_type = type;
	filter->priv->limit = value;
	filter->priv->sort_name = get_static_string (sort_name);
	filter->priv->sort_direction = sort_direction;
}


void
gth_filter_get_limit (GthFilter     *filter,
		      GthLimitType  *type,
		      goffset       *value,
		      const char   **sort_name,
		      GtkSortType   *sort_direction)
{
	if (type != NULL)
		*type = filter->priv->limit_type;
	if (value != NULL)
		*value = filter->priv->limit;
	if (sort_name != NULL)
		*sort_name = filter->priv->sort_name;
	if (sort_direction != NULL)
		*sort_direction = filter->priv->sort_direction;
}


void
gth_filter_set_test (GthFilter    *filter,
		     GthTestChain *test)
{
	if (filter->priv->test != NULL) {
		g_object_unref (filter->priv->test);
		filter->priv->test = NULL;
	}
	if (test != NULL) {
		filter->priv->test = g_object_ref (test);
		g_object_set (filter, "attributes", gth_test_get_attributes (GTH_TEST (test)), NULL);
	}
	else
		g_object_set (filter, "attributes", "", NULL);
}


GthTestChain *
gth_filter_get_test (GthFilter *filter)
{
	if (filter->priv->test != NULL)
		return g_object_ref (filter->priv->test);
	else
		return NULL;
}
