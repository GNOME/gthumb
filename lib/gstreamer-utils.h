#ifndef GSTREAMER_UTILS_H
#define GSTREAMER_UTILS_H

#include <glib.h>
#include <gio/gio.h>
#include <gst/gst.h>

G_BEGIN_DECLS

gboolean gstreamer_init (void);
gboolean read_video_metadata (GFile *file, GFileInfo *info, GCancellable *cancellable, GError **error);

/*GdkPixbuf * _gst_playbin_get_current_frame    (GstElement          *playbin,
					       GError             **error);*/

G_END_DECLS

#endif /* GSTREAMER_UTILS_H */
