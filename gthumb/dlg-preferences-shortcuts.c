/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2019 The Free Software Foundation, Inc.
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
#include <gtk/gtk.h>
#include "dlg-preferences-shortcuts.h"
#include "glib-utils.h"
#include "gth-accel-dialog.h"
#include "gth-browser.h"
#include "gth-main.h"
#include "gth-preferences.h"
#include "gtk-utils.h"
#include "main.h"


#define GET_WIDGET(name) _gtk_builder_get_widget (data->builder, (name))
#define BROWSER_DATA_KEY "shortcuts-preference-data"


enum {
	ACTION_NAME_COLUMN,
	DESCRIPTION_COLUMN,
	ACCELERATOR_LABEL_COLUMN
};


typedef struct {
	GthBrowser *browser;
	GtkBuilder *builder;
} BrowserData;


static void
browser_data_free (BrowserData *data)
{
	g_object_unref (data->builder);
	g_free (data);
}


typedef struct {
	BrowserData *browser_data;
	GthShortcut *shortcut;
	GtkWidget   *accel_label;
} RowData;


static RowData *
row_data_new (BrowserData *data,
	      GthShortcut *shortcut)
{
	RowData *row_data;

	row_data = g_new0 (RowData, 1);
	row_data->browser_data = data;
	row_data->shortcut = shortcut;
	row_data->accel_label = NULL;

	return row_data;
}


static void
row_data_free (RowData *row_data)
{
	if (row_data == NULL)
		return;
	g_free (row_data);
}


static void
accel_dialog_response_cb (GtkDialog *dialog,
			  gint       response_id,
			  gpointer   user_data)
{
	RowData         *row_data = user_data;
	guint            keycode;
	GdkModifierType  modifiers;

	switch (response_id) {
	case GTK_RESPONSE_OK:
		if (gth_accel_dialog_get_accel (GTH_ACCEL_DIALOG (dialog), &keycode, &modifiers)) {
			gth_shortcut_set_key (row_data->shortcut, keycode, modifiers);
			gtk_label_set_text (GTK_LABEL (row_data->accel_label), row_data->shortcut->label);
		}
		gtk_widget_destroy (GTK_WIDGET (dialog));
		break;

	case GTK_RESPONSE_CANCEL:
		gtk_widget_destroy (GTK_WIDGET (dialog));
		break;

	case GTH_ACCEL_BUTTON_RESPONSE_DELETE:
		gth_shortcut_set_key (row_data->shortcut, 0, 0);
		gtk_label_set_text (GTK_LABEL (row_data->accel_label), row_data->shortcut->label);
		gtk_widget_destroy (GTK_WIDGET (dialog));
		break;
	}
}


static void
shortcuts_list_row_activated_cb (GtkListBox    *box,
				 GtkListBoxRow *row,
				 gpointer       user_data)
{
	RowData   *row_data;
	GtkWidget *dialog;

	row_data = g_object_get_data (G_OBJECT (row), "shortcut-row-data");

	dialog = gth_accel_dialog_new (_("Shortcut"),
				       _gtk_widget_get_toplevel_if_window (GTK_WIDGET (box)),
				       row_data->shortcut->keyval,
				       row_data->shortcut->modifiers);
	g_signal_connect (dialog,
			  "response",
			  G_CALLBACK (accel_dialog_response_cb),
			  row_data);
	gtk_widget_show (dialog);
}


static GtkWidget *
_new_shortcut_row (GthShortcut *shortcut,
		   BrowserData *data)
{
	GtkWidget *row;
	RowData   *row_data;
	GtkWidget *box;
	GtkWidget *label;

	row = gtk_list_box_row_new ();
	row_data = row_data_new (data, shortcut);
	g_object_set_data_full (G_OBJECT (row), "shortcut-row-data", row_data, (GDestroyNotify) row_data_free);

	box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
	gtk_container_set_border_width (GTK_CONTAINER (box), 5);
	gtk_container_add (GTK_CONTAINER (row), box);

	label = gtk_label_new (shortcut->description);
	gtk_label_set_xalign (GTK_LABEL (label), 0.0);
	gtk_widget_set_margin_end (label, 12);
	gtk_size_group_add_widget (GTK_SIZE_GROUP (gtk_builder_get_object (data->builder, "column1_size_group")), label);
	gtk_box_pack_start (GTK_BOX (box), label, TRUE, TRUE, 0);

	row_data->accel_label = label = gtk_label_new ((shortcut->label != NULL) ? shortcut->label : "");
	gtk_label_set_xalign (GTK_LABEL (label), 0.0);
	gtk_widget_set_margin_end (label, 12);
	gtk_size_group_add_widget (GTK_SIZE_GROUP (gtk_builder_get_object (data->builder, "column2_size_group")), label);
	gtk_box_pack_start (GTK_BOX (box), label, FALSE, FALSE, 0);

	gtk_widget_show_all (row);

	return row;
}


