#include <math.h>
#include "lib/gth-image.h"
#include "lib/types.h"


typedef double ScaleReal;
typedef ScaleReal (*WeightFunc) (ScaleReal distance);
#ifdef HAVE_VECTOR_OPERATIONS
typedef ScaleReal v4r __attribute__ ((vector_size(sizeof(ScaleReal) * 4)));
#endif /* HAVE_VECTOR_OPERATIONS */


// gth_image_resize_if_larger
// Based on code from ImageMagick/magick/resize.c


inline static ScaleReal box (ScaleReal x) {
	return 1.0;
}


inline static ScaleReal triangle (ScaleReal x) {
	return (x < 1.0) ? 1.0 - x : 0.0;
}


static ScaleReal Cubic_P0, Cubic_P2, Cubic_P3;
static ScaleReal Cubic_Q0, Cubic_Q1, Cubic_Q2, Cubic_Q3;
static gsize coefficients_initialization = 0;


static void initialize_coefficients (ScaleReal B, ScaleReal C) {
	Cubic_P0 = (6.0 - 2.0 * B) / 6.0;
	//Cubic_P1 = 0.0;
	Cubic_P2 = (-18.0 + 12.0 * B + 6.0 * C) / 6.0;
	Cubic_P3 = ( 12.0 - 9.0 * B - 6.0 * C) / 6.0;
	Cubic_Q0 = (8.0 * B + 24.0 * C) / 6.0;
	Cubic_Q1 = (-12.0 * B - 48.0 * C) / 6.0;
	Cubic_Q2 = (6.0 * B + 30.0 * C) / 6.0;
	Cubic_Q3 = (-1.0 * B - 6.0 * C) / 6.0;
}


inline static ScaleReal cubic (ScaleReal x) {
	if (x < 1.0) {
		return Cubic_P0 + x * (x * (Cubic_P2 + x * Cubic_P3));
	}
	if (x < 2.0) {
		return Cubic_Q0 + x * (Cubic_Q1 + x * (Cubic_Q2 + x * Cubic_Q3));
	}
	return 0.0;
}


inline static ScaleReal sinc_fast (ScaleReal x) {
	if (x > 4.0) {
		const ScaleReal alpha = G_PI * x;
		return sin ((double) alpha) / alpha;
	}

	// The approximations only depend on x^2 (sinc is an even function).
	const ScaleReal xx = x*x;
	// Maximum absolute relative error 1.2e-12 < 1/2^39.
	const ScaleReal c0 = 0.173611111110910715186413700076827593074e-2L;
	const ScaleReal c1 = -0.289105544717893415815859968653611245425e-3L;
	const ScaleReal c2 = 0.206952161241815727624413291940849294025e-4L;
	const ScaleReal c3 = -0.834446180169727178193268528095341741698e-6L;
	const ScaleReal c4 = 0.207010104171026718629622453275917944941e-7L;
	const ScaleReal c5 = -0.319724784938507108101517564300855542655e-9L;
	const ScaleReal c6 = 0.288101675249103266147006509214934493930e-11L;
	const ScaleReal c7 = -0.118218971804934245819960233886876537953e-13L;
	const ScaleReal p = c0+xx*(c1+xx*(c2+xx*(c3+xx*(c4+xx*(c5+xx*(c6+xx*c7))))));
	const ScaleReal d0 = 1.0L;
	const ScaleReal d1 = 0.547981619622284827495856984100563583948e-1L;
	const ScaleReal d2 = 0.134226268835357312626304688047086921806e-2L;
	const ScaleReal d3 = 0.178994697503371051002463656833597608689e-4L;
	const ScaleReal d4 = 0.114633394140438168641246022557689759090e-6L;
	const ScaleReal q = d0+xx*(d1+xx*(d2+xx*(d3+xx*d4)));

	return ((xx-1.0)*(xx-4.0)*(xx-9.0)*(xx-16.0)/q*p);
}


inline static ScaleReal mitchell_netravali (ScaleReal x) {
	if (x >= 2.0) {
		return 0.0;
	}
	ScaleReal xx = x * x;
	if ((x >= 0.0) && (x < 1.0)) {
		x  = (21 * xx * x) - (36 * xx) + 16;
	}
	else /* (x >= 1.0) && (x < 2.0) */ {
		x  = (-7 * xx * x) + (36 * xx) - (60 * x) + 32;
	}
	return x / 18.0;
}


static struct {
	WeightFunc weight_func;
	ScaleReal support;
}
const FilterInfo[N_SCALE_FILTERS] = {
	{ box, .0 },
	{ box, .5 },
	{ triangle, 1.0 },
	{ cubic, 2.0 },
	{ sinc_fast, 2.0 },
	{ sinc_fast, 3.0 },
	{ mitchell_netravali, 2.0 },
};


