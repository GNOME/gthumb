/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2009 The Free Software Foundation, Inc.
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
#include "gth-main.h"
#include "gth-tags-entry.h"
#include "gth-tags-file.h"


enum {
	NAME_COLUMN,
	N_COLUMNS
};


struct _GthTagsEntryPrivate {
	char               **tags;
	GtkEntryCompletion  *completion;
	GtkListStore        *store;
	char                *new_tag;
	gboolean             action_create;
	gulong               monitor_event;
};


static gpointer parent_class = NULL;


static void
gth_tags_entry_finalize (GObject *obj)
{
	GthTagsEntry *self;

	self = GTH_TAGS_ENTRY (obj);

	g_signal_handler_disconnect (gth_main_get_default_monitor (), self->priv->monitor_event);

	g_free (self->priv->new_tag);
	g_object_unref (self->priv->completion);
	g_strfreev (self->priv->tags);

	G_OBJECT_CLASS (parent_class)->finalize (obj);
}


static void
gth_tags_entry_class_init (GthTagsEntryClass *klass)
{
	GObjectClass *object_class;

	parent_class = g_type_class_peek_parent (klass);
	g_type_class_add_private (klass, sizeof (GthTagsEntryPrivate));

	object_class = (GObjectClass*) (klass);
	object_class->finalize = gth_tags_entry_finalize;
}


static void
get_tag_limits (GthTagsEntry  *self,
		const char   **tag_start_p,
		const char   **tag_end_p)
{
	const char *text;
	const char *cursor_start;
	const char *tag_start;
	const char *tag_end;
	const char *tmp_tag_end;

	text = gtk_entry_get_text (GTK_ENTRY (self));
	cursor_start = g_utf8_offset_to_pointer (text, gtk_editable_get_position (GTK_EDITABLE (self)));

	if (g_utf8_get_char (cursor_start) == ',') {
		if (cursor_start != text + strlen (text))
			tag_start = g_utf8_next_char (cursor_start);
		else
			tag_start = text + strlen (text);
	}
	else {
		tag_start = g_utf8_strrchr (text, (gssize)(cursor_start - text), ',');
		if (tag_start == NULL)
			tag_start = text;
		else
			tag_start = g_utf8_next_char (tag_start);
	}

	tag_end = g_utf8_strchr (tag_start, -1, ',');
	if (tag_end == NULL)
		tag_end = text + strlen (text);

	while ((tag_start != tag_end) && g_unichar_isspace (g_utf8_get_char (tag_start)))
		tag_start = g_utf8_next_char (tag_start);

	tmp_tag_end = g_utf8_strrchr (tag_start, tag_end - tag_start, ' ');
	if (tmp_tag_end != NULL)
		tag_end = tmp_tag_end;

	*tag_start_p = tag_start;
	*tag_end_p = tag_end;
}


static gboolean
can_create_tag (GthTagsEntry *self,
		const char   *tag)
{
	int i;

	if (tag == NULL)
		return FALSE;
	if (tag[0] == '\0')
		return FALSE;

	for (i = 0; self->priv->tags[i] != NULL; i++)
		if (strcmp (self->priv->tags[i], tag) == 0)
			return FALSE;

	return TRUE;
}


static void
text_changed_cb (GthTagsEntry *self)
{
	const char *tag_start;
	const char *tag_end;

	g_free (self->priv->new_tag);
	self->priv->new_tag = NULL;

	if (self->priv->action_create) {
		gtk_entry_completion_delete_action (self->priv->completion, 0);
		self->priv->action_create = FALSE;
	}

	get_tag_limits (self, &tag_start, &tag_end);
	if (tag_start == tag_end)
		return;

	self->priv->new_tag = g_strndup (tag_start, tag_end - tag_start);
	self->priv->new_tag = g_strstrip (self->priv->new_tag);

	if (can_create_tag (self, self->priv->new_tag)) {
		char *action_text;

		action_text = g_strdup_printf (_("Create tag «%s»"), self->priv->new_tag);
		gtk_entry_completion_insert_action_text (self->priv->completion, 0, action_text);
		self->priv->action_create = TRUE;

		g_free (action_text);
	}
}


