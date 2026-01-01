#include <config.h>
#include <math.h>
#include "lib/gth-image.h"
#include "lib/types.h"

#define GET_RGBA(x, y, r, g, b, a) \
	if ((x >= 0) && (x < src_width) && (y >= 0) && (y < src_height)) { \
		src_pixel = p_src_row + (src_rowstride * y) + (4 * x); \
		PIXEL_TO_RGBA (src_pixel, r, g, b, a); \
	} \
	else { \
		r = r0; \
		g = g0; \
		b = b0; \
		a = a0; \
	}

static GthImage *premultiply_background (GthImage *image, guchar r0, guchar g0, guchar b0, guchar a0) {
	GthImage *image_with_background = gth_image_dup (image);
	int src_rowstride;
	guchar *p_src_row = gth_image_prepare_edit (image, &src_rowstride, NULL, NULL);
	int new_rowstride;
	guchar *p_new_row = gth_image_prepare_edit (image_with_background, &new_rowstride, NULL, NULL);
	guchar *src_pixel, *new_pixel;
	guint src_width = gth_image_get_width (image);
	guint src_height = gth_image_get_height (image);
	guchar r, g, b, a;
	int temp;

	for (guint yi = 0; yi < src_height; yi++) {
		src_pixel = p_src_row;
		new_pixel = p_new_row;
		for (guint xi = 0; xi < src_width; xi++) {
			a = 0xFF - src_pixel[PIXEL_ALPHA];
			PIXEL_MULTIPLY_ALPHA (r, r0, a);
			PIXEL_MULTIPLY_ALPHA (g, g0, a);
			PIXEL_MULTIPLY_ALPHA (b, b0, a);

			r += src_pixel[PIXEL_RED];
			g += src_pixel[PIXEL_GREEN];
			b += src_pixel[PIXEL_BLUE];
			*(guint32*) new_pixel = PACK_RGBA (r, g, b, 0xFF);

			src_pixel += 4;
			new_pixel += 4;
		}
		p_src_row += src_rowstride;
		p_new_row += new_rowstride;
	}
	return image_with_background;
}

static GthImage* rotate_nearest (GthImage *source, double degrees,
	guchar r0, guchar g0, guchar b0, guchar a0,
	GCancellable *cancellable)
{
	degrees = CLAMP (degrees, -90.0, 90.0);

	guint src_width = gth_image_get_width (source);
	guint src_height = gth_image_get_height (source);
	guchar r, g, b, a;
	int temp;
	float f_temp;

	// Create the rotated image

	double radiants = degrees / 180.0 * G_PI;
	double cos_angle = cos (radiants);
	double sin_angle = sin (radiants);
	guint new_width = (guint) round (cos_angle * src_width + fabs (sin_angle) * src_height);
	guint new_height = (guint) round (fabs (sin_angle) * src_width + cos_angle * src_height);
	GthImage *rotated = gth_image_new (new_width, new_height);

	int src_rowstride;
	guchar *p_src_row = gth_image_prepare_edit (source, &src_rowstride, NULL, NULL);

	int new_rowstride;
	guchar *p_new_row = gth_image_prepare_edit (rotated, &new_rowstride, NULL, NULL);

	guchar *src_pixel, *new_pixel;
	double half_new_width = (double) new_width / 2.0;
	double half_new_height = (double) new_height / 2.0;
	double half_src_width = (double) src_width / 2.0;
	double half_src_height = (double) src_height / 2.0;
	double x, y, src_x, src_y, fx, fy, gx, gy;
	int x0, y0, x1, y1;
	guchar r00, r01, r10, r11;
	guchar g00, g01, g10, g11;
	guchar b00, b01, b10, b11;
	guchar a00, a01, a10, a11;

	y = - half_new_height;
	for (guint yi = 0; yi < new_height; yi++) {
		new_pixel = p_new_row;
		x = - half_new_width;
		for (guint xi = 0; xi < new_width; xi++) {
			src_x = (cos_angle * x) - (sin_angle * y) + half_src_width;
			src_y = (sin_angle * x) + (cos_angle * y) + half_src_height;

			x0 = (int) round (src_x);
			y0 = (int) round (src_y);
			GET_RGBA (x0, y0, r00, g00, b00, a00);
			RGBA_TO_PIXEL (new_pixel, r00, g00, b00, a00);

			new_pixel += 4;
			x += 1.0;
		}

		p_new_row += new_rowstride;
		y += 1.0;

		if ((cancellable != NULL) && g_cancellable_is_cancelled (cancellable)) {
			g_object_unref (rotated);
			rotated = NULL;
			break;
		}
	}

	return rotated;
}

