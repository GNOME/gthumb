#include <config.h>
#include <math.h>
#include "lib/gth-image.h"
#include "lib/types.h"

static GthImage* rotate (GthImage *image, double degrees,
	guchar r0, guchar g0, guchar b0, guchar a0,
	GCancellable *cancellable)
{
	degrees = CLAMP (degrees, -90.0, 90.0);

	guint src_width = gth_image_get_width (image);
	guint src_height = gth_image_get_height (image);
	guchar r, g, b, a;
	guint temp;
	float f_temp;

	GthImage *image_with_background = NULL;
	if (a0 == 0xff) {
		// Pre-multiply the background color.

		image_with_background = gth_image_dup (image);

		int src_rowstride;
		guchar *p_src_row = gth_image_prepare_edit (image, &src_rowstride, NULL, NULL);
		int new_rowstride;
		guchar *p_new_row = gth_image_prepare_edit (image_with_background, &new_rowstride, NULL, NULL);
		guchar *src_pixel, *new_pixel;

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
	}
	else {
		image_with_background = g_object_ref (image);
	}

	// Create the rotated image

	double radiants = degrees / 180.0 * G_PI;
	double cos_angle = cos (radiants);
	double sin_angle = sin (radiants);
	guint new_width = (guint) round (cos_angle * src_width + fabs (sin_angle) * src_height);
	guint new_height = (guint) round (fabs (sin_angle) * src_width + cos_angle * src_height);
	GthImage *rotated = gth_image_new (new_width, new_height);

	int src_rowstride;
	guchar *p_src_row = gth_image_prepare_edit (image_with_background, &src_rowstride, NULL, NULL);

	int new_rowstride;
	guchar *p_new_row = gth_image_prepare_edit (rotated, &new_rowstride, NULL, NULL);

	// Bilinear interpolation
	//            fx
	//    v00------------v01
	//    |        |      |
	// fy |--------v      |
	//    |               |
	//    |               |
	//    |               |
	//    v10------------v11

#define INTERPOLATE(v, v00, v01, v10, v11, fx, fy) \
	f_temp = (1.0 - (fy)) * \
	      ((1.0 - (fx)) * (v00) + (fx) * (v01)) \
	      + \
	      (fy) * \
	      ((1.0 - (fx)) * (v10) + (fx) * (v11)); \
	v = CLAMP (f_temp, 0, 255);

#define GET_VALUES(r, g, b, a, x, y) \
	if ((x >= 0) && (x < src_width) && (y >= 0) && (y < src_height)) { \
		src_pixel = p_src_row + src_rowstride * y + 4 * x; \
		r = src_pixel[PIXEL_RED]; \
		g = src_pixel[PIXEL_GREEN]; \
		b = src_pixel[PIXEL_BLUE]; \
		a = src_pixel[PIXEL_ALPHA]; \
	} \
	else { \
		r = r0; \
		g = g0; \
		b = b0; \
		a = a0; \
	}

	guchar *src_pixel, *new_pixel;
	double half_new_width = (double) new_width / 2.0;
	double half_new_height = (double) new_height / 2.0;
	double half_src_width = (double) src_width / 2.0;
	double half_src_height = (double) src_height / 2.0;
	double x, y, x2, y2, fx, fy;
	int x2min, y2min, x2max, y2max;
	guchar r00, r01, r10, r11;
	guchar g00, g01, g10, g11;
	guchar b00, b01, b10, b11;
	guchar a00, a01, a10, a11;

	y = - half_new_height;
	for (guint yi = 0; yi < new_height; yi++) {
		new_pixel = p_new_row;
		x = - half_new_width;
		for (guint xi = 0; xi < new_width; xi++) {
			x2 = cos_angle * x - sin_angle * y + half_src_width;
			y2 = sin_angle * x + cos_angle * y + half_src_height;

			// if (high_quality) {
				// Bilinear interpolation.

				x2min = (int) x2;
				y2min = (int) y2;
				x2max = x2min + 1;
				y2max = y2min + 1;

				GET_VALUES (r00, g00, b00, a00, x2min, y2min);
				GET_VALUES (r01, g01, b01, a01, x2max, y2min);
				GET_VALUES (r10, g10, b10, a10, x2min, y2max);
				GET_VALUES (r11, g11, b11, a11, x2max, y2max);

				fx = x2 - x2min;
				fy = y2 - y2min;

				INTERPOLATE (r, r00, r01, r10, r11, fx, fy);
				INTERPOLATE (g, g00, g01, g10, g11, fx, fy);
				INTERPOLATE (b, b00, b01, b10, b11, fx, fy);
				INTERPOLATE (a, a00, a01, a10, a11, fx, fy);

				RGBA_TO_PIXEL (new_pixel, r, g, b, a);
			/*}
			else { // Nearest neighbor
				x2min = (int) round (x2);
				y2min = (int) round (y2);
				GET_VALUES (r, g, b, a, x2min, y2min);
				RGBA_TO_PIXEL (new_pixel, r, g, b, a);
			}*/

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

	g_object_unref (image_with_background);

#undef INTERPOLATE
#undef GET_VALUES

	return rotated;
}

GthImage * gth_image_rotate (GthImage *image, float degrees,
	GdkRGBA *background_color, GCancellable *cancellable)
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
		rotated = rotate (image, -degrees,
			background_color->red * 255.0,
			background_color->green * 255.0,
			background_color->blue * 255.0,
			background_color->alpha * 255.0,
			cancellable);
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
} RotateData;

static RotateData * rotate_data_new (GthImage *image, float angle, GdkRGBA *background_color) {
	RotateData *rotate_data = g_new0 (RotateData, 1);
	rotate_data->original = g_object_ref (image);
	rotate_data->angle = angle;
	rotate_data->background_color = background_color;
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
		cancellable);

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
	GCancellable *cancellable, GAsyncReadyCallback callback,
	gpointer user_data)
{
	GTask *task = g_task_new (NULL, cancellable, callback, user_data);
	g_task_set_task_data (task,
		rotate_data_new (image, degrees, background_color),
		(GDestroyNotify) rotate_data_free);
	g_task_run_in_thread (task, rotate_image_thread);
	g_object_unref (task);
}

GthImage * gth_image_rotate_finish (GthImage *self, GAsyncResult *result,
	GError **error)
{
	return g_task_propagate_pointer (G_TASK (result), error);
}
