#include <glib.h>
#include <glib-object.h>

typedef struct _GthOption GthOption;

GthOption * gth_option_new_void (int id);
GthOption * gth_option_new_bool (int id, gboolean value);
GthOption * gth_option_new_int (int id, int value);
GthOption * gth_option_new_uint (int id, unsigned int value);
GthOption * gth_option_new_enum (int id, int value);
GthOption * gth_option_new_flags (int id, int value);
GthOption * gth_option_new_intv (int id, int *value, int len);
GthOption * gth_option_new_uintv (int id, unsigned int *value, int len);
GthOption * gth_option_new_string (int id, char *value, int len);
GthOption * gth_option_ref (GthOption *opt);
void gth_option_unref (GthOption *opt);

int gth_option_get_id (GthOption *opt);
gboolean gth_option_get_bool (GthOption *opt, gboolean *value);
gboolean gth_option_get_int (GthOption *opt, int *value);
gboolean gth_option_get_uint (GthOption *opt, unsigned int *value);
gboolean gth_option_get_enum (GthOption *opt, int *value);
gboolean gth_option_get_flags (GthOption *opt, int *value);
gboolean gth_option_get_intv (GthOption *opt, int **value, int *len);
gboolean gth_option_get_uintv (GthOption *opt, unsigned int **value, int *len);
gboolean gth_option_get_string (GthOption *opt, char **value, int *len);

gboolean gth_option_equal (GthOption *opt, GthOption *other);

#define GTH_TYPE_OPTION (gth_option_get_type ())
GType gth_option_get_type (void) G_GNUC_CONST;
