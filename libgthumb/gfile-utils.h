/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2008 Free Software Foundation, Inc.
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

#ifndef GFILE_UTILS_H
#define GFILE_UTILS_H

#include <gio/gio.h>


#define UNREF(obj) {				\
	if (obj != NULL) {			\
		g_object_unref (obj);		\
		obj = NULL;			\
	}					\
}

/*
 * NOTE: All these functions accept/return _only_ uri-style GFiles.
 */

/* GFile to string */

char *        gfile_get_uri                    (GFile *file);
char *        gfile_get_path                   (GFile *file);

/* Warning */

void          gfile_warning                    (const char *msg,
	                                        GFile      *file,
	                                        GError     *err);
/* File utils */

gboolean      gfile_path_is_file               (GFile      *file);
gboolean      gfile_path_is_dir                (GFile      *file);
goffset       gfile_get_file_size              (GFile      *file);

/* Directory utils */

gboolean      gfile_ensure_dir_exists          (GFile      *dir,
			                        mode_t      mode,
			                        GError    **error);
guint64       gfile_get_destination_free_space (GFile      *file);
GFile *       gfile_get_temp_dir_name          (void);
gboolean      gfile_dir_remove_recursive       (GFile *dir);

#endif /* GFILE_UTILS_H */