static GtkWidget *
_new_shortcut_category_row (GthShortcutCategory category,
			    int                 n_category)
{
	GtkWidget  *row;
	GtkWidget  *box;
	const char *text;
	char       *esc_text;
	char       *markup_text;
	GtkWidget  *label;

	row = gtk_list_box_row_new ();
	gtk_list_box_row_set_activatable (GTK_LIST_BOX_ROW (row), FALSE);
	gtk_list_box_row_set_selectable (GTK_LIST_BOX_ROW (row), FALSE);

	box = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
	if (n_category > 1)
		gtk_widget_set_margin_top (box, 15);
	gtk_container_add (GTK_CONTAINER (row), box);

	text = _("Others");
	switch (category) {
	case GTH_SHORTCUT_CATEGORY_UI:
		text = _("Show/Hide");
		break;
	case GTH_SHORTCUT_CATEGORY_FILE_NAVIGATION:
		text = _("File Navigation");
		break;
	case GTH_SHORTCUT_CATEGORY_FILE_EDIT:
		text = _("File Edit");
		break;
	case GTH_SHORTCUT_CATEGORY_IMAGE_VIEW:
		text = _("Viewer");
		break;
	case GTH_SHORTCUT_CATEGORY_IMAGE_EDIT:
		text = _("Image Edit");
		break;
	case GTH_SHORTCUT_CATEGORY_SLIDESHOW:
		text = _("Slideshow");
		break;
	}

	esc_text = g_markup_escape_text (text, -1);
	markup_text = g_strdup_printf ("<b>%s</b>", esc_text);

	label =  gtk_label_new (NULL);
	gtk_label_set_markup (GTK_LABEL (label), markup_text);
	gtk_label_set_xalign (GTK_LABEL (label), 0.0);
	gtk_widget_set_margin_start (label, 5);
	gtk_widget_set_margin_end (label, 5);
	gtk_widget_set_margin_top (label, 5);
	gtk_widget_set_margin_bottom (label, 5);
	gtk_box_pack_start (GTK_BOX (box), label, FALSE, FALSE, 0);
	gtk_style_context_add_class (gtk_widget_get_style_context (label), GTK_STYLE_CLASS_DIM_LABEL);
	gtk_box_pack_start (GTK_BOX (box), gtk_separator_new (GTK_ORIENTATION_HORIZONTAL), FALSE, FALSE, 0);

	gtk_widget_show_all (row);

	g_free (markup_text);
	g_free (esc_text);

	return row;
}


static int
sort_shortcuts_by_category (gconstpointer a,
			    gconstpointer b)
{
	const GthShortcut *sa = * (GthShortcut **) a;
	const GthShortcut *sb = * (GthShortcut **) b;

	if (sa->category < sb->category)
		return -1;
	if (sa->category > sb->category)
		return 1;

	return g_strcmp0 (sa->description, sb->description);
}


void
shortcuts__dlg_preferences_construct_cb (GtkWidget  *dialog,
					 GthBrowser *browser,
					 GtkBuilder *dialog_builder)
{
	BrowserData *data;
	GtkWidget   *shortcuts_list;
	GPtrArray   *accel_v;
	int          last_category;
	int          n_category;
	int          i;
	GtkWidget   *label;
	GtkWidget   *page;

	data = g_new0 (BrowserData, 1);
	data->browser = browser;
	data->builder = _gtk_builder_new_from_file ("shortcuts-preferences.ui", NULL);

	g_object_set_data_full (G_OBJECT (dialog), BROWSER_DATA_KEY, data, (GDestroyNotify) browser_data_free);

	shortcuts_list = _gtk_builder_get_widget (data->builder, "shortcuts_list");
	accel_v = gth_window_get_shortcuts (GTH_WINDOW (browser));
	g_ptr_array_sort (accel_v, sort_shortcuts_by_category);
	last_category = -1;
	n_category = 0;
	for (i = 0; i < accel_v->len; i++) {
		GthShortcut *shortcut = g_ptr_array_index (accel_v, i);

		if (shortcut->category == GTH_SHORTCUT_CATEGORY_HIDDEN)
			continue;

		if (shortcut->category != last_category) {
			last_category = shortcut->category;
			n_category++;
			gtk_list_box_insert (GTK_LIST_BOX (shortcuts_list),
					     _new_shortcut_category_row (shortcut->category, n_category),
					     -1);
		}
		gtk_list_box_insert (GTK_LIST_BOX (shortcuts_list),
				     _new_shortcut_row (shortcut, data),
				     -1);
	}

	g_signal_connect (shortcuts_list,
			  "row-activated",
			  G_CALLBACK (shortcuts_list_row_activated_cb),
			  data);

	/* add the page to the preferences dialog */

	label = gtk_label_new (_("Shortcuts"));
	gtk_widget_show (label);

	page = _gtk_builder_get_widget (data->builder, "preferences_page");
	gtk_widget_show (page);
	gtk_notebook_append_page (GTK_NOTEBOOK (_gtk_builder_get_widget (dialog_builder, "notebook")), page, label);
}