// Filter


typedef struct {
	WeightFunc weight_func;
	ScaleReal support;
	gboolean cancelled;
} Filter;


static Filter *
filter_new (GthScaleFilter quality)
{
	Filter *filter;
	filter = g_new (Filter, 1);
	filter->weight_func = FilterInfo[quality].weight_func;
	filter->support = FilterInfo[quality].support;
	filter->cancelled = FALSE;
	return filter;
}


inline static ScaleReal
filter_get_support (Filter *filter)
{
	return filter->support;
}


inline static void
filter_destroy (Filter *filter)
{
	g_free (filter);
}


static ScaleReal
filter_get_weight (Filter *filter,
		   ScaleReal distance)
{
	ScaleReal window = 1.0;
	if (filter->weight_func == sinc_fast) {
		ScaleReal x = fabs (distance);
		if (x == 0) {
			window = 1.0;
		}
		else if ((x > 0) && (x < filter->support)) {
			window = filter->weight_func (x / filter->support);
		}
		else {
			window = 0.0;
		}
	}
	return window * filter->weight_func (fabs (distance));
}


static void
horizontal_scale_and_transpose (GthImage *image,
				GthImage *scaled,
				ScaleReal scale_factor,
				Filter *filter,
				GCancellable *cancellable)
{
	if (filter->cancelled)
		return;

	ScaleReal scale = MAX ((ScaleReal) 1.0 / scale_factor, 1.0);
	ScaleReal support = scale * filter_get_support (filter);
	if (support < 0.5) {
		support = 0.5;
		scale = 1.0;
	}

	int image_width = (int) gth_image_get_width (image);
	int scaled_width = (int) gth_image_get_width (scaled);
	int scaled_height = (int) gth_image_get_height (scaled);

	int src_rowstride;
	guchar *p_src = gth_image_get_pixels (image, NULL, &src_rowstride);

	int dest_rowstride;
	guchar *p_dest = gth_image_get_pixels (scaled, NULL, &dest_rowstride);

	ScaleReal *weights = g_new (ScaleReal, 2.0 * support + 3.0);

	int x, y, i, n, temp;
#ifdef HAVE_VECTOR_OPERATIONS
	v4r v_pixel, v_rgba;
#else
	ScaleReal r, g, b, a, w;
#endif /* HAVE_VECTOR_OPERATIONS */

	scale = 1.0 / scale;
	for (y = 0; y < scaled_height; y++) {
		if ((cancellable != NULL) && g_cancellable_is_cancelled (cancellable)) {
			goto out;
		}

		ScaleReal bisect = ((ScaleReal) y + 0.5) / scale_factor;
		int start = bisect - support + 0.5;
		start = MAX (start, 0);
		int stop = bisect + support + 0.5;
		stop = MIN (stop, image_width);

		ScaleReal density = 0.0;
		for (n = 0; n < stop - start; n++) {
			weights[n] = filter_get_weight (filter, scale * ((ScaleReal) (start + n) - bisect + 0.5));
			density += weights[n];
		}

		// g_assert (n == stop - start);
		// g_assert (stop - start <= (2.0 * support) + 3);

		if ((density != 0.0) && (density != 1.0)) {
			density = 1.0 / density;
			for (i = 0; i < n; i++) {
				weights[i] *= density;
			}
		}

		guchar *p_src_row = p_src + (start * 4);
		guchar *p_dest_pixel = p_dest;

		for (x = 0; x < scaled_width; x++) {
			guchar *p_src_pixel;

			p_src_pixel = p_src_row;

#ifdef HAVE_VECTOR_OPERATIONS

			v_rgba = (v4r) { 0.0, 0.0, 0.0, 0.0 };
			for (i = 0; i < n; i++) {
				v_pixel = (v4r) { p_src_pixel[0], p_src_pixel[1], p_src_pixel[2], p_src_pixel[3] };
				v_rgba = v_rgba + (v_pixel * weights[i]);
				p_src_pixel += 4;
			}
			v_rgba = v_rgba + 0.5;

			p_dest_pixel[0] = PIXEL_CLAMP (v_rgba[0]);
			p_dest_pixel[1] = PIXEL_CLAMP (v_rgba[1]);
			p_dest_pixel[2] = PIXEL_CLAMP (v_rgba[2]);
			p_dest_pixel[3] = PIXEL_CLAMP (v_rgba[3]);

#else /* !HAVE_VECTOR_OPERATIONS */

			r = g = b = a = 0.0;
			for (i = 0; i < n; i++) {
				w = weights[i];

				r += w * p_src_pixel[PIXEL_RED];
				g += w * p_src_pixel[PIXEL_GREEN];
				b += w * p_src_pixel[PIXEL_BLUE];
				a += w * p_src_pixel[PIXEL_ALPHA];

				p_src_pixel += 4;
			}

			p_dest_pixel[PIXEL_RED] = PIXEL_CLAMP (r + 0.5);
			p_dest_pixel[PIXEL_GREEN] = PIXEL_CLAMP (g + 0.5);
			p_dest_pixel[PIXEL_BLUE] = PIXEL_CLAMP (b + 0.5);
			p_dest_pixel[PIXEL_ALPHA] = PIXEL_CLAMP (a + 0.5);

#endif /* HAVE_VECTOR_OPERATIONS */

			p_dest_pixel += 4;
			p_src_row += src_rowstride;
		}

		p_dest += dest_rowstride;
	}

out:

	g_free (weights);
}


