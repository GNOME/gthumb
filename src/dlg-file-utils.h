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

#ifndef DLG_FILE_UTILS_H
#define DLG_FILE_UTILS_H

#include "gthumb-window.h"

gboolean dlg_file_delete          (GThumbWindow *window,
				   GList        *list,
				   const char   *message);

void     dlg_file_move            (GThumbWindow *window,
				   GList        *list);

void     dlg_file_copy            (GThumbWindow *window,
				   GList        *list);

void     dlg_file_rename_series   (GThumbWindow *window,
				   GList        *old_names,
				   GList        *new_names);

#endif /* DLG_FILE_UTILS_H */

