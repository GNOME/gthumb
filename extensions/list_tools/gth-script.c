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
#include <gthumb.h>
#include <gdk/gdkkeysyms.h>
#include "gth-script.h"
#include "shortcuts.h"


static void gth_script_dom_domizable_interface_init (DomDomizableInterface *iface);
static void gth_script_gth_duplicable_interface_init (GthDuplicableInterface *iface);


enum {
	PROP_0,
	PROP_ID,
	PROP_DISPLAY_NAME,
	PROP_COMMAND,
	PROP_VISIBLE,
	PROP_SHELL_SCRIPT,
	PROP_FOR_EACH_FILE,
	PROP_WAIT_COMMAND,
	PROP_ACCELERATOR
};


struct _GthScriptPrivate {
	char            *id;
	char            *display_name;
	char            *command;
	gboolean         visible;
	gboolean         shell_script;
	gboolean         for_each_file;
	gboolean         wait_command;
	char            *accelerator;
	char            *detailed_action;
};


G_DEFINE_TYPE_WITH_CODE (GthScript,
			 gth_script,
			 G_TYPE_OBJECT,
			 G_ADD_PRIVATE (GthScript)
			 G_IMPLEMENT_INTERFACE (DOM_TYPE_DOMIZABLE,
						gth_script_dom_domizable_interface_init)
			 G_IMPLEMENT_INTERFACE (GTH_TYPE_DUPLICABLE,
						gth_script_gth_duplicable_interface_init))


static DomElement*
gth_script_real_create_element (DomDomizable *base,
				DomDocument  *doc)
{
	GthScript  *self;
	DomElement *element;

	g_return_val_if_fail (DOM_IS_DOCUMENT (doc), NULL);

	self = GTH_SCRIPT (base);

	element = dom_document_create_element (doc, "script",
					       "id", self->priv->id,
					       "display-name", self->priv->display_name,
					       "command", self->priv->command,
					       "shell-script", (self->priv->shell_script ? "true" : "false"),
					       "for-each-file", (self->priv->for_each_file ? "true" : "false"),
					       "wait-command", (self->priv->wait_command ? "true" : "false"),
					       NULL);
	if (! self->priv->visible)
		dom_element_set_attribute (element, "display", "none");

	return element;
}


static void
gth_script_real_load_from_element (DomDomizable *base,
				   DomElement   *element)
{
	GthScript *self;

	g_return_if_fail (DOM_IS_ELEMENT (element));

	self = GTH_SCRIPT (base);
	g_object_set (self,
		      "id", dom_element_get_attribute (element, "id"),
		      "display-name", dom_element_get_attribute (element, "display-name"),
		      "command", dom_element_get_attribute (element, "command"),
		      "visible", (g_strcmp0 (dom_element_get_attribute (element, "display"), "none") != 0),
		      "shell-script", (g_strcmp0 (dom_element_get_attribute (element, "shell-script"), "true") == 0),
		      "for-each-file", (g_strcmp0 (dom_element_get_attribute (element, "for-each-file"), "true") == 0),
		      "wait-command", (g_strcmp0 (dom_element_get_attribute (element, "wait-command"), "true") == 0),
		      "accelerator", "",
		      NULL);
}


static GObject *
gth_script_real_duplicate (GthDuplicable *duplicable)
{
	GthScript *script = GTH_SCRIPT (duplicable);
	GthScript *new_script;

	new_script = gth_script_new ();
	g_object_set (new_script,
		      "id", script->priv->id,
		      "display-name", script->priv->display_name,
		      "command", script->priv->command,
		      "visible", script->priv->visible,
		      "shell-script", script->priv->shell_script,
		      "for-each-file", script->priv->for_each_file,
		      "wait-command", script->priv->wait_command,
		      "accelerator", script->priv->accelerator,
		      NULL);

	return (GObject *) new_script;
}


static void
gth_script_finalize (GObject *base)
{
	GthScript *self;

	self = GTH_SCRIPT (base);
	g_free (self->priv->id);
	g_free (self->priv->display_name);
	g_free (self->priv->command);
	g_free (self->priv->accelerator);
	g_free (self->priv->detailed_action);

	G_OBJECT_CLASS (gth_script_parent_class)->finalize (base);
}


