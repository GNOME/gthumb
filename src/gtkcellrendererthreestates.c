/*
 *  GThumb
 *
 *  Copyright (C) 2001 The Free Software Foundation, Inc.
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

/* gtkcellrenderertoggle.c
 * Copyright (C) 2000  Red Hat, Inc.,  Jonathan Blandford <jrb@redhat.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include <stdlib.h>
#include <gtk/gtksignal.h>
#include "gtkcellrendererthreestates.h"
#include "gthumb-marshal.h"

static void gtk_cell_renderer_three_states_get_property (GObject                    *object,
							 guint                       param_id,
							 GValue                     *value,
							 GParamSpec                 *pspec);
static void gtk_cell_renderer_three_states_set_property (GObject                    *object,
							 guint                       param_id,
							 const GValue               *value,
							 GParamSpec                 *pspec);
static void gtk_cell_renderer_three_states_init         (GtkCellRendererThreeStates      *celltext);
static void gtk_cell_renderer_three_states_class_init   (GtkCellRendererThreeStatesClass *class);
static void gtk_cell_renderer_three_states_get_size     (GtkCellRenderer            *cell,
							 GtkWidget                  *widget,
							 GdkRectangle               *cell_area,
							 gint                       *x_offset,
							 gint                       *y_offset,
							 gint                       *width,
							 gint                       *height);
static void gtk_cell_renderer_three_states_render       (GtkCellRenderer            *cell,
							 GdkWindow                  *window,
							 GtkWidget                  *widget,
							 GdkRectangle               *background_area,
							 GdkRectangle               *cell_area,
							 GdkRectangle               *expose_area,
							 guint                       flags);
static gboolean gtk_cell_renderer_three_states_activate (GtkCellRenderer            *cell,
							 GdkEvent                   *event,
							 GtkWidget                  *widget,
							 const gchar                *path,
							 GdkRectangle               *background_area,
							 GdkRectangle               *cell_area,
							 guint                       flags);


enum {
  TOGGLED,
  LAST_SIGNAL
};

enum {
  PROP_ZERO,
  PROP_ACTIVATABLE,
  PROP_STATE,
  PROP_RADIO,
  PROP_HAS_THIRD_STATE
};


#define THREE_STATES_WIDTH 12

static guint three_states_cell_signals[LAST_SIGNAL] = { 0 };


GtkType
gtk_cell_renderer_three_states_get_type (void)
{
  static GtkType type = 0;

  if (!type)
    {
      static const GTypeInfo info =
      {
	sizeof (GtkCellRendererThreeStatesClass),
	NULL,		/* base_init */
	NULL,		/* base_finalize */
	(GClassInitFunc) gtk_cell_renderer_three_states_class_init,
	NULL,		/* class_finalize */
	NULL,		/* class_data */
	sizeof (GtkCellRendererThreeStates),
	0,              /* n_preallocs */
	(GInstanceInitFunc) gtk_cell_renderer_three_states_init,
      };

      type = g_type_register_static (GTK_TYPE_CELL_RENDERER, "GtkCellRendererThreeStates", &info, 0);
    }

  return type;
}

static void
gtk_cell_renderer_three_states_init (GtkCellRendererThreeStates *cell_3states)
{
  GtkCellRenderer *renderer = GTK_CELL_RENDERER (cell_3states);

  cell_3states->activatable = TRUE;
  cell_3states->state = 0;
  cell_3states->radio = FALSE;
  cell_3states->has_third_state = FALSE;
  renderer->mode = GTK_CELL_RENDERER_MODE_ACTIVATABLE;
  renderer->xpad = 2;
  renderer->ypad = 2;
}

