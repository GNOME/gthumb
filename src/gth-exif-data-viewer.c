/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2003, 2004, 2005 Free Software Foundation, Inc.
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


#include <config.h>
#include <string.h>

#include <glib/gi18n.h>
#include <gtk/gtk.h>
#include <libgnomevfs/gnome-vfs-mime.h>
#include <libgnomevfs/gnome-vfs-file-info.h>
#include <libgnomevfs/gnome-vfs-utils.h>

#include "file-utils.h"
#include "glib-utils.h"
#include "gth-exif-utils.h"
#include "gth-exif-data-viewer.h"
#include "image-viewer.h"

#include <libexif/exif-data.h>
#include <libexif/exif-content.h>
#include <libexif/exif-entry.h>
#include <libexif/exif-tag.h>
#include <libexif/exif-ifd.h>
#include <libexif/exif-mnote-data.h>


typedef enum {
	GTH_METADATA_CATEGORY_FILE = 0,
	GTH_METADATA_CATEGORY_EXIF_CAMERA,
	GTH_METADATA_CATEGORY_EXIF_IMAGE,
	GTH_METADATA_CATEGORY_EXIF_CONDITIONS,
	GTH_METADATA_CATEGORY_MAKERNOTE,
	GTH_METADATA_CATEGORY_GPS,
	GTH_METADATA_CATEGORY_EXIF_THUMBNAIL,
	GTH_METADATA_CATEGORY_VERSIONS,
	GTH_METADATA_CATEGORY_OTHER,
	GTH_METADATA_CATEGORIES
} GthMetadataCategory;


static char *metadata_category_name[GTH_METADATA_CATEGORIES] = 
{
	N_("Filesystem Data"),
       	N_("General Information"), 
	N_("Image Structure"), 
	N_("Picture-Taking Conditions"), 
	N_("Maker Notes"), 
	N_("GPS Coordinates"), 
	N_("Embedded Thumbnail"), 
	N_("Versions & Interoperability"), 
	N_("Other") 
};


/* The mapping between exif tags and categories was taken (and heavily modified)
 * from the eog/libeog/eog-info-view-exif.c file from the eog image viewer
 * source code, which is released under the terms of the GNU General Public
 * License.
 *
 * Note: tags are displayed in the same order they appear in this list.
 *
 * The layout of tags within a jpeg file is grouped into blocks (IFDs) called:
 * IFD0 - basic data about the main image
 * IFD1 - basic data about the thumbnail, if present
 * EXIF - more data about the main image
 * GPS - GPS related info
 * INTEROPERABILITY - compability info
 *
 */
