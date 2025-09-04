#include "lib/util.h"
#include "lib/pixel.h"


gboolean scale_keeping_ratio_min (
	guint *width,
	guint *height,
	guint min_width,
	guint min_height,
	guint max_width,
	guint max_height,
	gboolean allow_upscaling)
{
	if ((*width < max_width) && (*height < max_height) && !allow_upscaling)
		return FALSE;

	if (((*width < min_width) || (*height < min_height)) && !allow_upscaling)
		return FALSE;

	double w = *width;
	double h = *height;
	double min_w = min_width;
	double min_h = min_height;
	double max_w = max_width;
	double max_h = max_height;
	double factor = MAX (MIN (max_w / w, max_h / h), MAX (min_w / w, min_h / h));
	guint new_width  = (guint) MAX (floor (w * factor + 0.50), 1);
	guint new_height = (guint) MAX (floor (h * factor + 0.50), 1);
	gboolean modified = (new_width != *width) || (new_height != *height);

	*width = new_width;
	*height = new_height;

	return modified;
}


gboolean scale_keeping_ratio (
	guint *width,
	guint *height,
	guint max_width,
	guint max_height,
	gboolean allow_upscaling)
{
	return scale_keeping_ratio_min (
		width,
		height,
		0,
		0,
		max_width,
		max_height,
		allow_upscaling);
}


gboolean scale_if_larger (
	guint *width,
	guint *height,
	guint size)
{
	return scale_keeping_ratio (width, height, size, size, FALSE);
}


gboolean transformation_changes_size (GthTransform transform) {
	return (transform == GTH_TRANSFORM_ROTATE_90)
		|| (transform == GTH_TRANSFORM_ROTATE_270)
		|| (transform == GTH_TRANSFORM_TRANSPOSE)
		|| (transform == GTH_TRANSFORM_TRANSVERSE);
}


void get_transformation_steps (
	GthTransform transform,
	int width,
	int height,
	int *destination_width_p,
	int *destination_height_p,
	int *line_start_p,
	int *line_step_p,
	int *pixel_step_p)
{
	int destination_stride;
	int destination_width = 0;
	int destination_height = 0;
	int line_start = 0;
	int line_step = 0;
	int pixel_step = 0;

	switch (transform) {
	case GTH_TRANSFORM_NONE:
	default:
		destination_width = width;
		destination_height = height;
		destination_stride = destination_width * PIXEL_BYTES;
		line_start = 0;
		line_step = destination_stride;
		pixel_step = PIXEL_BYTES;
		break;

	case GTH_TRANSFORM_FLIP_H:
		destination_width = width;
		destination_height = height;
		destination_stride = destination_width * PIXEL_BYTES;
		line_start = (destination_width - 1) * PIXEL_BYTES;
		line_step = destination_stride;
		pixel_step = - PIXEL_BYTES;
		break;

	case GTH_TRANSFORM_ROTATE_180:
		destination_width = width;
		destination_height = height;
		destination_stride = destination_width * PIXEL_BYTES;
		line_start = ((destination_height - 1) * destination_stride) + ((destination_width - 1) * PIXEL_BYTES);
		line_step = - destination_stride;
		pixel_step = - PIXEL_BYTES;
		break;

	case GTH_TRANSFORM_FLIP_V:
		destination_width = width;
		destination_height = height;
		destination_stride = destination_width * PIXEL_BYTES;
		line_start = (destination_height - 1) * destination_stride;
		line_step = -destination_stride;
		pixel_step = PIXEL_BYTES;
		break;

	case GTH_TRANSFORM_TRANSPOSE:
		destination_width = height;
		destination_height = width;
		destination_stride = destination_width * PIXEL_BYTES;
		line_start = 0;
		line_step = PIXEL_BYTES;
		pixel_step = destination_stride;
		break;

	case GTH_TRANSFORM_ROTATE_90:
		destination_width = height;
		destination_height = width;
		destination_stride = destination_width * PIXEL_BYTES;
		line_start = (destination_width - 1) * PIXEL_BYTES;
		line_step = - PIXEL_BYTES;
		pixel_step = destination_stride;
		break;

	case GTH_TRANSFORM_TRANSVERSE:
		destination_width = height;
		destination_height = width;
		destination_stride = destination_width * PIXEL_BYTES;
		line_start = ((destination_height - 1) * destination_stride) + ((destination_width - 1) * PIXEL_BYTES);
		line_step = - PIXEL_BYTES;
		pixel_step = - destination_stride;
		break;

	case GTH_TRANSFORM_ROTATE_270:
		destination_width = height;
		destination_height = width;
		destination_stride = destination_width * PIXEL_BYTES;
		line_start = (destination_height - 1) * destination_stride;
		line_step = PIXEL_BYTES;
		pixel_step = - destination_stride;
		break;
	}

	if (destination_width_p != NULL)
		*destination_width_p = destination_width;
	if (destination_height_p != NULL)
		*destination_height_p = destination_height;
	if (line_start_p != NULL)
		*line_start_p = line_start;
	if (line_step_p != NULL)
		*line_step_p = line_step;
	if (pixel_step_p != NULL)
		*pixel_step_p = pixel_step;
}