static void
gtk_cell_renderer_three_states_class_init (GtkCellRendererThreeStatesClass *class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);
  GtkCellRendererClass *cell_class = GTK_CELL_RENDERER_CLASS (class);

  object_class->get_property = gtk_cell_renderer_three_states_get_property;
  object_class->set_property = gtk_cell_renderer_three_states_set_property;

  cell_class->get_size = gtk_cell_renderer_three_states_get_size;
  cell_class->render = gtk_cell_renderer_three_states_render;
  cell_class->activate = gtk_cell_renderer_three_states_activate;
  
  g_object_class_install_property (object_class,
				   PROP_STATE,
				   g_param_spec_uint ("state",
						      "Button state",
						      "The state of the button",
						      0, 2, 0,
						      G_PARAM_READABLE |
						      G_PARAM_WRITABLE));
  
  g_object_class_install_property (object_class,
				   PROP_ACTIVATABLE,
				   g_param_spec_boolean ("activatable",
							 "Activatable",
							 "The three states button can be activated",
							 TRUE,
							 G_PARAM_READABLE |
							 G_PARAM_WRITABLE));
  
  g_object_class_install_property (object_class,
				   PROP_RADIO,
				   g_param_spec_boolean ("radio",
							 "Radio state",
							 "Draw the three states button as a radio button",
							 FALSE,
							 G_PARAM_READABLE |
							 G_PARAM_WRITABLE));

  g_object_class_install_property (object_class,
				   PROP_HAS_THIRD_STATE,
				   g_param_spec_boolean ("has_third_state",
							 "Third state",
							 "The button has the third state",
							 FALSE,
							 G_PARAM_READABLE |
							 G_PARAM_WRITABLE));


  three_states_cell_signals[TOGGLED] =
    g_signal_new ("toggled",
		  G_TYPE_FROM_CLASS (class),
		  G_SIGNAL_RUN_LAST,
		  G_STRUCT_OFFSET (GtkCellRendererThreeStatesClass, toggled),
		  NULL, NULL,
		  gthumb_marshal_VOID__STRING,
		  G_TYPE_NONE, 1,
		  G_TYPE_STRING);
}

static void
gtk_cell_renderer_three_states_get_property (GObject     *object,
					     guint        param_id,
					     GValue      *value,
					     GParamSpec  *pspec)
{
  GtkCellRendererThreeStates *cell_3states = GTK_CELL_RENDERER_THREE_STATES (object);
  
  switch (param_id)
    {
    case PROP_STATE:
      g_value_set_uint (value, cell_3states->state);
      break;
    case PROP_ACTIVATABLE:
      g_value_set_boolean (value, cell_3states->activatable);
      break;
    case PROP_RADIO:
      g_value_set_boolean (value, cell_3states->radio);
      break;
    case PROP_HAS_THIRD_STATE:
      g_value_set_boolean (value, cell_3states->has_third_state);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
      break;
    }
}


static void
gtk_cell_renderer_three_states_set_property (GObject      *object,
					     guint         param_id,
					     const GValue *value,
					     GParamSpec   *pspec)
{
  GtkCellRendererThreeStates *cell_3states = GTK_CELL_RENDERER_THREE_STATES (object);
  
  switch (param_id)
    {
    case PROP_STATE:
      cell_3states->state = g_value_get_uint (value);
      g_object_notify (G_OBJECT(object), "state");
      break;
    case PROP_ACTIVATABLE:
      cell_3states->activatable = g_value_get_boolean (value);
      g_object_notify (G_OBJECT(object), "activatable");
      break;
    case PROP_RADIO:
      cell_3states->radio = g_value_get_boolean (value);
      g_object_notify (G_OBJECT(object), "radio");
      break;
    case PROP_HAS_THIRD_STATE:
      cell_3states->has_third_state = g_value_get_boolean (value);
      g_object_notify (G_OBJECT(object), "has_third_state");
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
      break;
    }
}