#define MAX_TAGS_PER_CATEGORY 50
static ExifTag exif_tag_category_map[GTH_METADATA_CATEGORIES][MAX_TAGS_PER_CATEGORY] = {

	/* GTH_METADATA_CATEGORY_FILE */ 
	/* The filesystem info is generated and inserted by gthumb in the
	   correct order, so we don't need to sort it. */
	{ -1 },

	/* GTH_METADATA_CATEGORY_EXIF_CAMERA */
	/* This is general information about the camera, date and user,
	   that exists in the IFD0 or EXIF blocks. */
	{ EXIF_TAG_MAKE,
	  EXIF_TAG_MODEL,

  	  EXIF_TAG_DATE_TIME_ORIGINAL,
	  EXIF_TAG_SUB_SEC_TIME_ORIGINAL,
	  EXIF_TAG_DATE_TIME_DIGITIZED,
	  EXIF_TAG_SUB_SEC_TIME_DIGITIZED,
	  EXIF_TAG_DATE_TIME,
	  EXIF_TAG_SUB_SEC_TIME,

	  EXIF_TAG_USER_COMMENT,

	  EXIF_TAG_ARTIST,
	  EXIF_TAG_COPYRIGHT,

	  EXIF_TAG_DOCUMENT_NAME,
	  EXIF_TAG_IMAGE_DESCRIPTION,
	  EXIF_TAG_IMAGE_UNIQUE_ID,

	  EXIF_TAG_SOFTWARE,

	  EXIF_TAG_RELATED_SOUND_FILE,
	  -1 },

 	/* GTH_METADATA_CATEGORY_EXIF_IMAGE */
	/* These tags describe the main image data structures, and
	   come from the IFD0 and EXIF blocks. */
	{ EXIF_TAG_PIXEL_X_DIMENSION,
	  EXIF_TAG_PIXEL_Y_DIMENSION,
	  EXIF_TAG_ORIENTATION,

	  EXIF_TAG_COLOR_SPACE,
	  EXIF_TAG_TRANSFER_FUNCTION,
	  EXIF_TAG_PRIMARY_CHROMATICITIES,
	  EXIF_TAG_PHOTOMETRIC_INTERPRETATION,
	  EXIF_TAG_PLANAR_CONFIGURATION,
	  EXIF_TAG_COMPRESSION,

	  EXIF_TAG_IMAGE_WIDTH,
	  EXIF_TAG_IMAGE_LENGTH,

	  EXIF_TAG_X_RESOLUTION,
	  EXIF_TAG_Y_RESOLUTION,
	  EXIF_TAG_RESOLUTION_UNIT,

	  EXIF_TAG_COMPONENTS_CONFIGURATION,
	  EXIF_TAG_COMPRESSED_BITS_PER_PIXEL,

	  EXIF_TAG_BITS_PER_SAMPLE,
	  EXIF_TAG_SAMPLES_PER_PIXEL,

	  EXIF_TAG_STRIP_OFFSETS,
	  EXIF_TAG_ROWS_PER_STRIP,
	  EXIF_TAG_STRIP_BYTE_COUNTS,

	  EXIF_TAG_JPEG_INTERCHANGE_FORMAT_LENGTH,

	  EXIF_TAG_YCBCR_COEFFICIENTS,
	  EXIF_TAG_YCBCR_SUB_SAMPLING,
	  EXIF_TAG_YCBCR_POSITIONING,
	  -1 },

        /* GTH_METADATA_CATEGORY_EXIF_CONDITIONS */
	/* These tags describe the conditions when the photo was taken,
	   and are located in the IFD0 or EXIF blocks. */
	{ EXIF_TAG_ISO_SPEED_RATINGS,
	  EXIF_TAG_FNUMBER,
	  EXIF_TAG_APERTURE_VALUE,

	  EXIF_TAG_EXPOSURE_TIME,
	  EXIF_TAG_EXPOSURE_PROGRAM,
	  EXIF_TAG_EXPOSURE_INDEX,
	  EXIF_TAG_EXPOSURE_BIAS_VALUE,
	  EXIF_TAG_EXPOSURE_MODE,

	  EXIF_TAG_METERING_MODE,
	  EXIF_TAG_LIGHT_SOURCE,
	  EXIF_TAG_FLASH,
	  EXIF_TAG_FLASH_ENERGY,

	  EXIF_TAG_SUBJECT_DISTANCE,
	  EXIF_TAG_SUBJECT_DISTANCE_RANGE,
	  EXIF_TAG_SUBJECT_AREA,
	  EXIF_TAG_SUBJECT_LOCATION,

	  EXIF_TAG_FOCAL_LENGTH,
	  EXIF_TAG_FOCAL_LENGTH_IN_35MM_FILM,
	  EXIF_TAG_FOCAL_PLANE_X_RESOLUTION,
	  EXIF_TAG_FOCAL_PLANE_Y_RESOLUTION,
	  EXIF_TAG_FOCAL_PLANE_RESOLUTION_UNIT,

	  EXIF_TAG_DEVICE_SETTING_DESCRIPTION,

	  EXIF_TAG_OECF,
	  EXIF_TAG_SPATIAL_FREQUENCY_RESPONSE, 

	  EXIF_TAG_WHITE_BALANCE,
	  EXIF_TAG_WHITE_POINT,
	  EXIF_TAG_REFERENCE_BLACK_WHITE,

	  EXIF_TAG_BRIGHTNESS_VALUE,
	  EXIF_TAG_CONTRAST,
	  EXIF_TAG_SATURATION,
	  EXIF_TAG_SHARPNESS,

	  EXIF_TAG_GAIN_CONTROL,

	  EXIF_TAG_SPECTRAL_SENSITIVITY,
	  EXIF_TAG_MAX_APERTURE_VALUE,
	  EXIF_TAG_SHUTTER_SPEED_VALUE,
	  EXIF_TAG_SENSING_METHOD,
	  EXIF_TAG_CFA_PATTERN,

	  EXIF_TAG_SCENE_TYPE,
	  EXIF_TAG_SCENE_CAPTURE_TYPE,

	  EXIF_TAG_CUSTOM_RENDERED,

	  EXIF_TAG_DIGITAL_ZOOM_RATIO,

	  EXIF_TAG_FILE_SOURCE,
	  -1 },

	/* GTH_METADATA_CATEGORY_MAKERNOTE */ 
	/* These tags are semi-proprietary and vary from manufacturer to 
	   manufacturer, so we don't bother trying to sort them. They are
	   listed in the order that they appear in the jpeg file. */
	{ -1 },

	/* GTH_METADATA_CATEGORY_GPS */
	/* GPS data is stored in a special IFD (GPS) */
	{ EXIF_TAG_GPS_LATITUDE,
	  EXIF_TAG_GPS_LATITUDE_REF,
	  EXIF_TAG_GPS_LONGITUDE,
	  EXIF_TAG_GPS_LONGITUDE_REF,
	  EXIF_TAG_GPS_ALTITUDE,
	  EXIF_TAG_GPS_ALTITUDE_REF,
	  EXIF_TAG_GPS_TIME_STAMP,
	  EXIF_TAG_GPS_SATELLITES,
	  EXIF_TAG_GPS_STATUS,
	  EXIF_TAG_GPS_MEASURE_MODE,
	  EXIF_TAG_GPS_DOP,
	  EXIF_TAG_GPS_SPEED_REF,
	  EXIF_TAG_GPS_SPEED,
	  EXIF_TAG_GPS_TRACK_REF,
	  EXIF_TAG_GPS_TRACK,
	  EXIF_TAG_GPS_IMG_DIRECTION_REF,
	  EXIF_TAG_GPS_IMG_DIRECTION,
	  EXIF_TAG_GPS_MAP_DATUM,
	  EXIF_TAG_GPS_DEST_LATITUDE_REF,
	  EXIF_TAG_GPS_DEST_LATITUDE,
	  EXIF_TAG_GPS_DEST_LONGITUDE_REF,
	  EXIF_TAG_GPS_DEST_LONGITUDE,
	  EXIF_TAG_GPS_DEST_BEARING_REF,
	  EXIF_TAG_GPS_DEST_BEARING,
	  EXIF_TAG_GPS_DEST_DISTANCE_REF,
	  EXIF_TAG_GPS_DEST_DISTANCE,
	  EXIF_TAG_GPS_PROCESSING_METHOD,
	  EXIF_TAG_GPS_AREA_INFORMATION,
	  EXIF_TAG_GPS_DATE_STAMP,
	  EXIF_TAG_GPS_DIFFERENTIAL,
 	  EXIF_TAG_GPS_VERSION_ID,
	  -1 },

	/* GTH_METADATA_CATEGORY_EXIF_THUMBNAIL */ 
	/* There are normally only a few of these tags, so we don't bother
	   sorting them. They are displayed in the order that they appear in
	   the file. IFD0 (main image) and IFD1 (thumbnail) share many of the
	   same tags. The IFD0 tags are sorted by the structures above. The 
	   IFD1 tags are placed into this category. */
	{ -1 },

	/* GTH_METADATA_CATEGORY_VERSIONS */
	/* From IFD0, EXIF,or INTEROPERABILITY blocks. */
	{ EXIF_TAG_EXIF_VERSION,
	  EXIF_TAG_FLASH_PIX_VERSION,
	  EXIF_TAG_INTEROPERABILITY_INDEX,
	  EXIF_TAG_INTEROPERABILITY_VERSION,
	  EXIF_TAG_RELATED_IMAGE_FILE_FORMAT,
          EXIF_TAG_RELATED_IMAGE_WIDTH,
          EXIF_TAG_RELATED_IMAGE_LENGTH,
	  -1 },

	/* GTH_METADATA_CATEGORY_OTHER */ 
	/* New and unrecognized tags automatically go here. */
	{ -1 }
};


