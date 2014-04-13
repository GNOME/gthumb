/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2005-2008 Free Software Foundation, Inc.
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
#include "gth-window.h"
#include "gth-window-title.h"
#include "gtk-utils.h"
#include "main.h"


G_DEFINE_TYPE (GthWindow, gth_window, GTK_TYPE_APPLICATION_WINDOW)


enum  {
	PROP_0,
	PROP_N_PAGES,
	PROP_USE_HEADER_BAR
};


struct _GthWindowPrivate {
	int              n_pages;
	gboolean         use_header_bar;
	int              current_page;
	GtkWidget       *grid;
	GtkWidget       *stack;
	GtkWidget       *headerbar;
	GtkWidget       *title;
	GtkWidget       *menubar;
	GtkWidget       *toolbar;
	GtkWidget       *infobar;
	GtkWidget       *statusbar;
	GtkWidget      **toolbars;
	GtkWidget      **contents;
	GtkWidget      **pages;
	GthWindowSize   *window_size;
	GtkWindowGroup  *window_group;
	GtkAccelGroup   *accel_group;
};


static void
gth_window_set_n_pages (GthWindow *self,
			int        n_pages)
{
	int i;

	if (self->priv->n_pages != 0) {
		g_critical ("The number of pages of a GthWindow can be set only once.");
		return;
	}

	self->priv->n_pages = n_pages;

	self->priv->grid = gtk_grid_new ();
	gtk_widget_show (self->priv->grid);
	gtk_container_add (GTK_CONTAINER (self), self->priv->grid);

	self->priv->stack = gtk_stack_new ();
	gtk_stack_set_transition_type (GTK_STACK (self->priv->stack), GTK_STACK_TRANSITION_TYPE_CROSSFADE);
	gtk_widget_show (self->priv->stack);
	gtk_grid_attach (GTK_GRID (self->priv->grid),
			 self->priv->stack,
			 0, 2,
			 1, 1);

	self->priv->toolbars = g_new0 (GtkWidget *, n_pages);
	self->priv->contents = g_new0 (GtkWidget *, n_pages);
	self->priv->pages = g_new0 (GtkWidget *, n_pages);

	for (i = 0; i < n_pages; i++) {
		GtkWidget *page;

		self->priv->pages[i] = page = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
		gtk_widget_show (page);
		gtk_container_add (GTK_CONTAINER (self->priv->stack), page);

		self->priv->toolbars[i] = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
		gtk_widget_show (self->priv->toolbars[i]);
		gtk_box_pack_start (GTK_BOX (page), self->priv->toolbars[i], FALSE, FALSE, 0);

		self->priv->contents[i] = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
		gtk_widget_hide (self->priv->contents[i]);
		gtk_box_pack_start (GTK_BOX (page), self->priv->contents[i], TRUE, TRUE, 0);
	}

	self->priv->window_size = g_new0 (GthWindowSize, n_pages);
	for (i = 0; i < n_pages; i++)
		self->priv->window_size[i].saved = FALSE;
}


static void
_gth_window_add_header_bar (GthWindow *self)
{
	self->priv->headerbar = gtk_header_bar_new ();
	gtk_widget_show (self->priv->headerbar);
	gtk_header_bar_set_show_close_button (GTK_HEADER_BAR (self->priv->headerbar), TRUE);
	self->priv->title = gth_window_title_new ();
	gtk_widget_show (self->priv->title);
	gtk_header_bar_set_custom_title (GTK_HEADER_BAR (self->priv->headerbar), self->priv->title);
	gtk_window_set_titlebar (GTK_WINDOW (self), self->priv->headerbar);
}


static void
gth_window_set_property (GObject      *object,
			 guint         property_id,
			 const GValue *value,
			 GParamSpec   *pspec)
{
	GthWindow *self;

	self = GTH_WINDOW (object);

	switch (property_id) {
	case PROP_N_PAGES:
		gth_window_set_n_pages (self, g_value_get_int (value));
		break;
	case PROP_USE_HEADER_BAR:
		self->priv->use_header_bar = g_value_get_boolean (value);
		if (self->priv->use_header_bar)
			_gth_window_add_header_bar (self);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
		break;
	}
}


static void
gth_window_get_property (GObject    *object,
		         guint       property_id,
		         GValue     *value,
		         GParamSpec *pspec)
{
	GthWindow *self;

        self = GTH_WINDOW (object);

	switch (property_id) {
	case PROP_N_PAGES:
		g_value_set_int (value, self->priv->n_pages);
		break;
	case PROP_USE_HEADER_BAR:
		g_value_set_boolean (value, self->priv->use_header_bar);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
		break;
	}
}


