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

#ifndef GTH_ORGANIZE_TASK_H
#define GTH_ORGANIZE_TASK_H

#include <glib-object.h>
#include <gthumb.h>

typedef enum {
	GTH_GROUP_POLICY_DIGITALIZED_DATE,
	GTH_GROUP_POLICY_MODIFIED_DATE
} GthGroupPolicy;

#define GTH_TYPE_ORGANIZE_TASK         (gth_organize_task_get_type ())
#define GTH_ORGANIZE_TASK(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), GTH_TYPE_ORGANIZE_TASK, GthOrganizeTask))
#define GTH_ORGANIZE_TASK_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST ((k), GTH_TYPE_ORGANIZE_TASK, GthOrganizeTaskClass))
#define GTH_IS_ORGANIZE_TASK(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), GTH_TYPE_ORGANIZE_TASK))
#define GTH_IS_ORGANIZE_TASK_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), GTH_TYPE_ORGANIZE_TASK))
#define GTH_ORGANIZE_TASK_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS((o), GTH_TYPE_ORGANIZE_TASK, GthOrganizeTaskClass))

typedef struct _GthOrganizeTask         GthOrganizeTask;
typedef struct _GthOrganizeTaskPrivate  GthOrganizeTaskPrivate;
typedef struct _GthOrganizeTaskClass    GthOrganizeTaskClass;

struct _GthOrganizeTask
{
	GthTask __parent;
	GthOrganizeTaskPrivate *priv;
};

struct _GthOrganizeTaskClass
{
	GthTaskClass __parent_class;
};

GType       gth_organize_task_get_type                 (void) G_GNUC_CONST;
GthTask *   gth_organize_task_new                      (GthBrowser      *browser,
						        GFile           *folder,
						        GthGroupPolicy   group_policy);
void        gth_organize_task_set_recursive            (GthOrganizeTask *self,
							gboolean         recursive);
void        gth_organize_task_set_create_singletons    (GthOrganizeTask *self,
						        gboolean         create);
void        gth_organize_task_set_singletons_catalog   (GthOrganizeTask *self,
							const char      *catalog_name);

#endif /* GTH_ORGANIZE_TASK_H */