GthImage *
gth_image_resize_if_larger (GthImage		*image,
			    guint		 size,
			    GthScaleFilter	 quality,
			    GCancellable	*cancellable)
{
	guint original_width = gth_image_get_width (image);
	guint original_height = gth_image_get_height (image);
	guint scaled_width = original_width;
	guint scaled_height = original_height;
	if (!scale_if_larger (&scaled_width, &scaled_height, size)) {
		//return gth_image_dup (image);
		return g_object_ref (image);
	}

	GthImage *scaled = gth_image_new (scaled_width, scaled_height);
	if (scaled == NULL) {
		return NULL;
	}

	gth_image_copy_metadata (image, scaled);
	if (!gth_image_has_original_size (image)) {
		gth_image_set_original_size (scaled, original_width, original_height);
	}

	if (g_once_init_enter (&coefficients_initialization)) {
		initialize_coefficients (1.0, 0.0);
		g_once_init_leave (&coefficients_initialization, 1);
	}

	GthImage *tmp = gth_image_new (original_height, scaled_width);
	ScaleReal x_factor = (ScaleReal) scaled_width / original_width;
	ScaleReal y_factor = (ScaleReal) scaled_height / original_height;
	Filter *filter = filter_new (quality);
	horizontal_scale_and_transpose (image, tmp, x_factor, filter, cancellable);
	horizontal_scale_and_transpose (tmp, scaled, y_factor, filter, cancellable);

	filter_destroy (filter);
	g_object_unref (tmp);

	return scaled;
}


// gth_image_resize_if_larger_async


typedef struct {
	GthImage *original;
	guint size;
	GthScaleFilter quality;
} ResizeData;


static ResizeData *
resize_data_new (GthImage	*image,
		 guint		 size,
		 GthScaleFilter	 quality)
{
	ResizeData *resize_data;

	resize_data = g_new0 (ResizeData, 1);
	resize_data->original = g_object_ref (image);
	resize_data->size = size;
	resize_data->quality = quality;

	return resize_data;
}


static void
resize_data_free (ResizeData *resize_data)
{
	g_object_unref (resize_data->original);
	g_free (resize_data);
}


static void
resize_image_thread (GTask		*task,
		     gpointer		 source_object,
		     gpointer		 task_data,
		     GCancellable	*cancellable)
{
	ResizeData *resize_data = g_task_get_task_data (task);
	GthImage *resized_image = gth_image_resize_if_larger (
		resize_data->original,
		resize_data->size,
		resize_data->quality,
		cancellable);

	if (resized_image == NULL) {
		g_task_return_error (task, g_error_new_literal (G_IO_ERROR, G_IO_ERROR_FAILED, "Image is null"));
		return;
	}

	if ((cancellable != NULL) && g_cancellable_is_cancelled (cancellable)) {
		g_object_unref (resized_image);
		g_task_return_error (task, g_error_new_literal (G_IO_ERROR, G_IO_ERROR_CANCELLED, ""));
		return;
	}

	g_task_return_pointer (task, resized_image, (GDestroyNotify) g_object_unref);
}


void
gth_image_resize_if_larger_async (GthImage		*image,
				  guint			 size,
				  GthScaleFilter	 quality,
				  GCancellable		*cancellable,
				  GAsyncReadyCallback	 callback,
				  gpointer		 user_data)
{
	GTask *task = g_task_new (NULL, cancellable, callback, user_data);
	g_task_set_task_data (
		task,
		resize_data_new (image,	size, quality),
		(GDestroyNotify) resize_data_free);
	g_task_run_in_thread (task, resize_image_thread);
	g_object_unref (task);
}


GthImage *
gth_image_resize_if_larger_finish (GthImage	 *self,
				   GAsyncResult	 *result,
				   GError	**error)
{
	return g_task_propagate_pointer (G_TASK (result), error);
}