static void
gth_window_finalize (GObject *object)
{
	GthWindow *window = GTH_WINDOW (object);

	g_free (window->priv->toolbars);
	g_free (window->priv->contents);
	g_free (window->priv->pages);
	g_free (window->priv->window_size);
	g_object_unref (window->priv->window_group);
	g_object_unref (window->priv->accel_group);

	G_OBJECT_CLASS (gth_window_parent_class)->finalize (object);
}


static gboolean
gth_window_delete_event (GtkWidget   *widget,
			 GdkEventAny *event)
{
	gth_window_close ((GthWindow*) widget);
	return TRUE;
}


static void
gth_window_real_close (GthWindow *window)
{
	/* virtual */
}


static void
gth_window_real_set_current_page (GthWindow *window,
				  int        page)
{
	int i;

	if (window->priv->current_page == page)
		return;

	window->priv->current_page = page;
	gtk_stack_set_visible_child (GTK_STACK (window->priv->stack), window->priv->pages[page]);

	for (i = 0; i < window->priv->n_pages; i++)
		if (i == page)
			gtk_widget_show (window->priv->contents[i]);
		else
			gtk_widget_hide (window->priv->contents[i]);
}


static void
gth_window_realize (GtkWidget *widget)
{
	GdkScreen      *screen;
	GBytes         *bytes;
	gconstpointer   css_data;
	gsize           css_data_size;
	GtkCssProvider *css_provider;
	GError         *error = NULL;

	GTK_WIDGET_CLASS (gth_window_parent_class)->realize (widget);

	screen = gtk_widget_get_screen (widget);
	gtk_icon_theme_append_search_path (gtk_icon_theme_get_for_screen (screen), GTHUMB_ICON_DIR);

	bytes = g_resources_lookup_data ("/org/gnome/gThumb/resources/gthumb.css", 0, &error);
	if (bytes == NULL) {
		g_warning ("%s", error->message);
		g_error_free (error);
		return;
	}

	css_data = g_bytes_get_data (bytes, &css_data_size);
	css_provider = gtk_css_provider_new ();
	if (! gtk_css_provider_load_from_data (css_provider, css_data, css_data_size, &error)) {
		g_warning ("%s", error->message);
		g_error_free (error);
	}
	gtk_style_context_add_provider_for_screen (screen,
						   GTK_STYLE_PROVIDER (css_provider),
						   GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);

	g_object_unref (css_provider);
}


static void
gth_window_class_init (GthWindowClass *klass)
{
	GObjectClass   *gobject_class;
	GtkWidgetClass *widget_class;

	g_type_class_add_private (klass, sizeof (GthWindowPrivate));

	gobject_class = (GObjectClass*) klass;
	gobject_class->set_property = gth_window_set_property;
	gobject_class->get_property = gth_window_get_property;
	gobject_class->finalize = gth_window_finalize;

	widget_class = (GtkWidgetClass*) klass;
	widget_class->delete_event = gth_window_delete_event;
	widget_class->realize = gth_window_realize;

	klass->close = gth_window_real_close;
	klass->set_current_page = gth_window_real_set_current_page;

	g_object_class_install_property (G_OBJECT_CLASS (klass),
					 PROP_N_PAGES,
					 g_param_spec_int ("n-pages",
							   "n-pages",
							   "n-pages",
							   0,
							   G_MAXINT,
							   1,
							   G_PARAM_READWRITE));
	g_object_class_install_property (G_OBJECT_CLASS (klass),
					 PROP_USE_HEADER_BAR,
					 g_param_spec_boolean ("use-header-bar",
							       "use-header-bar",
							       "use-header-bar",
							       FALSE,
							       G_PARAM_READWRITE));
}


static void
gth_window_init (GthWindow *window)
{
	window->priv = G_TYPE_INSTANCE_GET_PRIVATE (window, GTH_TYPE_WINDOW, GthWindowPrivate);
	window->priv->grid = NULL;
	window->priv->contents = NULL;
	window->priv->pages = NULL;
	window->priv->n_pages = 0;
	window->priv->current_page = GTH_WINDOW_PAGE_UNDEFINED;
	window->priv->menubar = NULL;
	window->priv->toolbar = NULL;
	window->priv->infobar = NULL;
	window->priv->statusbar = NULL;
	window->priv->headerbar = NULL;
	window->priv->use_header_bar = FALSE;

	window->priv->window_group = gtk_window_group_new ();
	gtk_window_group_add_window (window->priv->window_group, GTK_WINDOW (window));

	window->priv->accel_group = gtk_accel_group_new ();
	gtk_window_add_accel_group (GTK_WINDOW (window), window->priv->accel_group);

	gtk_window_set_application (GTK_WINDOW (window), Main_Application);
}


