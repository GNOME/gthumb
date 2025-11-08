#include <config.h>
#include <librsvg/rsvg.h>
#include <math.h>
#include "load-svg.h"

#define DEFAULT_DPI 96.0
#define DEFAULT_SIZE 1024

// GthImageSvg (private class)
#define GTH_IMAGE_SVG(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), gth_image_svg_get_type(), GthImageSvg))


typedef struct {
	GthImage __parent;
	RsvgHandle *rsvg;
	double scale_factor;
	guint original_max_size;
	double x_offset;
	double y_offset;
} GthImageSvg;


typedef GthImageClass GthImageSvgClass;

//static gpointer gth_image_svg_parent_class;

//GType gth_image_svg_get_type (void);

G_DEFINE_TYPE (GthImageSvg, gth_image_svg, GTH_TYPE_IMAGE)


static void gth_image_svg_finalize (GObject *object) {
	GthImageSvg *self = GTH_IMAGE_SVG (object);
	if (self->rsvg != NULL)
		g_object_unref (self->rsvg);
	G_OBJECT_CLASS (gth_image_svg_parent_class)->finalize (object);
}


static void gth_image_svg_init (GthImageSvg *self) {
	self->rsvg = NULL;
	self->scale_factor = 1.0;
}


static void gth_image_init_pixels_from_cairo_surface (GthImage *image, cairo_surface_t *surface) {
	cairo_surface_flush (surface);

	guint width = (guint) cairo_image_surface_get_width (surface);
	guint height = (guint) cairo_image_surface_get_height (surface);
	int src_stride = cairo_image_surface_get_stride (surface);
	guchar *src_pixels = cairo_image_surface_get_data (surface);

	gth_image_init_pixels (image, width, height);
	int dest_stride;
	guchar *dest_pixels = gth_image_prepare_edit (image, &dest_stride, NULL, NULL);

	const guchar *src_row = src_pixels;
	guchar *dest_row = dest_pixels;
	size_t row_size = MIN (dest_stride, src_stride);

	for (int h = 0; h < height; h++) {
		memcpy (dest_row, src_row, row_size);
		src_row += src_stride;
		dest_row += dest_stride;
	}
}


static GthImage * gth_image_svg_new (RsvgHandle *rsvg, guint max_size, GError **error) {
	if (max_size == 0) {
		max_size = DEFAULT_SIZE;
	}

	guint width = max_size;
	guint height = max_size;

	double iwidth, iheight;
	if (rsvg_handle_get_intrinsic_size_in_pixels (rsvg, &iwidth, &iheight)) {
		width = (guint) ceil (iwidth);
		height = (guint) ceil (iheight);
	}
	else {
		gboolean has_viewbox;
		RsvgRectangle viewbox;
		rsvg_handle_get_intrinsic_dimensions (rsvg,
			NULL, NULL, NULL, NULL,
			&has_viewbox, &viewbox);
		if (has_viewbox) {
			width = (guint) ceil (viewbox.width);
			height = (guint) ceil (viewbox.height);
		}
	}

	GthImageSvg *image = (GthImageSvg *) g_object_new (gth_image_svg_get_type (), NULL);
	gth_image_set_original_size (GTH_IMAGE (image), width, height);
	image->rsvg = g_object_ref (rsvg);
	image->original_max_size = MAX (width, height);
	image->scale_factor = (double) max_size / image->original_max_size;
	image->x_offset = - ((double) image->original_max_size - width) / 2.0;
	image->y_offset = - ((double) image->original_max_size - height) / 2.0;

	cairo_surface_t *surface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, width, height);
	cairo_t *cr = cairo_create (surface);
	cairo_scale (cr, 1.0 / image->scale_factor, 1.0 / image->scale_factor);
	RsvgRectangle viewport = {
		.x = image->x_offset,
		.y = image->y_offset,
		.width = (double) max_size,
		.height = (double) max_size,
	};
	if (rsvg_handle_render_document (image->rsvg, cr, &viewport, error)) {
		gth_image_init_pixels_from_cairo_surface (GTH_IMAGE (image), surface);
		gth_image_set_has_alpha (GTH_IMAGE (image), TRUE);
	}
	else {
		g_object_unref (image);
		image = NULL;
	}

	cairo_destroy (cr);
	cairo_surface_destroy (surface);

	return (GthImage *) image;
}


static gboolean gth_image_svg_get_is_scalable (GthImage *base) {
	return TRUE;
}


static cairo_surface_t * gth_image_svg_get_scaled_texture (GthImage *base, double scale_factor, guint x, guint y, guint width, guint height) {
	GthImageSvg *self = GTH_IMAGE_SVG (base);
	guint original_width, original_height;
	gth_image_get_original_size (base, &original_width, &original_height);
	cairo_surface_t *surface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, width, height);
	cairo_t *cr = cairo_create (surface);
	RsvgRectangle viewport = {
		.x = - ((double) x) * scale_factor,
		.y = - ((double) y) * scale_factor,
		.width = (double) original_width * scale_factor,
		.height = (double) original_height * scale_factor
	};
	if (!rsvg_handle_render_document (self->rsvg, cr, &viewport, NULL)) {
		cairo_surface_destroy (surface);
		surface = NULL;
	}
	cairo_destroy (cr);
	return surface;
}


static void gth_image_svg_class_init (GthImageSvgClass *klass) {
	GObjectClass *object_class = G_OBJECT_CLASS (klass);
	object_class->finalize = gth_image_svg_finalize;

	GthImageClass *image_class = GTH_IMAGE_CLASS (klass);
	image_class->get_is_scalable = gth_image_svg_get_is_scalable;
	image_class->get_scaled_texture = gth_image_svg_get_scaled_texture;
}


GthImage* load_svg (GBytes *bytes, guint requested_size, GCancellable *cancellable, GError **error) {
	GthImage *image = NULL;
	GInputStream *stream = g_memory_input_stream_new_from_bytes (bytes);
	RsvgHandle *rsvg = rsvg_handle_new ();
	rsvg_handle_set_dpi (rsvg, DEFAULT_DPI);
	if (rsvg_handle_read_stream_sync (rsvg, stream, cancellable, error)) {
		image = gth_image_svg_new (rsvg, requested_size, error);
	}
	g_object_unref (rsvg);
	g_object_unref (stream);
	return image;
}
