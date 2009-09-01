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
#include <glib/gi18n.h>
#include "gth-transition.h"
#include "preferences.h"


#define GET_WIDGET(name) _gtk_builder_get_widget (data->builder, (name))
#define BROWSER_DATA_KEY "slideshow-preference-data"


enum {
	TRANSITION_COLUMN_ID,
	TRANSITION_COLUMN_DISPLAY_NAME
};


typedef struct {
	GtkBuilder *builder;
	GtkWidget  *transition_combobox;
} BrowserData;


static void
browser_data_free (BrowserData *data)
{
	g_object_unref (data->builder);
	g_free (data);
}


static void
transition_combobox_changed_cb (GtkComboBox *combo_box,
				BrowserData *data)
{
	GtkTreeIter   iter;
	GtkTreeModel *tree_model;
	char         *transition_id;

	if (! gtk_combo_box_get_active_iter (GTK_COMBO_BOX (data->transition_combobox), &iter))
		return;

	tree_model = gtk_combo_box_get_model (GTK_COMBO_BOX (data->transition_combobox));
	gtk_tree_model_get (tree_model, &iter, TRANSITION_COLUMN_ID, &transition_id, -1);
	eel_gconf_set_string (PREF_SLIDESHOW_TRANSITION, transition_id);

	g_free (transition_id);
}


static void
automatic_checkbutton_toggled_cb (GtkToggleButton *button,
				  BrowserData     *data)
{
	eel_gconf_set_boolean (PREF_SLIDESHOW_AUTOMATIC, gtk_toggle_button_get_active (button));
}


static void
wrap_around_checkbutton_toggled_cb (GtkToggleButton *button,
				    BrowserData     *data)
{
	eel_gconf_set_boolean (PREF_SLIDESHOW_WRAP_AROUND, gtk_toggle_button_get_active (button));
}


static void
change_delay_spinbutton_value_changed_cb (GtkSpinButton *spinbutton,
				          BrowserData   *data)
{
	eel_gconf_set_float (PREF_SLIDESHOW_CHANGE_DELAY, gtk_spin_button_get_value (spinbutton));
}


void
ss__dlg_preferences_construct_cb (GtkWidget  *dialog,
				  GthBrowser *browser,
				  GtkBuilder *dialog_builder)
{
	BrowserData     *data;
	GtkWidget       *notebook;
	GtkWidget       *page;
	GtkListStore    *model;
	GtkCellRenderer *renderer;
	char            *current_transition;
	GList           *transitions;
	GList           *scan;
	GtkTreeIter      iter;
	int              i, i_active;
	GtkWidget       *label;

	data = g_new0 (BrowserData, 1);
	data->builder = _gtk_builder_new_from_file ("slideshow-preferences.ui", "slideshow");

	notebook = _gtk_builder_get_widget (dialog_builder, "notebook");

	page = _gtk_builder_get_widget (data->builder, "preferences_page");
	gtk_widget_show (page);

	model = gtk_list_store_new (2, G_TYPE_STRING, G_TYPE_STRING);
	data->transition_combobox = gtk_combo_box_new_with_model (GTK_TREE_MODEL (model));
	g_object_unref (model);

	renderer = gtk_cell_renderer_text_new ();
	gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (data->transition_combobox),
				    renderer,
				    TRUE);
	gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (data->transition_combobox),
					renderer,
					"text", TRANSITION_COLUMN_DISPLAY_NAME,
					NULL);

	current_transition = eel_gconf_get_string (PREF_SLIDESHOW_TRANSITION, DEFAULT_TRANSITION);
	transitions = gth_main_get_registered_objects (GTH_TYPE_TRANSITION);
	for (i = 0, i_active = 0, scan = transitions; scan; scan = scan->next, i++) {
		GthTransition *transition = scan->data;

		if (strcmp (gth_transition_get_id (transition), current_transition) == 0)
			i_active = i;

		gtk_list_store_append (model, &iter);
		gtk_list_store_set (model, &iter,
				    TRANSITION_COLUMN_ID, gth_transition_get_id (transition),
				    TRANSITION_COLUMN_DISPLAY_NAME, gth_transition_get_display_name (transition),
				    -1);
	}

	if (strcmp ("random", current_transition) == 0)
		i_active = i;
	gtk_list_store_append (model, &iter);
	gtk_list_store_set (model, &iter,
			    TRANSITION_COLUMN_ID, "random",
			    TRANSITION_COLUMN_DISPLAY_NAME, _("Random"),
			    -1);

	gtk_combo_box_set_active (GTK_COMBO_BOX (data->transition_combobox), i_active);
	gtk_widget_show (data->transition_combobox);
	gtk_container_add (GTK_CONTAINER (GET_WIDGET ("transition_box")), data->transition_combobox);
	g_free (current_transition);

	gtk_spin_button_set_value (GTK_SPIN_BUTTON (GET_WIDGET ("change_delay_spinbutton")), eel_gconf_get_float (PREF_SLIDESHOW_CHANGE_DELAY, 5));
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (GET_WIDGET ("automatic_checkbutton")), eel_gconf_get_boolean (PREF_SLIDESHOW_AUTOMATIC, TRUE));
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (GET_WIDGET ("wrap_around_checkbutton")), eel_gconf_get_boolean (PREF_SLIDESHOW_WRAP_AROUND, FALSE));

	g_signal_connect (G_OBJECT (data->transition_combobox),
			  "changed",
			  G_CALLBACK (transition_combobox_changed_cb),
			  data);
	g_signal_connect (G_OBJECT (GET_WIDGET ("automatic_checkbutton")),
			  "toggled",
			  G_CALLBACK (automatic_checkbutton_toggled_cb),
			  data);
	g_signal_connect (G_OBJECT (GET_WIDGET ("wrap_around_checkbutton")),
			  "toggled",
			  G_CALLBACK (wrap_around_checkbutton_toggled_cb),
			  data);
	g_signal_connect (G_OBJECT (GET_WIDGET ("change_delay_spinbutton")),
			  "value-changed",
			  G_CALLBACK (change_delay_spinbutton_value_changed_cb),
			  data);

	label = gtk_label_new (_("Slideshow"));
	gtk_widget_show (label);

	gtk_notebook_append_page (GTK_NOTEBOOK (notebook), page, label);

	g_object_set_data_full (G_OBJECT (dialog), BROWSER_DATA_KEY, data, (GDestroyNotify) browser_data_free);
}