void
gth_window_close (GthWindow *window)
{
	GTH_WINDOW_GET_CLASS (window)->close (window);
}


void
gth_window_attach (GthWindow     *window,
		   GtkWidget     *child,
		   GthWindowArea  area)
{
	int position;

	g_return_if_fail (window != NULL);
	g_return_if_fail (GTH_IS_WINDOW (window));
	g_return_if_fail (child != NULL);
	g_return_if_fail (GTK_IS_WIDGET (child));

	switch (area) {
	case GTH_WINDOW_MENUBAR:
		window->priv->menubar = child;
		position = 0;
		break;
	case GTH_WINDOW_TOOLBAR:
		window->priv->toolbar = child;
		position = 1;
		break;
	case GTH_WINDOW_INFOBAR:
		window->priv->infobar = child;
		position = 3;
		break;
	case GTH_WINDOW_STATUSBAR:
		window->priv->statusbar = child;
		position = 4;
		break;
	default:
		return;
	}

	gtk_widget_set_vexpand (child, FALSE);
	gtk_grid_attach (GTK_GRID (window->priv->grid),
			  child,
			  0, position,
			  1, 1);
}


void
gth_window_attach_toolbar (GthWindow *window,
			   int        page,
			   GtkWidget *child)
{
	g_return_if_fail (window != NULL);
	g_return_if_fail (GTH_IS_WINDOW (window));
	g_return_if_fail (page >= 0 && page < window->priv->n_pages);
	g_return_if_fail (child != NULL);
	g_return_if_fail (GTK_IS_WIDGET (child));

	_gtk_container_remove_children (GTK_CONTAINER (window->priv->toolbars[page]), NULL, NULL);
	gtk_style_context_add_class (gtk_widget_get_style_context (child), GTK_STYLE_CLASS_PRIMARY_TOOLBAR);
	gtk_widget_set_hexpand (child, TRUE);
	gtk_container_add (GTK_CONTAINER (window->priv->toolbars[page]), child);
}


void
gth_window_attach_content (GthWindow *window,
			   int        page,
			   GtkWidget *child)
{
	g_return_if_fail (window != NULL);
	g_return_if_fail (GTH_IS_WINDOW (window));
	g_return_if_fail (page >= 0 && page < window->priv->n_pages);
	g_return_if_fail (child != NULL);
	g_return_if_fail (GTK_IS_WIDGET (child));

	_gtk_container_remove_children (GTK_CONTAINER (window->priv->contents[page]), NULL, NULL);
	gtk_widget_set_hexpand (child, TRUE);
	gtk_widget_set_vexpand (child, TRUE);
	gtk_container_add (GTK_CONTAINER (window->priv->contents[page]), child);
}


void
gth_window_set_current_page (GthWindow *window,
			     int        page)
{
	GTH_WINDOW_GET_CLASS (window)->set_current_page (window, page);
}


int
gth_window_get_current_page (GthWindow *window)
{
	return window->priv->current_page;
}


static void
hide_widget (GtkWidget *widget)
{
	if (widget != NULL)
		gtk_widget_hide (widget);
}


static void
show_widget (GtkWidget *widget)
{
	if (widget != NULL)
		gtk_widget_show (widget);
}


void
gth_window_show_only_content (GthWindow *window,
			      gboolean   only_content)
{
	int i;

	if (only_content) {
		for (i = 0; i < window->priv->n_pages; i++)
			hide_widget (window->priv->toolbars[i]);
		hide_widget (window->priv->menubar);
		hide_widget (window->priv->toolbar);
		hide_widget (window->priv->statusbar);
	}
	else {
		for (i = 0; i < window->priv->n_pages; i++)
			show_widget (window->priv->toolbars[i]);
		show_widget (window->priv->menubar);
		show_widget (window->priv->toolbar);
		show_widget (window->priv->statusbar);
	}
}


GtkWidget *
gth_window_get_area (GthWindow     *window,
		     GthWindowArea  area)
{
	switch (area) {
	case GTH_WINDOW_MENUBAR:
		return window->priv->menubar;
		break;
	case GTH_WINDOW_TOOLBAR:
		return window->priv->toolbar;
		break;
	case GTH_WINDOW_INFOBAR:
		return window->priv->infobar;
		break;
	case GTH_WINDOW_STATUSBAR:
		return window->priv->statusbar;
		break;
	default:
		break;
	}

	return NULL;
}


