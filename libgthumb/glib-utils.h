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

#ifndef _GLIB_UTILS_H
#define _GLIB_UTILS_H

#include <glib-object.h>

#define g_signal_handlers_disconnect_by_data(instance, data) \
    g_signal_handlers_disconnect_matched ((instance), G_SIGNAL_MATCH_DATA, \
                                          0, 0, NULL, NULL, (data))
#define g_signal_handlers_block_by_data(instance, data) \
    g_signal_handlers_block_matched ((instance), G_SIGNAL_MATCH_DATA, \
                                     0, 0, NULL, NULL, (data))
#define g_signal_handlers_unblock_by_data(instance, data) \
    g_signal_handlers_unblock_matched ((instance), G_SIGNAL_MATCH_DATA, \
                                       0, 0, NULL, NULL, (data))


char *   _g_strdup_with_max_size   (const char *s,
				    int         max_size);

char **  _g_get_template_from_text (const char *template);

char *   _g_get_name_from_template (char      **template,
				    int         num);

char *   _g_substitute             (const char *from,
				    const char  this,
				    const char *with_this);

#endif /* _GLIB_UTILS_H */
