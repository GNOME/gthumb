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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the GNU
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


#ifndef __GTK_ORIENTATION_BOX_H__
#define __GTK_ORIENTATION_BOX_H__


#include <gdk/gdk.h>
#include <gtk/gtkbox.h>


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


#define GTK_TYPE_ORIENTATION_BOX            (gtk_orientation_box_get_type ())
#define GTK_ORIENTATION_BOX(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GTK_TYPE_ORIENTATION_BOX, GtkOrientationBox))
#define GTK_ORIENTATION_BOX_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GTK_TYPE_ORIENTATION_BOX, GtkOrientationBoxClass))
#define GTK_IS_ORIENTATION_BOX(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GTK_TYPE_ORIENTATION_BOX))
#define GTK_IS_ORIENTATION_BOX_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GTK_TYPE_ORIENTATION_BOX))
#define GTK_ORIENTATION_BOX_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GTK_TYPE_ORIENTATION_BOX, GtkOrientationBoxClass))


typedef struct _GtkOrientationBox	 GtkOrientationBox;
typedef struct _GtkOrientationBoxPrivate GtkOrientationBoxPrivate;
typedef struct _GtkOrientationBoxClass   GtkOrientationBoxClass;

struct _GtkOrientationBox
{
  GtkBox box;
  GtkOrientationBoxPrivate *priv;
};

struct _GtkOrientationBoxClass
{
  GtkBoxClass parent_class;
};

GType	        gtk_orientation_box_get_type      (void) G_GNUC_CONST;
GtkWidget*      gtk_orientation_box_new           (gboolean           homogeneous,
						   int                spacing,
						   GtkOrientation     orientation);
GtkOrientation  gtk_orientation_box_get_orient    (GtkOrientationBox *box);
void            gtk_orientation_box_set_orient    (GtkOrientationBox *box,
						   GtkOrientation     orientation);

#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif /* __GTK_ORIENTATION_BOX_H__ */