char * _g_utf8_try_from_any (const char *str) {
	if (str == NULL)
		return NULL;
	char *utf8_str;
	if (!g_utf8_validate (str, -1, NULL))
		utf8_str = g_locale_to_utf8 (str, -1, NULL, NULL, NULL);
	else
		utf8_str = g_strdup (str);
	return utf8_str;
}

char * _g_utf8_from_any (const char *str) {
	if (str == NULL)
		return NULL;
	char *utf8_str = _g_utf8_try_from_any (str);
	return (utf8_str != NULL) ? utf8_str : g_strdup (_("(invalid value)"));
}

char * _g_utf8_replace (const char *str, const char *from, const char *to) {
	GString *result = g_string_new (str);
	g_string_replace (result, from, to, 0);
	return g_string_free (result, FALSE);
}

gboolean _g_utf8_all_spaces (const char *str) {
	while (str != NULL) {
		gunichar ch = g_utf8_get_char (str);
		if (ch == 0)
			break;
		if (!g_unichar_isspace (ch))
			return FALSE;
		str = g_utf8_next_char (str);
	}
	return TRUE;
}

void _g_string_list_free (GList *string_list) {
	if (string_list == NULL)
		return;
	g_list_foreach (string_list, (GFunc) g_free, NULL);
	g_list_free (string_list);
}

GList * _g_string_list_dup (GList *string_list) {
	GList *new_list = NULL;
	for (GList *scan = string_list; scan; scan = scan->next)
		new_list = g_list_prepend (new_list, g_strdup (scan->data));
	return g_list_reverse (new_list);
}

gpointer _g_object_ref (gpointer object) {
	if (object != NULL)
		g_object_ref (object);
	return object;
}

void _g_object_unref (gpointer object) {
	if (object != NULL)
		g_object_unref (object);
}

void _g_str_set (char **str, const char *value) {
	if (*str == value)
		return;
	if (*str != NULL) {
		g_free (*str);
		*str = NULL;
	}
	if (value != NULL) {
		*str = g_strdup (value);
	}
}

const char * _g_utf8_after_ascii_space (const char *str) {
	while (str != NULL) {
		gunichar c = g_utf8_get_char (str);
		if (c == 0)
			break;
		if (c == ' ')
			return g_utf8_next_char (str);
		str = g_utf8_next_char (str);
	}
	return NULL;
}

const char *Known_Properties[] = { "lang=", "charset=" };

char * _g_utf8_remove_string_properties (const char *str) {
	const char *after_properties = str;

	for (int i = 0; i < G_N_ELEMENTS (Known_Properties); /* void */) {
		if (g_str_has_prefix (after_properties, Known_Properties[i])) {
			after_properties = _g_utf8_after_ascii_space (after_properties);
			i = 0;
		}
		else {
			i++;
		}
	}

	return (after_properties != NULL) ? g_strdup (after_properties) : NULL;
}

static int try_parse_int (const char *chars, int *idx, int expected_digits) {
	int digits = 0;
	int result = 0;
	int index = *idx;
	while ((chars[index] != 0) && (digits < expected_digits)) {
		char ch = chars[index];
		if ((ch >= '0') && (ch <= '9')) {
			digits++;
			result = (result * 10) + (ch - '0');
			index++;
		}
		else {
			break;
		}
	}
	*idx = index;
	return (digits == expected_digits) ? result : -1;
}