static char *
detailed_action_from_id (char *id)
{
	GVariant *param;
	char     *detailed_action;

	param = g_variant_new_string (id);
	detailed_action = g_action_print_detailed_name ("exec-script", param);

	g_variant_unref (param);

	return detailed_action;
}


static void
gth_script_set_property (GObject      *object,
		         guint         property_id,
		         const GValue *value,
		         GParamSpec   *pspec)
{
	GthScript *self;

        self = GTH_SCRIPT (object);

	switch (property_id) {
	case PROP_ID:
		g_free (self->priv->id);
		self->priv->id = g_value_dup_string (value);
		if (self->priv->id == NULL)
			self->priv->id = g_strdup ("");
		g_free (self->priv->detailed_action);
		self->priv->detailed_action = detailed_action_from_id (self->priv->id);
		break;
	case PROP_DISPLAY_NAME:
		g_free (self->priv->display_name);
		self->priv->display_name = g_value_dup_string (value);
		if (self->priv->display_name == NULL)
			self->priv->display_name = g_strdup ("");
		break;
	case PROP_COMMAND:
		g_free (self->priv->command);
		self->priv->command = g_value_dup_string (value);
		if (self->priv->command == NULL)
			self->priv->command = g_strdup ("");
		break;
	case PROP_VISIBLE:
		self->priv->visible = g_value_get_boolean (value);
		break;
	case PROP_SHELL_SCRIPT:
		self->priv->shell_script = g_value_get_boolean (value);
		break;
	case PROP_FOR_EACH_FILE:
		self->priv->for_each_file = g_value_get_boolean (value);
		break;
	case PROP_WAIT_COMMAND:
		self->priv->wait_command = g_value_get_boolean (value);
		break;
	case PROP_ACCELERATOR:
		g_free (self->priv->accelerator);
		self->priv->accelerator = g_value_dup_string (value);
		break;
	default:
		break;
	}
}


static void
gth_script_get_property (GObject    *object,
		         guint       property_id,
		         GValue     *value,
		         GParamSpec *pspec)
{
	GthScript *self;

        self = GTH_SCRIPT (object);

	switch (property_id) {
	case PROP_ID:
		g_value_set_string (value, self->priv->id);
		break;
	case PROP_DISPLAY_NAME:
		g_value_set_string (value, self->priv->display_name);
		break;
	case PROP_COMMAND:
		g_value_set_string (value, self->priv->command);
		break;
	case PROP_VISIBLE:
		g_value_set_boolean (value, self->priv->visible);
		break;
	case PROP_SHELL_SCRIPT:
		g_value_set_boolean (value, self->priv->shell_script);
		break;
	case PROP_FOR_EACH_FILE:
		g_value_set_boolean (value, self->priv->for_each_file);
		break;
	case PROP_WAIT_COMMAND:
		g_value_set_boolean (value, self->priv->wait_command);
		break;
	case PROP_ACCELERATOR:
		g_value_set_string (value, self->priv->accelerator);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
		break;
	}
}


static void
gth_script_class_init (GthScriptClass *klass)
{
	GObjectClass *object_class;

	object_class = G_OBJECT_CLASS (klass);
	object_class->set_property = gth_script_set_property;
	object_class->get_property = gth_script_get_property;
	object_class->finalize = gth_script_finalize;

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
					 PROP_COMMAND,
					 g_param_spec_string ("command",
                                                              "Command",
                                                              "The command to execute",
                                                              NULL,
                                                              G_PARAM_READWRITE));
	g_object_class_install_property (object_class,
					 PROP_VISIBLE,
					 g_param_spec_boolean ("visible",
							       "Visible",
							       "Whether this script should be visible in the script list",
							       FALSE,
							       G_PARAM_READWRITE));
	g_object_class_install_property (object_class,
					 PROP_SHELL_SCRIPT,
					 g_param_spec_boolean ("shell-script",
							       "Shell Script",
							       "Whether to execute the command inside a terminal (with sh)",
							       TRUE,
							       G_PARAM_READWRITE));
	g_object_class_install_property (object_class,
					 PROP_FOR_EACH_FILE,
					 g_param_spec_boolean ("for-each-file",
							       "Each File",
							       "Whether to execute the command on file at a time",
							       FALSE,
							       G_PARAM_READWRITE));
	g_object_class_install_property (object_class,
					 PROP_WAIT_COMMAND,
					 g_param_spec_boolean ("wait-command",
							       "Wait command",
							       "Whether to wait command to finish",
							       FALSE,
							       G_PARAM_READWRITE));
	g_object_class_install_property (object_class,
					 PROP_ACCELERATOR,
					 g_param_spec_string ("accelerator",
							      "Accelerator",
							      "The keyboard shortcut to activate the script",
							      "",
							      G_PARAM_READWRITE));
}


