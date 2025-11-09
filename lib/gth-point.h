#ifndef GTH_POINT_H
#define GTH_POINT_H

#include <glib.h>

G_BEGIN_DECLS

typedef struct {
	double x;
	double y;
} GthPoint;

double gth_point_distance (GthPoint *p1, GthPoint *p2);

G_END_DECLS

#endif /* GTH_POINT_H */