enum {
	NAME_COLUMN,
	VALUE_COLUMN,
	POS_COLUMN,
	NUM_COLUMNS
};


struct _GthExifDataViewerPrivate
{
	char                *path;
	gboolean             view_file_info;
	GtkWidget           *scrolled_win;
	GtkWidget           *image_exif_view;
	GtkTreeStore        *image_exif_model;
	GtkTreeRowReference *category_root[GTH_METADATA_CATEGORIES];
	ImageViewer         *viewer;
};


static GtkHBoxClass *parent_class = NULL;


static void
gth_exif_data_viewer_destroy (GtkObject *object)
{
	GthExifDataViewer *edv;

	edv = GTH_EXIF_DATA_VIEWER (object);

	if (edv->priv != NULL) {
		g_free (edv->priv->path);
		edv->priv->path = NULL;
		g_free (edv->priv);
		edv->priv = NULL;
	}

	GTK_OBJECT_CLASS (parent_class)->destroy (object);
}


static void
gth_exif_data_viewer_class_init (GthExifDataViewerClass *class)
{
	GtkObjectClass *object_class;

	parent_class = g_type_class_peek_parent (class);
	object_class = (GtkObjectClass*) class;

	object_class->destroy = gth_exif_data_viewer_destroy;
}


static void
gth_exif_data_viewer_init (GthExifDataViewer *edv)
{
	int i;

	edv->priv = g_new0 (GthExifDataViewerPrivate, 1);
	edv->priv->view_file_info = TRUE;

	for (i = 0; i < GTH_METADATA_CATEGORIES; i++)
		edv->priv->category_root[i] = NULL;
}


