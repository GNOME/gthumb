/* GTK - The GIMP Toolkit
 * Copyright (C) 1995-1997 Peter Mattis, Spencer Kimball and Josh MacDonald
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

/*
 * Modified by the GTK+ Team and others 1997-2000.  See the AUTHORS
 * file for a list of people on the GTK+ Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * GTK+ at ftp://ftp.gtk.org/pub/gtk/. 
 */

/*
 * Modified by Paolo Bacchilega.
 */


#include "gtkorientationbox.h"
#include <gtk/gtkhbox.h>
#include <gtk/gtkvbox.h>


static void gtk_orientation_box_class_init    (GtkOrientationBoxClass   *klass);
static void gtk_orientation_box_init          (GtkOrientationBox        *box);
static void gtk_orientation_box_finalize      (GObject                  *box);
static void gtk_orientation_box_size_request  (GtkWidget      *widget,
					       GtkRequisition *requisition);
static void gtk_orientation_box_size_allocate (GtkWidget      *widget,
					       GtkAllocation  *allocation);

static GtkBoxClass *parent_class = NULL;

struct _GtkOrientationBoxPrivate {
  GtkWidget      *hbox;
  GtkWidget      *vbox;
  GtkOrientation  orientation;
};


GType
gtk_orientation_box_get_type (void)
{
  static GType orientation_box_type = 0;

  if (!orientation_box_type)
    {
      static const GTypeInfo orientation_box_info =
      {
	sizeof (GtkOrientationBoxClass),
	NULL,		/* base_init */
	NULL,		/* base_finalize */
	(GClassInitFunc) gtk_orientation_box_class_init,
	NULL,		/* class_finalize */
	NULL,		/* class_data */
	sizeof (GtkOrientationBox),
	0,		/* n_preallocs */
	(GInstanceInitFunc) gtk_orientation_box_init,
      };

      orientation_box_type = g_type_register_static (GTK_TYPE_BOX, 
						     "GtkOrientationBox",
						     &orientation_box_info, 0);
    }
  
  return orientation_box_type;
}

static void
gtk_orientation_box_class_init (GtkOrientationBoxClass *class)
{
  GObjectClass   *gobject_class;
  GtkWidgetClass *widget_class;

  parent_class = g_type_class_peek_parent (class);

  gobject_class = (GObjectClass*) class;
  gobject_class->finalize = gtk_orientation_box_finalize;

  widget_class = (GtkWidgetClass*) class;
  widget_class->size_request = gtk_orientation_box_size_request;
  widget_class->size_allocate = gtk_orientation_box_size_allocate;
}

static void
gtk_orientation_box_init (GtkOrientationBox *orientation_box)
{
  GtkOrientationBoxPrivate *priv;

  priv = g_new0 (GtkOrientationBoxPrivate, 1);
  orientation_box->priv = priv;
  priv->orientation = GTK_ORIENTATION_HORIZONTAL;
}