static void
gth_script_dom_domizable_interface_init (DomDomizableInterface *iface)
{
	iface->create_element = gth_script_real_create_element;
	iface->load_from_element = gth_script_real_load_from_element;
}


static void
gth_script_gth_duplicable_interface_init (GthDuplicableInterface *iface)
{
	iface->duplicate = gth_script_real_duplicate;
}


static void
gth_script_init (GthScript *self)
{
	self->priv = gth_script_get_instance_private (self);
	self->priv->id = NULL;
	self->priv->display_name = NULL;
	self->priv->command = NULL;
	self->priv->visible = FALSE;
	self->priv->shell_script = FALSE;
	self->priv->for_each_file = FALSE;
	self->priv->wait_command = FALSE;
	self->priv->accelerator = NULL;
	self->priv->detailed_action = NULL;
}


GthScript*
gth_script_new (void)
{
	GthScript *script;
	char      *id;

	id = _g_str_random (ID_LENGTH);
	script = (GthScript *) g_object_new (GTH_TYPE_SCRIPT, "id", id, NULL);
	g_free (id);

	return script;
}


const char *
gth_script_get_id (GthScript *script)
{
	return script->priv->id;
}


const char *
gth_script_get_display_name (GthScript *script)
{
	return script->priv->display_name;
}


const char *
gth_script_get_command (GthScript *script)
{
	return script->priv->command;
}


const char *
gth_script_get_detailed_action (GthScript *self)
{
	return self->priv->detailed_action;
}


gboolean
gth_script_is_visible (GthScript *script)
{
	return script->priv->visible;
}


gboolean
gth_script_is_shell_script (GthScript  *script)
{
	return script->priv->shell_script;
}


gboolean
gth_script_for_each_file (GthScript *script)
{
	return script->priv->for_each_file;
}


gboolean
gth_script_wait_command (GthScript *script)
{
	return script->priv->wait_command;
}


char *
gth_script_get_requested_attributes (GthScript *script)
{
	GRegex   *re;
	char    **a;
	int       i, n, j;
	char    **b;
	char     *attributes;

	re = g_regex_new ("%attr\\{([^}]+)\\}", 0, 0, NULL);
	a = g_regex_split (re, script->priv->command, 0);
	for (i = 0, n = 0; a[i] != NULL; i++)
		if ((i > 0) && (i % 2 == 0))
			n++;
	if (n == 0)
		return NULL;

	b = g_new (char *, n + 1);
	for (i = 1, j = 0; a[i] != NULL; i += 2, j++) {
		b[j] = g_strstrip (a[i]);
	}
	b[j] = NULL;
	attributes = g_strjoinv (",", b);

	g_free (b);
	g_strfreev (a);
	g_regex_unref (re);

	return attributes;
}


/* -- gth_script_get_command_line -- */


typedef struct {
	GtkWindow  *parent;
	GthScript  *script;
	GList      *file_list;
	GError    **error;
	gboolean    quote_values;
	GList      *asked_values;
	GList      *last_asked_value;;
} ReplaceData;


typedef char * (*GetFileDataValueFunc) (GthFileData *file_data);


typedef struct {
	int        n_param;
	char      *prompt;
	char      *default_value;
	char      *value;
	GtkWidget *entry;
} AskedValue;


static AskedValue *
asked_value_new (int n_param)
{
	AskedValue *asked_value;

	asked_value = g_new (AskedValue, 1);
	asked_value->n_param = n_param;
	asked_value->prompt = g_strdup (_("Enter a value:"));;
	asked_value->default_value = NULL;
	asked_value->value = NULL;
	asked_value->entry = NULL;

	return asked_value;
}


static void
asked_value_free (AskedValue *asked_value)
{
	g_free (asked_value->prompt);
	g_free (asked_value->default_value);
	g_free (asked_value->value);
	g_free (asked_value);
}


