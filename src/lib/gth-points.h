#ifndef GTH_POINTS_H
#define GTH_POINTS_H

#include <glib.h>
#include "lib/gth-point.h"
#include "lib/types.h"

G_BEGIN_DECLS

typedef struct {
	guint ref_count;
	GthPoint *value_points;
	int value_size;
	GthPoint *red_points;
	int red_size;
	GthPoint *green_points;
	int green_size;
	GthPoint *blue_points;
	int blue_size;
} GthPoints;

GthPoints * gth_points_new (GthPoint *value_points, int value_size,
	GthPoint *red_points, int red_size,
	GthPoint *green_points, int green_size,
	GthPoint *blue_points, int blue_size);
void gth_points_init (GthPoints *points,
	GthPoint *value_points, int value_size,
	GthPoint *red_points, int red_size,
	GthPoint *green_points, int green_size,
	GthPoint *blue_points, int blue_size);
void gth_points_ref (GthPoints *points);
void gth_points_unref (GthPoints *points);
GthPoint * gth_points_get_channel (GthPoints *points, GthChannel channel, int *size);
guchar * gth_points_get_value_map (GthPoints *points, int *size);

G_END_DECLS

#endif /* GTH_POINTS_H */
