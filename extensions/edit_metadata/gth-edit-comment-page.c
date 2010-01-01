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
#include <gio/gio.h>
#include "gth-edit-comment-page.h"
#include "gth-edit-metadata-dialog.h"


#define GTH_EDIT_COMMENT_PAGE_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), GTH_TYPE_EDIT_COMMENT_PAGE, GthEditCommentPagePrivate))
#define GET_WIDGET(name) _gtk_builder_get_widget (self->priv->builder, (name))


typedef enum {
	NO_DATE = 0,
	FOLLOWING_DATE,
	CURRENT_DATE,
	PHOTO_DATE,
	LAST_MODIFIED_DATE,
	CREATION_DATE,
	NO_CHANGE
} DateOption;


static gpointer gth_edit_comment_page_parent_class = NULL;


struct _GthEditCommentPagePrivate {
	GFileInfo  *info;
	GtkBuilder *builder;
	GtkWidget  *date_combobox;
	GtkWidget  *date_selector;
	GtkWidget  *tags_entry;
};


void
gth_edit_comment_page_real_set_file (GthEditMetadataPage *base,
		 		     GthFileData         *file_data)
{
	GthEditCommentPage  *self;
	GtkTextBuffer       *buffer;
	GthMetadata         *metadata;
	GthStringList       *tags;
	GthMetadataProvider *provider;
	gboolean             no_provider;

	self = GTH_EDIT_COMMENT_PAGE (base);

	_g_object_unref (self->priv->info);
	self->priv->info = g_file_info_new ();
	g_file_info_copy_into (file_data->info, self->priv->info);

	buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (GET_WIDGET ("note_text")));
	metadata = (GthMetadata *) g_file_info_get_attribute_object (file_data->info, "general::description");
	if (metadata != NULL) {
		GtkTextIter iter;

		gtk_text_buffer_set_text (buffer, gth_metadata_get_formatted (metadata), -1);
		gtk_text_buffer_get_iter_at_line (buffer, &iter, 0);
		gtk_text_buffer_place_cursor (buffer, &iter);
	}
	else
		gtk_text_buffer_set_text (buffer, "", -1);

	metadata = (GthMetadata *) g_file_info_get_attribute_object (file_data->info, "general::title");
	if (metadata != NULL)
		gtk_entry_set_text (GTK_ENTRY (GET_WIDGET ("title_entry")), gth_metadata_get_formatted (metadata));
	else
		gtk_entry_set_text (GTK_ENTRY (GET_WIDGET ("title_entry")), "");

	metadata = (GthMetadata *) g_file_info_get_attribute_object (file_data->info, "general::location");
	if (metadata != NULL)
		gtk_entry_set_text (GTK_ENTRY (GET_WIDGET ("place_entry")), gth_metadata_get_formatted (metadata));
	else
		gtk_entry_set_text (GTK_ENTRY (GET_WIDGET ("place_entry")), "");

	metadata = (GthMetadata *) g_file_info_get_attribute_object (file_data->info, "general::datetime");
	if (metadata != NULL) {
		gtk_combo_box_set_active (GTK_COMBO_BOX (self->priv->date_combobox), FOLLOWING_DATE);
		gth_time_selector_set_exif_date (GTH_TIME_SELECTOR (self->priv->date_selector), gth_metadata_get_raw (metadata));
	}
	else {
		gtk_combo_box_set_active (GTK_COMBO_BOX (self->priv->date_combobox), NO_DATE);
		gth_time_selector_set_exif_date (GTH_TIME_SELECTOR (self->priv->date_selector), "");
	}

	tags = (GthStringList *) g_file_info_get_attribute_object (file_data->info, "general::tags");
	if (tags != NULL) {
		char *value;

		value = gth_string_list_join (tags, ", ");
		gth_tags_entry_set_text (GTH_TAGS_ENTRY (self->priv->tags_entry), value);

		g_free (value);
	}
	else
		gth_tags_entry_set_text (GTH_TAGS_ENTRY (self->priv->tags_entry), "");

	gtk_widget_grab_focus (GET_WIDGET ("note_text"));

	no_provider = TRUE;

  	provider = gth_main_get_metadata_writer ("general::description", gth_file_data_get_mime_type (file_data));
	gtk_widget_set_sensitive (GET_WIDGET ("note_text"), provider != NULL);
	if (no_provider && (provider != NULL))
		no_provider = FALSE;
	_g_object_unref (provider);

	provider = gth_main_get_metadata_writer ("general::location", gth_file_data_get_mime_type (file_data));
	gtk_widget_set_sensitive (GET_WIDGET ("place_entry"), provider != NULL);
	if (no_provider && (provider != NULL))
		no_provider = FALSE;
	_g_object_unref (provider);

	provider = gth_main_get_metadata_writer ("general::datetime", gth_file_data_get_mime_type (file_data));
	gtk_widget_set_sensitive (self->priv->date_combobox, provider != NULL);
	gtk_widget_set_sensitive (self->priv->date_selector, provider != NULL);
	if (no_provider && (provider != NULL))
		no_provider = FALSE;
	_g_object_unref (provider);

	provider = gth_main_get_metadata_writer ("general::tags", gth_file_data_get_mime_type (file_data));
	gtk_widget_set_sensitive (self->priv->tags_entry, provider != NULL);
	if (no_provider && (provider != NULL))
		no_provider = FALSE;
	_g_object_unref (provider);

	provider = gth_main_get_metadata_writer ("general::rating", gth_file_data_get_mime_type (file_data));
	gtk_widget_set_sensitive (GET_WIDGET ("rating_spinbutton"), provider != NULL);
	if (no_provider && (provider != NULL))
		no_provider = FALSE;
	_g_object_unref (provider);

	if (no_provider)
		gtk_widget_hide (GTK_WIDGET (self));
	else
		gtk_widget_show (GTK_WIDGET (self));
}


