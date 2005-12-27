/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2005 Free Software Foundation, Inc.
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

/* Base on gs-fade.c from gnome-screensaver: 
 *
 * Copyright (C) 2004-2005 William Jon McCann <mccann@jhu.edu>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 * Authors: William Jon McCann <mccann@jhu.edu>
 *
 */

#ifndef __GS_FADE_H
#define __GS_FADE_H

G_BEGIN_DECLS

#define GS_TYPE_FADE         (gs_fade_get_type ())
#define GS_FADE(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), GS_TYPE_FADE, GSFade))
#define GS_FADE_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), GS_TYPE_FADE, GSFadeClass))
#define GS_IS_FADE(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), GS_TYPE_FADE))
#define GS_IS_FADE_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), GS_TYPE_FADE))
#define GS_FADE_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), GS_TYPE_FADE, GSFadeClass))


typedef enum {
        GS_FADE_DIRECTION_IN,
        GS_FADE_DIRECTION_OUT,
} GsFadeDirection;

typedef struct GSFadePrivate GSFadePrivate;

typedef struct
{
        GObject        parent;
        GSFadePrivate *priv;
} GSFade;

typedef struct
{
        GObjectClass   parent_class;

        void          (* faded)        (GSFade          *fade,
                                        GsFadeDirection  direction);
} GSFadeClass;

GType       gs_fade_get_type         (void);
GSFade    * gs_fade_new              (void);
void        gs_fade_set_timeout      (GSFade *fade,
                                      guint   timeout);
void        gs_fade_in               (GSFade *fade);
void        gs_fade_out              (GSFade *fade);
void        gs_fade_complete         (GSFade *fade);
void        gs_fade_reset            (GSFade *fade);
gboolean    gs_fade_is_fading        (GSFade *fade);
gboolean    gs_fade_is_fading_in     (GSFade *fade);
gboolean    gs_fade_is_fading_out    (GSFade *fade);

#endif /* __GS_FADE_H */
