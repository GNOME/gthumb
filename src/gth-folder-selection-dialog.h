/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

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

#ifndef GTH_FOLDER_SELECTION_DIALOG_H
#define GTH_FOLDER_SELECTION_DIALOG_H


#include <gtk/gtk.h>


#define GTH_TYPE_FOLDER_SELECTION         (gth_folder_selection_get_type ())
#define GTH_FOLDER_SELECTION(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), GTH_TYPE_FOLDER_SELECTION, GthFolderSelection))
#define GTH_FOLDER_SELECTION_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST ((k), GTH_TYPE_FOLDER_SELECTION, GthFolderSelectionClass))
#define GTH_IS_FOLDER_SELECTION(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), GTH_TYPE_FOLDER_SELECTION))
#define GTH_IS_FOLDER_SELECTION_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), GTH_TYPE_FOLDER_SELECTION))
#define GTH_FOLDER_SELECTION_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS((o), GTH_TYPE_FOLDER_SELECTION, GthFolderSelectionClass))


typedef struct _GthFolderSelection         GthFolderSelection;
typedef struct _GthFolderSelectionPrivate  GthFolderSelectionPrivate;
typedef struct _GthFolderSelectionClass    GthFolderSelectionClass;


struct _GthFolderSelection
{
	GtkDialog __parent;
	GthFolderSelectionPrivate *priv;
};


struct _GthFolderSelectionClass
{
	GtkDialogClass __parent_class;
};


GType         gth_folder_selection_get_type             (void) G_GNUC_CONST;
GtkWidget *   gth_folder_selection_new                  (const char         *title);
void          gth_folder_selection_set_folder           (GthFolderSelection *fsel,
							 const char         *folder);
const char *  gth_folder_selection_get_folder           (GthFolderSelection *fsel);
gboolean      gth_folder_selection_get_goto_destination (GthFolderSelection *fsel);


#endif /* GTHUMB_FOLDER_SELECTION_DIALOG_H */
