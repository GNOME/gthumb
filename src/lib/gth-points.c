#include "lib/gth-points.h"
#include "lib/gth-curve.h"
#include "lib/util.h"

GthPoints * gth_points_new (GthPoint *value_points, int value_size,
	GthPoint *red_points, int red_size,
	GthPoint *green_points, int green_size,
	GthPoint *blue_points, int blue_size)
{
	GthPoints *points = g_new (GthPoints, 1);
	gth_points_init (points,
		value_points, value_size,
		red_points, red_size,
		green_points, green_size,
		blue_points, blue_size);
	points->ref_count = 1;
	return points;
}

void gth_points_init (GthPoints *points,
	GthPoint *value_points, int value_size,
	GthPoint *red_points, int red_size,
	GthPoint *green_points, int green_size,
	GthPoint *blue_points, int blue_size)
{
	points->ref_count = 0;
	points->value_points = value_points;
	points->value_size = value_size;
	points->red_points = red_points;
	points->red_size = red_size;
	points->green_points = green_points;
	points->green_size = green_size;
	points->blue_points = blue_points;
	points->blue_size = blue_size;
}

void gth_points_ref (GthPoints *points) {
	g_return_if_fail (points != NULL);
	points->ref_count++;
}

void gth_points_unref (GthPoints *points) {
	g_return_if_fail (points != NULL);
	g_return_if_fail (points->ref_count > 0);
	points->ref_count--;
	if (points->ref_count == 0) {
		g_free (points->value_points);
		g_free (points->red_points);
		g_free (points->green_points);
		g_free (points->blue_points);
		g_free (points);
	}
}

GthPoint * gth_points_get_channel (GthPoints *points, GthChannel channel, int *size) {
	g_return_val_if_fail (points != NULL, NULL);
	GthPoint *result = NULL;
	switch (channel) {
	case GTH_CHANNEL_VALUE:
		result = points->value_points;
		if (size) *size = points->value_size;
		break;
	case GTH_CHANNEL_RED:
		result = points->red_points;
		if (size) *size = points->red_size;
		break;
	case GTH_CHANNEL_GREEN:
		result = points->green_points;
		if (size) *size = points->green_size;
		break;
	case GTH_CHANNEL_BLUE:
		result = points->blue_points;
		if (size) *size = points->blue_size;
		break;
	default:
		result = NULL;
		if (size) *size = 0;
		break;
	}
	return result;
}

guchar * gth_points_get_value_map (GthPoints *points, int *size) {
	g_return_val_if_fail (points != NULL, NULL);

	GthCurve *curve[4];
	curve[0] = (GthCurve *) gth_bezier_new (points->value_points, points->value_size);
	curve[1] = (GthCurve *) gth_bezier_new (points->red_points, points->red_size);
	curve[2] = (GthCurve *) gth_bezier_new (points->green_points, points->green_size);
	curve[3] = (GthCurve *) gth_bezier_new (points->blue_points, points->blue_size);

	guchar *value_map = g_new (guchar, VALUE_MAP_SIZE);
	int idx = 0;
	for (int channel = 0; channel < 4; channel++) {
		for (int value = 0; value < VALUE_MAP_COLUMNS; value++) {
			double u = gth_curve_eval (curve[channel], value);
			if (channel != 0) {
				u = value_map[(int) u];
			}
			value_map[idx] = (guchar) u;
			idx++;
		}
		g_object_unref (curve[channel]);
	}
	if (size != NULL) *size = VALUE_MAP_SIZE;
	return value_map;
}
