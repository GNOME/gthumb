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

#include "gth-toggle-menu-tool-button.h"

struct _GthToggleMenuToolButtonPrivate {
	GtkWidget *toggle_button;
	GtkMenu   *menu;
};

enum {
	SHOW_MENU,
	LAST_SIGNAL
};

enum {
	PROP_0,
	PROP_MENU
};

static gpointer parent_class = NULL;
static int signals[LAST_SIGNAL];


static void
gth_toggle_menu_tool_button_state_changed (GtkWidget    *widget,
					   GtkStateType  previous_state)
{
	GthToggleMenuToolButton *button = GTH_TOGGLE_MENU_TOOL_BUTTON (widget);

	if (! GTK_WIDGET_IS_SENSITIVE (widget) && (button->priv->menu != NULL))
		gtk_menu_shell_deactivate (GTK_MENU_SHELL (button->priv->menu));
}


static void
gth_toggle_menu_tool_button_set_property (GObject      *object,
					  guint         prop_id,
					  const GValue *value,
					  GParamSpec   *pspec)
{
	GthToggleMenuToolButton *button = GTH_TOGGLE_MENU_TOOL_BUTTON (object);

	switch (prop_id) {
	case PROP_MENU:
		gth_toggle_menu_tool_button_set_menu (button, g_value_get_object (value));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
		break;
	}
}


static void
gth_toggle_menu_tool_button_get_property (GObject    *object,
					  guint       prop_id,
					  GValue     *value,
					  GParamSpec *pspec)
{
	GthToggleMenuToolButton *button = GTH_TOGGLE_MENU_TOOL_BUTTON (object);

	switch (prop_id) {
	case PROP_MENU:
		g_value_set_object (value, button->priv->menu);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
		break;
	}
}


/* Callback for the "deactivate" signal on the pop-up menu.
 * This is used so that we unset the state of the toggle button
 * when the pop-up menu disappears.
 */
static int
menu_deactivate_cb (GtkMenuShell            *menu_shell,
		    GthToggleMenuToolButton *button)
{
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button->priv->toggle_button), FALSE);
	return TRUE;
}


static void
menu_position_func (GtkMenu                 *menu,
                    int                     *x,
                    int                     *y,
                    gboolean                *push_in,
                    GthToggleMenuToolButton *button)
{
	GtkWidget        *widget = GTK_WIDGET (button);
	GtkRequisition    req;
	GtkRequisition    menu_req;
	GtkOrientation    orientation;
	GtkTextDirection  direction;
	GdkRectangle      monitor;
	int               monitor_num;
	GdkScreen        *screen;

	gtk_widget_size_request (GTK_WIDGET (button->priv->menu), &menu_req);

	orientation = gtk_tool_item_get_orientation (GTK_TOOL_ITEM (button));
	direction = gtk_widget_get_direction (widget);

	screen = gtk_widget_get_screen (GTK_WIDGET (menu));
	monitor_num = gdk_screen_get_monitor_at_window (screen, widget->window);
	if (monitor_num < 0)
		monitor_num = 0;
	gdk_screen_get_monitor_geometry (screen, monitor_num, &monitor);

	if (orientation == GTK_ORIENTATION_HORIZONTAL) {
		gdk_window_get_origin (widget->window, x, y);
		*x += widget->allocation.x;
		*y += widget->allocation.y;

		if (direction == GTK_TEXT_DIR_LTR)
			*x += MAX (widget->allocation.width - menu_req.width, 0);
		else if (menu_req.width > widget->allocation.width)
			*x -= menu_req.width - widget->allocation.width;

		if ((*y + widget->allocation.height + menu_req.height) <= monitor.y + monitor.height)
			*y += widget->allocation.height;
		else if ((*y - menu_req.height) >= monitor.y)
			*y -= menu_req.height;
		else if (monitor.y + monitor.height - (*y + widget->allocation.height) > *y)
			*y += widget->allocation.height;
		else
			*y -= menu_req.height;
	}
	else {
		gdk_window_get_origin (GTK_BUTTON (widget)->event_window, x, y);
		gtk_widget_size_request (widget, &req);

		if (direction == GTK_TEXT_DIR_LTR)
			*x += widget->allocation.width;
		else
			*x -= menu_req.width;

		if ((*y + menu_req.height > monitor.y + monitor.height) &&
		    (*y + widget->allocation.height - monitor.y > monitor.y + monitor.height - *y))
		{
			*y += widget->allocation.height - menu_req.height;
		}
	}

	*push_in = FALSE;
}


