public class Gth.Template {
	enum State {
		START,
		ENUMERATOR,
		CODE,
		ARG,
		TEXT;
	}

	static bool valid_special_code (unichar ch) {
		return (ch != 0) && (ch != '%') && (ch != '{') && !ch.isspace ();
	}

	public static string[] tokenize (string template, TemplateFlags flags = TemplateFlags.DEFAULT) {
		var tokens = new GenericArray<string> ();
		var split_enumerator = !(TemplateFlags.NO_ENUMERATOR in flags);
		var state = State.START;
		var offset = 0;
		var prev_offset = 0;
		unichar ch;
		var token_start = offset;
		var n_open_brackets = 0;
		while (template.get_next_char (ref offset, out ch)) {
			var next_offset = offset;
			unichar next_ch = 0;
			var has_next = template.get_next_char (ref next_offset, out next_ch);
			var code_prefix = (ch == '%') && has_next && Template.valid_special_code (next_ch);
			var enumerator_prefix = split_enumerator && (ch == '#');
			var add_to_token = false;
			var save_token = false;

#if DEBUG_TOKENIZER
			stdout.printf ("> ch: '%s'\n", ch.to_string ());
#endif

			switch (state) {
			case State.START:
				token_start = prev_offset;
				add_to_token = true;
				if (enumerator_prefix) {
					state = State.ENUMERATOR;
				}
				else if (code_prefix) {
					state = State.CODE;
				}
				else {
					state = State.TEXT;
				}
				break;

			case State.ENUMERATOR:
				if (ch != '#') {
					add_to_token = false;
					save_token = true;
					state = State.START;
				}
				else {
					add_to_token = true;
				}
				break;

			case State.CODE:
				if (next_ch == '{') {
					add_to_token = true;
					state = State.ARG;
					n_open_brackets = 0;
				}
				else {
					add_to_token = true;
					save_token = true;
					state = State.START;
				}
				break;

			case State.ARG:
				if (ch == '{') {
					n_open_brackets++;
				}
				else if (ch == '}') {
					n_open_brackets--;
				}
				if ((n_open_brackets == 0) && (ch == '}') && (next_ch != '{')) {
					add_to_token = true;
					save_token = true;
					state = State.START;
				}
				else {
					add_to_token = true;
				}
				break;

			case State.TEXT:
				if (enumerator_prefix || code_prefix) {
					add_to_token = false;
					save_token = true;
					state = State.START;
				}
				else {
					add_to_token = true;
				}
				break;
			}

#if DEBUG_TOKENIZER
			stdout.printf ("  new state: %s\n", state.to_string ());
			stdout.printf ("  save_token: %s\n", save_token.to_string ());
			stdout.printf ("  add_to_token: %s\n", add_to_token.to_string ());
			stdout.printf ("  n_open_brackets: %d\n", n_open_brackets);
#endif

			if (!add_to_token) {
				offset = prev_offset;
			}

			if (save_token) {
				tokens.add (template.slice (token_start, offset));
				token_start = offset;
			}

			prev_offset = offset;
		}
		if (offset > token_start) {
			tokens.add (template.slice (token_start, offset));
		}
		return tokens.data;
	}

	public static unichar get_token_code (string token) {
		if (token == null) {
			return 0;
		}

		int offset = 0;
		unichar ch;

		// First character
		if (!token.get_next_char (ref offset, out ch)) {
			return 0;
		}
		if (ch != '%') {
			return 0;
		}

		// Second character
		if (!token.get_next_char (ref offset, out ch)) {
			return 0;
		}

		return Template.valid_special_code (ch) ? ch : 0;
	}

	public static string[] get_token_args (string token) {
		var args = new GenericArray<string> ();
		var state = State.START;
		var offset = 0;
		var prev_offset = 0;
		unichar ch;
		var arg_start = offset;
		var n_open_brackets = 0;
		while (token.get_next_char (ref offset, out ch)) {
			var next_offset = offset;
			unichar next_ch = 0;
			var has_next = token.get_next_char (ref next_offset, out next_ch);
			var save_arg = false;

			switch (state) {
			case State.START:
				if ((ch == '%') && Template.valid_special_code (next_ch)) {
					offset = next_offset;// Skip the code
					state = State.CODE;
				}
				break;

			case State.CODE:
				if (ch == '{') {
					n_open_brackets = 1;
					arg_start = offset;
					state = State.ARG;
				}
				else {
					// No arguments for this code
					return args.data;
				}
				break;

			case State.ARG:
				if (ch == '}') {
					n_open_brackets--;
					if (n_open_brackets == 0) {
						args.add (token.slice (arg_start, prev_offset).strip ());
						state = State.CODE;
					}
				}
				else {
					if (ch == '{') {
						n_open_brackets++;
					}
				}
				break;
			}

#if DEBUG_TOKENIZER
			stdout.printf ("  new state: %s\n", state.to_string ());
			stdout.printf ("  n_open_brackets: %d\n", n_open_brackets);
#endif

			prev_offset = offset;
		}
		return args.data;
	}

