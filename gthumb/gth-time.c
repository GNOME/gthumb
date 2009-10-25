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
 
#include <config.h>
#include <stdlib.h>
#include "gth-time.h"


#define INVALID_HOUR (24)
#define INVALID_MIN (60)
#define INVALID_SEC (60)
#define INVALID_USEC (1000000)


GthTime * 
gth_time_new (void)
{
	GthTime *time;
	
	time = g_new (GthTime, 1);
	gth_time_clear (time);
	
	return time;
}


void
gth_time_free (GthTime *time)
{
	g_free (time);
}


void
gth_time_clear (GthTime *time)
{
	time->hour = INVALID_HOUR;
	time->min = INVALID_MIN;
	time->sec = INVALID_SEC;
	time->usec = INVALID_USEC;
}


gboolean 
gth_time_valid (GthTime *time)
{ 
	return (time->hour < INVALID_HOUR) && (time->min < INVALID_MIN) && (time->sec < INVALID_SEC) && (time->usec < INVALID_USEC);
}


void
gth_time_set_hms (GthTime *time, 
	 	  guint8   hour, 
		  guint8   min, 
		  guint8   sec,
		  guint    usec)
{
	time->hour = hour;
	time->min = min;
	time->sec = sec;
	time->usec = usec;
}


GthDateTime *
gth_datetime_new (void)
{
	GthDateTime *dt;

	dt = g_new0 (GthDateTime, 1);
	dt->date = g_date_new ();
	dt->time = gth_time_new ();

	return dt;
}


void
gth_datetime_free (GthDateTime *dt)
{
	g_date_free (dt->date);
	gth_time_free (dt->time);
	g_free (dt);
}


void
gth_datetime_clear (GthDateTime *dt)
{
	gth_time_clear (dt->time);
	g_date_clear (dt->date, 1);
}


gboolean
gth_datetime_valid (GthDateTime *dt)
{
	return gth_time_valid (dt->time) && g_date_valid (dt->date);
}


gboolean
gth_datetime_from_exif_date (GthDateTime *dt,
			     const char  *exif_date)
{
	GDateYear   year;
	GDateMonth  month;
	GDateDay    day;
	long        val;

	g_return_val_if_fail (exif_date != NULL, FALSE);
	g_return_val_if_fail (dt != NULL, FALSE);

	while (g_ascii_isspace (*exif_date))
		exif_date++;

	if (*exif_date == '\0')
		return FALSE;

	if (! g_ascii_isdigit (*exif_date))
		return FALSE;

	/* YYYY */

	val = g_ascii_strtoull (exif_date, (char **)&exif_date, 10);
	year = val;
	
	if (*exif_date != ':')
		return FALSE;
	
	/* MM */
	
	exif_date++;
	month = g_ascii_strtoull (exif_date, (char **)&exif_date, 10);
      
	if (*exif_date != ':')
		return FALSE;

	/* DD */
      
	exif_date++;
	day = g_ascii_strtoull (exif_date, (char **)&exif_date, 10);
	
  	if (*exif_date != ' ')
		return FALSE;
  	
	g_date_set_dmy (dt->date, day, month, year);

  	/* hh */
  	
  	val = g_ascii_strtoull (exif_date, (char **)&exif_date, 10);
  	dt->time->hour = val;
  	
  	if (*exif_date != ':')
		return FALSE;
  	
  	/* mm */
  	
	exif_date++;
	dt->time->min = g_ascii_strtoull (exif_date, (char **)&exif_date, 10);
      
	if (*exif_date != ':')
		return FALSE;
      
      	/* ss */
      
	exif_date++;
	dt->time->sec = strtoul (exif_date, (char **)&exif_date, 10);

	/* usec */

	dt->time->usec = 0;
	if ((*exif_date == ',') || (*exif_date == '.')) {
		glong mul = 100000;

		while (g_ascii_isdigit (*++exif_date)) {
			dt->time->usec += (*exif_date - '0') * mul;
			mul /= 10;
		}
	}

	while (g_ascii_isspace (*exif_date))
		exif_date++;

	return *exif_date == '\0';
}


gboolean
gth_datetime_from_struct_tm (GthDateTime *dt,
			     struct tm   *tm)
{
	if (tm->tm_hour < 0) {
		gth_datetime_clear (dt);
	}
	else {
		gth_time_set_hms (dt->time, tm->tm_hour, tm->tm_min, tm->tm_sec, 0);
		g_date_set_dmy (dt->date, tm->tm_mday, tm->tm_mon + 1, 1900 + tm->tm_year);
	}
}


char *
gth_datetime_to_exif_date (GthDateTime *dt)
{
	if (gth_datetime_valid (dt))
		return g_strdup_printf ("%4d:%02d:%02d %02d:%02d:%02d",
					g_date_get_year (dt->date),
					g_date_get_month (dt->date),
					g_date_get_day (dt->date),
					dt->time->hour,
					dt->time->min,
					dt->time->sec);
	else
		return g_strdup ("");
}


void
gth_datetime_to_struct_tm (GthDateTime *dt,
		           struct tm   *tm)
{
	g_date_to_struct_tm (dt->date, tm);
	tm->tm_hour = dt->time->hour;
	tm->tm_min = dt->time->min;
	tm->tm_sec = dt->time->sec;
}


/* started from the glib function g_date_strftime */
char *
gth_datetime_strftime (GthDateTime *dt,
		       const char  *format)
{
	gsize   locale_format_len = 0;
	char   *locale_format;
	GError *error = NULL;
	struct  tm tm;	
	gsize   tmpbufsize;
	char   *tmpbuf;
	gsize   tmplen;
	char   *retval;
		
	locale_format = g_locale_from_utf8 (format, -1, NULL, &locale_format_len, &error);
	if (error != NULL) {
		g_warning (G_STRLOC "Error converting format to locale encoding: %s\n", error->message);
		g_error_free (error);
		return NULL;
	}

	gth_datetime_to_struct_tm (dt, &tm);
	tmpbufsize = MAX (128, locale_format_len * 2);
	while (TRUE) {
		tmpbuf = g_malloc (tmpbufsize);

		/* Set the first byte to something other than '\0', to be able to
		 * recognize whether strftime actually failed or just returned "".
		 */
		tmpbuf[0] = '\1';
		tmplen = strftime (tmpbuf, tmpbufsize, locale_format, &tm);

		if ((tmplen == 0) && (tmpbuf[0] != '\0')) {
			g_free (tmpbuf);
			tmpbufsize *= 2;

			if (tmpbufsize > 65536) {
				g_warning (G_STRLOC "Maximum buffer size for gth_datetime_strftime exceeded: giving up\n");
				g_free (locale_format);
				return NULL;
			}
		}
		else
			break;
	}
	g_free (locale_format);

	retval = g_locale_to_utf8 (tmpbuf, tmplen, NULL, NULL, &error);
	if (error != NULL) {
		g_warning (G_STRLOC "Error converting results of strftime to UTF-8: %s\n", error->message);
		g_error_free (error);
		return NULL;
	}

	g_free (tmpbuf);

	return retval;
}

