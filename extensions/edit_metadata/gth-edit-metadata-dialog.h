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
 
#ifndef GTH_EDIT_METADATA_DIALOG_H
#define GTH_EDIT_METADATA_DIALOG_H

#include <gtk/gtk.h>
#include <gthumb.h>

G_BEGIN_DECLS

#define GTH_TYPE_EDIT_METADATA_DIALOG            (gth_edit_metadata_dialog_get_type ())
#define GTH_EDIT_METADATA_DIALOG(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GTH_TYPE_EDIT_METADATA_DIALOG, GthEditMetadataDialog))
#define GTH_EDIT_METADATA_DIALOG_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GTH_TYPE_EDIT_METADATA_DIALOG, GthEditMetadataDialogClass))
#define GTH_IS_EDIT_METADATA_DIALOG(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GTH_TYPE_EDIT_METADATA_DIALOG))
#define GTH_IS_EDIT_METADATA_DIALOG_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GTH_TYPE_EDIT_METADATA_DIALOG))
#define GTH_EDIT_METADATA_DIALOG_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GTH_TYPE_EDIT_METADATA_DIALOG, GthEditMetadataDialogClass))

#define GTH_TYPE_EDIT_METADATA_PAGE               (gth_edit_metadata_page_get_type ())
#define GTH_EDIT_METADATA_PAGE(obj)               (G_TYPE_CHECK_INSTANCE_CAST ((obj), GTH_TYPE_EDIT_METADATA_PAGE, GthEditMetadataPage))
#define GTH_IS_EDIT_METADATA_PAGE(obj)            (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GTH_TYPE_EDIT_METADATA_PAGE))
#define GTH_EDIT_METADATA_PAGE_GET_INTERFACE(obj) (G_TYPE_INSTANCE_GET_INTERFACE ((obj), GTH_TYPE_EDIT_METADATA_PAGE, GthEditMetadataPageIface))

typedef struct _GthEditMetadataDialog GthEditMetadataDialog;
typedef struct _GthEditMetadataDialogClass GthEditMetadataDialogClass;
typedef struct _GthEditMetadataDialogPrivate GthEditMetadataDialogPrivate;

struct _GthEditMetadataDialog {
	GtkDialog parent_instance;
	GthEditMetadataDialogPrivate *priv;
};

struct _GthEditMetadataDialogClass {
	GtkDialogClass parent_class;
};

typedef struct _GthEditMetadataPage GthEditMetadataPage;
typedef struct _GthEditMetadataPageIface GthEditMetadataPageIface;

struct _GthEditMetadataPageIface {
	GTypeInterface parent_iface;
	void         (*set_file_list) (GthEditMetadataPage *self,
				       GList               *file_list /* GthFileData list */);
	void         (*update_info)   (GthEditMetadataPage *self,
				       GFileInfo           *info,
				       gboolean             only_modified_fields);
	const char * (*get_name)      (GthEditMetadataPage *self);
};

/* GthEditMetadataDialog */

GType          gth_edit_metadata_dialog_get_type       (void);
GtkWidget *    gth_edit_metadata_dialog_new            (void);
void           gth_edit_metadata_dialog_set_file_list  (GthEditMetadataDialog *dialog,
						        GList                 *file_list /* GthFileData list */);
void           gth_edit_metadata_dialog_update_info    (GthEditMetadataDialog *dialog,
							GList                 *file_list /* GthFileData list */);

/* GthEditMetadataPage */

GType          gth_edit_metadata_page_get_type         (void);
void           gth_edit_metadata_page_set_file_list    (GthEditMetadataPage   *self,
							GList                 *file_list /* GthFileData list */);
void           gth_edit_metadata_page_update_info      (GthEditMetadataPage   *self,
						        GFileInfo             *info,
							gboolean               only_modified_fields);
const char *   gth_edit_metadata_page_get_name         (GthEditMetadataPage   *self);

G_END_DECLS

#endif /* GTH_EDIT_METADATA_DIALOG_H */
