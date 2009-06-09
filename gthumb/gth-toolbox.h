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

#ifndef GTH_TOOLBOX_H
#define GTH_TOOLBOX_H

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define GTH_TYPE_TOOLBOX              (gth_toolbox_get_type ())
#define GTH_TOOLBOX(obj)              (G_TYPE_CHECK_INSTANCE_CAST ((obj), GTH_TYPE_TOOLBOX, GthToolbox))
#define GTH_TOOLBOX_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), GTH_TYPE_TOOLBOX, GthToolboxClass))
#define GTH_IS_TOOLBOX(obj)           (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GTH_TYPE_TOOLBOX))
#define GTH_IS_TOOLBOX_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), GTH_TYPE_TOOLBOX))
#define GTH_TOOLBOX_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS((obj), GTH_TYPE_TOOLBOX, GthToolboxClass))

#define GTH_TYPE_FILE_TOOL               (gth_file_tool_get_type ())
#define GTH_FILE_TOOL(obj)               (G_TYPE_CHECK_INSTANCE_CAST ((obj), GTH_TYPE_FILE_TOOL, GthFileTool))
#define GTH_IS_FILE_TOOL(obj)            (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GTH_TYPE_FILE_TOOL))
#define GTH_FILE_TOOL_GET_INTERFACE(obj) (G_TYPE_INSTANCE_GET_INTERFACE ((obj), GTH_TYPE_FILE_TOOL, GthFileToolIface))

typedef struct _GthToolbox        GthToolbox;
typedef struct _GthToolboxClass   GthToolboxClass;
typedef struct _GthToolboxPrivate GthToolboxPrivate;

struct _GthToolbox
{
	GtkVBox __parent;
	GthToolboxPrivate *priv;
};

struct _GthToolboxClass
{
	GtkVBoxClass __parent_class;
};

typedef struct _GthFileTool GthFileTool;
typedef struct _GthFileToolIface GthFileToolIface;

struct _GthFileToolIface {
	GTypeInterface parent_iface;
	void  (*update_sensitivity) (GthFileTool *self,
				     GtkWidget   *window);
};

GType          gth_toolbox_get_type              (void);
GtkWidget *    gth_toolbox_new                   (const char  *name);
void           gth_toolbox_update_sensitivity    (GthToolbox  *toolbox);
GType          gth_file_tool_get_type            (void);
void           gth_file_tool_update_sensitivity  (GthFileTool *self,
						  GtkWidget   *window);

G_END_DECLS

#endif /* GTH_TOOLBOX_H */
