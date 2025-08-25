#ifndef EXIV2_UTILS_H
#define EXIV2_UTILS_H

#include <glib.h>
#include <gio/gio.h>
#include <lib/gth-image.h>

G_BEGIN_DECLS

extern const char *_LAST_DATE_TAG_NAMES[];
extern const char *_ORIGINAL_DATE_TAG_NAMES[];
extern const char *_DESCRIPTION_TAG_NAMES[];
extern const char *_TITLE_TAG_NAMES[];
extern const char *_LOCATION_TAG_NAMES[];
extern const char *_KEYWORDS_TAG_NAMES[];
extern const char *_RATING_TAG_NAMES[];

gboolean exiv2_read_metadata_from_buffer (GBytes *buffer, GFileInfo *info, gboolean update_general_attributes, GError **error);
int exiv2_get_coordinates (GFileInfo *info, double *out_latitude, double *out_longitude);
char * exiv2_decimal_coordinates_to_string (double latitude, double longitude);
gboolean exiv2_can_write_metadata (const char *mime_type);
GBytes * exiv2_write_metadata_to_buffer (GBytes *buffer, GFileInfo *info, GthImage *image_data, GError **error);
GBytes * exiv2_clear_metadata (GBytes *buffer, GError **error);

G_END_DECLS

#endif /* EXIV2_UTILS_H */
