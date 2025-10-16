#ifndef GTH_METADATA_H
#define GTH_METADATA_H

#include <glib.h>
#include <glib-object.h>
#include <gio/gio.h>
#include "gth-string-list.h"

G_BEGIN_DECLS

typedef enum {
	GTH_METADATA_TYPE_STRING,
	GTH_METADATA_TYPE_STRING_LIST,
	GTH_METADATA_TYPE_POINT,
} GthMetadataType;

typedef struct {
	const char *id;
	const char *display_name;
	int sort_order;
} GthMetadataCategory;

typedef enum {
	GTH_METADATA_HIDDEN = 1 << 0,
	GTH_METADATA_ALLOW_IN_FILE_LIST = 1 << 1,
	GTH_METADATA_ALLOW_IN_PROPERTIES_VIEW = 1 << 2,
	GTH_METADATA_ALLOW_IN_PRINT = 1 << 3
} GthMetadataFlags;

#define GTH_METADATA_ALLOW_EVERYWHERE (GTH_METADATA_ALLOW_IN_FILE_LIST | GTH_METADATA_ALLOW_IN_PROPERTIES_VIEW | GTH_METADATA_ALLOW_IN_PRINT)

typedef struct {
	const char *id;
	const char *display_name;
	const char *category;
	GthMetadataFlags flags;
	const char *type;
	int sort_order;
} GthMetadataInfo;

#define GTH_TYPE_METADATA (gth_metadata_get_type ())
#define GTH_METADATA(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), GTH_TYPE_METADATA, GthMetadata))
#define GTH_METADATA_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), GTH_TYPE_METADATA, GthMetadataClass))
#define GTH_IS_METADATA(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GTH_TYPE_METADATA))
#define GTH_IS_METADATA_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GTH_TYPE_METADATA))
#define GTH_METADATA_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), GTH_TYPE_METADATA, GthMetadataClass))

typedef struct _GthMetadata GthMetadata;
typedef struct _GthMetadataClass GthMetadataClass;
typedef struct _GthMetadataPrivate GthMetadataPrivate;

struct _GthMetadata {
	GObject parent_instance;
	GthMetadataPrivate *priv;
};

struct _GthMetadataClass {
	GObjectClass parent_class;
};

GType             gth_metadata_get_type             (void);
GthMetadata *     gth_metadata_new                  (void);
GthMetadata *     gth_metadata_new_typed            (const char      *value_type,
						     const char      *raw,
						     const char      *formatted);
GthMetadata *     gth_metadata_new_for_string       (const char      *raw,
						     const char      *formatted);
GthMetadata *     gth_metadata_new_for_string_list  (GthStringList   *list);
GthMetadata *     gth_metadata_new_for_point        (double           x,
						     double           y);
GthMetadata *     gth_metadata_new_sRGBColorSpace   ();
GthMetadataType   gth_metadata_get_data_type        (GthMetadata     *metadata);
const char *      gth_metadata_get_id               (GthMetadata     *metadata);
const char *      gth_metadata_get_raw              (GthMetadata     *metadata);
GthStringList *   gth_metadata_get_string_list      (GthMetadata     *metadata);
gboolean          gth_metadata_get_point            (GthMetadata     *metadata,
						     double          *x,
						     double          *y);
const char *      gth_metadata_get_formatted        (GthMetadata     *metadata);
const char *      gth_metadata_get_value_type       (GthMetadata     *metadata);
GthMetadata *     gth_metadata_dup                  (GthMetadata     *metadata);

void gth_metadata_info_init ();
GthMetadataInfo * gth_metadata_info_register (const char *id, const char *display_name, const char *category, GthMetadataFlags flags, const char *type);
GthMetadataInfo * gth_metadata_info_get (const char *id);
GPtrArray * gth_metadata_info_get_all ();
GthMetadataInfo * gth_metadata_info_copy (GthMetadataInfo *info);
void gth_metadata_info_destroy (GthMetadataInfo *info);

void gth_metadata_category_init ();
void gth_metadata_category_register (const char *id, const char *display_name);
GthMetadataCategory * gth_metadata_category_get (const char *id);
GthMetadataCategory * gth_metadata_category_new (const char *id, const char *display_name, int sort_order);
GthMetadataCategory * gth_metadata_category_copy (GthMetadataCategory *category);
void gth_metadata_category_free (GthMetadataCategory *category);

G_END_DECLS

#endif /* GTH_METADATA_H */
