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

#ifndef _VIEWER_STREAM_H_
#define _VIEWER_STREAM_H_

#include <bonobo/bonobo-object.h>
#include "viewer-control.h"

G_BEGIN_DECLS

#define VIEWER_TYPE_STREAM         (viewer_stream_get_type ())
#define VIEWER_STREAM(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), VIEWER_TYPE_STREAM, ViewerStream))
#define VIEWER_STREAM_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), VIEWER_TYPE_STREAM, ViewerStreamClass))
#define VIEWER_IS_STREAM(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), VIEWER_TYPE_STREAM))
#define VIEWER_IS_STREAM_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), VIEWER_TYPE_STREAM))
#define VIEWER_STREAM_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), VIEWER_TYPE_STREAM, ViewerStreamClass))

typedef struct {
	BonoboPersist __parent;

	/*< private >*/

	ViewerControl *control;
} ViewerStream;

typedef struct {
	BonoboPersistClass __parent_class;

	POA_Bonobo_PersistStream__epv epv;
} ViewerStreamClass;

GType          viewer_stream_get_type (void);
BonoboObject * viewer_stream_new      (ViewerControl *control);

G_END_DECLS

#endif /* _VIEWER_STREAM_H_ */
