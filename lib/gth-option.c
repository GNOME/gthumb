#include "lib/gth-option.h"

#undef G_LOG_DOMAIN
#define G_LOG_DOMAIN "GthOption"

enum GthOptionType {
	TYPE_VOID,
	TYPE_BOOL,
	TYPE_INT,
	TYPE_UINT,
	TYPE_ENUM,
	TYPE_FLAGS,
	TYPE_INTV,
	TYPE_UINTV,
	TYPE_STRING,
};

struct _GthOption {
	int ref;
	int id;
	enum GthOptionType type;
	union {
		gboolean bool_v;
		int int_v;
		unsigned int uint_v;
		int enum_v;
		int flags_v;
		int *intv_v;
		unsigned int *uintv_v;
		char *string;
	} data;
	int array_len;
};

GthOption * gth_option_new_void (int id) {
	GthOption *opt = g_new (GthOption, 1);
	opt->ref = 1;
	opt->id = id;
	opt->type = TYPE_VOID;
	//opt->data NOT SET
	opt->array_len = 0;
	return opt;
}

GthOption * gth_option_new_bool (int id, gboolean value) {
	GthOption *opt = g_new (GthOption, 1);
	opt->ref = 1;
	opt->id = id;
	opt->type = TYPE_BOOL;
	opt->data.bool_v = value;
	opt->array_len = 0;
	return opt;
}

GthOption * gth_option_new_int (int id, int value) {
	GthOption *opt = g_new (GthOption, 1);
	opt->ref = 1;
	opt->id = id;
	opt->type = TYPE_INT;
	opt->data.int_v = value;
	opt->array_len = 0;
	return opt;
}

GthOption * gth_option_new_uint (int id, unsigned int value) {
	GthOption *opt = g_new (GthOption, 1);
	opt->ref = 1;
	opt->id = id;
	opt->type = TYPE_UINT;
	opt->data.uint_v = value;
	opt->array_len = 0;
	return opt;
}

GthOption * gth_option_new_enum (int id, int value) {
	GthOption *opt = g_new (GthOption, 1);
	opt->ref = 1;
	opt->id = id;
	opt->type = TYPE_ENUM;
	opt->data.enum_v = value;
	opt->array_len = 0;
	return opt;
}

GthOption * gth_option_new_flags (int id, int value) {
	GthOption *opt = g_new (GthOption, 1);
	opt->ref = 1;
	opt->id = id;
	opt->type = TYPE_FLAGS;
	opt->data.flags_v = value;
	opt->array_len = 0;
	return opt;
}

GthOption * gth_option_new_intv (int id, int *value, int len) {
	GthOption *opt = g_new (GthOption, 1);
	opt->ref = 1;
	opt->id = id;
	opt->type = TYPE_INTV;
	opt->data.intv_v = g_new (int, len);
	for (int i = 0; i < len; i++)
		opt->data.intv_v[i] = value[i];
	opt->array_len = len;
	return opt;
}

GthOption * gth_option_new_uintv (int id, unsigned int *value, int len) {
	GthOption *opt = g_new (GthOption, 1);
	opt->ref = 1;
	opt->id = id;
	opt->type = TYPE_UINTV;
	opt->data.uintv_v = g_new (unsigned int, len);
	for (int i = 0; i < len; i++)
		opt->data.uintv_v[i] = value[i];
	opt->array_len = len;
	return opt;
}

GthOption * gth_option_new_string (int id, char *value, int len) {
	GthOption *opt = g_new (GthOption, 1);
	opt->ref = 1;
	opt->id = id;
	opt->type = TYPE_STRING;
	opt->data.string = g_strndup (value, len);
	opt->array_len = len;
	return opt;
}

GthOption * gth_option_ref (GthOption *opt) {
	opt->ref++;
	return opt;
}

void gth_option_unref (GthOption *opt) {
	opt->ref--;
	if (opt->ref == 0) {
		switch (opt->type) {
		case TYPE_INTV:
			g_free (opt->data.intv_v);
			break;
		case TYPE_UINTV:
			g_free (opt->data.uintv_v);
			break;
		case TYPE_STRING:
			g_free (opt->data.string);
			break;
		default:
			break;
		}
		g_free (opt);
	}
}