static char *
create_file_list (GList                *file_list,
		  GetFileDataValueFunc  func,
		  gboolean              quote_value)
{
	GString *s;
	GList   *scan;

	s = g_string_new ("");
	for (scan = file_list; scan; scan = scan->next) {
		GthFileData *file_data = scan->data;
		char        *value;
		char        *quoted;

		value = func (file_data);
		if (quote_value)
			quoted = g_shell_quote (value);
		else
			quoted = g_strdup (value);

		g_string_append (s, quoted);
		if (scan->next != NULL)
			g_string_append (s, " ");

		g_free (quoted);
		g_free (value);
	}

	return g_string_free (s, FALSE);
}


static char *
get_uri_func (GthFileData *file_data)
{
	return g_file_get_uri (file_data->file);
}


static char *
get_filename_func (GthFileData *file_data)
{
	return g_file_get_path (file_data->file);
}


static char *
get_basename_func (GthFileData *file_data)
{
	return g_file_get_basename (file_data->file);
}


static char *
get_basename_wo_ext_func (GthFileData *file_data)
{
	char *basename;
	char *basename_wo_ext;

	basename = g_file_get_basename (file_data->file);
	basename_wo_ext = _g_path_remove_extension (basename);

	g_free (basename);

	return basename_wo_ext;
}


static char *
get_ext_func (GthFileData *file_data)
{
	char *path;
	char *ext;

	path = g_file_get_path (file_data->file);
	ext = g_strdup (_g_path_get_extension (path));

	g_free (path);

	return ext;
}


static char *
get_parent_func (GthFileData *file_data)
{
	GFile *parent;
	char  *path;

	parent = g_file_get_parent (file_data->file);
	path = g_file_get_path (parent);

	g_object_unref (parent);

	return path;
}


static void
thumb_loader_ready_cb (GObject      *source_object,
		       GAsyncResult *result,
		       gpointer      user_data)
{
	GtkBuilder      *builder = user_data;
	cairo_surface_t *image;

	if (! gth_thumb_loader_load_finish (GTH_THUMB_LOADER (source_object),
		  	 	 	    result,
		  	 	 	    &image,
		  	 	 	    NULL))
	{
		return;
	}

	if (image != NULL) {
		GdkPixbuf *pixbuf;

		pixbuf = _gdk_pixbuf_new_from_cairo_surface (image);
		gtk_image_set_from_pixbuf (GTK_IMAGE (_gtk_builder_get_widget (builder, "request_image")), pixbuf);

		g_object_unref (pixbuf);
		cairo_surface_destroy (image);
	}

	g_object_unref (builder);
}


static char *
create_attribute_list (GList    *file_list,
		       char     *match,
		       gboolean  quote_value)
{
	GRegex    *re;
	char     **a;
	char      *attribute = NULL;
	gboolean   first_value = TRUE;
	GString   *s;
	GList     *scan;

	re = g_regex_new ("%attr{([^}]+)}", 0, 0, NULL);
	a = g_regex_split (re, match, 0);
	if (g_strv_length (a) >= 2)
		attribute = g_strstrip (a[1]);

	if (attribute == NULL) {
		g_strfreev (a);
		g_regex_unref (re);
		return NULL;
	}

	s = g_string_new ("");
	for (scan = file_list; scan; scan = scan->next) {
		GthFileData *file_data = scan->data;
		char        *value;
		char        *quoted;

		value = gth_file_data_get_attribute_as_string (file_data, attribute);
		if (value == NULL)
			continue;

		if (value != NULL) {
			char *tmp_value;

			tmp_value = _g_utf8_replace_pattern (value, "[\r\n]", " ");
			g_free (value);
			value = tmp_value;
		}
		if (quote_value)
			quoted = g_shell_quote (value);
		else
			quoted = g_strdup (value);

		if (! first_value)
			g_string_append (s, " ");
		g_string_append (s, quoted);

		first_value = FALSE;

		g_free (quoted);
		g_free (value);
	}

	g_strfreev (a);
	g_regex_unref (re);

	return g_string_free (s, FALSE);
}


static char *
get_timestamp (void)
{
	GDateTime *now;
	char      *str;

	now = g_date_time_new_now_local ();
	str = g_date_time_format (now, "%Y-%m-%d %H.%M.%S");

	g_date_time_unref (now);

	return str;
}