static void
gtk_orientation_box_finalize (GObject *object)
{
  GtkOrientationBox *orientation_box;

  orientation_box = GTK_ORIENTATION_BOX (object);

  if (orientation_box->priv != NULL) {
    g_free (orientation_box->priv);
    gtk_widget_destroy (orientation_box->priv->hbox);
    gtk_widget_destroy (orientation_box->priv->vbox);
    orientation_box->priv = NULL;
  }

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

GtkWidget*
gtk_orientation_box_new (gboolean homogeneous,
			 gint spacing,
			 GtkOrientation orientation)
{
  GtkOrientationBox *orientation_box;

  orientation_box = g_object_new (GTK_TYPE_ORIENTATION_BOX, NULL);

  GTK_BOX (orientation_box)->spacing = spacing;
  GTK_BOX (orientation_box)->homogeneous = homogeneous ? TRUE : FALSE;

  orientation_box->priv->orientation = orientation;
  orientation_box->priv->hbox = gtk_hbox_new (homogeneous, spacing);
  orientation_box->priv->vbox = gtk_vbox_new (homogeneous, spacing);

  return GTK_WIDGET (orientation_box);
}


static void
gtk_hbox_size_request (GtkWidget      *widget,
		       GtkRequisition *requisition)
{
  GtkBox *box;
  GtkBoxChild *child;
  GList *children;
  gint nvis_children;
  gint width;

  box = GTK_BOX (widget);
  requisition->width = 0;
  requisition->height = 0;
  nvis_children = 0;

  children = box->children;
  while (children)
    {
      child = children->data;
      children = children->next;

      if (GTK_WIDGET_VISIBLE (child->widget))
	{
	  GtkRequisition child_requisition;

	  gtk_widget_size_request (child->widget, &child_requisition);

	  if (box->homogeneous)
	    {
	      width = child_requisition.width + child->padding * 2;
	      requisition->width = MAX (requisition->width, width);
	    }
	  else
	    {
	      requisition->width += child_requisition.width + child->padding * 2;
	    }

	  requisition->height = MAX (requisition->height, child_requisition.height);

	  nvis_children += 1;
	}
    }

  if (nvis_children > 0)
    {
      if (box->homogeneous)
	requisition->width *= nvis_children;
      requisition->width += (nvis_children - 1) * box->spacing;
    }

  requisition->width += GTK_CONTAINER (box)->border_width * 2;
  requisition->height += GTK_CONTAINER (box)->border_width * 2;
}

static void
gtk_hbox_size_allocate (GtkWidget     *widget,
			GtkAllocation *allocation)
{
  GtkBox *box;
  GtkBoxChild *child;
  GList *children;
  GtkAllocation child_allocation;
  gint nvis_children;
  gint nexpand_children;
  gint child_width;
  gint width;
  gint extra;
  gint x;
  GtkTextDirection direction;

  box = GTK_BOX (widget);
  widget->allocation = *allocation;

  direction = gtk_widget_get_direction (widget);
  
  nvis_children = 0;
  nexpand_children = 0;
  children = box->children;

  while (children)
    {
      child = children->data;
      children = children->next;

      if (GTK_WIDGET_VISIBLE (child->widget))
	{
	  nvis_children += 1;
	  if (child->expand)
	    nexpand_children += 1;
	}
    }

  if (nvis_children > 0)
    {
      if (box->homogeneous)
	{
	  width = (allocation->width -
		   GTK_CONTAINER (box)->border_width * 2 -
		   (nvis_children - 1) * box->spacing);
	  extra = width / nvis_children;
	}
      else if (nexpand_children > 0)
	{
	  width = (gint) allocation->width - (gint) widget->requisition.width;
	  extra = width / nexpand_children;
	}
      else
	{
	  width = 0;
	  extra = 0;
	}

      x = allocation->x + GTK_CONTAINER (box)->border_width;
      child_allocation.y = allocation->y + GTK_CONTAINER (box)->border_width;
      child_allocation.height = MAX (1, (gint) allocation->height - (gint) GTK_CONTAINER (box)->border_width * 2);

      children = box->children;
      while (children)
	{
	  child = children->data;
	  children = children->next;

	  if ((child->pack == GTK_PACK_START) && GTK_WIDGET_VISIBLE (child->widget))
	    {
	      if (box->homogeneous)
		{
		  if (nvis_children == 1)
		    child_width = width;
		  else
		    child_width = extra;

		  nvis_children -= 1;
		  width -= extra;
		}
	      else
		{
		  GtkRequisition child_requisition;

		  gtk_widget_get_child_requisition (child->widget, &child_requisition);

		  child_width = child_requisition.width + child->padding * 2;

		  if (child->expand)
		    {
		      if (nexpand_children == 1)
			child_width += width;
		      else
			child_width += extra;

		      nexpand_children -= 1;
		      width -= extra;
		    }
		}

	      if (child->fill)
		{
		  child_allocation.width = MAX (1, (gint) child_width - (gint) child->padding * 2);
		  child_allocation.x = x + child->padding;
		}
	      else
		{
		  GtkRequisition child_requisition;

		  gtk_widget_get_child_requisition (child->widget, &child_requisition);
		  child_allocation.width = child_requisition.width;
		  child_allocation.x = x + (child_width - child_allocation.width) / 2;
		}

	      if (direction == GTK_TEXT_DIR_RTL)
		child_allocation.x = allocation->x + allocation->width - (child_allocation.x - allocation->x) - child_allocation.width;

	      gtk_widget_size_allocate (child->widget, &child_allocation);

	      x += child_width + box->spacing;
	    }
	}

      x = allocation->x + allocation->width - GTK_CONTAINER (box)->border_width;

      children = box->children;
      while (children)
	{
	  child = children->data;
	  children = children->next;

	  if ((child->pack == GTK_PACK_END) && GTK_WIDGET_VISIBLE (child->widget))
	    {
	      GtkRequisition child_requisition;
	      gtk_widget_get_child_requisition (child->widget, &child_requisition);

              if (box->homogeneous)
                {
                  if (nvis_children == 1)
                    child_width = width;
                  else
                    child_width = extra;

                  nvis_children -= 1;
                  width -= extra;
                }
              else
                {
		  child_width = child_requisition.width + child->padding * 2;

                  if (child->expand)
                    {
                      if (nexpand_children == 1)
                        child_width += width;
                      else
                        child_width += extra;

                      nexpand_children -= 1;
                      width -= extra;
                    }
                }

              if (child->fill)
                {
                  child_allocation.width = MAX (1, (gint)child_width - (gint)child->padding * 2);
                  child_allocation.x = x + child->padding - child_width;
                }
              else
                {
		  child_allocation.width = child_requisition.width;
                  child_allocation.x = x + (child_width - child_allocation.width) / 2 - child_width;
                }

	      if (direction == GTK_TEXT_DIR_RTL)
		child_allocation.x = allocation->x + allocation->width - (child_allocation.x - allocation->x) - child_allocation.width;

              gtk_widget_size_allocate (child->widget, &child_allocation);

              x -= (child_width + box->spacing);
	    }
	}
    }
}

static void
gtk_vbox_size_request (GtkWidget      *widget,
		       GtkRequisition *requisition)
{
  GtkBox *box;
  GtkBoxChild *child;
  GtkRequisition child_requisition;
  GList *children;
  gint nvis_children;
  gint height;

  box = GTK_BOX (widget);
  requisition->width = 0;
  requisition->height = 0;
  nvis_children = 0;

  children = box->children;
  while (children)
    {
      child = children->data;
      children = children->next;

      if (GTK_WIDGET_VISIBLE (child->widget))
	{
	  gtk_widget_size_request (child->widget, &child_requisition);

	  if (box->homogeneous)
	    {
	      height = child_requisition.height + child->padding * 2;
	      requisition->height = MAX (requisition->height, height);
	    }
	  else
	    {
	      requisition->height += child_requisition.height + child->padding * 2;
	    }

	  requisition->width = MAX (requisition->width, child_requisition.width);

	  nvis_children += 1;
	}
    }

  if (nvis_children > 0)
    {
      if (box->homogeneous)
	requisition->height *= nvis_children;
      requisition->height += (nvis_children - 1) * box->spacing;
    }

  requisition->width += GTK_CONTAINER (box)->border_width * 2;
  requisition->height += GTK_CONTAINER (box)->border_width * 2;
}

static void
gtk_vbox_size_allocate (GtkWidget     *widget,
			GtkAllocation *allocation)
{
  GtkBox *box;
  GtkBoxChild *child;
  GList *children;
  GtkAllocation child_allocation;
  gint nvis_children;
  gint nexpand_children;
  gint child_height;
  gint height;
  gint extra;
  gint y;

  box = GTK_BOX (widget);
  widget->allocation = *allocation;

  nvis_children = 0;
  nexpand_children = 0;
  children = box->children;

  while (children)
    {
      child = children->data;
      children = children->next;

      if (GTK_WIDGET_VISIBLE (child->widget))
	{
	  nvis_children += 1;
	  if (child->expand)
	    nexpand_children += 1;
	}
    }

  if (nvis_children > 0)
    {
      if (box->homogeneous)
	{
	  height = (allocation->height -
		   GTK_CONTAINER (box)->border_width * 2 -
		   (nvis_children - 1) * box->spacing);
	  extra = height / nvis_children;
	}
      else if (nexpand_children > 0)
	{
	  height = (gint) allocation->height - (gint) widget->requisition.height;
	  extra = height / nexpand_children;
	}
      else
	{
	  height = 0;
	  extra = 0;
	}

      y = allocation->y + GTK_CONTAINER (box)->border_width;
      child_allocation.x = allocation->x + GTK_CONTAINER (box)->border_width;
      child_allocation.width = MAX (1, (gint) allocation->width - (gint) GTK_CONTAINER (box)->border_width * 2);

      children = box->children;
      while (children)
	{
	  child = children->data;
	  children = children->next;

	  if ((child->pack == GTK_PACK_START) && GTK_WIDGET_VISIBLE (child->widget))
	    {
	      if (box->homogeneous)
		{
		  if (nvis_children == 1)
		    child_height = height;
		  else
		    child_height = extra;

		  nvis_children -= 1;
		  height -= extra;
		}
	      else
		{
		  GtkRequisition child_requisition;

		  gtk_widget_get_child_requisition (child->widget, &child_requisition);
		  child_height = child_requisition.height + child->padding * 2;

		  if (child->expand)
		    {
		      if (nexpand_children == 1)
			child_height += height;
		      else
			child_height += extra;

		      nexpand_children -= 1;
		      height -= extra;
		    }
		}

	      if (child->fill)
		{
		  child_allocation.height = MAX (1, child_height - (gint)child->padding * 2);
		  child_allocation.y = y + child->padding;
		}
	      else
		{
		  GtkRequisition child_requisition;

		  gtk_widget_get_child_requisition (child->widget, &child_requisition);
		  child_allocation.height = child_requisition.height;
		  child_allocation.y = y + (child_height - child_allocation.height) / 2;
		}

	      gtk_widget_size_allocate (child->widget, &child_allocation);

	      y += child_height + box->spacing;
	    }
	}

      y = allocation->y + allocation->height - GTK_CONTAINER (box)->border_width;

      children = box->children;
      while (children)
	{
	  child = children->data;
	  children = children->next;

	  if ((child->pack == GTK_PACK_END) && GTK_WIDGET_VISIBLE (child->widget))
	    {
	      GtkRequisition child_requisition;
	      gtk_widget_get_child_requisition (child->widget, &child_requisition);

              if (box->homogeneous)
                {
                  if (nvis_children == 1)
                    child_height = height;
                  else
                    child_height = extra;

                  nvis_children -= 1;
                  height -= extra;
                }
              else
                {
		  child_height = child_requisition.height + child->padding * 2;

                  if (child->expand)
                    {
                      if (nexpand_children == 1)
                        child_height += height;
                      else
                        child_height += extra;

                      nexpand_children -= 1;
                      height -= extra;
                    }
                }

              if (child->fill)
                {
                  child_allocation.height = MAX (1, child_height - (gint)child->padding * 2);
                  child_allocation.y = y + child->padding - child_height;
                }
              else
                {
		  child_allocation.height = child_requisition.height;
                  child_allocation.y = y + (child_height - child_allocation.height) / 2 - child_height;
                }

              gtk_widget_size_allocate (child->widget, &child_allocation);

              y -= (child_height + box->spacing);
	    }
	}
    }
}

static void
gtk_orientation_box_size_request (GtkWidget      *widget,
				  GtkRequisition *requisition)
{
  GtkOrientationBox *orientation_box;

  orientation_box = GTK_ORIENTATION_BOX (widget);

  if (orientation_box->priv->orientation == GTK_ORIENTATION_HORIZONTAL)
    gtk_hbox_size_request (widget, requisition);
  else
    gtk_vbox_size_request (widget, requisition);
}

static void
gtk_orientation_box_size_allocate (GtkWidget     *widget,
				   GtkAllocation *allocation)
{
  GtkOrientationBox *orientation_box;

  orientation_box = GTK_ORIENTATION_BOX (widget);

  if (orientation_box->priv->orientation == GTK_ORIENTATION_HORIZONTAL)
    gtk_hbox_size_allocate (widget, allocation);
  else
    gtk_vbox_size_allocate (widget, allocation);
}

GtkOrientation
gtk_orientation_box_get_orient (GtkOrientationBox *box)
{
  g_return_val_if_fail (GTK_IS_ORIENTATION_BOX (box), -1);
  return box->priv->orientation;
}

void
gtk_orientation_box_set_orient (GtkOrientationBox *box,
				GtkOrientation     orientation)
{
  g_return_if_fail (GTK_IS_ORIENTATION_BOX (box));
  box->priv->orientation = orientation;
  gtk_widget_queue_resize (GTK_WIDGET (box));
}
