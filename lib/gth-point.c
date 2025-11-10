#include "lib/gth-point.h"
#include "lib/util.h"

void gth_point_init (GthPoint *p, double x, double y) {
	p->x = x;
	p->y = y;
}

void gth_point_init_interpolate (GthPoint *p, GthPoint *start, GthPoint *end, double alpha) {
	p->x = (start->x * (1.0 - alpha)) + (end->x * alpha);
	p->y = (start->y * (1.0 - alpha)) + (end->y * alpha);
}

double gth_point_distance (GthPoint *p1, GthPoint *p2) {
	return sqrt (SQR (p1->x - p2->x) + SQR (p1->y - p2->y));
}