/**
 * gtk_cell_renderer_toggle_new:
 * 
 * Creates a new #GtkCellRendererToggle. Adjust rendering
 * parameters using object properties. Object properties can be set
 * globally (with g_object_set()). Also, with #GtkTreeViewColumn, you
 * can bind a property to a value in a #GtkTreeModel. For example, you
 * can bind the "active" property on the cell renderer to a boolean value
 * in the model, thus causing the check button to reflect the state of
 * the model.
 * 
 * Return value: the new cell renderer
 **/
GtkCellRenderer *
gtk_cell_renderer_three_states_new (void)
{
  return GTK_CELL_RENDERER (g_object_new (GTK_TYPE_CELL_RENDERER_THREE_STATES, NULL));
}

static void
gtk_cell_renderer_three_states_get_size (GtkCellRenderer *cell,
					 GtkWidget       *widget,
					 GdkRectangle    *cell_area,
					 gint            *x_offset,
					 gint            *y_offset,
					 gint            *width,
					 gint            *height)
{
  gint calc_width;
  gint calc_height;

  calc_width = (gint) cell->xpad * 2 + THREE_STATES_WIDTH;
  calc_height = (gint) cell->ypad * 2 + THREE_STATES_WIDTH;

  if (width)
    *width = calc_width;

  if (height)
    *height = calc_height;

  if (cell_area)
    {
      if (x_offset)
	{
	  *x_offset = cell->xalign * (cell_area->width - calc_width);
	  *x_offset = MAX (*x_offset, 0);
	}
      if (y_offset)
	{
	  *y_offset = cell->yalign * (cell_area->height - calc_height);
	  *y_offset = MAX (*y_offset, 0);
	}
    }
}

static void
gtk_cell_renderer_three_states_render (GtkCellRenderer *cell,
				       GdkWindow       *window,
				       GtkWidget       *widget,
				       GdkRectangle    *background_area,
				       GdkRectangle    *cell_area,
				       GdkRectangle    *expose_area,
				       guint            flags)
{
  GtkCellRendererThreeStates *cell_3states = (GtkCellRendererThreeStates *) cell;
  gint width, height;
  gint x_offset, y_offset;
  GtkShadowType shadow;
  GtkStateType state;
  
  gtk_cell_renderer_three_states_get_size (cell, widget, cell_area,
					   &x_offset, &y_offset,
					   &width, &height);
  width -= cell->xpad*2;
  height -= cell->ypad*2;

  if (width <= 0 || height <= 0)
    return;

  if (cell_3states->state == 1)
    shadow = GTK_SHADOW_IN;
  else  if (cell_3states->state == 0)
    shadow = GTK_SHADOW_OUT;
  else  if (cell_3states->state == 2)
    shadow = GTK_SHADOW_ETCHED_IN;

  if ((flags & GTK_CELL_RENDERER_SELECTED) == GTK_CELL_RENDERER_SELECTED)
    {
      if (GTK_WIDGET_HAS_FOCUS (widget))
	state = GTK_STATE_SELECTED;
      else
	state = GTK_STATE_ACTIVE;
    }
  else
    {
      if (cell_3states->activatable)
        state = GTK_STATE_NORMAL;
      else
        state = GTK_STATE_INSENSITIVE;
    }

  if (cell_3states->radio)
    {
      gtk_paint_option (widget->style,
                        window,
                        state, shadow,
                        cell_area, widget, "cellradio",
                        cell_area->x + x_offset + cell->xpad,
                        cell_area->y + y_offset + cell->ypad,
                        width - 1, height - 1);
    }
  else
    gtk_paint_check (widget->style,
		     window,
		     state, shadow,
		     cell_area, widget, "cellcheck",
		     cell_area->x + x_offset + cell->xpad,
		     cell_area->y + y_offset + cell->ypad,
		     width - 1, height - 1);
}