void
gth_edit_comment_page_real_update_info (GthEditMetadataPage *base,
					GFileInfo           *info)
{
	GthEditCommentPage  *self;
	GtkTextBuffer       *buffer;
	GtkTextIter          start;
	GtkTextIter          end;
	char                *text;
	GthMetadata         *metadata;
	int                  i;
	char               **tagv;
	GList               *tags;
	GthStringList       *string_list;
	GthDateTime         *date_time;
	char                *exif_date;

	self = GTH_EDIT_COMMENT_PAGE (base);

	/* caption */

	metadata = g_object_new (GTH_TYPE_METADATA,
				 "id", "general::title",
				 "raw", gtk_entry_get_text (GTK_ENTRY (GET_WIDGET ("title_entry"))),
				 "formatted", gtk_entry_get_text (GTK_ENTRY (GET_WIDGET ("title_entry"))),
				 NULL);
	g_file_info_set_attribute_object (info, "general::title", G_OBJECT (metadata));
	g_object_unref (metadata);

	/* comment */

	buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (GET_WIDGET ("note_text")));
	gtk_text_buffer_get_bounds (buffer, &start, &end);
	text = gtk_text_buffer_get_text (buffer, &start, &end, FALSE);
	metadata = g_object_new (GTH_TYPE_METADATA,
				 "id", "general::description",
				 "raw", text,
				 "formatted", text,
				 NULL);
	g_file_info_set_attribute_object (info, "general::description", G_OBJECT (metadata));
	g_object_unref (metadata);
	g_free (text);

	/* location */

	metadata = g_object_new (GTH_TYPE_METADATA,
				 "id", "general::location",
				 "raw", gtk_entry_get_text (GTK_ENTRY (GET_WIDGET ("place_entry"))),
				 "formatted", gtk_entry_get_text (GTK_ENTRY (GET_WIDGET ("place_entry"))),
				 NULL);
	g_file_info_set_attribute_object (info, "general::location", G_OBJECT (metadata));
	g_object_unref (metadata);

	/* date */

	date_time = gth_datetime_new ();
	gth_time_selector_get_value (GTH_TIME_SELECTOR (self->priv->date_selector), date_time);
	exif_date = gth_datetime_to_exif_date (date_time);
	metadata = g_object_new (GTH_TYPE_METADATA,
				 "id", "general::datetime",
				 "raw", exif_date,
				 "formatted", exif_date,
				 NULL);
	g_file_info_set_attribute_object (info, "general::datetime", G_OBJECT (metadata));
	g_object_unref (metadata);
	gth_datetime_free (date_time);

	/* tags */

	tagv = gth_tags_entry_get_tags (GTH_TAGS_ENTRY (self->priv->tags_entry), TRUE);
	tags = NULL;
	for (i = 0; tagv[i] != NULL; i++)
		tags = g_list_prepend (tags, tagv[i]);
	tags = g_list_reverse (tags);
	string_list = gth_string_list_new (tags);
	g_file_info_set_attribute_object (info, "general::tags", G_OBJECT (string_list));

	g_free (exif_date);
	g_object_unref (string_list);
	g_strfreev (tagv);
	g_list_free (tags);
}


