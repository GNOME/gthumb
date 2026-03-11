public class Gth.ScriptTemplate {
	public class Code {
		public const unichar URI = 'U';
		public const unichar PATH = 'F';
		public const unichar BASENAME = 'B';
		public const unichar BASENAME_NO_EXTENSION = 'N';
		public const unichar EXTENSION = 'E';
		public const unichar PARENT_PATH = 'P';
		public const unichar TIMESTAMP = 'T';
		public const unichar ASK_VALUE = '?';
		public const unichar FILE_ATTRIBUTE = 'A';
		public const unichar QUOTE = 'Q';
	}

	public GenericList<FileData> files;
	public string template;

	public ScriptTemplate (string _template, TemplateFlags _flags) {
		template = _template;
		flags = _flags;
		script = null;
		parameters = null;
	}

	public async string read_parameters (Gth.MainWindow window, string title, bool can_skip, Job job) throws Error {
		parameters = new GenericArray<ScriptParameter> ();
		Template.for_each_token (template, flags, (parent_code, code, args) => {
			if (code == Code.ASK_VALUE) {
				var asked_value = new ScriptParameter ();
				asked_value.title = args[0].strip ();
				if (args[1] != null) {
					asked_value.default_value = args[1].strip ();
				}
				parameters.add (asked_value);
			}
			return false;
		});
		if (parameters.length > 0) {
			var read_param = new ReadScriptParameters ();
			if (files.length () == 1) {
				read_param.file = files.first ();
			}
			yield read_param.ask_user (window, title, parameters, can_skip, job);
		}
		return eval ();
	}

	public string eval (unichar parent_code = 0) {
		if (TemplateFlags.PREVIEW in flags) {
			if (files == null) {
				files = new GenericList<FileData>();
				files.model.append (new FileData (File.new_for_uri ("file:///home/user/images/filename.jpeg")));
			}
		}
		return Template.eval_with_parent_code (template, flags, parent_code, (flags, parent_code, code, args, str) => {
			return eval_template (flags, parent_code, code, args, str);
		});
	}

	bool eval_template (Gth.TemplateFlags flags, unichar parent_code, unichar code, string[] args, StringBuilder str) {
		var preview = TemplateFlags.PREVIEW in flags;
		var quote_values = !(TemplateFlags.PARTIAL in flags) && (parent_code == 0);
		var highlight = preview && (code != 0) && (parent_code == 0);

		if (parent_code == Code.TIMESTAMP) {
			if (preview) {
				str.append ("<span foreground=\"#4696f8\">");
				var now = new GLib.DateTime.now_local ();
				str.append (now.format ("%" + code.to_string ()));
				str.append ("</span>");
			}
			else {
				// strftime code, return the code itself.
				Template.append_template_code (str, code, args);
			}
			return false;
		}

		if (highlight) {
			str.append ("<span foreground=\"#4696f8\">");
		}

		switch (code) {
		case Code.URI:
			append_file_list (str, quote_values, (file_data) => file_data.file.get_uri ());
			break;

		case Code.PATH:
			append_file_list (str, quote_values, (file_data) => file_data.file.get_path ());
			break;

		case Code.BASENAME:
			append_file_list (str, quote_values, (file_data) => file_data.file.get_basename ());
			break;

		case Code.BASENAME_NO_EXTENSION:
			append_file_list (str, quote_values, (file_data) => {
				var basename = file_data.file.get_basename ();
				return Util.remove_extension (basename);
			});
			break;

		case Code.EXTENSION:
			append_file_list (str, quote_values, (file_data) => {
				var basename = file_data.file.get_basename ();
				return "." + Util.get_extension (basename);
			});
			break;

		case Code.PARENT_PATH:
			append_file_list (str, quote_values, (file_data) => {
				var parent = file_data.file.get_parent ();
				return parent.get_path ();
			});
			break;

		case Code.TIMESTAMP:
			var now = new GLib.DateTime.now_local ();
			var format = args[0];
			var timestamp = now.format (format ?? DEFAULT_STRFTIME_FORMAT);
			if (quote_values) {
				timestamp = Shell.quote (timestamp);
			}
			str.append (timestamp);
			break;

		case Code.ASK_VALUE:
			if (preview) {
				if ((args[0] != null) && !Strings.all_spaces (args[1])) 	{
					str.append (args[1]);
				}
				else if (!Strings.all_spaces (args[0])) {
					str.append (Markup.printf_escaped ("{ %s }", args[0]));
				}
				else {
					str.append_unichar (code);
				}
			}
			else if (parameters != null) {
				var parameter = parameters[last_parameter];
				if (parameter != null) {
					if (quote_values) {
						str.append (Shell.quote (parameter.value));
					}
					else {
						str.append (parameter.value);
					}
				}
				last_parameter++;
			}
			break;

		case Code.FILE_ATTRIBUTE:
			if (preview) {
				str.append_printf ("{ %s }", args[0]);
			}
			else {
				append_attribute_list (str, quote_values, args[0]);
			}
			break;

		case Code.QUOTE:
			if (args[0] != null) {
				str.append (Shell.quote (args[0]));
			}
			break;

		default:
			// Code not recognized, return the code itself.
			Template.append_template_code (str, code, args);
			break;
		}

		if (highlight) {
			str.append ("</span>");
		}

		return false;
	}

	void append_attribute_list (StringBuilder str, bool quote_values, string id) {
		if (id == null) {
			return;
		}
		var idx = 0;
		foreach (unowned var file in files) {
			var value = file.get_attribute_as_string (id);
			if (value == null) {
				continue;
			}
			if (idx > 0) {
				str.append_c (' ');
			}
			value = Strings.remove_newlines (value);
			if (quote_values) {
				value = Shell.quote (value);
			}
			str.append (value);
			idx++;
		}
	}

	void append_file_list (StringBuilder str, bool quote_value, ValueFunc value_func) {
		var idx = 0;
		foreach (unowned var file in files) {
			var value = value_func (file);
			var quoted = quote_value ? Shell.quote (value) : value;
			if (idx > 0) {
				str.append_c (' ');
			}
			str.append (quoted);
			idx++;
		}
	}

	Script script;
	TemplateFlags flags;
	int last_parameter = 0;
	GenericArray<ScriptParameter> parameters;

	delegate string ValueFunc (FileData file_data);
}

public class Gth.ScriptParameter {
	public string title;
	public string default_value;
	public string value;

	public ScriptParameter () {
		title = _("Value");
		default_value = null;
		value = null;
	}
}