static void
gth_exif_data_viewer_construct (GthExifDataViewer *edv)
{
	GtkCellRenderer   *renderer;
	GtkTreeViewColumn *column;
	GValue             value = { 0, };

	edv->priv->scrolled_win = gtk_scrolled_window_new (NULL, NULL);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (edv->priv->scrolled_win), GTK_POLICY_AUTOMATIC, GTK_POLICY_ALWAYS);
	gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (edv->priv->scrolled_win), GTK_SHADOW_ETCHED_IN);
	gtk_container_add (GTK_CONTAINER (edv), edv->priv->scrolled_win);

	edv->priv->image_exif_view = gtk_tree_view_new ();
	gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (edv->priv->image_exif_view), TRUE);
	gtk_tree_view_set_rules_hint (GTK_TREE_VIEW (edv->priv->image_exif_view), TRUE);
	edv->priv->image_exif_model = gtk_tree_store_new (3,
							  G_TYPE_STRING,
							  G_TYPE_STRING,
							  G_TYPE_INT);
	gtk_tree_view_set_model (GTK_TREE_VIEW (edv->priv->image_exif_view),
				 GTK_TREE_MODEL (edv->priv->image_exif_model));
	g_object_unref (edv->priv->image_exif_model);
	gtk_container_add (GTK_CONTAINER (edv->priv->scrolled_win), edv->priv->image_exif_view);

	/**/

	renderer = gtk_cell_renderer_text_new ();
	column = gtk_tree_view_column_new_with_attributes (_("Field"),
							   renderer,
							   "text", NAME_COLUMN,
							   NULL);
	gtk_tree_view_column_set_expand (column, FALSE);
	gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_AUTOSIZE);
	gtk_tree_view_append_column (GTK_TREE_VIEW (edv->priv->image_exif_view),
				     column);

	/**/

	renderer = gtk_cell_renderer_text_new ();
	column = gtk_tree_view_column_new_with_attributes (_("Value "),
							   renderer,
							   "text", VALUE_COLUMN,
							   NULL);
	gtk_tree_view_column_set_expand (column, TRUE);
	gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_AUTOSIZE);

	g_value_init (&value, PANGO_TYPE_ELLIPSIZE_MODE);
	g_value_set_enum (&value, PANGO_ELLIPSIZE_END);
	g_object_set_property (G_OBJECT (renderer), "ellipsize", &value);
	g_value_unset (&value);

	gtk_tree_view_append_column (GTK_TREE_VIEW (edv->priv->image_exif_view),
				     column);

	gtk_tree_sortable_set_sort_column_id (GTK_TREE_SORTABLE (edv->priv->image_exif_model), POS_COLUMN, GTK_SORT_ASCENDING);
}


GType
gth_exif_data_viewer_get_type ()
{
        static GType type = 0;

        if (! type) {
                GTypeInfo type_info = {
			sizeof (GthExifDataViewerClass),
			NULL,
			NULL,
			(GClassInitFunc) gth_exif_data_viewer_class_init,
			NULL,
			NULL,
			sizeof (GthExifDataViewer),
			0,
			(GInstanceInitFunc) gth_exif_data_viewer_init
		};

		type = g_type_register_static (GTK_TYPE_HBOX,
					       "GthExifDataViewer",
					       &type_info,
					       0);
	}

        return type;
}