const char *
gth_edit_comment_page_real_get_name (GthEditMetadataPage *self)
{
	return _("General");
}


static void
gth_edit_comment_page_finalize (GObject *object)
{
	GthEditCommentPage *self;

	self = GTH_EDIT_COMMENT_PAGE (object);

	_g_object_unref (self->priv->info);
	g_object_unref (self->priv->builder);

	G_OBJECT_CLASS (gth_edit_comment_page_parent_class)->finalize (object);
}


static void
gth_edit_comment_page_class_init (GthEditCommentPageClass *klass)
{
	gth_edit_comment_page_parent_class = g_type_class_peek_parent (klass);
	g_type_class_add_private (klass, sizeof (GthEditCommentPagePrivate));

	G_OBJECT_CLASS (klass)->finalize = gth_edit_comment_page_finalize;
}


static char *
get_date_from_option (GthEditCommentPage *self,
		      DateOption          option)
{
	GTimeVal     timeval;
	GthDateTime *date_time;
	char        *exif_date;
	GthMetadata *metadata;

	_g_time_val_reset (&timeval);

	switch (option) {
	case NO_DATE:
		return g_strdup ("");
	case FOLLOWING_DATE:
		date_time = gth_datetime_new ();
		gth_time_selector_get_value (GTH_TIME_SELECTOR (self->priv->date_selector), date_time);
		exif_date = gth_datetime_to_exif_date (date_time);
		_g_time_val_from_exif_date (exif_date, &timeval);
		g_free (exif_date);
		gth_datetime_free (date_time);
		break;
	case CURRENT_DATE:
		g_get_current_time (&timeval);
		break;
	case PHOTO_DATE:
		metadata = (GthMetadata *) g_file_info_get_attribute_object (self->priv->info, "Embedded::Photo::DateTimeOriginal");
		if (metadata != NULL)
			_g_time_val_from_exif_date (gth_metadata_get_raw (metadata), &timeval);
		else
			return g_strdup ("");
		break;
	case LAST_MODIFIED_DATE:
		timeval.tv_sec = g_file_info_get_attribute_uint64 (self->priv->info, "time::modified");
		timeval.tv_usec = g_file_info_get_attribute_uint32 (self->priv->info, "time::modified-usec");
		break;
	case CREATION_DATE:
		timeval.tv_sec = g_file_info_get_attribute_uint64 (self->priv->info, "time::created");
		timeval.tv_usec = g_file_info_get_attribute_uint32 (self->priv->info, "time::created-usec");
		break;
	case NO_CHANGE:
		metadata = (GthMetadata *) g_file_info_get_attribute_object (self->priv->info, "general::datetime");
		if (metadata != NULL)
			_g_time_val_from_exif_date (gth_metadata_get_raw (metadata), &timeval);
		else
			return g_strdup ("");
		break;
	}

	return _g_time_val_to_exif_date (&timeval);
}