	public static bool token_has_code (string token, unichar code) {
		return Template.get_token_code (token) == code;
	}

	static void _for_each_token (string? template, TemplateFlags flags, unichar parent_code, TemplateForEachFunc? func)	{
		if ((template == null) || (func == null)) {
			return;
		}
		var with_enumerator = !(TemplateFlags.NO_ENUMERATOR in flags);
		var stop = false;
		var tokens = Template.tokenize (template);
		foreach (unowned var token in tokens) {
			var code = Template.get_token_code (token);
			if (code != 0) {
				var args = Template.get_token_args (token);
				foreach (unowned var arg in args) {
					_for_each_token (arg, flags, code, func);
				}
				stop = func (parent_code, code, args);
			}
			else if (with_enumerator && (token.get_char (0) == '#')) {
				stop = func (parent_code, '#', { token });
			}
			else {
				stop = func (parent_code, code, { token });
			}
		}
	}

	public static void for_each_token (string? template, TemplateFlags flags, TemplateForEachFunc? func) {
		_for_each_token (template, flags, 0, func);
	}

	static string _eval (string template, TemplateFlags flags, unichar parent_code, TemplateEvalFunc? func) {
		var result = new StringBuilder ();
		var with_enumerator = !(TemplateFlags.NO_ENUMERATOR in flags);
		var stop = false;
		var tokens = Template.tokenize (template);
		foreach (unowned var token in tokens) {
			var code = Template.get_token_code (token);
			if (code != 0) {
				if (func != null) {
					var args = new GenericArray<string>();
					var input_args = Template.get_token_args (token);
					foreach (unowned var arg in input_args) {
						args.add (_eval (arg, flags, code, func));
					}
					stop = func (flags, parent_code, code, args.data, result);
				}
			}
			else if (with_enumerator && (token.get_char (0) == '#')) {
				if (func != null) {
					stop = func (flags, parent_code, '#', { token }, result);
				}
			}
			else {
				// Literal string
				if (TemplateFlags.PREVIEW in flags) {
					result.append (Markup.escape_text (token));
				}
				else {
					result.append (token);
				}
			}
		}
		return result.str;
	}

	public static string eval (string template, TemplateFlags flags, TemplateEvalFunc? func) {
		return _eval (template, flags, 0, func);
	}

	public static void append_template_code (StringBuilder str,	unichar code, string[] args) {
		if (code == 0) {
			// Literal string, not a special code.
			if (args[0] != null) {
				str.append (args[0]);
			}
		}
		else {
			str.append_c ('%');
			str.append_unichar (code);
			foreach (unowned var arg in args) {
				str.append_printf ("{ %s }", arg);
			}
		}
	}

	public static string? replace_enumerator (string token, int number) {
		int offset = 0;
		unichar ch;
		while (token.get_next_char (ref offset, out ch)) {
			if (ch != '#') {
				return null; // Syntax error
			}
		}
		// offset == token length
		if (offset == 0) {
			return null;
		}
		var format = "%%0%dd".printf (offset);
		return format.printf (number);
	}
}

[Flags]
public enum Gth.TemplateFlags {
	DEFAULT,
	NO_ENUMERATOR,
	PREVIEW,
	PARTIAL;
}

public delegate bool Gth.TemplateForEachFunc (
	unichar parent_code,
	unichar code,
	string[] args);

public delegate bool Gth.TemplateEvalFunc (
	Gth.TemplateFlags flags,
	unichar parent_code,
	unichar code,
	string[] args,
	StringBuilder result);

public delegate string Gth.TemplatePreviewFunc (
	string template,
	Gth.TemplateFlags flags);