static gboolean
match_func (GtkEntryCompletion *completion,
            const char         *key,
            GtkTreeIter        *iter,
            gpointer            user_data)
{
	GthTagsEntry *self = user_data;
	char         *name;
	gboolean      result;

	if (self->priv->new_tag == NULL)
		return TRUE;

	if (self->priv->new_tag[0] == '\0')
		return TRUE;

	gtk_tree_model_get (GTK_TREE_MODEL (self->priv->store), iter,
			    NAME_COLUMN, &name,
			    -1);

	if (name != NULL) {
		char *k1;
		char *k2;

		k1 = g_utf8_casefold (self->priv->new_tag, -1);
		k2 = g_utf8_casefold (name, strlen (self->priv->new_tag));

		result = g_utf8_collate (k1, k2) == 0;


		g_free (k2);
		g_free (k1);
	}
	else
		result = FALSE;

	g_free (name);

	return result;
}


static gboolean
completion_match_selected_cb (GtkEntryCompletion *widget,
                              GtkTreeModel       *model,
                              GtkTreeIter        *iter,
                              gpointer            user_data)
{
	GthTagsEntry *self = user_data;
	const char   *tag_start;
	const char   *tag_end;
	const char   *text;
	char         *head;
	const char   *tail;
	char         *tag_name = NULL;

	get_tag_limits (self, &tag_start, &tag_end);
	text = gtk_entry_get_text (GTK_ENTRY (self));
	head = g_strndup (text, tag_start - text);
	head = g_strstrip (head);
	if (tag_end[0] != '\0')
		tail = g_utf8_next_char (tag_end);
	else
		tail = tag_end;
	gtk_tree_model_get (model, iter,
			    NAME_COLUMN, &tag_name,
			    -1);

	if (tag_name != NULL) {
		char *new_text;

		new_text = g_strconcat (head,
					" ",
					tag_name,
					", " /*(tail != tag_end ? ", " : "")*/,
					tail,
					NULL);
		gtk_entry_set_text (GTK_ENTRY (self), new_text);
		gtk_editable_set_position (GTK_EDITABLE (self), -1);

		g_free (new_text);
		g_free (tag_name);
	}

	g_free (head);

	return TRUE;
}


static void
completion_action_activated_cb (GtkEntryCompletion *widget,
                                int                 index,
                                gpointer            user_data)
{
	GthTagsEntry *self = user_data;
	const char   *tag_start;
	const char   *tag_end;
	const char   *text;
	char         *head;
	const char   *tail;
	char         *tag_name = NULL;
	char         *new_text;
	GthTagsFile  *tags_file;

	if (index != 0)
		return;

	get_tag_limits (self, &tag_start, &tag_end);
	text = gtk_entry_get_text (GTK_ENTRY (self));
	head = g_strndup (text, tag_start - text);
	head = g_strstrip (head);
	if (tag_end[0] != '\0')
		tail = g_utf8_next_char (tag_end);
	else
		tail = tag_end;
	tag_name = g_strndup (tag_start, tag_end - tag_start);
	tag_name = g_strstrip (tag_name);
	new_text = g_strconcat (head,
				" ",
				tag_name,
				", ",
				tail,
				NULL);

	gtk_entry_set_text (GTK_ENTRY (self), new_text);
	gtk_editable_set_position (GTK_EDITABLE (self), -1);

	tags_file = gth_main_get_default_tag_file ();
	gth_tags_file_add (tags_file, tag_name);
	gth_main_tags_changed ();

	g_free (new_text);
	g_free (tag_name);
	g_free (head);
}


static int
sort_tag_by_name (gconstpointer a,
		  gconstpointer b,
		  gpointer      user_data)
{
	char *sa = * (char **) a;
	char *sb = * (char **) b;

	return g_utf8_collate (sa, sb);
}


static void
update_tag_list (GthTagsEntry *self)
{
	GthTagsFile *tags;
	int          i;

	tags = gth_main_get_default_tag_file ();

	g_strfreev (self->priv->tags);
	self->priv->tags = g_strdupv (gth_tags_file_get_tags (tags));

	for (i = 0; self->priv->tags[i] != NULL; i++) {
		GtkTreeIter iter;

		gtk_list_store_append (self->priv->store, &iter);
		gtk_list_store_set (self->priv->store, &iter,
				    NAME_COLUMN, self->priv->tags[i],
				    -1);
	}

	if ((self->priv->tags == NULL) || self->priv->tags[0] == NULL) {
		char *default_tags[] = { N_("Holidays"),
                                         N_("Temporary"),
                                         N_("Screenshots"),
                                         N_("Science"),
                                         N_("Favorite"),
                                         N_("Important"),
                                         N_("GNOME"),
                                         N_("Games"),
                                         N_("Party"),
                                         N_("Birthday"),
                                         N_("Astronomy"),
                                         N_("Family"),
                                         NULL };

		if (self->priv->tags != NULL)
			g_strfreev (self->priv->tags);

		self->priv->tags = g_new (char *, g_strv_length (default_tags) + 1);
		for (i = 0; default_tags[i] != NULL; i++)
			self->priv->tags[i] = g_strdup (_(default_tags[i]));
		self->priv->tags[i] = NULL;
	}

	g_qsort_with_data (self->priv->tags,
			   g_strv_length (self->priv->tags),
			   sizeof (char *),
			   sort_tag_by_name,
			   NULL);

	gtk_list_store_clear (self->priv->store);
	for (i = 0; self->priv->tags[i] != NULL; i++) {
		GtkTreeIter iter;

		gtk_list_store_append (self->priv->store, &iter);
		gtk_list_store_set (self->priv->store, &iter,
				    NAME_COLUMN, self->priv->tags[i],
				    -1);
	}
}


