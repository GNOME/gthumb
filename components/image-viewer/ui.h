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

#ifndef UI_H
#define UI_H


#include "viewer-control.h"

void setup_transparency_menu (ViewerControl *control);

void setup_zoom_quality_menu (ViewerControl *control);

void update_menu_sensitivity (ViewerControl *control);

/* -- verbs -- */

void verb_rotate         (BonoboUIComponent *component, 
			  gpointer callback_data, 
			  const char *cname);

void verb_rotate_180     (BonoboUIComponent *component, 
			  gpointer callback_data, 
			  const char *cname);

void verb_flip           (BonoboUIComponent *component, 
			  gpointer callback_data, 
			  const char *cname);

void verb_mirror         (BonoboUIComponent *component, 
			  gpointer callback_data,
			  const char *cname);

void verb_start_stop_ani (BonoboUIComponent *component, 
			  gpointer callback_data,
			  const char *cname);

void verb_step_ani       (BonoboUIComponent *component, 
			  gpointer callback_data, 
			  const char *cname);

void verb_print_image    (BonoboUIComponent *component, 
			  gpointer callback_data, 
			  const char *cname);

void verb_save_image     (BonoboUIComponent *component, 
			  gpointer callback_data, 
			  const char *cname);

#endif /* UI_H */