static void
popup_menu_under_button (GthToggleMenuToolButton *button,
                         GdkEventButton          *event)
{
	g_signal_emit (button, signals[SHOW_MENU], 0);

	if (button->priv->menu == NULL)
		return;

	gtk_menu_popup (button->priv->menu, NULL, NULL,
			(GtkMenuPositionFunc) menu_position_func,
			button,
			event ? event->button : 0,
			event ? event->time : gtk_get_current_event_time ());
}


static gboolean
real_button_toggled_cb (GtkToggleButton         *togglebutton,
                        GthToggleMenuToolButton *button)
{

	if (button->priv->menu == NULL)
		return FALSE;

	if (gtk_toggle_button_get_active (togglebutton) && ! GTK_WIDGET_VISIBLE (button->priv->menu)) {
		/* we get here only when the menu is activated by a key
		 * press, so that we can select the first menu item */
		popup_menu_under_button (button, NULL);
		gtk_menu_shell_select_first (GTK_MENU_SHELL (button->priv->menu), FALSE);
	}

	return FALSE;
}


static gboolean
real_button_button_press_event_cb (GtkWidget               *widget,
                                   GdkEventButton          *event,
                                   GthToggleMenuToolButton *button)
{
	if ((event->button == 1) && (button->priv->menu != NULL))  {
		popup_menu_under_button (button, event);
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (widget), TRUE);

		return TRUE;
	}
	else
		return FALSE;
}


static void
gth_toggle_menu_tool_button_destroy (GtkObject *object)
{
	GthToggleMenuToolButton *button;

	button = GTH_TOGGLE_MENU_TOOL_BUTTON (object);

	if (button->priv->menu != NULL) {
		g_signal_handlers_disconnect_by_func (button->priv->menu,
						      menu_deactivate_cb,
						      button);
		gtk_menu_detach (button->priv->menu);

		g_signal_handlers_disconnect_by_func (button->priv->toggle_button,
						      real_button_toggled_cb,
						      button);
		g_signal_handlers_disconnect_by_func (button->priv->toggle_button,
						      real_button_button_press_event_cb,
						      button);
	}

	GTK_OBJECT_CLASS (parent_class)->destroy (object);
}


static void
gth_toggle_menu_tool_button_class_init (GthToggleMenuToolButtonClass *klass)
{
	GObjectClass     *object_class;
	GtkObjectClass   *gtk_object_class;
	GtkWidgetClass   *widget_class;
	/*GtkToolItemClass *toolitem_class;*/

	parent_class = g_type_class_peek_parent (klass);
	g_type_class_add_private (klass, sizeof (GthToggleMenuToolButtonPrivate));

	object_class = (GObjectClass *)klass;
	object_class->set_property = gth_toggle_menu_tool_button_set_property;
	object_class->get_property = gth_toggle_menu_tool_button_get_property;

	gtk_object_class = (GtkObjectClass *)klass;
	gtk_object_class->destroy = gth_toggle_menu_tool_button_destroy;

	widget_class = (GtkWidgetClass *)klass;
	widget_class->state_changed = gth_toggle_menu_tool_button_state_changed;

	/*toolitem_class = (GtkToolItemClass *)klass;
	toolitem_class->toolbar_reconfigured = gth_toggle_menu_tool_button_toolbar_reconfigured;*/

	/**
	 * GthToggleMenuToolButton::show-menu:
	 * @button: the object on which the signal is emitted
	 *
	 * The ::show-menu signal is emitted before the menu is shown.
	 *
	 * It can be used to populate the menu on demand, using
	 * gth_toggle_menu_tool_button_get_menu().

	 * Note that even if you populate the menu dynamically in this way,
	 * you must set an empty menu on the #GthToggleMenuToolButton beforehand,
	 * since the arrow is made insensitive if the menu is not set.
	 */
	signals[SHOW_MENU] =
		g_signal_new ("show-menu",
			      G_OBJECT_CLASS_TYPE (klass),
			      G_SIGNAL_RUN_FIRST,
			      G_STRUCT_OFFSET (GthToggleMenuToolButtonClass, show_menu),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__VOID,
			      G_TYPE_NONE, 0);

	g_object_class_install_property (object_class,
                                        PROP_MENU,
                                        g_param_spec_object ("menu",
                                                             "Menu",
                                                             "The dropdown menu",
                                                             GTK_TYPE_MENU,
                                                             G_PARAM_READABLE | G_PARAM_WRITABLE));
}


