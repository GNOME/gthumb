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
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <config.h>
#include <glib/gi18n.h>
#include <gtk/gtk.h>
#include <gthumb.h>
#include <extensions/image_rotation/rotation-utils.h>
#include "gth-contact-sheet-creator.h"
#include "preferences.h"

#define DEFAULT_THUMB_SIZE 128

static GObjectClass *parent_class = NULL;


struct _GthContactSheetCreatorPrivate {
	GthBrowser        *browser;
	GList             *gfile_list;             /* GFile list */

	/* options */

	char              *header;
	char              *footer;
	GFile             *destination;            /* Save files in this location. */
	GthFileDataSort   *sort_type;
	gboolean           sort_inverse;
	int                images_per_index;
	gboolean           single_index;
	int                columns_per_page;
	int                rows_per_page;
	gboolean           squared_thumbnails;
	int                thumb_width;
	int                thumb_height;
	char              *thumbnail_caption;
};


static void
gth_contact_sheet_creator_exec (GthTask *task)
{
	/* TODO */
}


static void
gth_contact_sheet_creator_cancelled (GthTask *task)
{
	/* TODO */
}


static void
gth_contact_sheet_creator_finalize (GObject *object)
{
	GthContactSheetCreator *self;

	g_return_if_fail (GTH_IS_CONTACT_SHEET_CREATOR (object));

	self = GTH_CONTACT_SHEET_CREATOR (object);
	g_free (self->priv->header);
	g_free (self->priv->footer);
	g_free (self->priv->thumbnail_caption);
	_g_object_list_unref (self->priv->gfile_list);

	G_OBJECT_CLASS (parent_class)->finalize (object);
}


static void
gth_contact_sheet_creator_class_init (GthContactSheetCreatorClass *klass)
{
	GObjectClass *object_class;
	GthTaskClass *task_class;

	parent_class = g_type_class_peek_parent (klass);
	g_type_class_add_private (klass, sizeof (GthContactSheetCreatorPrivate));

	object_class = G_OBJECT_CLASS (klass);
	object_class->finalize = gth_contact_sheet_creator_finalize;

	task_class = GTH_TASK_CLASS (klass);
	task_class->exec = gth_contact_sheet_creator_exec;
	task_class->cancelled = gth_contact_sheet_creator_cancelled;
}


static void
gth_contact_sheet_creator_init (GthContactSheetCreator *self)
{
	self->priv = G_TYPE_INSTANCE_GET_PRIVATE (self, GTH_TYPE_CONTACT_SHEET_CREATOR, GthContactSheetCreatorPrivate);
	self->priv->header = NULL;
	self->priv->footer = NULL;
	self->priv->sort_type = NULL;
	self->priv->sort_inverse = FALSE;
	self->priv->images_per_index = 0;
	self->priv->columns_per_page = 0;
	self->priv->rows_per_page = 0;
	self->priv->single_index = FALSE;
	self->priv->thumb_width = DEFAULT_THUMB_SIZE;
	self->priv->thumb_height = DEFAULT_THUMB_SIZE;
	self->priv->thumbnail_caption = NULL;
}


GType
gth_contact_sheet_creator_get_type (void)
{
	static GType type = 0;

	if (! type) {
		GTypeInfo type_info = {
			sizeof (GthContactSheetCreatorClass),
			NULL,
			NULL,
			(GClassInitFunc) gth_contact_sheet_creator_class_init,
			NULL,
			NULL,
			sizeof (GthContactSheetCreator),
			0,
			(GInstanceInitFunc) gth_contact_sheet_creator_init
                };

                type = g_type_register_static (GTH_TYPE_TASK,
					       "GthContactSheetCreator",
					       &type_info,
					       0);
        }

	return type;
}


GthTask *
gth_contact_sheet_creator_new (GthBrowser *browser,
			       GList      *file_list)
{
	GthContactSheetCreator *self;

	g_return_val_if_fail (browser != NULL, NULL);

	self = (GthContactSheetCreator *) g_object_new (GTH_TYPE_CONTACT_SHEET_CREATOR, NULL);
	self->priv->browser = browser;
	self->priv->gfile_list = _g_object_list_ref (file_list);

	return (GthTask *) self;
}


void
gth_contact_sheet_creator_set_header (GthContactSheetCreator *self,
				      const char             *value)
{

}


void
gth_contact_sheet_creator_set_footer (GthContactSheetCreator *self,
				      const char             *value)
{

}


void
gth_contact_sheet_creator_set_destination (GthContactSheetCreator *self,
					   GFile                  *destination)
{

}


void
gth_contact_sheet_creator_set_filename_template (GthContactSheetCreator *self,
						 const char             *filename_template)
{

}


void
gth_contact_sheet_creator_set_filetype (GthContactSheetCreator *self,
					const char             *filetype)
{

}


void
gth_contact_sheet_creator_set_write_image_map (GthContactSheetCreator *self,
					       gboolean                value)
{

}


void
gth_contact_sheet_creator_set_theme (GthContactSheetCreator *self,
				     const char             *theme_name)
{

}


void
gth_contact_sheet_creator_set_images_per_index (GthContactSheetCreator *self,
					        int                     value)
{

}


void
gth_contact_sheet_creator_set_single_index (GthContactSheetCreator *self,
					    gboolean                value)
{

}


void
gth_contact_sheet_creator_set_columns (GthContactSheetCreator *self,
			   	       int                     columns)
{

}


void
gth_contact_sheet_creator_set_sort_order (GthContactSheetCreator *self,
					  GthFileDataSort        *sort_type,
					  gboolean                sort_inverse)
{

}


void
gth_contact_sheet_creator_set_same_size (GthContactSheetCreator *self,
					 gboolean                value)
{

}


void
gth_contact_sheet_creator_set_thumb_size (GthContactSheetCreator *self,
					  gboolean                squared,
					  int                     width,
					  int                     height)
{

}


void
gth_contact_sheet_creator_set_thumbnail_caption (GthContactSheetCreator *self,
						 const char             *caption)
{

}
