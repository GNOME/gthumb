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

#include <libbonobo.h>
#include <libgnomevfs/gnome-vfs-utils.h>
#include <libgnomevfs/gnome-vfs-ops.h>
#include "file-utils.h"
#include "viewer-stream.h"
#include "viewer-nautilus-view.h"
#include "iids.h"
#include "ui.h"


#define BUFFER_SIZE (16*1024)


static GnomeVFSFileSize 
get_uri_size (const char *uri)
{
	GnomeVFSFileInfo *info;
	GnomeVFSResult    result;
	GnomeVFSFileSize  size;

	info = gnome_vfs_file_info_new ();
	result = gnome_vfs_get_file_info (uri, info, (GNOME_VFS_FILE_INFO_DEFAULT | GNOME_VFS_FILE_INFO_FOLLOW_LINKS)); 
	size = 0;
	if (result == GNOME_VFS_OK)
		size = info->size;

	gnome_vfs_file_info_unref (info);

	return size;
}


static time_t 
get_uri_mtime (const char *uri)
{
	GnomeVFSFileInfo *info;
	GnomeVFSResult    result;
	time_t            mtime;

	info = gnome_vfs_file_info_new ();
	result = gnome_vfs_get_file_info (uri, info, (GNOME_VFS_FILE_INFO_DEFAULT | GNOME_VFS_FILE_INFO_FOLLOW_LINKS));
	mtime = 0;
	if (result == GNOME_VFS_OK)
		mtime = info->mtime;

	gnome_vfs_file_info_unref (info);

	return mtime;
}


static char *
get_status (ImageViewer *viewer,
	    const char  *location)
{
	char       *text;
	char        time_txt[50];
	char       *size_txt;
	char       *file_size_txt;
	char       *escaped_name;
	char       *utf8_name;
	int         width, height;
	time_t      timer;
	struct tm  *tm;

	timer = get_uri_mtime (location);
	tm = localtime (&timer);
	strftime (time_txt, 50, _("%d %b %Y, %H:%M"), tm);

	utf8_name = g_locale_to_utf8 (file_name_from_path (location), -1, 
				      0, 0, 0);
	escaped_name = g_markup_escape_text (utf8_name, -1);

	width = image_viewer_get_image_width (viewer);
	height = image_viewer_get_image_height (viewer);
	size_txt = g_strdup_printf (_("%d x %d pixels"), width, height);

	file_size_txt = gnome_vfs_format_file_size_for_display (get_uri_size (location));

	/**/

	text = g_strdup_printf ("%s - %s - %s",
				size_txt,
				file_size_txt,
				time_txt);

	g_free (utf8_name);
	g_free (escaped_name);
	g_free (size_txt);
	g_free (file_size_txt);

	return text;
}


static void 
viewer_stream_load (PortableServer_Servant  servant,
		    Bonobo_Stream           stream,
		    const CORBA_char       *type,
		    CORBA_Environment      *ev)
{
	BonoboObject        *object;
	ViewerControl       *control;
	ViewerNautilusView  *nautilus_view;
	Nautilus_ViewFrame   view_frame;
	GdkPixbufLoader     *loader;
	Bonobo_Stream_iobuf *buffer;
	int                  length;
	int                  tot_length = 0;

	object        = bonobo_object_from_servant (servant);
	control       = VIEWER_STREAM (object)->control;
	nautilus_view = VIEWER_NAUTILUS_VIEW (control->priv->nautilus_view);

	if (nautilus_view != CORBA_OBJECT_NIL)
		view_frame = nautilus_view->view_frame;
	else
		view_frame = CORBA_OBJECT_NIL;

	loader = gdk_pixbuf_loader_new ();

	do {
		if (view_frame != CORBA_OBJECT_NIL)
			Nautilus_ViewFrame_report_load_underway (view_frame, ev);

		Bonobo_Stream_read (stream, BUFFER_SIZE, &buffer, ev);

		if (ev->_major != CORBA_NO_EXCEPTION) {
			CORBA_exception_set (ev, 
					     CORBA_USER_EXCEPTION,
					     ex_Bonobo_IOError, 
					     NULL);
			CORBA_free (buffer);
			break;
		}

		length = buffer->_length;

		if ((length > 0) && 
		    ! gdk_pixbuf_loader_write (loader, 
					       buffer->_buffer, 
					       length,
					       NULL)) {
			CORBA_exception_set (ev, 
					     CORBA_USER_EXCEPTION,
					     ex_Bonobo_Persist_WrongDataType,
					     NULL);
			CORBA_free (buffer);
			break;
		}
		tot_length += length;
		CORBA_free (buffer);
	} while (length > 0);

	gdk_pixbuf_loader_close (loader, NULL);

	if (ev->_major == CORBA_NO_EXCEPTION) {
		if (tot_length == 0)
			image_viewer_set_void (control->priv->viewer);
		else
			image_viewer_load_from_pixbuf_loader (control->priv->viewer, loader);

		if (view_frame != CORBA_OBJECT_NIL) {
			char *status;
			Nautilus_ViewFrame_report_load_complete (view_frame, ev);
			status = get_status (control->priv->viewer, nautilus_view->location);
			Nautilus_ViewFrame_report_status (view_frame, status, ev);
			g_free (status);
		}

	} else if (view_frame != CORBA_OBJECT_NIL)
		Nautilus_ViewFrame_report_load_failed (view_frame, ev);

	g_object_unref (loader);

	if (view_frame != CORBA_OBJECT_NIL) {
		bonobo_object_release_unref (view_frame, ev);
		nautilus_view->view_frame = CORBA_OBJECT_NIL;
	}

	update_menu_sensitivity (control); 
}


static Bonobo_Persist_ContentTypeList *
get_content_types (BonoboPersist *persist, CORBA_Environment *ev)
{
	 return bonobo_persist_generate_content_types (
		 14, 
		 "image/bmp",
		 "image/jpeg",
		 "image/gif",
		 "image/png",
		 "image/tiff",
		 "image/x-bmp",
		 "image/x-cmu-raster",
		 "image/x-ico",
		 "image/x-png",
		 "image/x-portable-anymap",
		 "image/x-portable-bitmap",
		 "image/x-portable-graymap",
		 "image/x-portable-pixmap",
		 "image/x-tga",
		 "image/x-xpixmap",
		 "image/xpm");
}


static void
viewer_stream_class_init (ViewerStreamClass *klass)
{
        BonoboPersistClass *persist_class = BONOBO_PERSIST_CLASS (klass);
 	POA_Bonobo_PersistStream__epv *epv = &klass->epv;

	persist_class->get_content_types = get_content_types;
 
	epv->load = viewer_stream_load;
	epv->save = NULL;
}


static void
viewer_stream_init (ViewerStream *stream)
{
	stream->control = NULL;
}


BONOBO_TYPE_FUNC_FULL (
	ViewerStream,         /* Glib class name */
	Bonobo_PersistStream, /* CORBA interface name */
	BONOBO_TYPE_PERSIST,  /* parent type */
	viewer_stream);       /* local prefix ie. 'echo'_class_init */


BonoboObject * 
viewer_stream_new (ViewerControl *control)
{
	ViewerStream *stream;

	stream = VIEWER_STREAM (g_object_new (VIEWER_TYPE_STREAM, NULL));
	bonobo_persist_construct (BONOBO_PERSIST (stream), OAFIID);
	stream->control = control;

	return (BonoboObject*) stream;
}