GtkWidget *
gth_window_get_header_bar (GthWindow *window)
{
	return window->priv->headerbar;
}


void
gth_window_save_page_size (GthWindow *window,
			   int        page,
			   int        width,
			   int        height)
{
	g_return_if_fail (window != NULL);
	g_return_if_fail (GTH_IS_WINDOW (window));
	g_return_if_fail (page >= 0 && page < window->priv->n_pages);

	window->priv->window_size[page].width = width;
	window->priv->window_size[page].height = height;
	window->priv->window_size[page].saved = TRUE;
}


void
gth_window_apply_saved_size (GthWindow *window,
			     int        page)
{
	g_return_if_fail (window != NULL);
	g_return_if_fail (GTH_IS_WINDOW (window));
	g_return_if_fail (page >= 0 && page < window->priv->n_pages);

	if (! window->priv->window_size[page].saved)
		return;

	gtk_window_resize (GTK_WINDOW (window),
			   window->priv->window_size[page].width,
			   window->priv->window_size[page].height);
}


void
gth_window_clear_saved_size (GthWindow *window,
			     int        page)
{
	g_return_if_fail (window != NULL);
	g_return_if_fail (GTH_IS_WINDOW (window));
	g_return_if_fail (page >= 0 && page < window->priv->n_pages);

	window->priv->window_size[page].saved = FALSE;
}


gboolean
gth_window_get_page_size (GthWindow *window,
			  int        page,
			  int       *width,
			  int       *height)
{
	g_return_val_if_fail (window != NULL, FALSE);
	g_return_val_if_fail (GTH_IS_WINDOW (window), FALSE);
	g_return_val_if_fail (page >= 0 && page < window->priv->n_pages, FALSE);

	if (! window->priv->window_size[page].saved)
		return FALSE;

	if (width != NULL)
		*width = window->priv->window_size[page].width;
	if (height != NULL)
		*height = window->priv->window_size[page].height;

	return TRUE;
}


void
gth_window_set_title (GthWindow  *window,
		      const char *title,
		      GList	 *emblems)
{
	if (window->priv->use_header_bar) {
		gth_window_title_set_title (GTH_WINDOW_TITLE (window->priv->title), title);
		gth_window_title_set_emblems (GTH_WINDOW_TITLE (window->priv->title), emblems);
	}
	else
		gtk_window_set_title (GTK_WINDOW (window), title);
}


GtkAccelGroup *
gth_window_get_accel_group (GthWindow *window)
{
	if (window->priv->accel_group == NULL) {
		window->priv->accel_group = gtk_accel_group_new ();
		gtk_window_add_accel_group (GTK_WINDOW (window), window->priv->accel_group);
	}

	return window->priv->accel_group;
}


void
gth_window_add_accelerators (GthWindow			*window,
			     const GthAccelerator	*accelerators,
			     int		 	 n_accelerators)
{
	GtkAccelGroup *accel_group;
	int            i;

	accel_group = gth_window_get_accel_group (window);
	for (i = 0; i < n_accelerators; i++) {
		const GthAccelerator *acc = accelerators + i;

		_gtk_window_add_accelerator_for_action (GTK_WINDOW (window),
							accel_group,
							acc->action_name,
							acc->accelerator,
							NULL);
	}
}


void
gth_window_enable_action (GthWindow  *window,
			  const char *action_name,
			  gboolean    enabled)
{
	GAction *action;

	action = g_action_map_lookup_action (G_ACTION_MAP (window), action_name);
	g_object_set (action, "enabled", enabled, NULL);
}


gboolean
gth_window_get_action_state (GthWindow  *window,
			     const char *action_name)
{
	GAction  *action;
	GVariant *state;
	gboolean  value;

	action = g_action_map_lookup_action (G_ACTION_MAP (window), action_name);
	g_return_val_if_fail (action != NULL, FALSE);
	state = g_action_get_state (action);
	value = g_variant_get_boolean (state);

	g_variant_unref (state);

	return value;
}


void
gth_window_change_action_state (GthWindow  *window,
			        const char *action_name,
			        gboolean    value)
{
	GAction  *action;
	GVariant *old_state;
	GVariant *new_state;

	action = g_action_map_lookup_action (G_ACTION_MAP (window), action_name);
	g_return_if_fail (action != NULL);

	old_state = g_action_get_state (action);
	new_state = g_variant_new_boolean (value);
	if ((old_state == NULL) || ! g_variant_equal (old_state, new_state))
		g_action_change_state (action, new_state);

	if (old_state != NULL)
		g_variant_unref (old_state);
}
