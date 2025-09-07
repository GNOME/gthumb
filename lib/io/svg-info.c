#include "svg-info.h"

enum SvgState {
	SVG_STATE_START,
	SVG_STATE_SVG_TAG,
	SVG_STATE_VIEWBOX,
	SVG_STATE_COMMENT
};

struct Tokenizer {
	guchar *buffer;
	int buffer_size;
	int start;
	int next;
};


static gboolean tokenizer_set_start (struct Tokenizer *tok, int start) {
	tok->start = start;
	tok->next = start;
	return TRUE;
}


static gboolean tokenizer_match_ch (struct Tokenizer *tok, guchar ch) {
	if ((tok->next < tok->buffer_size) && (tok->buffer[tok->next] == ch)) {
		tok->next++;
		return TRUE;
	}
	return FALSE;
}


static gboolean tokenizer_match_string (struct Tokenizer *tok, const char *str) {
	int index = 0;
	while (tok->next < tok->buffer_size) {
		if (str[index] == 0) {
			break;
		}
		if (tok->buffer[tok->next] != str[index]) {
			return FALSE;
		}
		tok->next++;
		index++;
	}
	return str[index] == 0;
}


static gboolean tokenizer_match_until_end_of_string (struct Tokenizer *tok) {
	while (tok->next < tok->buffer_size) {
		if (tok->buffer[tok->next] == '"') {
			return TRUE;
		}
		tok->next++;
	}
	return FALSE;
}


static gboolean tokenizer_get_next (struct Tokenizer *tok, int *next) {
	*next = tok->next;
	return TRUE;
}


enum SvgLengthState {
	SVG_LENGTH_STATE_DECIMAL,
	SVG_LENGTH_STATE_FRACTION,
	SVG_LENGTH_STATE_UNIT,
	SVG_LENGTH_STATE_END
};


static gboolean _svg_parse_length (char *buffer, int buffer_size, int *start, int *x) {
	enum SvgLengthState state = SVG_LENGTH_STATE_DECIMAL;
	int digits = 0;
	int number = 0;
	int index;
	for (index = *start; index < buffer_size; index++) {
		char ch = buffer[index];
		switch (state) {
		case SVG_LENGTH_STATE_DECIMAL:
			if ((ch >= '0') && (ch <= '9')) {
				number = (number * 10) + (ch - '0');
				digits++;
				continue;
			}
			else if (ch == '.') {
				state = SVG_LENGTH_STATE_FRACTION;
				continue;
			}
			else if ((ch >= 'a') && (ch <= 'z')) {
				state = SVG_LENGTH_STATE_UNIT;
				continue;
			}
			else if (ch == ' ') {
				continue;
			}
			state = SVG_LENGTH_STATE_END;
			break;

		case SVG_LENGTH_STATE_FRACTION:
			if ((ch >= '0') && (ch <= '9')) {
				// Ignored
				continue;
			}
			else if ((ch >= 'a') && (ch <= 'z')) {
				state = SVG_LENGTH_STATE_UNIT;
				continue;
			}
			state = SVG_LENGTH_STATE_END;
			break;

		case SVG_LENGTH_STATE_UNIT:
			if ((ch >= 'a') && (ch <= 'z')) {
				continue;
			}
			state = SVG_LENGTH_STATE_END;
			break;
		}
		if (state == SVG_LENGTH_STATE_END) {
			break;
		}
	}
	if (digits == 0) {
		return FALSE;
	}
	if (x != NULL) *x = number;
	*start = index;
	return TRUE;
}


static gboolean _svg_parse_integer (char *buffer, int start, int end, int *x) {
	return _svg_parse_length (buffer, end, &start, x);
}