static gint
gtk_cell_renderer_three_states_activate (GtkCellRenderer *cell,
					 GdkEvent        *event,
					 GtkWidget       *widget,
					 const gchar     *path,
					 GdkRectangle    *background_area,
					 GdkRectangle    *cell_area,
					 guint            flags)
{
  GtkCellRendererThreeStates *cell_3states;
  
  cell_3states = GTK_CELL_RENDERER_THREE_STATES (cell);
  if (cell_3states->activatable)
    {
      g_signal_emit (G_OBJECT (cell), 
		     three_states_cell_signals[TOGGLED], 
		     0,
		     path);
      return TRUE;
    }

  return FALSE;
}

/**
 * gtk_cell_renderer_toggle_set_radio:
 * @toggle: a #GtkCellRendererToggle
 * @radio: %TRUE to make the toggle look like a radio button
 * 
 * If @radio is %TRUE, the cell renderer renders a radio toggle
 * (i.e. a toggle in a group of mutually-exclusive toggles).
 * If %FALSE, it renders a check toggle (a standalone boolean option).
 * This can be set globally for the cell renderer, or changed just
 * before rendering each cell in the model (for #GtkTreeView, you set
 * up a per-row setting using #GtkTreeViewColumn to associate model
 * columns with cell renderer properties).
 **/
void
gtk_cell_renderer_three_states_set_radio (GtkCellRendererThreeStates *three_states,
					  gboolean                    radio)
{
  g_return_if_fail (GTK_IS_CELL_RENDERER_THREE_STATES (three_states));

  three_states->radio = radio;
}

/**
 * gtk_cell_renderer_toggle_get_radio:
 * @toggle: a #GtkCellRendererToggle
 *
 * Returns wether we're rendering radio toggles rather than checkboxes. 
 * 
 * Return value: %TRUE if we're rendering radio toggles rather than checkboxes
 **/
gboolean
gtk_cell_renderer_three_states_get_radio (GtkCellRendererThreeStates *three_states)
{
  g_return_val_if_fail (GTK_IS_CELL_RENDERER_THREE_STATES (three_states), FALSE);

  return three_states->radio;
}

/**
 * gtk_cell_renderer_toggle_get_active:
 * @toggle: a #GtkCellRendererToggle
 *
 * Returns whether the cell renderer is active. See
 * gtk_cell_renderer_toggle_set_active().
 *
 * Return value: %TRUE if the cell renderer is active.
 **/
guint
gtk_cell_renderer_three_states_get_state (GtkCellRendererThreeStates *three_states)
{
  g_return_val_if_fail (GTK_IS_CELL_RENDERER_THREE_STATES (three_states), FALSE);

  return three_states->state;
}

/**
 * gtk_cell_renderer_toggle_set_active:
 * @toggle: a #GtkCellRendererToggle.
 * @setting: the value to set.
 *
 * Activates or deactivates a cell renderer.
 **/
void
gtk_cell_renderer_three_states_set_state (GtkCellRendererThreeStates *three_states,
					  guint                       state) 
{
  g_return_if_fail (GTK_IS_CELL_RENDERER_THREE_STATES (three_states));

  g_object_set (G_OBJECT (three_states), "state", state, NULL);
}

guint
gtk_cell_renderer_three_states_get_next_state  (GtkCellRendererThreeStates *three_states)
{
  g_return_val_if_fail (GTK_IS_CELL_RENDERER_THREE_STATES (three_states), FALSE);

  if (three_states->has_third_state) 
    return (three_states->state + 1) % 3;
  else
    return (three_states->state + 1) % 2;
}

gboolean
gtk_cell_renderer_three_states_has_third_state (GtkCellRendererThreeStates *three_states)
{
  g_return_val_if_fail (GTK_IS_CELL_RENDERER_THREE_STATES (three_states), FALSE);

  return three_states->has_third_state;
}

void
gtk_cell_renderer_three_states_set_has_third_state (GtkCellRendererThreeStates *three_states,
						    gboolean                    setting)
{
  g_return_if_fail (GTK_IS_CELL_RENDERER_THREE_STATES (three_states));

  three_states->has_third_state = setting;
}
