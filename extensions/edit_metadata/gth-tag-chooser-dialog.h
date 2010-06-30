/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2010 Free Software Foundation, Inc.
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
 
#ifndef GTH_TAG_CHOOSER_DIALOG_H
#define GTH_TAG_CHOOSER_DIALOG_H

#include <gtk/gtk.h>
#include <gthumb.h>

G_BEGIN_DECLS

#define GTH_TYPE_TAG_CHOOSER_DIALOG            (gth_tag_chooser_dialog_get_type ())
#define GTH_TAG_CHOOSER_DIALOG(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GTH_TYPE_TAG_CHOOSER_DIALOG, GthTagChooserDialog))
#define GTH_TAG_CHOOSER_DIALOG_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GTH_TYPE_TAG_CHOOSER_DIALOG, GthTagChooserDialogClass))
#define GTH_IS_TAG_CHOOSER_DIALOG(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GTH_TYPE_TAG_CHOOSER_DIALOG))
#define GTH_IS_TAG_CHOOSER_DIALOG_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GTH_TYPE_TAG_CHOOSER_DIALOG))
#define GTH_TAG_CHOOSER_DIALOG_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GTH_TYPE_TAG_CHOOSER_DIALOG, GthTagChooserDialogClass))

typedef struct _GthTagChooserDialog GthTagChooserDialog;
typedef struct _GthTagChooserDialogClass GthTagChooserDialogClass;
typedef struct _GthTagChooserDialogPrivate GthTagChooserDialogPrivate;

struct _GthTagChooserDialog {
	GtkDialog parent_instance;
	GthTagChooserDialogPrivate *priv;
};

struct _GthTagChooserDialogClass {
	GtkDialogClass parent_class;
};

/* GthTagChooserDialog */

GType          gth_tag_chooser_dialog_get_type  (void);
GtkWidget *    gth_tag_chooser_dialog_new       (void);
char **        gth_tag_chooser_dialog_get_tags  (GthTagChooserDialog *dialog);

G_END_DECLS

#endif /* GTH_TAG_CHOOSER_DIALOG_H */