static void
tags_changed_cb (GthMonitor   *monitor,
		 GthTagsEntry *self)
{
	update_tag_list (self);
}


static void
gth_tags_entry_instance_init (GthTagsEntry *self)
{
	self->priv = G_TYPE_INSTANCE_GET_PRIVATE (self, GTH_TYPE_TAGS_ENTRY, GthTagsEntryPrivate);
	self->priv->completion = gtk_entry_completion_new ();
	gtk_entry_completion_set_popup_completion (self->priv->completion, TRUE);
	gtk_entry_set_completion (GTK_ENTRY (self), self->priv->completion);
	self->priv->store = gtk_list_store_new (N_COLUMNS, G_TYPE_STRING);
	gtk_entry_completion_set_model (self->priv->completion, GTK_TREE_MODEL (self->priv->store));
	g_object_unref (self->priv->store);
	gtk_entry_completion_set_text_column (self->priv->completion, NAME_COLUMN);
	gtk_entry_completion_set_match_func (self->priv->completion, match_func, self, NULL);

	g_signal_connect (self,
			  "notify::text",
			  G_CALLBACK (text_changed_cb),
			  self);
	g_signal_connect (self->priv->completion,
			  "match-selected",
			  G_CALLBACK (completion_match_selected_cb),
			  self);
	g_signal_connect (self->priv->completion,
			  "action-activated",
			  G_CALLBACK (completion_action_activated_cb),
			  self);
	self->priv->monitor_event = g_signal_connect (gth_main_get_default_monitor (),
						      "tags-changed",
						      G_CALLBACK (tags_changed_cb),
						      self);
}


GType
gth_tags_entry_get_type (void)
{
	static GType type = 0;

	if (type == 0) {
		static const GTypeInfo g_define_type_info = {
			sizeof (GthTagsEntryClass),
			NULL,
			NULL,
			(GClassInitFunc) gth_tags_entry_class_init,
			NULL,
			NULL,
			sizeof (GthTagsEntry),
			0,
			(GInstanceInitFunc) gth_tags_entry_instance_init,
			NULL
		};
		type = g_type_register_static (GTK_TYPE_ENTRY,
					       "GthTagsEntry",
					       &g_define_type_info,
					       0);
	}

	return type;
}


GtkWidget *
gth_tags_entry_new (void)
{
	GthTagsEntry *self;

	self = g_object_new (GTH_TYPE_TAGS_ENTRY, NULL);
	update_tag_list (self);

	return (GtkWidget *) self;
}


char **
gth_tags_entry_get_tags (GthTagsEntry *self)
{
	GthTagsFile  *tags_file;
	char        **all_tags;
	char        **tags;
	int           i;
	int           j;

	tags_file = gth_main_get_default_tag_file ();

	all_tags = g_strsplit (gtk_entry_get_text (GTK_ENTRY (self)), ",", -1);
	tags = g_new0 (char *, g_strv_length (all_tags) + 1);
	for (i = 0, j = 0; all_tags[i] != NULL; i++) {
		all_tags[i] = g_strstrip (all_tags[i]);
		if (all_tags[i][0] != '\0') {
			tags[j] = g_strdup (all_tags[i]);
			gth_tags_file_add (tags_file, tags[j]);
			j++;
		}
	}
	g_strfreev (all_tags);

	for (i = 0; self->priv->tags[i] != NULL; i++)
		gth_tags_file_add (tags_file, self->priv->tags[i]);

	gth_main_tags_changed ();

	return tags;
}
