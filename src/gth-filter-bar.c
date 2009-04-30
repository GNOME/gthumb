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
#include <stdlib.h>
#include <time.h>

#include <glib/gi18n.h>
#include <gtk/gtk.h>
#include <libgnomeui/gnome-dateedit.h>
#include "gth-filter-bar.h"
#include "gth-category-selection-dialog.h"

enum {
	FILTER_TYPE_NONE,
	FILTER_TYPE_SEPARATOR,
	FILTER_TYPE_ALL,
	FILTER_TYPE_SCOPE,
	FILTER_TYPE_CUSTOM,
	FILTER_TYPE_PERSONALIZE
};

enum {
	ICON_COLUMN,
	NAME_COLUMN,
	TYPE_COLUMN,
	SUBTYPE_COLUMN,
	N_COLUMNS
};

enum {
        CHANGED,
        CLOSE_BUTTON_CLICKED,
        LAST_SIGNAL
};

struct _GthFilterBarPrivate
{
	gboolean      match_all;
	gboolean      has_focus;
	GtkWidget    *text_entry;
	GtkWidget    *text_op_combo_box;
	GtkWidget    *category_op_combo_box;
	GtkWidget    *date_op_combo_box;
	GtkWidget    *int_op_combo_box;
	GtkWidget    *size_combo_box;
	GtkWidget    *scope_combo_box;
	GtkWidget    *choose_categories_button;
	GtkWidget    *date_edit;
	GtkListStore *model;
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
	GObjectClass   *object_class;

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
         gth_filter_bar_signals[CLOSE_BUTTON_CLICKED] =
                g_signal_new ("close_button_clicked",
                              G_TYPE_FROM_CLASS (class),
                              G_SIGNAL_RUN_LAST,
                              G_STRUCT_OFFSET (GthFilterBarClass, close_button_clicked),
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
gth_filter_bar_changed (GthFilterBar *filter_bar)
{
	g_signal_emit (filter_bar, gth_filter_bar_signals[CHANGED], 0);
}


static void
text_entry_activate_cb (GtkEntry     *entry,
                        GthFilterBar *filter_bar)
{
	gth_filter_bar_changed (filter_bar);
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


typedef struct {
	char      *name;
	GthTestOp  op;
	gboolean   negative;
} GthOpData;


GthOpData text_op_data[] = { { N_("contains"), GTH_TEST_OP_CONTAINS, FALSE },
	 	     { N_("starts with"), GTH_TEST_OP_STARTS_WITH, FALSE },
		     { N_("ends with"), GTH_TEST_OP_ENDS_WITH, FALSE },
		     { N_("is"), GTH_TEST_OP_EQUAL, FALSE },
		     { N_("is not"), GTH_TEST_OP_EQUAL, TRUE },
		     { N_("does not contain"), GTH_TEST_OP_CONTAINS, TRUE } };


GthOpData int_op_data[] = { { N_("is equal to"), GTH_TEST_OP_EQUAL, FALSE },
		    { N_("is lower than"), GTH_TEST_OP_LOWER, FALSE },
		    { N_("is greater than"), GTH_TEST_OP_GREATER, FALSE } };


GthOpData date_op_data[] = { { N_("is equal to"), GTH_TEST_OP_EQUAL, FALSE },
		     { N_("is before"), GTH_TEST_OP_BEFORE, FALSE },
		     { N_("is after"), GTH_TEST_OP_AFTER, FALSE } };


GthOpData category_op_data[] = { { N_("is"), GTH_TEST_OP_CONTAINS, FALSE },
		         { N_("is not"), GTH_TEST_OP_CONTAINS, TRUE } };

static struct {
	char  *name;
	gsize  size;
} size_data[] = { { N_("KB"), 1024 },
		  { N_("MB"), 1024*1024 } };


static struct {
	char         *name;
	GthTestScope  scope;
} scope_data[] = { { N_("Filename"), GTH_TEST_SCOPE_FILENAME },
		   { N_("Comment"), GTH_TEST_SCOPE_COMMENT },
		   { N_("Place"), GTH_TEST_SCOPE_PLACE },
		   { N_("Date"), GTH_TEST_SCOPE_DATE },
		   { N_("Size"), GTH_TEST_SCOPE_SIZE },
		   { N_("Category"), GTH_TEST_SCOPE_KEYWORDS },
		   { N_("Text contains"), GTH_TEST_SCOPE_ALL } };


static void
scope_combo_box_changed_cb (GtkComboBox  *scope_combo_box,
			    GthFilterBar *filter_bar)
{
	GtkTreeIter iter;
	int         filter_type = FILTER_TYPE_NONE;
	int         scope_type = -1;

	if (! gtk_combo_box_get_active_iter (scope_combo_box, &iter))
		return;

	gtk_tree_model_get (GTK_TREE_MODEL (filter_bar->priv->model),
			    &iter,
			    TYPE_COLUMN, &filter_type,
			    SUBTYPE_COLUMN, &scope_type,
			    -1);

	if ((filter_type == FILTER_TYPE_SCOPE) && (scope_type == GTH_TEST_SCOPE_KEYWORDS))
		gtk_widget_show (filter_bar->priv->choose_categories_button);
	else
		gtk_widget_hide (filter_bar->priv->choose_categories_button);

	if ((filter_type == FILTER_TYPE_ALL)
	    || ((filter_type == FILTER_TYPE_SCOPE)
	        && ((scope_type == GTH_TEST_SCOPE_ALL)
	            || (scope_type == GTH_TEST_SCOPE_KEYWORDS)
	            || (scope_type == GTH_TEST_SCOPE_SIZE)
	            || (scope_type == GTH_TEST_SCOPE_DATE))))
		gtk_widget_hide (filter_bar->priv->text_op_combo_box);
	else
		gtk_widget_show (filter_bar->priv->text_op_combo_box);

	if ((filter_type == FILTER_TYPE_SCOPE) && (scope_type == GTH_TEST_SCOPE_DATE))
		gtk_widget_show (filter_bar->priv->date_op_combo_box);
	else
		gtk_widget_hide (filter_bar->priv->date_op_combo_box);

	if (scope_type == GTH_TEST_SCOPE_DATE)
		gtk_widget_show (filter_bar->priv->date_edit);
	else
		gtk_widget_hide (filter_bar->priv->date_edit);

	if ((filter_type == FILTER_TYPE_SCOPE) && (scope_type == GTH_TEST_SCOPE_SIZE)) {
		gtk_widget_show (filter_bar->priv->int_op_combo_box);
		gtk_widget_show (filter_bar->priv->size_combo_box);
	} else {
		gtk_widget_hide (filter_bar->priv->int_op_combo_box);
		gtk_widget_hide (filter_bar->priv->size_combo_box);
	}

	if ((filter_type == FILTER_TYPE_SCOPE) && (scope_type == GTH_TEST_SCOPE_KEYWORDS))
		gtk_widget_show (filter_bar->priv->category_op_combo_box);
	else
		gtk_widget_hide (filter_bar->priv->category_op_combo_box);

	if ((filter_type == FILTER_TYPE_ALL)
	    || ((filter_type == FILTER_TYPE_SCOPE)
	        && (scope_type == GTH_TEST_SCOPE_DATE)))
		gtk_widget_hide (filter_bar->priv->text_entry);
	else
		gtk_widget_show (filter_bar->priv->text_entry);

	if (filter_type == FILTER_TYPE_ALL)
		gth_filter_bar_changed (filter_bar);
}


static void
close_button_clicked_cb (GtkWidget    *button,
			 GthFilterBar *filter_bar)
{
	g_signal_emit (filter_bar, gth_filter_bar_signals[CLOSE_BUTTON_CLICKED], 0);
}


static gboolean
scope_combo_boxrow_separator_func (GtkTreeModel *model,
		    		   GtkTreeIter  *iter,
		    		   gpointer      data)
{
	int item_type = FILTER_TYPE_NONE;
	gtk_tree_model_get (model,
			    iter,
			    TYPE_COLUMN, &item_type,
			    -1);
	return (item_type == FILTER_TYPE_SEPARATOR);
}


static void
category_selection_response_cb (GthCategorySelection *csel,
				GtkResponseType       response,
				GthFilterBar         *filter_bar)
{
	if (response == GTK_RESPONSE_OK) {
		gtk_entry_set_text (GTK_ENTRY (filter_bar->priv->text_entry),
				    gth_category_selection_get_categories (csel));
		filter_bar->priv->match_all = gth_category_selection_get_match_all (csel);
		gth_filter_bar_changed (filter_bar);
	}

	if (response == GTK_RESPONSE_CANCEL)
		g_object_unref (csel);
}


static void
choose_categories_button_clicked_cb (GtkButton    *button,
				     GthFilterBar *filter_bar)
{
	GthCategorySelection *csel;
	GtkWidget *toplevel;

	toplevel = gtk_widget_get_toplevel (GTK_WIDGET (filter_bar));
 	if (! GTK_WIDGET_TOPLEVEL (toplevel))
 		toplevel = NULL;

	csel = gth_category_selection_new (GTK_WINDOW (toplevel), gtk_entry_get_text (GTK_ENTRY (filter_bar->priv->text_entry)), filter_bar->priv->match_all);
	g_signal_connect (G_OBJECT (csel),
			  "response",
			  G_CALLBACK (category_selection_response_cb),
			  filter_bar);
}


static void
date_edit_changed_cb (GnomeDateEdit *dateedit,
                      GthFilterBar  *filter_bar)
{
	gth_filter_bar_changed (filter_bar);
}


static void
date_op_combo_box_changed_cb (GtkComboBox  *widget,
                              GthFilterBar *filter_bar)
{
	gth_filter_bar_changed (filter_bar);
}


static void
gth_filter_bar_construct (GthFilterBar *filter_bar)
{
	GtkCellRenderer *renderer;
	GtkTreeIter      iter;
	GtkWidget       *label;
	GtkWidget       *button;
	GtkWidget       *image;
	int              i;

	GTK_BOX (filter_bar)->spacing = 6;
	gtk_container_set_border_width (GTK_CONTAINER (filter_bar), 2);

	/* choose category button */

	filter_bar->priv->choose_categories_button = gtk_button_new_with_label ("...");
	g_signal_connect (G_OBJECT (filter_bar->priv->choose_categories_button),
			  "clicked",
			  G_CALLBACK (choose_categories_button_clicked_cb),
			  filter_bar);

	/* date edit */

	filter_bar->priv->date_edit = gnome_date_edit_new_flags (time(NULL), 0);
	g_signal_connect (G_OBJECT (filter_bar->priv->date_edit),
			  "date-changed",
			  G_CALLBACK (date_edit_changed_cb),
			  filter_bar);
	g_signal_connect (G_OBJECT (filter_bar->priv->date_edit),
			  "time-changed",
			  G_CALLBACK (date_edit_changed_cb),
			  filter_bar);

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

	/* text operation combo box */

	filter_bar->priv->text_op_combo_box = gtk_combo_box_new_text ();
	for (i = 0; i < G_N_ELEMENTS (text_op_data); i++)
		gtk_combo_box_append_text (GTK_COMBO_BOX (filter_bar->priv->text_op_combo_box),
					   _(text_op_data[i].name));
	gtk_combo_box_set_active (GTK_COMBO_BOX (filter_bar->priv->text_op_combo_box), 0);

	/* int operation combo box */

	filter_bar->priv->int_op_combo_box = gtk_combo_box_new_text ();
	for (i = 0; i < G_N_ELEMENTS (int_op_data); i++)
		gtk_combo_box_append_text (GTK_COMBO_BOX (filter_bar->priv->int_op_combo_box),
					   _(int_op_data[i].name));
	gtk_combo_box_set_active (GTK_COMBO_BOX (filter_bar->priv->int_op_combo_box), 0);

	/* date operation combo box */

	filter_bar->priv->date_op_combo_box = gtk_combo_box_new_text ();
	for (i = 0; i < G_N_ELEMENTS (date_op_data); i++)
		gtk_combo_box_append_text (GTK_COMBO_BOX (filter_bar->priv->date_op_combo_box),
					   _(date_op_data[i].name));
	gtk_combo_box_set_active (GTK_COMBO_BOX (filter_bar->priv->date_op_combo_box), 0);
	g_signal_connect (G_OBJECT (filter_bar->priv->date_op_combo_box),
			  "changed",
			  G_CALLBACK (date_op_combo_box_changed_cb),
			  filter_bar);

	/* category operation combo box */

	filter_bar->priv->category_op_combo_box = gtk_combo_box_new_text ();
	for (i = 0; i < G_N_ELEMENTS (category_op_data); i++)
		gtk_combo_box_append_text (GTK_COMBO_BOX (filter_bar->priv->category_op_combo_box),
					   _(category_op_data[i].name));
	gtk_combo_box_set_active (GTK_COMBO_BOX (filter_bar->priv->category_op_combo_box), 0);

	/* scope combo box */

	filter_bar->priv->model = gtk_list_store_new (N_COLUMNS,
					       	      GDK_TYPE_PIXBUF,
					       	      G_TYPE_STRING,
					              G_TYPE_INT,
					              G_TYPE_INT);
	filter_bar->priv->scope_combo_box = gtk_combo_box_new_with_model (GTK_TREE_MODEL (filter_bar->priv->model));
	g_object_unref (filter_bar->priv->model);
	gtk_combo_box_set_row_separator_func (GTK_COMBO_BOX (filter_bar->priv->scope_combo_box),
					      scope_combo_boxrow_separator_func,
					      filter_bar,
					      NULL);

	/* size combo box */

	filter_bar->priv->size_combo_box = gtk_combo_box_new_text ();
	for (i = 0; i < G_N_ELEMENTS (size_data); i++)
		gtk_combo_box_append_text (GTK_COMBO_BOX (filter_bar->priv->size_combo_box),
					   _(size_data[i].name));
	gtk_combo_box_set_active (GTK_COMBO_BOX (filter_bar->priv->size_combo_box), 0);

	/* icon renderer */

	renderer = gtk_cell_renderer_pixbuf_new ();
	gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (filter_bar->priv->scope_combo_box),
				    renderer,
				    FALSE);
	gtk_cell_layout_set_attributes  (GTK_CELL_LAYOUT (filter_bar->priv->scope_combo_box),
					 renderer,
					 "pixbuf", ICON_COLUMN,
					 NULL);

	/* name renderer */

	renderer = gtk_cell_renderer_text_new ();
	gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (filter_bar->priv->scope_combo_box),
				    renderer,
				    TRUE);
	gtk_cell_layout_set_attributes  (GTK_CELL_LAYOUT (filter_bar->priv->scope_combo_box),
					 renderer,
					 "text", NAME_COLUMN,
					 NULL);

	/**/

	gtk_list_store_append (filter_bar->priv->model, &iter);
	gtk_list_store_set (filter_bar->priv->model, &iter,
			    TYPE_COLUMN, FILTER_TYPE_ALL,
			    NAME_COLUMN, _("All"),
			    -1);
	gtk_list_store_append (filter_bar->priv->model, &iter);
	gtk_list_store_set (filter_bar->priv->model, &iter,
			    TYPE_COLUMN, FILTER_TYPE_SEPARATOR,
			    -1);
	for (i = 0; i < G_N_ELEMENTS (scope_data); i++) {
		gtk_list_store_append (filter_bar->priv->model, &iter);
		gtk_list_store_set (filter_bar->priv->model, &iter,
				    TYPE_COLUMN, FILTER_TYPE_SCOPE,
				    SUBTYPE_COLUMN, scope_data[i].scope,
				    NAME_COLUMN, _(scope_data[i].name),
			    	    -1);
	}

	/* FIXME: todo for 2.12
	gtk_list_store_append (filter_bar->priv->model, &iter);
	gtk_list_store_set (filter_bar->priv->model, &iter,
			    TYPE_COLUMN, FILTER_TYPE_SEPARATOR,
			    -1);
	gtk_list_store_append (filter_bar->priv->model, &iter);
	gtk_list_store_set (filter_bar->priv->model, &iter,
			    TYPE_COLUMN, FILTER_TYPE_PERSONALIZE,
			    NAME_COLUMN, _("Personalize..."),
			    -1);
	*/

	g_signal_connect (G_OBJECT (filter_bar->priv->scope_combo_box),
			  "changed",
			  G_CALLBACK (scope_combo_box_changed_cb),
			  filter_bar);
	gtk_combo_box_set_active (GTK_COMBO_BOX (filter_bar->priv->scope_combo_box), 0);
	gtk_widget_show (filter_bar->priv->scope_combo_box);

	/* close button */

	button = gtk_button_new ();
	image = gtk_image_new_from_stock (GTK_STOCK_CLOSE, GTK_ICON_SIZE_MENU);
	gtk_container_add (GTK_CONTAINER (button), image);
	gtk_widget_show_all (button);
	gtk_button_set_relief (GTK_BUTTON (button), GTK_RELIEF_NONE);
        gtk_widget_set_tooltip_text (button,
                                     _("Close"));
	g_signal_connect (G_OBJECT (button),
			  "clicked",
			  G_CALLBACK (close_button_clicked_cb),
			  filter_bar);

	/* view label */

	label = gtk_label_new_with_mnemonic (_("S_how:"));
	gtk_label_set_mnemonic_widget (GTK_LABEL (label), filter_bar->priv->scope_combo_box);
	gtk_widget_show (label);

	/**/

	gtk_box_pack_start (GTK_BOX (filter_bar), label, FALSE, FALSE, 0);
	gtk_box_pack_start (GTK_BOX (filter_bar), filter_bar->priv->scope_combo_box, FALSE, FALSE, 0);
	gtk_box_pack_start (GTK_BOX (filter_bar), filter_bar->priv->text_op_combo_box, FALSE, FALSE, 0);
	gtk_box_pack_start (GTK_BOX (filter_bar), filter_bar->priv->int_op_combo_box, FALSE, FALSE, 0);
	gtk_box_pack_start (GTK_BOX (filter_bar), filter_bar->priv->date_op_combo_box, FALSE, FALSE, 0);
	gtk_box_pack_start (GTK_BOX (filter_bar), filter_bar->priv->category_op_combo_box, FALSE, FALSE, 0);
	gtk_box_pack_start (GTK_BOX (filter_bar), filter_bar->priv->text_entry, FALSE, FALSE, 0);
	gtk_box_pack_start (GTK_BOX (filter_bar), filter_bar->priv->choose_categories_button, FALSE, FALSE, 0);
	gtk_box_pack_start (GTK_BOX (filter_bar), filter_bar->priv->size_combo_box, FALSE, FALSE, 0);
	gtk_box_pack_start (GTK_BOX (filter_bar), filter_bar->priv->date_edit, FALSE, FALSE, 0);
	gtk_box_pack_end (GTK_BOX (filter_bar), button, FALSE, FALSE, 0);
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
	GtkTreeIter iter;
	int         filter_type = FILTER_TYPE_NONE;
	int         scope_type = -1;
	const char *text;
	GthFilter  *filter;
	GthOpData   op_data;
	GthTest    *test = NULL;
	gsize       size;

	if (! gtk_combo_box_get_active_iter (GTK_COMBO_BOX (filter_bar->priv->scope_combo_box), &iter))
		return NULL;

	gtk_tree_model_get (GTK_TREE_MODEL (filter_bar->priv->model),
			    &iter,
			    TYPE_COLUMN, &filter_type,
			    SUBTYPE_COLUMN, &scope_type,
			    -1);

	if (filter_type == FILTER_TYPE_ALL)
		return NULL;

	filter = gth_filter_new ();

	switch (filter_type) {
	case FILTER_TYPE_SCOPE:
		if (scope_type == GTH_TEST_SCOPE_DATE) {
			GDate *date;

			op_data = date_op_data[gtk_combo_box_get_active (GTK_COMBO_BOX (filter_bar->priv->date_op_combo_box))];
			date = g_date_new ();
			g_date_set_time_t (date, gnome_date_edit_get_time (GNOME_DATE_EDIT (filter_bar->priv->date_edit)));
			test = gth_test_new_with_date (scope_type, op_data.op, op_data.negative, date);
			g_date_free (date);

			gth_filter_add_test (filter, test);
			break;
		}

		/* text based filters */

		text = gtk_entry_get_text (GTK_ENTRY (filter_bar->priv->text_entry));
		if ((text == NULL) || (strlen (text) == 0)) {
			g_object_unref (filter);
			return NULL;
		}

		switch (scope_type) {
		case GTH_TEST_SCOPE_FILENAME:
		case GTH_TEST_SCOPE_COMMENT:
		case GTH_TEST_SCOPE_PLACE:
		case GTH_TEST_SCOPE_ALL:
			op_data = text_op_data[gtk_combo_box_get_active (GTK_COMBO_BOX (filter_bar->priv->text_op_combo_box))];
			test = gth_test_new_with_string (scope_type, op_data.op, op_data.negative, text);
			break;
		case GTH_TEST_SCOPE_SIZE:
			op_data = int_op_data[gtk_combo_box_get_active (GTK_COMBO_BOX (filter_bar->priv->int_op_combo_box))];
			size = atoi(text) * size_data[gtk_combo_box_get_active (GTK_COMBO_BOX (filter_bar->priv->size_combo_box))].size;
			test = gth_test_new_with_int (scope_type, op_data.op, op_data.negative, size);
			break;
		case GTH_TEST_SCOPE_KEYWORDS:
			op_data = category_op_data[gtk_combo_box_get_active (GTK_COMBO_BOX (filter_bar->priv->category_op_combo_box))];
			if (filter_bar->priv->match_all)
				op_data.op = GTH_TEST_OP_CONTAINS_ALL;
			else
				op_data.op = GTH_TEST_OP_CONTAINS;
			test = gth_test_new_with_string (scope_type, op_data.op, op_data.negative, text);
			break;
		default:
			break;
		}

		if (test != NULL)
			gth_filter_add_test (filter, test);
		break;

	case FILTER_TYPE_CUSTOM:
		break;

	default:
		break;
	}

	return filter;
}


gboolean
gth_filter_bar_has_focus (GthFilterBar *filter_bar)
{
	return filter_bar->priv->has_focus;
}
