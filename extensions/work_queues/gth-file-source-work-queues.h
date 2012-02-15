/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2012 Free Software Foundation, Inc.
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

#ifndef GTH_FILE_SOURCE_WORK_QUEUES_H
#define GTH_FILE_SOURCE_WORK_QUEUES_H

#include <gthumb.h>

#define GTH_TYPE_FILE_SOURCE_WORK_QUEUES         (gth_file_source_work_queues_get_type ())
#define GTH_FILE_SOURCE_WORK_QUEUES(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), GTH_TYPE_FILE_SOURCE_WORK_QUEUES, GthFileSourceWorkQueues))
#define GTH_FILE_SOURCE_WORK_QUEUES_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST ((k), GTH_TYPE_FILE_SOURCE_WORK_QUEUES, GthFileSourceWorkQueuesClass))
#define GTH_IS_FILE_SOURCE_WORK_QUEUES(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), GTH_TYPE_FILE_SOURCE_WORK_QUEUES))
#define GTH_IS_FILE_SOURCE_WORK_QUEUES_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), GTH_TYPE_FILE_SOURCE_WORK_QUEUES))
#define GTH_FILE_SOURCE_WORK_QUEUES_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS((o), GTH_TYPE_FILE_SOURCE_WORK_QUEUES, GthFileSourceWorkQueuesClass))

typedef struct _GthFileSourceWorkQueues         GthFileSourceWorkQueues;
typedef struct _GthFileSourceWorkQueuesPrivate  GthFileSourceWorkQueuesPrivate;
typedef struct _GthFileSourceWorkQueuesClass    GthFileSourceWorkQueuesClass;

struct _GthFileSourceWorkQueues
{
	GthFileSource __parent;
	GthFileSourceWorkQueuesPrivate *priv;
};

struct _GthFileSourceWorkQueuesClass
{
	GthFileSourceClass __parent_class;
};

GType gth_file_source_work_queues_get_type (void) G_GNUC_CONST;

#endif /* GTH_FILE_SOURCE_WORK_QUEUES_H */
