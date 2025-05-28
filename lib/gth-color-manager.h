#ifndef GTH_COLOR_MANAGER_H
#define GTH_COLOR_MANAGER_H

#include <glib-object.h>
#include <gtk/gtk.h>
#include "lib/gth-icc-profile.h"

G_BEGIN_DECLS

#define GTH_TYPE_COLOR_MANAGER         (gth_color_manager_get_type ())
#define GTH_COLOR_MANAGER(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), GTH_TYPE_COLOR_MANAGER, GthColorManager))
#define GTH_COLOR_MANAGER_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST ((k), GTH_TYPE_COLOR_MANAGER, GthColorManagerClass))
#define GTH_IS_COLOR_MANAGER(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), GTH_TYPE_COLOR_MANAGER))
#define GTH_IS_COLOR_MANAGER_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), GTH_TYPE_COLOR_MANAGER))
#define GTH_COLOR_MANAGER_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS((o), GTH_TYPE_COLOR_MANAGER, GthColorManagerClass))

typedef struct _GthColorManager         GthColorManager;
typedef struct _GthColorManagerPrivate  GthColorManagerPrivate;
typedef struct _GthColorManagerClass    GthColorManagerClass;

struct _GthColorManager {
	GObject __parent;
	GthColorManagerPrivate *priv;
};

struct _GthColorManagerClass {
	GObjectClass __parent_class;

	/*< signals >*/

	void	(*changed)	(GthColorManager *color_manager);
};

GType			gth_color_manager_get_type		(void) G_GNUC_CONST;
GthColorManager *	gth_color_manager_new			(void);
void			gth_color_manager_get_profile_async	(GthColorManager	 *color_manager,
								 char 			 *monitor_name,
								 GCancellable		 *cancellable,
								 GAsyncReadyCallback	  callback,
								 gpointer		  user_data);
GthIccProfile *		gth_color_manager_get_profile_finish	(GthColorManager	 *color_manager,
								 GAsyncResult		 *result,
								 GError			**error);
GthIccTransform *	gth_color_manager_get_transform		(GthColorManager	 *color_manager,
								 GthIccProfile		 *from_profile,
								 GthIccProfile		 *to_profile);

G_END_DECLS

#endif /* GTH_COLOR_MANAGER_H */