static gboolean
command_line_eval_cb (const GMatchInfo *info,
		      GString          *res,
		      gpointer          data)
{
	ReplaceData *replace_data = data;
	char        *r = NULL;
	char        *match;

	match = g_match_info_fetch (info, 0);
	if (strcmp (match, "%U") == 0)
		r = create_file_list (replace_data->file_list, get_uri_func, replace_data->quote_values);
	else if (strcmp (match, "%F") == 0)
		r = create_file_list (replace_data->file_list, get_filename_func, replace_data->quote_values);
	else if (strcmp (match, "%B") == 0)
		r = create_file_list (replace_data->file_list, get_basename_func, replace_data->quote_values);
	else if (strcmp (match, "%N") == 0)
		r = create_file_list (replace_data->file_list, get_basename_wo_ext_func, replace_data->quote_values);
	else if (strcmp (match, "%E") == 0)
		r = create_file_list (replace_data->file_list, get_ext_func, replace_data->quote_values);
	else if (strcmp (match, "%P") == 0)
		r = create_file_list (replace_data->file_list, get_parent_func, replace_data->quote_values);
	else if (strcmp (match, "%T") == 0)
		r = get_timestamp ();
	else if (strncmp (match, "%attr", 5) == 0) {
		r = create_attribute_list (replace_data->file_list, match, replace_data->quote_values);
		if (r == NULL)
			*replace_data->error = g_error_new_literal (GTH_TASK_ERROR, GTH_TASK_ERROR_FAILED, _("Malformed command"));
	}
	else if (strncmp (match, "%ask", 4) == 0) {
		if (replace_data->last_asked_value != NULL) {
			AskedValue *asked_value = replace_data->last_asked_value->data;
			r = g_strdup (asked_value->value);
			replace_data->last_asked_value = replace_data->last_asked_value->next;
		}
		if ((r != NULL) && replace_data->quote_values) {
			char *q;

			q = g_shell_quote (r);
			g_free (r);
			r = q;
		}
	}

	if (r != NULL)
		g_string_append (res, r);

	g_free (r);
	g_free (match);

	return FALSE;
}


static gboolean
ask_values (ReplaceData  *replace_data,
	    gboolean      can_skip,
	    GError      **error)
{
	GthFileData     *file_data;
	GtkBuilder      *builder;
	GtkWidget       *dialog;
	GthThumbLoader  *thumb_loader;
	int              result;

	if (replace_data->asked_values == NULL)
		return TRUE;

	file_data = (GthFileData *) replace_data->file_list->data;
	builder = gtk_builder_new_from_resource ("/org/gnome/gThumb/list_tools/data/ui/ask-values.ui");
	dialog = g_object_new (GTK_TYPE_DIALOG,
			       "title", "",
			       "transient-for", GTK_WINDOW (replace_data->parent),
			       "modal", TRUE,
			       "destroy-with-parent", FALSE,
			       "use-header-bar", _gtk_settings_get_dialogs_use_header (),
			       "resizable", TRUE,
			       NULL);
	gtk_container_add (GTK_CONTAINER (gtk_dialog_get_content_area (GTK_DIALOG (dialog))), _gtk_builder_get_widget (builder, "dialog_content"));
	gtk_dialog_add_buttons (GTK_DIALOG (dialog),
			        _GTK_LABEL_CANCEL, GTK_RESPONSE_CANCEL,
				_GTK_LABEL_EXECUTE, GTK_RESPONSE_OK,
				! can_skip ? NULL : (gth_script_for_each_file (replace_data->script) ? _("_Skip") : NULL), GTK_RESPONSE_NO,
				NULL);
	_gtk_dialog_add_class_to_response (GTK_DIALOG (dialog), GTK_RESPONSE_OK, GTK_STYLE_CLASS_SUGGESTED_ACTION);

	gtk_label_set_text (GTK_LABEL (_gtk_builder_get_widget (builder, "title_label")), gth_script_get_display_name (replace_data->script));
	gtk_label_set_text (GTK_LABEL (_gtk_builder_get_widget (builder, "filename_label")), g_file_info_get_display_name (file_data->info));

	{
		GtkWidget *prompts = _gtk_builder_get_widget (builder, "prompts");
		GList     *scan;

		for (scan = replace_data->asked_values; scan; scan = scan->next) {
			AskedValue *asked_value = scan->data;
			GtkWidget  *label;
			GtkWidget  *entry;
			GtkWidget  *box;

			label = gtk_label_new (asked_value->prompt);
			gtk_label_set_xalign (GTK_LABEL (label), 0.0);

			entry = gtk_entry_new ();
			if (asked_value->default_value != NULL)
				gtk_entry_set_text (GTK_ENTRY (entry), asked_value->default_value);
			gtk_widget_set_size_request (entry, 300, -1);

			box = gtk_box_new (GTK_ORIENTATION_VERTICAL, 6);
			gtk_box_pack_start (GTK_BOX (box), label, TRUE, FALSE, 0);
			gtk_box_pack_start (GTK_BOX (box), entry, TRUE, FALSE, 0);
			gtk_widget_show_all (box);
			gtk_box_pack_start (GTK_BOX (prompts), box, FALSE, FALSE, 0);

			asked_value->entry = entry;
		}
	}

	g_object_ref (builder);
	thumb_loader = gth_thumb_loader_new (128);
	gth_thumb_loader_load (thumb_loader,
			       file_data,
			       NULL,
			       thumb_loader_ready_cb,
			       builder);

	result = gtk_dialog_run (GTK_DIALOG (dialog));
	if (result == GTK_RESPONSE_OK) {
		for (GList *scan = replace_data->asked_values; scan; scan = scan->next) {
			AskedValue *asked_value = scan->data;

			g_free (asked_value->value);
			asked_value->value = g_utf8_normalize (gtk_entry_get_text (GTK_ENTRY (asked_value->entry)), -1, G_NORMALIZE_NFC);
		}
	}
	else if (error != NULL) {
		if (result == GTK_RESPONSE_NO)
			*error = g_error_new_literal (GTH_TASK_ERROR, GTH_TASK_ERROR_SKIP_TO_NEXT_FILE, "");
		else
			*error = g_error_new_literal (GTH_TASK_ERROR, GTH_TASK_ERROR_CANCELLED, "");
	}

	gtk_widget_destroy (dialog);
	g_object_unref (builder);

	return (result == GTK_RESPONSE_OK);
}


