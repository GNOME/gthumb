#ifndef EXIV2_UTILS_H
#define EXIV2_UTILS_H

#include <glib.h>
#include <gio/gio.h>

G_BEGIN_DECLS

extern const char *_LAST_DATE_TAG_NAMES[];
extern const char *_ORIGINAL_DATE_TAG_NAMES[];
extern const char *_DESCRIPTION_TAG_NAMES[];
extern const char *_TITLE_TAG_NAMES[];
extern const char *_LOCATION_TAG_NAMES[];
extern const char *_KEYWORDS_TAG_NAMES[];
extern const char *_RATING_TAG_NAMES[];

// gboolean   exiv2_read_metadata_from_file    (GFile             *file,
					     // GFileInfo         *info,
					     // gboolean           update_general_attributes,
					     // GCancellable      *cancellable,
					     // GError           **error);

void exiv2_update_general_attributes (GFileInfo *info);

gboolean exiv2_read_metadata_from_buffer (
	void *buffer,
	gsize buffer_size,
	GFileInfo *info,
	gboolean update_general_attributes,
	GError **error);

// GFile *    exiv2_get_sidecar                (GFile             *file);
// gboolean   exiv2_read_sidecar               (GFile             *file,
					     // GFileInfo         *info,
					     // gboolean           update_general_attributes);
// gboolean   exiv2_supports_writes            (const char        *mime_type);
// gboolean   exiv2_write_metadata  	    (GthImageSaveData  *data);
// gboolean   exiv2_write_metadata_to_buffer   (void              **buffer,
					     // gsize              *buffer_size,
					     // GFileInfo          *info,
					     // GthImage           *image_data, /* optional */
					     // GError           **error);
// gboolean   exiv2_clear_metadata             (void             **buffer,
					     // gsize             *buffer_size,
					     // GError           **error);
// GdkPixbuf *exiv2_generate_thumbnail         (const char        *uri,
					     // const char        *mime_type,
					     // int                size);

G_END_DECLS

#endif /* EXIV2_UTILS_H */
