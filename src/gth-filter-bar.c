/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2006 Free Software Foundation, Inc.
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
#include <gtk/gtk.h>

#include "gth-filter-bar.h"

enum {
        CHANGED,
        LAST_SIGNAL
};

struct _GthFilterBarPrivate
{
	gboolean   has_focus;
	GtkWidget *text_entry;
	GtkWidget *op_combo_box;
	GtkWidget *scope_combo_box;
};


static GtkHBoxClass *parent_class = NULL;
static guint gth_filter_bar_signals[LAST_SIGNAL] = { 0 };


static void
gth_filter_bar_finalize (GObject *object)
{
	GthFilterBar *filter_bar;

	filter_bar = GTH_FILTER_BAR (object);

	if (filter_bar->priv != NULL) {
		g_free (filter_bar->priv);
		filter_bar->priv = NULL;
	}

	G_OBJECT_CLASS (parent_class)->finalize (object);
}


static void
gth_filter_bar_class_init (GthFilterBarClass *class)
{
	GObjectClass *object_class;

	parent_class = g_type_class_peek_parent (class);
	object_class = (GObjectClass*) class;

	object_class->finalize = gth_filter_bar_finalize;

	gth_filter_bar_signals[CHANGED] =
                g_signal_new ("changed",
                              G_TYPE_FROM_CLASS (class),
                              G_SIGNAL_RUN_LAST,
                              G_STRUCT_OFFSET (GthFilterBarClass, changed),
                              NULL, NULL,
                              g_cclosure_marshal_VOID__VOID,
                              G_TYPE_NONE,
                              0);
}


static void
gth_filter_bar_init (GthFilterBar *filter_bar)
{
	filter_bar->priv = g_new0 (GthFilterBarPrivate, 1);
}


static void
text_entry_activate_cb (GtkEntry     *entry,
                        GthFilterBar *filter_bar)
{
	g_signal_emit (filter_bar, gth_filter_bar_signals[CHANGED], 0);
}


static gboolean
text_entry_focus_in_event_cb (GtkEntry      *entry,
			      GdkEventFocus *event,
                              GthFilterBar  *filter_bar)
{
	filter_bar->priv->has_focus = TRUE;
	return FALSE;
}


static gboolean
text_entry_focus_out_event_cb (GtkEntry      *entry,
			       GdkEventFocus *event,
                               GthFilterBar  *filter_bar)
{
	filter_bar->priv->has_focus = FALSE;
	return FALSE;
}


static struct {
	char      *name;
	GthTestOp  op;
	gboolean   negative;
} op_data[] = { { N_("contains"), GTH_TEST_OP_CONTAINS, FALSE },
		{ N_("starts with"), GTH_TEST_OP_STARTS_WITH, FALSE },
		{ N_("ends with"), GTH_TEST_OP_ENDS_WITH, FALSE },
		{ N_("is"), GTH_TEST_OP_EQUAL, FALSE },
		{ N_("is not"), GTH_TEST_OP_EQUAL, TRUE },
		{ N_("does not contain"), GTH_TEST_OP_CONTAINS, TRUE } };


enum {
	SCOPE_FILENAME,
	SCOPE_COMMENT,
	SCOPE_PLACE,
	SCOPE_DATE,
	SCOPE_SIZE,
	SCOPE_CATEGORY,
	SCOPE_SEARCH
};


static struct {
	char         *name;
	GthTestScope  scope;
} scope_data[] = { { N_("Filename"), GTH_TEST_SCOPE_FILENAME },
		   { N_("Comment"), GTH_TEST_SCOPE_COMMENT },
		   { N_("Place"), GTH_TEST_SCOPE_PLACE },
		   { N_("Date"), GTH_TEST_SCOPE_DATE },
		   { N_("Size"), GTH_TEST_SCOPE_SIZE },
		   { N_("Category"), GTH_TEST_SCOPE_KEYWORDS },
		   { N_("Search"), GTH_TEST_SCOPE_ALL } };


static void
scope_combo_box_changed_cb (GtkComboBox  *scope_combo_box,
			    GthFilterBar *filter_bar)
{
	int scope;

	scope = gtk_combo_box_get_active (scope_combo_box);
	if (scope == SCOPE_SEARCH)
		gtk_widget_hide (filter_bar->priv->op_combo_box);
	else
		gtk_widget_show (filter_bar->priv->op_combo_box);
}