static const char *
_g_utf8_find_matching_bracket (const char *string,
		               const char *open_bracket,
			       const char *closed_bracket)
{
	const char *s;
	gsize       i;
	gsize       string_len = g_utf8_strlen (string, -1);
	int         n_open_brackets = 1;

	s = string;
	for (i = 0; i <= string_len - 1; i++) {
		if (strncmp (s, open_bracket, 1) == 0)
			n_open_brackets++;
		else if (strncmp (s, closed_bracket, 1) == 0)
			n_open_brackets--;
		if (n_open_brackets == 0)
			return s;
		s = g_utf8_next_char(s);
	}

	return NULL;
}


static char **
_split_command_for_quotation (char *string)
{
	const char  *delimiter = "%quote{";
	char       **str_array;
	const char  *s;
	const char  *remainder;
	GSList      *string_list = NULL, *slist;
	int          n;

	remainder = string;
	s = _g_utf8_find_str (remainder, delimiter);
	if (s != NULL) {
		gsize delimiter_size = strlen (delimiter);

		while (s != NULL) {
			gsize  size = s - remainder;
			char  *new_string;

			new_string = g_new (char, size + 1);
			strncpy (new_string, remainder, size);
			new_string[size] = 0;

			string_list = g_slist_prepend (string_list, new_string);

			/* search the matching bracket */
			remainder = s + delimiter_size;
			s = _g_utf8_find_matching_bracket (remainder, "{", "}");
			if (s != NULL) {
				size = s - remainder;
				new_string = g_new (char, size + 1);
				strncpy (new_string, remainder, size);
				new_string[size] = 0;
				string_list = g_slist_prepend (string_list, new_string);

				remainder = s + 1 /* strlen("}") */;
				s = _g_utf8_find_str (remainder, delimiter);
			}
		}
	}

	if ((remainder != NULL) && (*remainder != 0))
		string_list = g_slist_prepend (string_list, g_strdup (remainder));

	n = g_slist_length (string_list);
	str_array = g_new (char*, n + 1);
	str_array[n--] = NULL;
	for (slist = string_list; slist; slist = slist->next)
		str_array[n--] = slist->data;

	g_slist_free (string_list);

	return str_array;
}