static GthImage* rotate_bilinear (GthImage *source, double degrees,
	guchar r0, guchar g0, guchar b0, guchar a0,
	GCancellable *cancellable)
{
	degrees = CLAMP (degrees, -90.0, 90.0);

	guint src_width = gth_image_get_width (source);
	guint src_height = gth_image_get_height (source);
	guchar r, g, b, a;
	int temp;
	float f_temp;

	// Create the rotated image

	double radiants = degrees / 180.0 * G_PI;
	double cos_angle = cos (radiants);
	double sin_angle = sin (radiants);
	guint new_width = (guint) round (cos_angle * src_width + fabs (sin_angle) * src_height);
	guint new_height = (guint) round (fabs (sin_angle) * src_width + cos_angle * src_height);
	GthImage *rotated = gth_image_new (new_width, new_height);

	int src_rowstride;
	guchar *p_src_row = gth_image_prepare_edit (source, &src_rowstride, NULL, NULL);

	int new_rowstride;
	guchar *p_new_row = gth_image_prepare_edit (rotated, &new_rowstride, NULL, NULL);

	// Bilinear interpolation
	//            fx
	//    v00------------v10
	//    |        |      |
	// fy |--------v      |
	//    |               |
	//    |               |
	//    |               |
	//    v01------------v11

#define BILINEAR(v, v00, v10, v01, v11) \
	f_temp = gy * (gx * v00 + fx * v10) \
		 + \
		 fy * (gx * v01 + fx * v11); \
	v = CLAMP (f_temp, 0, 255);

	guchar *src_pixel, *new_pixel;
	double half_new_width = (double) new_width / 2.0;
	double half_new_height = (double) new_height / 2.0;
	double half_src_width = (double) src_width / 2.0;
	double half_src_height = (double) src_height / 2.0;
	double x, y, src_x, src_y, fx, fy, gx, gy;
	int x0, y0, x1, y1;
	guchar r00, r01, r10, r11;
	guchar g00, g01, g10, g11;
	guchar b00, b01, b10, b11;
	guchar a00, a01, a10, a11;

	y = - half_new_height;
	for (guint yi = 0; yi < new_height; yi++) {
		new_pixel = p_new_row;
		x = - half_new_width;
		for (guint xi = 0; xi < new_width; xi++) {
			src_x = (cos_angle * x) - (sin_angle * y) + half_src_width;
			src_y = (sin_angle * x) + (cos_angle * y) + half_src_height;

			// Bilinear interpolation.
			x0 = (int) floor (src_x);
			y0 = (int) floor (src_y);
			x1 = x0 + 1;
			y1 = y0 + 1;

			GET_RGBA (x0, y0, r00, g00, b00, a00);
			GET_RGBA (x1, y0, r10, g10, b10, a10);
			GET_RGBA (x0, y1, r01, g01, b01, a01);
			GET_RGBA (x1, y1, r11, g11, b11, a11);

			fx = src_x - x0;
			fy = src_y - y0;
			gx = 1.0 - fx;
			gy = 1.0 - fy;

			BILINEAR (r, r00, r10, r01, r11);
			BILINEAR (g, g00, g10, g01, g11);
			BILINEAR (b, b00, b10, b01, b11);
			BILINEAR (a, a00, a10, a01, a11);

			RGBA_TO_PIXEL (new_pixel, r, g, b, a);

			new_pixel += 4;
			x += 1.0;
		}

		p_new_row += new_rowstride;
		y += 1.0;

		if ((cancellable != NULL) && g_cancellable_is_cancelled (cancellable)) {
			g_object_unref (rotated);
			rotated = NULL;
			break;
		}
	}

#undef BILINEAR

	return rotated;
}

static double bicubic (double x) {
	// g_assert (x >= 0 && x <= 2);
	if (x >= 2.0) {
		return 0.0;
	}
	double xx = x * x;
	if ((x >= 0.0) && (x < 1.0)) {
		x  = (xx * x) - (2.0 * xx) + 1.0;
	}
	else /* (x >= 1.0) && (x < 2.0) */ {
		x  = - (xx * x) + (5.0 * xx) - (8.0 * x) + 4.0;
	}
	return x;
}

static double catmull_rom (double x) {
	// g_assert (x >= 0 && x <= 2);
	if (x >= 2.0) {
		return 0.0;
	}
	double xx = x * x;
	if ((x >= 0.0) && (x < 1.0)) {
		x  = (3.0 * xx * x) - (5.0 * xx) + 2.0;
	}
	else /* (x >= 1.0) && (x < 2.0) */ {
		x  = - (xx * x) + (5.0 * xx) - (8.0 * x) + 4.0;
	}
	return x / 2.0;
}