int gth_option_get_id (GthOption *opt) {
	g_return_val_if_fail (opt != NULL, 0);
	return opt->id;
}

gboolean gth_option_get_bool (GthOption *opt, gboolean *value) {
	g_return_val_if_fail (opt != NULL, FALSE);
	if (opt->type != TYPE_BOOL)
		return FALSE;
	*value = opt->data.bool_v;
	return TRUE;
}

gboolean gth_option_get_int (GthOption *opt, int *value) {
	g_return_val_if_fail (opt != NULL, FALSE);
	if (opt->type != TYPE_INT)
		return FALSE;
	*value = opt->data.int_v;
	return TRUE;
}

gboolean gth_option_get_uint (GthOption *opt, unsigned int *value) {
	g_return_val_if_fail (opt != NULL, FALSE);
	if (opt->type != TYPE_UINT)
		return FALSE;
	*value = opt->data.uint_v;
	return TRUE;
}

gboolean gth_option_get_enum (GthOption *opt, int *value) {
	g_return_val_if_fail (opt != NULL, FALSE);
	if (opt->type != TYPE_ENUM)
		return FALSE;
	*value = opt->data.enum_v;
	return TRUE;
}

gboolean gth_option_get_flags (GthOption *opt, int *value) {
	g_return_val_if_fail (opt != NULL, FALSE);
	if (opt->type != TYPE_FLAGS)
		return FALSE;
	*value = opt->data.flags_v;
	return TRUE;
}

gboolean gth_option_get_intv (GthOption *opt, int **value, int *len) {
	g_return_val_if_fail (opt != NULL, FALSE);
	if (opt->type != TYPE_INTV)
		return FALSE;
	*value = g_new (int, opt->array_len);
	for (int i = 0; i < opt->array_len; i++)
		(*value)[i] = opt->data.intv_v[i];
	*len = opt->array_len;
	return TRUE;
}

gboolean gth_option_get_uintv (GthOption *opt, unsigned int **value, int *len) {
	g_return_val_if_fail (opt != NULL, FALSE);
	if (opt->type != TYPE_UINTV)
		return FALSE;
	*value = g_new (int, opt->array_len);
	for (int i = 0; i < opt->array_len; i++)
		(*value)[i] = opt->data.uintv_v[i];
	*len = opt->array_len;
	return TRUE;
}

gboolean gth_option_get_string (GthOption *opt, char **value, int *len) {
	g_return_val_if_fail (opt != NULL, FALSE);
	if (opt->type != TYPE_STRING)
		return FALSE;
	*value = g_strndup (opt->data.string, opt->array_len);
	*len = opt->array_len;
	return TRUE;
}

gboolean gth_option_equal (GthOption *opt, GthOption *other) {
	g_return_val_if_fail (opt != NULL, FALSE);
	g_return_val_if_fail (other != NULL, FALSE);
	if (opt->type != other->type)
		return FALSE;
	switch (opt->type) {
	case TYPE_VOID:
		return TRUE; // both void
	case TYPE_BOOL:
		return opt->data.bool_v == other->data.bool_v;
	case TYPE_INT:
		return opt->data.int_v == other->data.int_v;
	case TYPE_UINT:
		return opt->data.uint_v == other->data.uint_v;
	case TYPE_ENUM:
		return opt->data.enum_v == other->data.enum_v;
	case TYPE_FLAGS:
		return opt->data.flags_v == other->data.flags_v;
	case TYPE_INTV:
		if (opt->array_len != other->array_len)
			return FALSE;
		for (int i = 0; i < opt->array_len; i++)
			if (opt->data.intv_v[i] != other->data.intv_v[i])
				return FALSE;
		return TRUE;
	case TYPE_UINTV:
		if (opt->array_len != other->array_len)
			return FALSE;
		for (int i = 0; i < opt->array_len; i++)
			if (opt->data.uintv_v[i] != other->data.uintv_v[i])
				return FALSE;
		return TRUE;
	case TYPE_STRING:
		if (opt->array_len != other->array_len)
			return FALSE;
		return g_strcmp0 (opt->data.string, other->data.string) == 0;
	}
	return FALSE;
}

G_DEFINE_BOXED_TYPE (GthOption, gth_option, gth_option_ref, gth_option_unref)