GtkWidget*
gth_exif_data_viewer_new (gboolean view_file_info)
{
	GtkWidget *widget;

	widget = GTK_WIDGET (g_object_new (GTH_TYPE_EXIF_DATA_VIEWER, NULL));
	gth_exif_data_viewer_construct (GTH_EXIF_DATA_VIEWER (widget));
	gth_exif_data_viewer_view_file_info (GTH_EXIF_DATA_VIEWER (widget), view_file_info);

	return widget;
}


static gboolean
tag_is_present_in_category (GthExifDataViewer   *edv,
			    GthMetadataCategory  category,
			    const char          *tag_name)
{
	GtkTreeModel *model = GTK_TREE_MODEL (edv->priv->image_exif_model);
	GtkTreePath  *category_path;
	GtkTreeIter   category_iter, iter;

	if (tag_name == NULL)
		return FALSE;

	if (edv->priv->category_root[category] == NULL)
		return FALSE;

	category_path = gtk_tree_row_reference_get_path (edv->priv->category_root[category]);
	if (category_path == NULL)
		return FALSE;

	if (! gtk_tree_model_get_iter (model,
				       &category_iter,
				       category_path))
		return FALSE;

	gtk_tree_path_free (category_path);

	if (! gtk_tree_model_iter_children  (model, &iter, &category_iter))
		return FALSE;

	do {
		char *tag_name2;

		gtk_tree_model_get (model,
				    &iter,
				    NAME_COLUMN, &tag_name2,
				    -1);
		if ((tag_name2 != NULL)
		    && (strcmp (tag_name, tag_name2) == 0)) {
			g_free (tag_name2);
			return TRUE;
		}

		g_free (tag_name2);
	} while (gtk_tree_model_iter_next (model, &iter));

	return FALSE;
}


static void
add_to_exif_display_list (GthExifDataViewer   *edv,
			  GthMetadataCategory  category,
			  const char 	      *utf8_name,
			  const char	      *utf8_value,
	 		  int		       position)
{
	GtkTreeModel *model = GTK_TREE_MODEL (edv->priv->image_exif_model);
	GtkTreeIter   root_iter;
	GtkTreeIter   iter;

	if (edv->priv->category_root[category] == NULL) {
		GtkTreePath *path;
		gtk_tree_store_append (edv->priv->image_exif_model, &root_iter, NULL);
		gtk_tree_store_set (edv->priv->image_exif_model, &root_iter,
        		    	    NAME_COLUMN, _(metadata_category_name[category]),
			    	    VALUE_COLUMN, "",
			    	    POS_COLUMN, category,
                            	    -1);
		path = gtk_tree_model_get_path (model, &root_iter);
		edv->priv->category_root[category] = gtk_tree_row_reference_new (model, path);
		gtk_tree_path_free (path);
	}
	else {
		GtkTreePath *path;
		path = gtk_tree_row_reference_get_path (edv->priv->category_root[category]);
		gtk_tree_model_get_iter (model,
					 &root_iter,
					 path);
		gtk_tree_path_free (path);
	}

	gtk_tree_store_append (edv->priv->image_exif_model, &iter, &root_iter);
        gtk_tree_store_set (edv->priv->image_exif_model, &iter,
        		    NAME_COLUMN, utf8_name,
			    VALUE_COLUMN, utf8_value,
			    POS_COLUMN, position,
                            -1);
}