static void
date_combobox_changed_cb (GtkComboBox *widget,
			  gpointer     user_data)
{
	GthEditCommentPage *self = user_data;
	char               *value;

	value = get_date_from_option (self, gtk_combo_box_get_active (widget));
	gth_time_selector_set_exif_date (GTH_TIME_SELECTOR (self->priv->date_selector), value);

	g_free (value);
}


static void
gth_edit_comment_page_init (GthEditCommentPage *self)
{
	self->priv = GTH_EDIT_COMMENT_PAGE_GET_PRIVATE (self);
	self->priv->info = NULL;

	gtk_container_set_border_width (GTK_CONTAINER (self), 12);

	self->priv->builder = _gtk_builder_new_from_file ("edit-comment-page.ui", "edit_metadata");
  	gtk_box_pack_start (GTK_BOX (self), _gtk_builder_get_widget (self->priv->builder, "content"), TRUE, TRUE, 0);

  	self->priv->date_combobox = gtk_combo_box_new_text ();
  	_gtk_combo_box_append_texts (GTK_COMBO_BOX (self->priv->date_combobox),
  				     _("No date"),
  				     _("The following date"),
  				     _("Current date"),
  				     _("Date photo was taken"),
  				     _("Last modified date"),
  				     _("File creation date"),
  				     _("Do not modify"),
  				     NULL);
  	gtk_widget_show (self->priv->date_combobox);
  	gtk_box_pack_start (GTK_BOX (GET_WIDGET ("date_combobox_container")), self->priv->date_combobox, TRUE, TRUE, 0);

  	g_signal_connect (self->priv->date_combobox,
			  "changed",
			  G_CALLBACK (date_combobox_changed_cb),
			  self);

  	self->priv->date_selector = gth_time_selector_new ();
  	gtk_widget_show (self->priv->date_selector);
  	gtk_box_pack_start (GTK_BOX (GET_WIDGET ("date_selector_container")), self->priv->date_selector, FALSE, FALSE, 0);

  	self->priv->tags_entry = gth_tags_entry_new ();
  	gtk_widget_show (self->priv->tags_entry);
  	gtk_box_pack_start (GTK_BOX (GET_WIDGET ("tags_entry_container")), self->priv->tags_entry, FALSE, FALSE, 0);
}


static void
gth_edit_comment_page_gth_edit_comment_page_interface_init (GthEditMetadataPageIface *iface)
{
	iface->set_file = gth_edit_comment_page_real_set_file;
	iface->update_info = gth_edit_comment_page_real_update_info;
	iface->get_name = gth_edit_comment_page_real_get_name;
}


GType
gth_edit_comment_page_get_type (void)
{
	static GType type = 0;

	if (type == 0) {
		static const GTypeInfo g_define_type_info = {
			sizeof (GthEditCommentPageClass),
			(GBaseInitFunc) NULL,
			(GBaseFinalizeFunc) NULL,
			(GClassInitFunc) gth_edit_comment_page_class_init,
			(GClassFinalizeFunc) NULL,
			NULL,
			sizeof (GthEditCommentPage),
			0,
			(GInstanceInitFunc) gth_edit_comment_page_init,
			NULL
		};
		static const GInterfaceInfo gth_edit_comment_page_info = {
			(GInterfaceInitFunc) gth_edit_comment_page_gth_edit_comment_page_interface_init,
			(GInterfaceFinalizeFunc) NULL,
			NULL
		};
		type = g_type_register_static (GTK_TYPE_VBOX,
					       "GthEditCommentPage",
					       &g_define_type_info,
					       0);
		g_type_add_interface_static (type, GTH_TYPE_EDIT_METADATA_PAGE, &gth_edit_comment_page_info);
	}

	return type;
}