static double bspline_approx (double x) {
	// g_assert (x >= 0 && x <= 2);
	if (x >= 2.0) {
		return 0.0;
	}
	double xx = x * x;
	if ((x >= 0.0) && (x < 1.0)) {
		x  = (3.0 * xx * x) - (6.0 * xx) + 4.0;
	}
	else /* (x >= 1.0) && (x < 2.0) */ {
		x  = - (xx * x) + (6.0 * xx) - (12.0 * x) + 8.0;
	}
	return x / 6.0;
}

static GthImage* rotate_bicubic (GthImage *source, double degrees,
	guchar r0, guchar g0, guchar b0, guchar a0,
	GCancellable *cancellable)
{
	degrees = CLAMP (degrees, -90.0, 90.0);

	guint src_width = gth_image_get_width (source);
	guint src_height = gth_image_get_height (source);
	guchar r, g, b, a;
	int temp;
	float f_temp;

	// Create the rotated image

	double radiants = degrees / 180.0 * G_PI;
	double cos_angle = cos (radiants);
	double sin_angle = sin (radiants);
	guint new_width = (guint) round (cos_angle * src_width + fabs (sin_angle) * src_height);
	guint new_height = (guint) round (fabs (sin_angle) * src_width + cos_angle * src_height);
	GthImage *rotated = gth_image_new (new_width, new_height);

	int src_rowstride;
	guchar *p_src_row = gth_image_prepare_edit (source, &src_rowstride, NULL, NULL);

	int new_rowstride;
	guchar *p_new_row = gth_image_prepare_edit (rotated, &new_rowstride, NULL, NULL);

	guchar *src_pixel, *new_pixel;
	double half_new_width = (double) new_width / 2.0;
	double half_new_height = (double) new_height / 2.0;
	double half_src_width = (double) src_width / 2.0;
	double half_src_height = (double) src_height / 2.0;
	double x, y, src_x, src_y, dx, dy;
	int x0, y0, xi, yj;
	double qr, qg, qb, qa, pr, pg, pb, pa;
	double wx[4], wy[4], wi, wj;

	y = - half_new_height;
	for (guint dest_y = 0; dest_y < new_height; dest_y++) {
		new_pixel = p_new_row;
		x = - half_new_width;
		for (guint dest_x = 0; dest_x < new_width; dest_x++) {
			src_x = (cos_angle * x) - (sin_angle * y) + half_src_width;
			src_y = (sin_angle * x) + (cos_angle * y) + half_src_height;

			x0 = (int) floor (src_x);
			y0 = (int) floor (src_y);

			dx = fabs (src_x - x0);
			// g_assert (dx >= 0 && dx <= 1);
			wx[0] = catmull_rom (1.0 + dx);
			wx[1] = catmull_rom (dx);
			wx[2] = catmull_rom (1.0 - dx);
			wx[3] = catmull_rom (2.0 - dx);

			dy = fabs (src_y - y0);
			// g_assert (dy >= 0 && dy <= 1);
			wy[0] = catmull_rom (1.0 + dy);
			wy[1] = catmull_rom (dy);
			wy[2] = catmull_rom (1.0 - dy);
			wy[3] = catmull_rom (2.0 - dy);

			qr = qg = qb = qa = 0.0;
			for (int j = 0; j <= 3; j++) {
				yj = y0 + j - 1;
				pr = pg = pb = pa = 0.0;
				for (int i = 0; i <= 3; i++) {
					xi = x0 + i - 1;
					GET_RGBA (xi, yj, r, g, b, a);
					wi = wx[i];
					pr += wi * r;
					pg += wi * g;
					pb += wi * b;
					pa += wi * a;
				}
				wj = wy[j];
				qr += wj * pr;
				qg += wj * pg;
				qb += wj * pb;
				qa += wj * pa;
			}
			r = PIXEL_CLAMP (qr);
			g = PIXEL_CLAMP (qg);
			b = PIXEL_CLAMP (qb);
			a = PIXEL_CLAMP (qa);
			RGBA_TO_PIXEL (new_pixel, r, g, b, a);

			new_pixel += 4;
			x += 1.0;
		}

		p_new_row += new_rowstride;
		y += 1.0;

		if ((cancellable != NULL) && g_cancellable_is_cancelled (cancellable)) {
			g_object_unref (rotated);
			rotated = NULL;
			break;
		}
	}
	return rotated;
}

#undef GET_RGBA