static GthMetadataCategory
tag_category (ExifTag  tag,
	      ExifIfd  ifd,
	      int     *position)
{
	GthMetadataCategory category;

	switch (ifd) {
		/* Some IFDs require special handling to ensure proper categorization.
		   This is because:
		   	1) IFD0 and IFD1 may duplicate some tags (with different values)
			2) Some tag IDs (numeric values) overlap in the GPS and
			   INTEROPERABILITY IFDs.					*/
	case EXIF_IFD_1:
		/* Data in IFD1 is for the embedded thumbnail. Keep it separate, do not sort */
		return GTH_METADATA_CATEGORY_EXIF_THUMBNAIL;
		break;
	
	case EXIF_IFD_GPS:
		/* Go straight to the GPS category if this is in a GPS IFD, to
		   avoid the tag ID overlap problem. Do sort. */
		category = GTH_METADATA_CATEGORY_GPS;
		break;

	case EXIF_IFD_INTEROPERABILITY:
		/* Go straight to the version category if this is in an
		   interop IFD, to avoid the tag ID overlap problem. Do sort. */
		category = GTH_METADATA_CATEGORY_VERSIONS;
		break;

	default:
		/* Start the tag search at the beginning, and use the first match */
		category = 1;
	}

	while (category < GTH_METADATA_CATEGORIES) {
		int j = 0;
		while (exif_tag_category_map[category][j] != -1) {
			if  (exif_tag_category_map[category][j] == tag) {
				if (position != NULL)
					*position = j;
				return category;
			}
			j++;
		}
		category++;
	}

	return GTH_METADATA_CATEGORY_OTHER;
}


static void
update_exif_data (GthExifDataViewer *edv,
		  ExifData          *edata)
{
	const char   *path;
	unsigned int  i, j;
	gboolean      list_is_empty = TRUE;

	path = get_file_path_from_uri (edv->priv->path);
	if (path == NULL)
		return;

	if (edata == NULL)
		edata = exif_data_new_from_file (path);
	else
		exif_data_ref (edata);

	if (edata == NULL)
                return;

	/* Iterate through every IFD in the Exif data, checking for tags. The GPS tags are
	   stored in their own private IFD. */

        for (i = 0; i < EXIF_IFD_COUNT; i++) {
                ExifContent *content = edata->ifd[i];
		const char  *value;
		char        *utf8_name;
		char        *utf8_value;

                if (! edata->ifd[i] || ! edata->ifd[i]->count)
                        continue;

                for (j = 0; j < content->count; j++) {
                        ExifEntry *e = content->entries[j];

                        if (! content->entries[j])
                                continue;

			/* Accept all tags, but handle "maker notes" separately below */

			if (e->tag != EXIF_TAG_MAKER_NOTE) {
				GthMetadataCategory category;
				int                 position;

				/* The tag IDs for the GPS and non-GPS IFDs overlap slightly,
				   so it is important to use the exif_tag_get_name_in_ifd
				   function, and not the older exif_tag_get_name function. */

				value = exif_tag_get_name_in_ifd (e->tag, i);

				if (value == NULL)
					continue;

				utf8_name = g_strdup (value);

				value = get_exif_entry_value (e);
				if (value == NULL) {
					g_free (utf8_name);
					continue;
				}

				utf8_value = g_strdup (value);
				if ((utf8_value == NULL)
				    || (*utf8_value == 0)
				    || _g_utf8_all_spaces (utf8_value)) {
					g_free (utf8_name);
					g_free (utf8_value);
					continue;
				}

				/* Assign categories and sort positions. */
				category = tag_category (e->tag, i, &position);

				/* The "thumbnail" and "other" categories are not sorted
				   using the category map. They are simply presented in
				   the order they are found in the file. */
				if (   (category == GTH_METADATA_CATEGORY_OTHER)
				    || (category == GTH_METADATA_CATEGORY_EXIF_THUMBNAIL))
					position = j;

				add_to_exif_display_list (edv, category, utf8_name, utf8_value, position);

				g_free (utf8_name);
				g_free (utf8_value);
			}
			else {
				/* Maker Notes have their own sub-structure which must be
				   iterated through. libexif only knows how to handle certain
				   types of manufacturer note styles. */

				ExifMnoteData *mnote;
				unsigned int k;
				unsigned int subnote_count;
				char	     mnote_buf[1024];

				mnote = exif_data_get_mnote_data (edata);
				if (mnote == NULL)
					continue;

				/* Supported MakerNote Found */

				subnote_count = exif_mnote_data_count (mnote);

				/* Iterate through each "sub-note" */
				for (k = 0; k < subnote_count; k++) {
	                               	value = exif_mnote_data_get_title (mnote, k);

        	                       	if (value == NULL)
                	                	continue;

	      	                       	utf8_name = g_strdup (value);
                	               	if (tag_is_present_in_category (edv, GTH_METADATA_CATEGORY_MAKERNOTE, utf8_name)) {
                        	               	g_free (utf8_name);
                                	       	continue;
	                               	}

        	                       	exif_mnote_data_get_value (mnote, k, mnote_buf, sizeof (mnote_buf));

					utf8_value = g_strdup (mnote_buf);
	                               	if ((utf8_value == NULL)
        	                           || (*utf8_value == 0)
                	                   || _g_utf8_all_spaces (utf8_value)) {
                        	              	g_free (utf8_name);
                                	      	g_free (utf8_value);
                                        	continue;
	                        	}

					add_to_exif_display_list (edv, GTH_METADATA_CATEGORY_MAKERNOTE, utf8_name, utf8_value, k);

	   	                        g_free (utf8_name);
		                        g_free (utf8_value);
				}
			}

			list_is_empty = FALSE;
		}
	}

	exif_data_unref (edata);
}