char *
gth_script_get_command_line (GthScript  *script,
			     GtkWindow  *parent,
			     GList      *file_list /* GthFileData */,
			     gboolean    can_skip,
			     GError    **error)
{
	ReplaceData  *replace_data;
	GRegex       *re;
	char        **a;
	GString      *command_line;
	int           i;
	char         *result;

	replace_data = g_new0 (ReplaceData, 1);
	replace_data->parent = parent;
	replace_data->script = script;
	replace_data->file_list = file_list;
	replace_data->error = error;

	/* collect the values to ask to the user */

	replace_data->asked_values = NULL;
	re = g_regex_new ("(%ask)({[^}]+})?({[^}]+})?", 0, 0, NULL);
	if (re != NULL) {
		GRegex *param_re;

		param_re = g_regex_new ("{([^}]+)}", 0, 0, NULL);
		a = g_regex_split (re, script->priv->command, 0);
		for (i = 0; a[i] != NULL; i++) {

			if (g_strcmp0 (a[i], "%ask") == 0) {
				AskedValue *asked_value;
				GMatchInfo *match_info = NULL;
				int         n_param = 0;

				asked_value = asked_value_new (n_param);
				i++;
				while ((n_param < 2) && (a[i] != NULL) && g_regex_match (param_re, a[i], 0, &match_info)) {
					char *value;

					value = g_match_info_fetch (match_info, 1);
					n_param++;

					if (n_param == 1) {
						g_free (asked_value->prompt);
						asked_value->prompt = _g_utf8_strip (value);
					}
					else if (n_param == 2)
						asked_value->default_value = _g_utf8_strip (value);
					else
						g_assert_not_reached ();

					g_free (value);
					g_match_info_free (match_info);
					match_info = NULL;
					i++;
				}
				replace_data->asked_values = g_list_prepend (replace_data->asked_values, asked_value);

				g_match_info_free (match_info);
			}
		}

		g_strfreev (a);
		g_regex_unref (param_re);
		g_regex_unref (re);
	}

	replace_data->asked_values = g_list_reverse (replace_data->asked_values);
	if (! ask_values (replace_data, can_skip, error))
		return NULL;

	/* replace the parameters in the command line */

	re = g_regex_new ("%U|%F|%B|%N|%E|%P|%T|%ask({[^}]+}({[^}]+})?)?|%attr{[^}]+}", 0, 0, NULL);

	replace_data->quote_values = FALSE;
	replace_data->last_asked_value = replace_data->asked_values;
	command_line = g_string_new ("");
	a = _split_command_for_quotation (script->priv->command);
	for (i = 0; a[i] != NULL; i++) {
		if ((i % 2) == 1) { /* the quote content */
			char *sub_result;
			char *quoted;

			sub_result = g_regex_replace_eval (re, a[i], -1, 0, 0, command_line_eval_cb, replace_data, error);
			quoted = g_shell_quote (g_strstrip (sub_result));
			g_string_append (command_line, quoted);

			g_free (quoted);
			g_free (sub_result);
		}
		else
			g_string_append (command_line, a[i]);
	}

	replace_data->quote_values = TRUE;
	replace_data->last_asked_value = replace_data->asked_values;
	result = g_regex_replace_eval (re, command_line->str, -1, 0, 0, command_line_eval_cb, replace_data, error);

	g_strfreev (a);
	g_string_free (command_line, TRUE);
	g_regex_unref (re);
	g_list_free_full (replace_data->asked_values, (GDestroyNotify) asked_value_free);
	g_free (replace_data);

	return result;
}


const char *
gth_script_get_accelerator (GthScript *self)
{
	g_return_val_if_fail (GTH_IS_SCRIPT (self), NULL);
	return self->priv->accelerator;
}


GthShortcut *
gth_script_create_shortcut (GthScript *self)
{
	GthShortcut *shortcut;

	shortcut = gth_shortcut_new ("exec-script", g_variant_new_string (gth_script_get_id (self)));
	shortcut->description = g_strdup (self->priv->display_name);
	shortcut->context = GTH_SHORTCUT_CONTEXT_BROWSER_VIEWER;
	shortcut->category = GTH_SHORTCUT_CATEGORY_LIST_TOOLS;
	gth_shortcut_set_accelerator (shortcut, self->priv->accelerator);
	shortcut->default_accelerator = g_strdup ("");

	return shortcut;
}
