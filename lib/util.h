#ifndef LIB_UTIL_H
#define LIB_UTIL_H

#include <math.h>
#include <glib.h>
#include <glib/gi18n-lib.h>
#include "lib/types.h"

G_BEGIN_DECLS

gboolean scale_keeping_ratio_min (
	guint *width,
	guint *height,
	guint min_width,
	guint min_height,
	guint max_width,
	guint max_height,
	gboolean allow_upscaling);

gboolean scale_keeping_ratio (
	guint *width,
	guint *height,
	guint max_width,
	guint max_height,
	gboolean allow_upscaling);

gboolean scale_if_larger (
	guint *width,
	guint *height,
	guint size);

gboolean transformation_changes_size (GthTransform transform);

void get_transformation_steps (
	GthTransform transform,
	int width,
	int height,
	int *destination_width_p,
	int *destination_height_p,
	int *line_start_p,
	int *line_step_p,
	int *pixel_step_p);

char * _g_utf8_try_from_any (const char *str);
char * _g_utf8_from_any (const char *str);
char * _g_utf8_replace (const char *str, const char *from, const char *to);
gboolean _g_utf8_all_spaces (const char *str);
void _g_string_list_free (GList *string_list);
GList * _g_string_list_dup (GList *string_list);
gpointer _g_object_ref (gpointer object);
void _g_object_unref (gpointer object);
void _g_str_set (char **str, const char *value);
const char * _g_utf8_after_ascii_space (const char *str);
char * _g_utf8_remove_string_properties (const char *str);
bool _g_parse_exif_date (const char *exif_date,
	int *out_year, int *out_month, int *out_day,
	int *out_hour, int *out_minute, int *out_second,
	double *out_usecond);
GDateTime * _g_date_time_new_from_exif_date (const char *exif_date);
const char * guess_mime_type (const guchar* buffer, gsize buffer_size);
char * _g_format_duration_for_display (gint64 msecs, int *hours, int *minutes);
char * _g_format_duration_not_localized (gint64 msecs);
char * _g_format_double (double value, int max_decimal_digits);

G_END_DECLS

#endif /* LIB_UTIL_H */
