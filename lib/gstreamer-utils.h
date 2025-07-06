#ifndef GSTREAMER_UTILS_H
#define GSTREAMER_UTILS_H

#include <glib.h>
#include <gio/gio.h>
#include <gst/gst.h>

G_BEGIN_DECLS

gboolean gstreamer_init (void);
gboolean gstreamer_read_metadata_from_file (GFile *file, GFileInfo *info, GCancellable *cancellable, GError **error);
/*GdkPixbuf * _gst_playbin_get_current_frame    (GstElement          *playbin,
					       GError             **error);
GthImage *  gstreamer_thumbnail_generator     (GInputStream        *istream,
					       GthFileData         *file_data,
					       int                  requested_size,
					       int                 *original_width,
					       int                 *original_height,
					       gboolean            *loaded_original,
					       gpointer             user_data,
					       GCancellable        *cancellable,
					       GError             **error);*/

G_END_DECLS

#endif /* GSTREAMER_UTILS_H */