bool _g_parse_exif_date (const char *exif_date,
	int *out_year, int *out_month, int *out_day,
	int *out_hour, int *out_minute, int *out_second,
	double *out_usecond)
{
	if (exif_date == NULL) {
		//g_print ("> [1]\n");
		return FALSE;
	}

	int idx = 0;
	while (exif_date[idx] == ' ') {
		idx++;
	}

	if (exif_date[idx] == 0) {
		//g_print ("> [2]\n");
		return FALSE;
	}

	int year = try_parse_int (exif_date, &idx, 4);
	if ((year < 1) || (year >= 10000)) {
		//g_print ("> [3]\n");
		return FALSE;
	}

	if (exif_date[idx++] != ':') {
		//g_print ("> [4]\n");
		return FALSE;
	}

	int month = try_parse_int (exif_date, &idx, 2);
	if ((month < 1) || (month > 12)) {
		//g_print ("> [5]\n");
		return FALSE;
	}

	if (exif_date[idx++] != ':') {
		//g_print ("> [6]\n");
		return FALSE;
	}

	int day = try_parse_int (exif_date, &idx, 2);
	if ((day < 1) || (day > 31)) {
		//g_print ("> [7]\n");
		return FALSE;
	}

	if (exif_date[idx++] != ' ') {
		//g_print ("> [8]\n");
		return FALSE;
	}

	int hour = try_parse_int (exif_date, &idx, 2);
	if ((hour < 0) || (hour > 23)) {
		//g_print ("> [9]\n");
		return FALSE;
	}

	if (exif_date[idx++] != ':') {
		//g_print ("> [10]\n");
		return FALSE;
	}

	int minute = try_parse_int (exif_date, &idx, 2);
	if ((minute < 0) || (minute > 59)) {
		//g_print ("> [11]\n");
		return FALSE;
	}

	if (exif_date[idx++] != ':') {
		//g_print ("> [12]\n");
		return FALSE;
	}

	int second = try_parse_int (exif_date, &idx, 2);
	if ((second < 0) || (second > 59)) {
		//g_print ("> [13]\n");
		return FALSE;
	}

	double usecond = 0.0;
	if ((exif_date[idx] == ',') || (exif_date[idx] == '.')) {
		idx++;
		double weight = 0.1;
		while ((weight > 0.0) && (exif_date[idx] != 0)) {
			char ch = exif_date[idx];
			if ((ch >= '0') && (ch <= '9')) {
				usecond += weight * (ch - '0');
				weight /= 10.0;
				idx++;
			}
			else {
				break;
			}
		}
	}

	while (exif_date[idx] == ' ')
		idx++;

	if (exif_date[idx] != 0) {
		//g_print ("> [14]\n");
		return FALSE;
	}

	if (out_year != NULL)
		*out_year = year;
	if (out_month != NULL)
		*out_month = month;
	if (out_day != NULL)
		*out_day = day;
	if (out_hour != NULL)
		*out_hour = hour;
	if (out_minute != NULL)
		*out_minute = minute;
	if (out_second != NULL)
		*out_second = second;
	if (out_usecond != NULL)
		*out_usecond = usecond;
	return TRUE;
}

GDateTime * _g_date_time_new_from_exif_date (const char *exif_date) {
	int year, month, day, hour, minute, second;
	double usecond;
	if (!_g_parse_exif_date (exif_date, &year, &month, &day, &hour, &minute, &second, &usecond))
		return NULL;
	return g_date_time_new_local (year, month, day, hour, minute, (double) second + usecond);
}

char * _g_date_time_to_exif_date (GDateTime *date_time) {
	return g_strdup_printf ("%4d:%02d:%02d %02d:%02d:%02d",
		g_date_time_get_year (date_time),
		g_date_time_get_month (date_time),
		g_date_time_get_day_of_month (date_time),
		g_date_time_get_hour (date_time),
		g_date_time_get_minute (date_time),
		g_date_time_get_second (date_time));
}

char * _g_date_time_to_xmp_date (GDateTime *date_time) {
	GTimeSpan utf_offset = g_date_time_get_utc_offset (date_time) / 1000000;
	return g_strdup_printf ("%4d-%02d-%02dT%02d:%02d:%02d%+03d:%02d",
		g_date_time_get_year (date_time),
		g_date_time_get_month (date_time),
		g_date_time_get_day_of_month (date_time),
		g_date_time_get_hour (date_time),
		g_date_time_get_minute (date_time),
		g_date_time_get_second (date_time),
		utf_offset / 3600,
		utf_offset % 3600);
}

