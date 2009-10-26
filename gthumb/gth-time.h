/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2009 The Free Software Foundation, Inc.
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
  
#ifndef GTH_TIME_H
#define GTH_TIME_H

#include <glib.h>

G_BEGIN_DECLS

typedef struct {
	guint8 hour;
	guint8 min;
	guint8 sec;
	guint  usec;
} GthTime;

typedef struct {
	GDate   *date;
	GthTime *time;
} GthDateTime;


GthTime *     gth_time_new     		   (void);
void          gth_time_free    		   (GthTime     *time);
void          gth_time_clear   		   (GthTime     *time);
gboolean      gth_time_valid   		   (GthTime     *time);
void          gth_time_set_hms 		   (GthTime     *time, 
	 		 		    guint8       hour, 
		 			    guint8       min, 
					    guint8       sec,
				 	    guint        usec);
GthDateTime * gth_datetime_new             (void);
void          gth_datetime_free            (GthDateTime *dt);
void          gth_datetime_clear           (GthDateTime *dt);
gboolean      gth_datetime_valid           (GthDateTime *dt);
gboolean      gth_datetime_from_exif_date  (GthDateTime *dt,
					    const char  *exif_date);
void          gth_datetime_from_struct_tm  (GthDateTime *dt,
					    struct tm   *tm);
char *        gth_datetime_to_exif_date    (GthDateTime *dt);
void          gth_datetime_to_struct_tm    (GthDateTime *dt,
					    struct tm   *tm);
char *        gth_datetime_strftime        (GthDateTime *dt,
					    const char  *format);

G_END_DECLS

#endif /* GTH_TIME_H */