static void
gth_toggle_menu_tool_button_init (GthToggleMenuToolButton *button)
{
	button->priv = G_TYPE_INSTANCE_GET_PRIVATE (button, GTH_TYPE_TOGGLE_MENU_TOOL_BUTTON, GthToggleMenuToolButtonPrivate);
	button->priv->menu = NULL;

	button->priv->toggle_button = gtk_bin_get_child (GTK_BIN (button));
	g_signal_connect (button->priv->toggle_button,
			  "toggled",
			  G_CALLBACK (real_button_toggled_cb),
			  button);
	g_signal_connect (button->priv->toggle_button,
			  "button-press-event",
		          G_CALLBACK (real_button_button_press_event_cb),
		          button);
}


GType
gth_toggle_menu_tool_button_get_type (void)
{
        static GType type = 0;

        if (! type) {
                GTypeInfo type_info = {
			sizeof (GthToggleMenuToolButtonClass),
			NULL,
			NULL,
			(GClassInitFunc) gth_toggle_menu_tool_button_class_init,
			NULL,
			NULL,
			sizeof (GthToggleMenuToolButton),
			0,
			(GInstanceInitFunc) gth_toggle_menu_tool_button_init
		};
		type = g_type_register_static (GTK_TYPE_TOGGLE_TOOL_BUTTON,
					       "GthToggleMenuToolButton",
					       &type_info,
					       0);
	}

        return type;
}


GtkToolItem *
gth_toggle_menu_tool_button_new (void)
{
	return (GtkToolItem *) g_object_new (GTH_TYPE_TOGGLE_MENU_TOOL_BUTTON, NULL);
}


GtkToolItem *
gth_toggle_menu_tool_button_new_from_stock (const gchar *stock_id)
{
	g_return_val_if_fail (stock_id != NULL, NULL);

	return (GtkToolItem *) g_object_new (GTH_TYPE_TOGGLE_MENU_TOOL_BUTTON,
					     "stock-id", stock_id,
					     NULL);
}


static void
menu_detacher (GtkWidget *widget,
               GtkMenu   *menu)
{
	GthToggleMenuToolButton *button = GTH_TOGGLE_MENU_TOOL_BUTTON (widget);

	g_return_if_fail (button->priv->menu == menu);

	button->priv->menu = NULL;
}


void
gth_toggle_menu_tool_button_set_menu (GthToggleMenuToolButton *button,
				      GtkWidget               *menu)
{
	g_return_if_fail (GTH_IS_TOGGLE_MENU_TOOL_BUTTON (button));
	g_return_if_fail (GTK_IS_MENU (menu) || menu == NULL);

	if (button->priv->menu != GTK_MENU (menu)) {
		if ((button->priv->menu != NULL) && GTK_WIDGET_VISIBLE (button->priv->menu))
			gtk_menu_shell_deactivate (GTK_MENU_SHELL (button->priv->menu));

		if (button->priv->menu != NULL) {
			g_signal_handlers_disconnect_by_func (button->priv->menu,
							      menu_deactivate_cb,
							      button);
			gtk_menu_detach (button->priv->menu);
		}

		button->priv->menu = GTK_MENU (menu);

		if (button->priv->menu != NULL) {
			gtk_menu_attach_to_widget (button->priv->menu,
						   GTK_WIDGET (button),
						   menu_detacher);

			gtk_widget_set_sensitive (button->priv->toggle_button, TRUE);

			g_signal_connect (button->priv->menu,
					  "deactivate",
					  G_CALLBACK (menu_deactivate_cb),
					  button);
		}
		else
			gtk_widget_set_sensitive (button->priv->toggle_button, FALSE);
	}

	g_object_notify (G_OBJECT (button), "menu");
}


GtkWidget *
gth_toggle_menu_tool_button_get_menu (GthToggleMenuToolButton *button)
{
	g_return_val_if_fail (GTH_IS_TOGGLE_MENU_TOOL_BUTTON (button), NULL);

	return GTK_WIDGET (button->priv->menu);
}