const char * guess_mime_type (const guchar* buffer, gsize buffer_size) {
	static const struct MagicInfo {
		const char * const mime_type;
		const unsigned int offset;
		const unsigned int bytes;
		const char * const id;
	} MAGIC_IDS[] = {
		// Some magic ids taken from file-4.21 tarball.
		{ "image/png",  0,  8, "\x89PNG\x0d\x0a\x1a\x0a" },
		{ "image/tiff", 0,  4, "MM\x00\x2a" },
		{ "image/tiff", 0,  4, "II\x2a\x00" },
		{ "image/gif",  0,  4, "GIF8" },
		{ "image/jpeg", 0,  3, "\xff\xd8\xff" },
		{ "image/webp", 8,  7, "WEBPVP8" },
		{ "image/jxl",  0,  2, "\xff\x0a" },
		{ "image/jxl",  0, 12, "\x00\x00\x00\x0cJXL\x20\x0d\x0a\x87\x0a" },
		{ "image/heic", 4, 8, "ftypmif1" },
		{ "image/heic", 4, 8, "ftypmsf1" },
		{ "image/heic", 4, 8, "ftypheic" },
		{ "image/heic", 4, 8, "ftypheix" },
		{ "image/heic", 4, 8, "ftyphevx" },
		{ "image/heic", 4, 8, "ftypheim" },
		{ "image/heic", 4, 8, "ftypheis" },
		{ "image/heic", 4, 8, "ftypavic" },
		{ "image/heic", 4, 8, "ftyphevm" },
		{ "image/heic", 4, 8, "ftyphevs" },
		{ "image/heic", 4, 8, "ftypavcs" },
		{ "image/avif", 4, 8, "ftypavif" },
		{ "image/avif", 4, 8, "ftypavis" },
		{ "image/avif", 4, 8, "ftyprisx" },
		{ "image/avif", 4, 8, "ftypROSS" },
		{ "image/avif", 4, 8, "ftypsdv " },
		{ "image/avif", 4, 8, "ftypssc1" },
		{ "image/avif", 4, 8, "ftypssc2" },
		{ "image/avif", 4, 8, "ftypSEAU" },
		{ "image/avif", 4, 8, "ftypSEBK" },
		{ "image/avif", 4, 8, "ftypsenv" },
		{ "image/avif", 4, 8, "ftypsims" },
		{ "image/avif", 4, 8, "ftypsisx" },
		{ "image/avif", 4, 8, "ftypssss" },
		{ "image/avif", 4, 8, "ftypuvvu" },
	};

	for (int i = 0; i < G_N_ELEMENTS (MAGIC_IDS); i++) {
		const struct MagicInfo * const info = &MAGIC_IDS[i];

		if ((info->offset + info->bytes) <= buffer_size) {
			if (memcmp (buffer + info->offset, info->id, info->bytes) == 0)
				return info->mime_type;
		}
	}
	return NULL;
}

char * _g_format_duration_not_localized (gint64 msecs) {
	int sec, min, hour, _time;

	_time = (int) (msecs / 1000);
	sec = _time % 60;
	_time = _time - sec;
	min = (_time % (60*60)) / 60;
	_time = _time - (min * 60);
	hour = _time / (60*60);

	return g_strdup_printf ("%d∶%02d∶%02d", hour, min, sec);
}

/* this is totem_time_to_string renamed, thanks to the authors :) */
char * _g_format_duration_for_display (gint64 msecs, int *hours, int *minutes) {
	int sec, min, hour, _time;

	_time = (int) (msecs / 1000);
	sec = _time % 60;
	_time = _time - sec;
	min = (_time % (60*60)) / 60;
	_time = _time - (min * 60);
	hour = _time / (60*60);

	if (hours != NULL) {
		*hours = hour;
	}
	if (minutes != NULL) {
		*minutes = min;
	}

	if (hour > 0) {
		/* hour:minutes:seconds */
		/* Translators: This is a time format, like "9∶05∶02" for 9
		* hours, 5 minutes, and 2 seconds. You may change "∶" to
		* the separator that your locale uses or use "%Id" instead
		* of "%d" if your locale uses localized digits.
		*/
		return g_strdup_printf (C_("long time format", "%d∶%02d∶%02d"), hour, min, sec);
	}

	/* minutes:seconds */
	/* Translators: This is a time format, like "5∶02" for 5
	* minutes and 2 seconds. You may change "∶" to the
	* separator that your locale uses or use "%Id" instead of
	* "%d" if your locale uses localized digits.
	*/
	return g_strdup_printf (C_("short time format", "%d∶%02d"), min, sec);
}

