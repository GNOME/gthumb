#include "lib/gth-point.h"
#include "lib/util.h"

double gth_point_distance (GthPoint *p1, GthPoint *p2) {
	return sqrt (SQR (p1->x - p2->x) + SQR (p1->y - p2->y));
}
