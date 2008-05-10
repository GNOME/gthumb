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

#ifndef _GTH_SORT_UTILS_H
#define _GTH_SORT_UTILS_H

#include "file-data.h"

int gth_sort_by_comment_then_name        (const char       *string1, 
					  const char       *string2,
			                  const char       *name1, 
			                  const char       *name2);
int gth_sort_by_size_then_name           (goffset           size1, 
					  goffset           size2,
				          const char       *name1, 
				          const char       *name2);
int gth_sort_by_filetime_then_name       (time_t            time1, 
					  time_t            time2,
				          const char       *name1, 
				          const char       *name2);
int gth_sort_by_filename_but_ignore_path (const char       *name1, 
					  const char       *name2);
int gth_sort_by_full_path                (const char       *path1, 
					  const char       *path2);
int gth_sort_by_exiftime_then_name       (FileData         *fd1, 
					  FileData         *fd2);
int gth_sort_none			 (gconstpointer  ptr1,
					  gconstpointer  ptr2);

#endif /* _GTH_SORT_UTILS_H */