static gboolean _svg_parse_viewbox (char *buffer, int start, int end, int *x, int *y, int *width, int *height) {
	if (!_svg_parse_length (buffer, end, &start, x)) {
		return FALSE;
	}
	if (!_svg_parse_length (buffer, end, &start, y)) {
		return FALSE;
	}
	if (!_svg_parse_length (buffer, end, &start, width)) {
		return FALSE;
	}
	if (!_svg_parse_length (buffer, end, &start, height)) {
		return FALSE;
	}
	return TRUE;
}


static gboolean tokenizer_debug (const char *str) {
	g_print ("  %s\n", str);
	return TRUE;
}


gboolean load_svg_info (const char *buffer, int buffer_size, GthImageInfo *image_info, GCancellable *cancellable) {
	//g_print ("> load_svg_info\n");
	image_info->width = -1;
	image_info->height = -1;
	struct Tokenizer tokenizer;
	tokenizer.buffer = buffer;
	tokenizer.buffer_size = buffer_size;
	enum SvgState state = SVG_STATE_START;
	int offset = 0;
	int value_start, value_end;
	while (offset < buffer_size) {
		if ((state == SVG_STATE_START)
			&& tokenizer_set_start (&tokenizer, offset)
			&& tokenizer_match_ch (&tokenizer, '<')
			&& tokenizer_match_string (&tokenizer, "svg"))
		{
			//g_print ("  SVG_TAG\n");
			tokenizer_get_next (&tokenizer, &offset);
			state = SVG_STATE_SVG_TAG;
			continue;
		}

		if ((state == SVG_STATE_SVG_TAG)
			&& tokenizer_set_start (&tokenizer, offset)
			&& tokenizer_match_string (&tokenizer, "viewBox")
			&& tokenizer_match_ch (&tokenizer, '=')
			&& tokenizer_match_ch (&tokenizer, '"')
			&& tokenizer_get_next (&tokenizer, &value_start)
			&& tokenizer_match_until_end_of_string (&tokenizer)
			&& tokenizer_get_next (&tokenizer, &value_end)
			&& tokenizer_match_ch (&tokenizer, '"'))
		{
			tokenizer_get_next (&tokenizer, &offset);
			int x, y, width, height;
			if (_svg_parse_viewbox (buffer, value_start, value_end, &x, &y, &width, &height)) {
				image_info->width = width;
				image_info->height = height;
				return TRUE;
			}
			continue;
		}

		if ((state == SVG_STATE_SVG_TAG)
			&& tokenizer_set_start (&tokenizer, offset)
			&& tokenizer_match_string (&tokenizer, "width")
			&& tokenizer_match_ch (&tokenizer, '=')
			&& tokenizer_match_ch (&tokenizer, '"')
			&& tokenizer_get_next (&tokenizer, &value_start)
			&& tokenizer_match_until_end_of_string (&tokenizer)
			&& tokenizer_get_next (&tokenizer, &value_end)
			&& tokenizer_match_ch (&tokenizer, '"'))
		{
			if (!_svg_parse_integer (buffer, value_start, value_end, &image_info->width)) {
				return FALSE;
			}
		}

		if ((state == SVG_STATE_SVG_TAG)
			&& tokenizer_set_start (&tokenizer, offset)
			&& tokenizer_match_string (&tokenizer, "height")
			&& tokenizer_match_ch (&tokenizer, '=')
			&& tokenizer_match_ch (&tokenizer, '"')
			&& tokenizer_get_next (&tokenizer, &value_start)
			&& tokenizer_match_until_end_of_string (&tokenizer)
			&& tokenizer_get_next (&tokenizer, &value_end)
			&& tokenizer_match_ch (&tokenizer, '"'))
		{
			if (!_svg_parse_integer (buffer, value_start, value_end, &image_info->height)) {
				return FALSE;
			}
		}

		if ((state == SVG_STATE_SVG_TAG)
			&& tokenizer_set_start (&tokenizer, offset)
			&& tokenizer_match_ch (&tokenizer, '>'))
		{
			// End of SVG tag
			return (image_info->width != -1) && (image_info->height != -1);
		}
		offset++;
	}
	return FALSE;
}
