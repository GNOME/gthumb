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

#ifndef AUTO_COMPLETION_H
#define AUTO_COMPLETION_H

#include <glib.h>
#include <gtk/gtkwidget.h>
#include "gth-browser.h"

void      auto_compl_reset                 (void);
int       auto_compl_get_n_alternatives    (const char *path);
char *    auto_compl_get_common_prefix     (void);
GList *   auto_compl_get_alternatives      (void);
void      auto_compl_show_alternatives     (GthBrowser *browser,
					    GtkWidget  *entry);
void      auto_compl_hide_alternatives     (void);
gboolean  auto_compl_alternatives_visible  (void);

#endif /* AUTO_COMPLETION_H */
