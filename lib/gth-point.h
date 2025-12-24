#ifndef GTH_POINT_H
#define GTH_POINT_H

#include <glib.h>

G_BEGIN_DECLS

typedef struct {
	double x;
	double y;
} GthPoint;

void gth_point_init (GthPoint *p, double x, double y);
void gth_point_init_interpolate (GthPoint *result, GthPoint *p1, GthPoint *p2, double alpha);
double gth_point_distance (GthPoint *p1, GthPoint *p2);
char * gth_point_to_string (GthPoint *p);

G_END_DECLS

#endif /* GTH_POINT_H */