char * _g_format_double (double value, int max_decimal_digits) {
	double int_part, fract_part;
	fract_part = modf (value, &int_part);

	GString *result = g_string_new ("");
	if (int_part < 0) {
		g_string_append_c (result, '-');
	}

	// Integer
	char *local_format = g_strdup_printf ("%d", (int) int_part);
	char *p = local_format;
	while (*p != 0) {
		if ((*p >= '0') && (*p <= '9')) {
			g_string_append_c (result, *p);
		}
		p++;
	}
	g_free (local_format);

	// Dot
	g_string_append_c (result, '.');

	// Fractional
	if (fract_part < 0) {
		fract_part = -fract_part;
	}
	local_format = g_strdup_printf ("%f", fract_part);
	p = local_format;
	gboolean after_symbol = FALSE;
	int digits = 0;
	while (*p != 0) {
		if ((*p >= '0') && (*p <= '9')) {
			if (after_symbol) {
				g_string_append_c (result, *p);
				digits++;
				if (digits >= max_decimal_digits) {
					break;
				}
			}
		}
		else if (!after_symbol) {
			after_symbol = TRUE;
		}
		p++;
	}
	g_free (local_format);
	return g_string_free (result, FALSE);
}


// _g_file_info_swap_attributes


typedef struct {
	GFileAttributeType type;
	union {
		char      *string;
		char     **stringv;
		gboolean   boolean;
		guint32    uint32;
		gint32     int32;
		guint64    uint64;
		gint64     int64;
		gpointer   object;
	} v;
} _GFileAttributeValue;


static void _g_file_attribute_value_free (_GFileAttributeValue *attr_value) {
	switch (attr_value->type) {
	case G_FILE_ATTRIBUTE_TYPE_STRING:
	case G_FILE_ATTRIBUTE_TYPE_BYTE_STRING:
		g_free (attr_value->v.string);
		break;
	case G_FILE_ATTRIBUTE_TYPE_STRINGV:
		g_strfreev (attr_value->v.stringv);
		break;
	case G_FILE_ATTRIBUTE_TYPE_OBJECT:
		g_object_unref (attr_value->v.object);
		break;
	default:
		break;
	}

	g_free (attr_value);
}


static _GFileAttributeValue * _g_file_info_get_value (GFileInfo *info, const char *attr) {
	_GFileAttributeValue  *attr_value;
	GFileAttributeType     type;
	gpointer               value;
	GFileAttributeStatus   status;

	attr_value = g_new (_GFileAttributeValue, 1);
	attr_value->type = G_FILE_ATTRIBUTE_TYPE_INVALID;

	if (! g_file_info_get_attribute_data (info, attr, &type, &value, &status))
		return attr_value;

	attr_value->type = type;
	switch (type) {
	case G_FILE_ATTRIBUTE_TYPE_INVALID:
		break;
	case G_FILE_ATTRIBUTE_TYPE_STRING:
	case G_FILE_ATTRIBUTE_TYPE_BYTE_STRING:
		attr_value->v.string = g_strdup ((char *) value);
		break;
	case G_FILE_ATTRIBUTE_TYPE_STRINGV:
		attr_value->v.stringv = g_strdupv ((char **) value);
		break;
	case G_FILE_ATTRIBUTE_TYPE_BOOLEAN:
		attr_value->v.boolean = * ((gboolean *) value);
		break;
	case G_FILE_ATTRIBUTE_TYPE_UINT32:
		attr_value->v.uint32 = * ((guint32 *) value);
		break;
	case G_FILE_ATTRIBUTE_TYPE_INT32:
		attr_value->v.int32 = * ((gint32 *) value);
		break;
	case G_FILE_ATTRIBUTE_TYPE_UINT64:
		attr_value->v.uint64 = * ((guint64 *) value);
		break;
	case G_FILE_ATTRIBUTE_TYPE_INT64:
		attr_value->v.int64 = * ((gint64 *) value);
		break;
	case G_FILE_ATTRIBUTE_TYPE_OBJECT:
		attr_value->v.object = g_object_ref ((GObject *) value);
		break;
	default:
		g_warning ("unknown attribute type: %d", type);
		break;
	}

	return attr_value;
}


