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

#ifndef GTH_EDIT_COMMENT_PAGE_H
#define GTH_EDIT_COMMENT_PAGE_H

#include <glib.h>
#include <glib-object.h>
#include <gthumb.h>

#define GTH_TYPE_EDIT_COMMENT_PAGE         (gth_edit_comment_page_get_type ())
#define GTH_EDIT_COMMENT_PAGE(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), GTH_TYPE_EDIT_COMMENT_PAGE, GthEditCommentPage))
#define GTH_EDIT_COMMENT_PAGE_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST ((k), GTH_TYPE_EDIT_COMMENT_PAGE, GthEditCommentPageClass))
#define GTH_IS_EDIT_COMMENT_PAGE(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), GTH_TYPE_EDIT_COMMENT_PAGE))
#define GTH_IS_EDIT_COMMENT_PAGE_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), GTH_TYPE_EDIT_COMMENT_PAGE))
#define GTH_EDIT_COMMENT_PAGE_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS((o), GTH_TYPE_EDIT_COMMENT_PAGE, GthEditCommentPageClass))

typedef struct _GthEditCommentPage         GthEditCommentPage;
typedef struct _GthEditCommentPagePrivate  GthEditCommentPagePrivate;
typedef struct _GthEditCommentPageClass    GthEditCommentPageClass;

struct _GthEditCommentPage
{
	GtkVBox __parent;
	GthEditCommentPagePrivate *priv;
};

struct _GthEditCommentPageClass
{
	GtkVBoxClass __parent_class;	
};

GType gth_edit_comment_page_get_type (void) G_GNUC_CONST;

#endif /* GTH_EDIT_COMMENT_PAGE_H */
