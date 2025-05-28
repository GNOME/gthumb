#ifndef LOAD_PNG_H
#define LOAD_PNG_H

#include <gtk/gtk.h>
#include "lib/gth-image.h"

G_BEGIN_DECLS

GthImage * load_png (GBytes		 *buffer,
		     guint		  requested_size,
		     GCancellable	 *cancellable,
		     GError		**error);

G_END_DECLS

#endif /* LOAD_PNG_H */