static void
update_file_info (GthExifDataViewer *edv)
{
	char              *utf8_name;
	int                width, height;
	char              *size_txt;
	time_t             mtime;
	struct tm         *tm;
	char               time_txt[50], *utf8_time_txt;
	double             sec;
	GnomeVFSFileSize   file_size;
	char              *file_size_txt;

	if (edv->priv->viewer == NULL)
		return;

	utf8_name = g_filename_display_basename (edv->priv->path);

	if (!image_viewer_is_void (IMAGE_VIEWER (edv->priv->viewer))) {
		width = image_viewer_get_image_width (edv->priv->viewer);
		height = image_viewer_get_image_height (edv->priv->viewer);
	} else {
		width = 0;
		height = 0;
	}
	size_txt = g_strdup_printf (_("%d x %d pixels"), width, height);

	mtime = get_file_mtime (edv->priv->path);
	tm = localtime (&mtime);
	strftime (time_txt, 50, _("%d %B %Y, %H:%M"), tm);
	utf8_time_txt = g_locale_to_utf8 (time_txt, -1, 0, 0, 0);
	sec = g_timer_elapsed (image_loader_get_timer (edv->priv->viewer->loader),  NULL);

	file_size = get_file_size (edv->priv->path);
	file_size_txt = gnome_vfs_format_file_size_for_display (file_size);

	/**/

	add_to_exif_display_list (edv, GTH_METADATA_CATEGORY_FILE, _("Name"), utf8_name, -6);
	add_to_exif_display_list (edv, GTH_METADATA_CATEGORY_FILE, _("Dimensions"), size_txt, -5);
	add_to_exif_display_list (edv, GTH_METADATA_CATEGORY_FILE, _("Size"), file_size_txt, -4);
	add_to_exif_display_list (edv, GTH_METADATA_CATEGORY_FILE, _("Modified"), utf8_time_txt, -3);
	add_to_exif_display_list (edv, GTH_METADATA_CATEGORY_FILE, _("Type"), gnome_vfs_mime_get_description (get_mime_type (edv->priv->path)), -2);

	/**/

	g_free (utf8_time_txt);
	g_free (utf8_name);
	g_free (size_txt);
	g_free (file_size_txt);
}


void
gth_exif_data_viewer_view_file_info (GthExifDataViewer *edv,
				     gboolean           view_file_info)
{
	edv->priv->view_file_info = view_file_info;
}


static void
set_path (GthExifDataViewer *edv,
	  const char        *path)
{
	g_return_if_fail (edv != NULL);

	g_free (edv->priv->path);
	edv->priv->path = NULL;
	if (path != NULL)
		edv->priv->path = g_strdup (path);
}


void
gth_exif_data_viewer_update (GthExifDataViewer *edv,
			     ImageViewer       *viewer,
			     const char        *path,
			     gpointer           exif_data)
{
	int i;

	set_path (edv, path);

	if (viewer != NULL)
		edv->priv->viewer = viewer;

	for (i = 0; i < GTH_METADATA_CATEGORIES; i++)
		if (edv->priv->category_root[i] != NULL) {
			gtk_tree_row_reference_free (edv->priv->category_root[i]);
			edv->priv->category_root[i] = NULL;
		}
	gtk_tree_store_clear (edv->priv->image_exif_model);

	if (path == NULL)
		return;

	if (edv->priv->view_file_info)
		update_file_info (edv);

	update_exif_data (edv, exif_data);

	gtk_tree_view_expand_all (GTK_TREE_VIEW (edv->priv->image_exif_view));
}


GtkWidget *
gth_exif_data_viewer_get_view (GthExifDataViewer *edv)
{
	return edv->priv->image_exif_view;
}