static void _g_file_info_set_value (GFileInfo *info, const char *attr, _GFileAttributeValue *attr_value) {
	gpointer value = NULL;

	if (attr_value->type == G_FILE_ATTRIBUTE_TYPE_INVALID)
		return;

	switch (attr_value->type) {
	case G_FILE_ATTRIBUTE_TYPE_STRING:
	case G_FILE_ATTRIBUTE_TYPE_BYTE_STRING:
		value = attr_value->v.string;
		break;
	case G_FILE_ATTRIBUTE_TYPE_STRINGV:
		value = attr_value->v.stringv;
		break;
	case G_FILE_ATTRIBUTE_TYPE_BOOLEAN:
		value = &attr_value->v.boolean;
		break;
	case G_FILE_ATTRIBUTE_TYPE_UINT32:
		value = &attr_value->v.uint32;
		break;
	case G_FILE_ATTRIBUTE_TYPE_INT32:
		value = &attr_value->v.int32;
		break;
	case G_FILE_ATTRIBUTE_TYPE_UINT64:
		value = &attr_value->v.uint64;
		break;
	case G_FILE_ATTRIBUTE_TYPE_INT64:
		value = &attr_value->v.int64;
		break;
	case G_FILE_ATTRIBUTE_TYPE_OBJECT:
		value = attr_value->v.object;
		break;
	default:
		g_warning ("Unknown attribute type: %d", attr_value->type);
		break;
	}

	g_file_info_set_attribute (info, attr, attr_value->type, value);
}


void _g_file_info_swap_attributes (GFileInfo *info, const char *attr1, const char *attr2) {
	_GFileAttributeValue *value1;
	_GFileAttributeValue *value2;

	value1 = _g_file_info_get_value (info, attr1);
	value2 = _g_file_info_get_value (info, attr2);

	_g_file_info_set_value (info, attr1, value2);
	_g_file_info_set_value (info, attr2, value1);

	_g_file_attribute_value_free (value1);
	_g_file_attribute_value_free (value2);
}


void _g_file_info_copy_attributes (GFileInfo *src, GFileInfo *dest) {
	char **attributes = g_file_info_list_attributes (src, NULL);
	if (attributes == NULL) {
		return;
	}

	for (int idx = 0; attributes[idx] != NULL; idx++) {
		const char *attr = attributes[idx];
		GFileAttributeType attr_type;
		gpointer attr_value_p;
		GFileAttributeStatus attr_status;

		g_print ("  COPY ATTRIBUTE %s [1]\n", attr);

		if (!g_file_info_get_attribute_data (src, attr, &attr_type,
			&attr_value_p, &attr_status)) {
			g_print ("  [1.1]\n");
			continue;
		}

		/*if (attr_status != G_FILE_ATTRIBUTE_STATUS_SET) {
			g_print ("  [1.2]\n");
			continue;
		}*/

		g_print ("  COPY ATTRIBUTE %s [2]\n", attr);

		switch (attr_type) {
		case G_FILE_ATTRIBUTE_TYPE_STRING:
			g_file_info_set_attribute_string (dest, attr, (char *) attr_value_p);
			break;
		case G_FILE_ATTRIBUTE_TYPE_BYTE_STRING:
			g_file_info_set_attribute_byte_string (dest, attr, (char *) attr_value_p);
			break;
		case G_FILE_ATTRIBUTE_TYPE_STRINGV:
			g_file_info_set_attribute_stringv (dest, attr, (char **) attr_value_p);
			break;
		case G_FILE_ATTRIBUTE_TYPE_BOOLEAN:
			g_file_info_set_attribute_boolean (dest, attr, * ((gboolean *) attr_value_p));
			break;
		case G_FILE_ATTRIBUTE_TYPE_UINT32:
			g_file_info_set_attribute_uint32 (dest, attr, * ((guint32 *) attr_value_p));
			break;
		case G_FILE_ATTRIBUTE_TYPE_INT32:
			g_file_info_set_attribute_int32 (dest, attr, * ((gint32 *) attr_value_p));
			break;
		case G_FILE_ATTRIBUTE_TYPE_UINT64:
			g_file_info_set_attribute_uint64 (dest, attr, * ((guint64 *) attr_value_p));
			break;
		case G_FILE_ATTRIBUTE_TYPE_INT64:
			g_file_info_set_attribute_int64 (dest, attr, * ((gint64 *) attr_value_p));
			break;
		case G_FILE_ATTRIBUTE_TYPE_OBJECT:
			g_file_info_set_attribute_object (dest, attr, (GObject *) attr_value_p);
			break;
		default:
			g_warning ("Unknown attribute type: %d", attr_type);
			break;
		}
	}

	g_strfreev (attributes);
}
