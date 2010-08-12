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
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <config.h>
#include <glib/gi18n.h>
#include "preferences.h"


#define BROWSER_DATA_KEY "image-viewer-preference-data"


typedef struct {
	GtkBuilder *builder;
	GtkWidget  *change_zoom_combobox;
	GtkWidget  *transp_type_combobox;
} BrowserData;


static void
browser_data_free (BrowserData *data)
{
	g_object_unref (data->builder);
	g_free (data);
}


static void
zoom_quality_changed_cb (GtkComboBox *combo_box,
			 BrowserData     *data)
{
	eel_gconf_set_enum (PREF_ZOOM_QUALITY, GTH_TYPE_ZOOM_QUALITY, gtk_combo_box_get_active (combo_box));
}



static void
zoom_change_changed_cb (GtkComboBox *combo_box,
			BrowserData     *data)
{
	eel_gconf_set_enum (PREF_ZOOM_CHANGE, GTH_TYPE_ZOOM_CHANGE, gtk_combo_box_get_active (combo_box));
}


static void
reset_scrollbars_toggled_cb (GtkToggleButton *button,
			     BrowserData     *data)
{
	eel_gconf_set_boolean (PREF_RESET_SCROLLBARS, gtk_toggle_button_get_active (button));
}


static void
transp_type_changed_cb (GtkComboBox *combo_box,
			BrowserData *data)
{
	eel_gconf_set_enum (PREF_TRANSP_TYPE, GTH_TYPE_TRANSP_TYPE, gtk_combo_box_get_active (combo_box));
}


void
image_viewer__dlg_preferences_construct_cb (GtkWidget  *dialog,
					    GthBrowser *browser,
					    GtkBuilder *dialog_builder)
{
	BrowserData *data;
	GtkWidget   *notebook;
	GtkWidget   *page;
	GtkWidget   *label;

	data = g_new0 (BrowserData, 1);
	data->builder = _gtk_builder_new_from_file ("image-viewer-preferences.ui", "image_viewer");

	notebook = _gtk_builder_get_widget (dialog_builder, "notebook");

	page = _gtk_builder_get_widget (data->builder, "preferences_page");
	gtk_widget_show (page);

	data->change_zoom_combobox = _gtk_combo_box_new_with_texts (_("Set to actual size"),
								    _("Keep previous zoom"),
								    _("Fit to window"),
								    _("Fit to window if larger"),
								    _("Fit to width"),
								    _("Fit to width if larger"),
								    NULL);
	gtk_combo_box_set_active (GTK_COMBO_BOX (data->change_zoom_combobox), eel_gconf_get_enum (PREF_ZOOM_CHANGE, GTH_TYPE_ZOOM_CHANGE, GTH_ZOOM_CHANGE_FIT_SIZE_IF_LARGER));
	gtk_widget_show (data->change_zoom_combobox);
	gtk_container_add (GTK_CONTAINER (_gtk_builder_get_widget (data->builder, "zoom_change_box")), data->change_zoom_combobox);

	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (_gtk_builder_get_widget (data->builder, "toggle_reset_scrollbars")), eel_gconf_get_boolean (PREF_RESET_SCROLLBARS, TRUE));

	data->transp_type_combobox = _gtk_combo_box_new_with_texts (_("White"),
								    _("None"),
								    _("Black"),
								    _("Checked"),
								    NULL);
	gtk_combo_box_set_active (GTK_COMBO_BOX (data->transp_type_combobox), eel_gconf_get_enum (PREF_TRANSP_TYPE, GTH_TYPE_TRANSP_TYPE, GTH_TRANSP_TYPE_NONE));
	gtk_widget_show (data->transp_type_combobox);
	gtk_container_add (GTK_CONTAINER (_gtk_builder_get_widget (data->builder, "transp_type_box")), data->transp_type_combobox);

	gtk_combo_box_set_active (GTK_COMBO_BOX (_gtk_builder_get_widget (data->builder, "zoom_quality_combobox")), eel_gconf_get_enum (PREF_ZOOM_QUALITY, GTH_TYPE_ZOOM_QUALITY, GTH_ZOOM_QUALITY_LOW));

	g_signal_connect (_gtk_builder_get_widget (data->builder, "zoom_quality_combobox"),
			  "changed",
			  G_CALLBACK (zoom_quality_changed_cb),
			  data);
	g_signal_connect (G_OBJECT (data->change_zoom_combobox),
			  "changed",
			  G_CALLBACK (zoom_change_changed_cb),
			  data);
	g_signal_connect (G_OBJECT (data->transp_type_combobox),
			  "changed",
			  G_CALLBACK (transp_type_changed_cb),
			  data);
	g_signal_connect (G_OBJECT (_gtk_builder_get_widget (data->builder, "toggle_reset_scrollbars")),
			  "toggled",
			  G_CALLBACK (reset_scrollbars_toggled_cb),
			  data);

	label = gtk_label_new (_("Viewer"));
	gtk_widget_show (label);

	gtk_notebook_append_page (GTK_NOTEBOOK (notebook), page, label);

	g_object_set_data_full (G_OBJECT (dialog), BROWSER_DATA_KEY, data, (GDestroyNotify) browser_data_free);
}