GthImage * gth_image_rotate (GthImage *image, float degrees,
	GdkRGBA *background_color, GthRotateFilter filter,
	GCancellable *cancellable)
{
	g_return_val_if_fail (image != NULL, NULL);
	g_return_val_if_fail (background_color != NULL, NULL);

	GthImage *rotated = NULL;
	GthImage *transformed = NULL;
	if (degrees >= 90.0) {
		image = transformed = gth_image_apply_transform (image, GTH_TRANSFORM_ROTATE_90, cancellable);
		degrees -= 90.0;
	}
	else if (degrees <= -90.0) {
		image = transformed = gth_image_apply_transform (image, GTH_TRANSFORM_ROTATE_270, cancellable);
		degrees += 90.0;
	}
	if (image != NULL) {
		GthImage *image_with_background = NULL;
		int temp;
		guchar bg_red = background_color->red * 255;
		guchar bg_green = background_color->green * 255;
		guchar bg_blue = background_color->blue * 255;
		guchar bg_alpha = background_color->alpha * 255;
		PIXEL_MULTIPLY_ALPHA (bg_red, bg_red, bg_alpha);
		PIXEL_MULTIPLY_ALPHA (bg_green, bg_green, bg_alpha);
		PIXEL_MULTIPLY_ALPHA (bg_blue, bg_blue, bg_alpha);
		if (background_color->alpha == 0xFF) {
			image_with_background = premultiply_background (image,
				bg_red, bg_green, bg_blue, bg_alpha);
		}
		else {
			image_with_background = g_object_ref (image);
		}
		if (filter == GTH_ROTATE_FILTER_NONE) {
			rotated = rotate_nearest (image_with_background,
				-degrees,
				bg_red, bg_green, bg_blue, bg_alpha,
				cancellable);
		}
		else if (filter == GTH_ROTATE_FILTER_BILINEAR) {
			rotated = rotate_bilinear (image_with_background,
				-degrees,
				bg_red, bg_green, bg_blue, bg_alpha,
				cancellable);
		}
		else if (filter == GTH_ROTATE_FILTER_BICUBIC) {
			rotated = rotate_bicubic (image_with_background,
				-degrees,
				bg_red, bg_green, bg_blue, bg_alpha,
				cancellable);
		}
		gth_image_set_has_alpha (rotated, (bg_alpha < 255) || gth_image_get_has_alpha_if_valid (image));
		g_object_ref (image_with_background);
	}
	if (transformed != NULL) {
		g_object_unref (transformed);
	}
	return rotated;
}

// gth_image_rotate_async

typedef struct {
	GthImage *original;
	float angle;
	GdkRGBA *background_color;
	GthRotateFilter filter;
} RotateData;

static RotateData * rotate_data_new (GthImage *image, float angle, GdkRGBA *background_color, GthRotateFilter filter) {
	RotateData *rotate_data = g_new0 (RotateData, 1);
	rotate_data->original = g_object_ref (image);
	rotate_data->angle = angle;
	rotate_data->background_color = background_color;
	rotate_data->filter = filter;
	return rotate_data;
}

static void rotate_data_free (RotateData *rotate_data) {
	g_object_unref (rotate_data->original);
	g_free (rotate_data);
}

static void rotate_image_thread (GTask *task, gpointer source_object,
	gpointer task_data, GCancellable *cancellable)
{
	RotateData *rotate_data = g_task_get_task_data (task);
	GthImage *rotated_image = gth_image_rotate (rotate_data->original,
		rotate_data->angle, rotate_data->background_color,
		rotate_data->filter, cancellable);

	if ((cancellable != NULL) && g_cancellable_is_cancelled (cancellable)) {
		if (rotated_image != NULL) {
			g_object_unref (rotated_image);
		}
		g_task_return_error (task, g_error_new_literal (G_IO_ERROR, G_IO_ERROR_CANCELLED, ""));
		return;
	}
	if (rotated_image == NULL) {
		g_task_return_error (task, g_error_new_literal (G_IO_ERROR, G_IO_ERROR_FAILED, "Image is null"));
		return;
	}
	g_task_return_pointer (task, rotated_image, (GDestroyNotify) g_object_unref);
}

void
gth_image_rotate_async (GthImage *image, float degrees, GdkRGBA *background_color,
	GthRotateFilter filter, GCancellable *cancellable,
	GAsyncReadyCallback callback, gpointer user_data)
{
	GTask *task = g_task_new (NULL, cancellable, callback, user_data);
	g_task_set_task_data (task,
		rotate_data_new (image, degrees, background_color, filter),
		(GDestroyNotify) rotate_data_free);
	g_task_run_in_thread (task, rotate_image_thread);
	g_object_unref (task);
}

GthImage * gth_image_rotate_finish (GthImage *self, GAsyncResult *result,
	GError **error)
{
	return g_task_propagate_pointer (G_TASK (result), error);
}