static void
gth_filter_bar_construct (GthFilterBar *filter_bar)
{
	int i;

	GTK_BOX (filter_bar)->spacing = 6;
	gtk_container_set_border_width (GTK_CONTAINER (filter_bar), 6);

	/* text entry */

	filter_bar->priv->text_entry = gtk_entry_new ();
	g_signal_connect (G_OBJECT (filter_bar->priv->text_entry),
			  "activate",
			  G_CALLBACK (text_entry_activate_cb),
			  filter_bar);
	g_signal_connect (G_OBJECT (filter_bar->priv->text_entry),
			  "focus-in-event",
			  G_CALLBACK (text_entry_focus_in_event_cb),
			  filter_bar);
	g_signal_connect (G_OBJECT (filter_bar->priv->text_entry),
			  "focus-out-event",
			  G_CALLBACK (text_entry_focus_out_event_cb),
			  filter_bar);
	gtk_widget_set_size_request (filter_bar->priv->text_entry, 150, -1);
	gtk_widget_show (filter_bar->priv->text_entry);

	/* operation combo box */

	filter_bar->priv->op_combo_box = gtk_combo_box_new_text ();
	for (i = 0; i < G_N_ELEMENTS (op_data); i++)
		gtk_combo_box_append_text (GTK_COMBO_BOX (filter_bar->priv->op_combo_box),
					   _(op_data[i].name));
	gtk_combo_box_set_active (GTK_COMBO_BOX (filter_bar->priv->op_combo_box), 0);
	gtk_widget_show (filter_bar->priv->op_combo_box);

	/* scope combo box */

	filter_bar->priv->scope_combo_box = gtk_combo_box_new_text ();
	for (i = 0; i < G_N_ELEMENTS (scope_data); i++)
		gtk_combo_box_append_text (GTK_COMBO_BOX (filter_bar->priv->scope_combo_box),
					   _(scope_data[i].name));
	gtk_combo_box_set_active (GTK_COMBO_BOX (filter_bar->priv->scope_combo_box), 0);
	g_signal_connect (G_OBJECT (filter_bar->priv->scope_combo_box),
			  "changed",
			  G_CALLBACK (scope_combo_box_changed_cb),
			  filter_bar);
	gtk_widget_show (filter_bar->priv->scope_combo_box);

	/**/

	gtk_box_pack_end (GTK_BOX (filter_bar), filter_bar->priv->text_entry, FALSE, FALSE, 0);
	gtk_box_pack_end (GTK_BOX (filter_bar), filter_bar->priv->op_combo_box, FALSE, FALSE, 0);
	gtk_box_pack_end (GTK_BOX (filter_bar), filter_bar->priv->scope_combo_box, FALSE, FALSE, 0);
}


GType
gth_filter_bar_get_type (void)
{
        static GType type = 0;

        if (! type) {
                GTypeInfo type_info = {
			sizeof (GthFilterBarClass),
			NULL,
			NULL,
			(GClassInitFunc) gth_filter_bar_class_init,
			NULL,
			NULL,
			sizeof (GthFilterBar),
			0,
			(GInstanceInitFunc) gth_filter_bar_init
		};

		type = g_type_register_static (GTK_TYPE_HBOX,
					       "GthFilterBar",
					       &type_info,
					       0);
	}

        return type;
}


GtkWidget*
gth_filter_bar_new (void)
{
	GtkWidget *widget;

	widget = GTK_WIDGET (g_object_new (GTH_TYPE_FILTER_BAR, NULL));
	gth_filter_bar_construct (GTH_FILTER_BAR (widget));

	return widget;
}


GthFilter *
gth_filter_bar_get_filter (GthFilterBar *filter_bar)
{
	const char *text;
	GthFilter  *filter;
	int         scope;
	int         op;


	text = gtk_entry_get_text (GTK_ENTRY (filter_bar->priv->text_entry));
	if ((text == NULL) || (strlen (text) == 0))
		return NULL;

	filter = gth_filter_new ();
	scope = gtk_combo_box_get_active (GTK_COMBO_BOX (filter_bar->priv->scope_combo_box));
	op = gtk_combo_box_get_active (GTK_COMBO_BOX (filter_bar->priv->op_combo_box));
	gth_filter_add_test (filter, gth_test_new_with_string (scope_data[scope].scope, op_data[op].op, op_data[op].negative, text));

	return filter;
}


gboolean
gth_filter_bar_has_focus (GthFilterBar *filter_bar)
{
	return filter_bar->priv->has_focus;
}
