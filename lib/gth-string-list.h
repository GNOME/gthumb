#ifndef GTH_STRING_LIST_H
#define GTH_STRING_LIST_H

#include <glib.h>
#include <glib-object.h>

G_BEGIN_DECLS

#define GTH_TYPE_STRING_LIST (gth_string_list_get_type ())
#define GTH_STRING_LIST(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), GTH_TYPE_STRING_LIST, GthStringList))
#define GTH_STRING_LIST_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), GTH_TYPE_STRING_LIST, GthStringListClass))
#define GTH_IS_STRING_LIST(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GTH_TYPE_STRING_LIST))
#define GTH_IS_STRING_LIST_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GTH_TYPE_STRING_LIST))
#define GTH_STRING_LIST_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), GTH_TYPE_STRING_LIST, GthStringListClass))

typedef struct _GthStringList GthStringList;
typedef struct _GthStringListClass GthStringListClass;
typedef struct _GthStringListPrivate GthStringListPrivate;

struct _GthStringList {
	GObject parent_instance;
	GthStringListPrivate * priv;
};

struct _GthStringListClass {
	GObjectClass parent_class;
};

GType             gth_string_list_get_type            (void);
GthStringList *   gth_string_list_new                 (GList          *list);
GthStringList *   gth_string_list_new_from_strv       (char          **strv);
GthStringList *   gth_string_list_new_from_ptr_array  (GPtrArray      *array);
GList *           gth_string_list_get_list            (GthStringList  *list);
void              gth_string_list_set_list            (GthStringList  *list,
						       GList          *value);
char *            gth_string_list_join                (GthStringList  *list,
						       const char     *separator);
gboolean          gth_string_list_equal               (GthStringList  *list1,
						       GthStringList  *list2);
gboolean	  gth_string_list_equal_custom	      (GthStringList  *list1,
						       GthStringList  *list2,
						       GCompareFunc    compare_func);
void              gth_string_list_append              (GthStringList  *list1,
		       	       	       	       	       GthStringList  *list2);
void              gth_string_list_concat              (GthStringList  *list1,
		       	       	       	       	       GthStringList  *list2);
gboolean          gth_string_list_is_empty            (GthStringList  *list);

/* utilities */

GHashTable *      _g_hash_table_from_string_list      (GthStringList *list);

G_END_DECLS

#endif /* GTH_STRING_LIST_H */
