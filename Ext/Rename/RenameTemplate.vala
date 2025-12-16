public class Gth.RenameTemplate {
	public class Code {
		public const unichar BASENAME = 'F';
		public const unichar BASENAME_NO_EXTENSION = 'G';
		public const unichar EXTENSION = 'E';
		public const unichar ORIGINAL_ENUMERATOR = 'N';
		public const unichar CREATION_TIME = 'C';
		public const unichar MODIFICATION_TIME = 'M';
		public const unichar DIGITALIZATION_TIME = 'D';
		public const unichar TIMESTAMP = 'T';
		public const unichar FILE_ATTRIBUTE = 'A';
	}

	public string template;
	public FileData file_data;
	public int enumerator_index = 0;

	public RenameTemplate (string _template, TemplateFlags _flags) {
		template = _template;
		flags = _flags;
		script = null;
		parameters = null;
	}

	FileData preview_file = null;
	GLib.DateTime preview_date = null;

	public string get_preview (unichar parent_code = 0) {
		if (preview_date == null) {
			preview_date = new GLib.DateTime.now_local ();
		}
		if (preview_file == null) {
			preview_file = new FileData (File.new_for_uri ("file:///home/user/images/filename.jpeg"));
			preview_file.info.set_creation_date_time (preview_date);
			preview_file.info.set_modification_date_time (preview_date);
			var exif_date = Lib.date_time_to_exif_date (preview_date);
			var metadata = new Metadata.for_string (exif_date);
			preview_file.info.set_attribute_object ("Exif::Photo::DateTimeDigitized", metadata);
		}
		return get_new_name (preview_file, parent_code);
	}

	public string get_new_name (FileData file_data, unichar parent_code = 0) {
		return Template.eval_with_parent_code (template, flags, parent_code, (flags, parent_code, code, args, str) => {
			return eval_template (file_data, flags, parent_code, code, args, str);
		});
	}

	bool eval_template (FileData file_data, Gth.TemplateFlags flags, unichar parent_code, unichar code, string[] args, StringBuilder str) {
		var preview = TemplateFlags.PREVIEW in flags;

		if ((parent_code == Code.TIMESTAMP)
			|| (parent_code == Code.CREATION_TIME)
			|| (parent_code == Code.MODIFICATION_TIME)
			|| (parent_code == Code.DIGITALIZATION_TIME))
		{
			var highlight = preview && !(TemplateFlags.NO_HIGHLIGHT in flags);
			if (preview) {
				var format = "%" + code.to_string ();
				var value = preview_date.format (format);
				// If the format is not valid show it as literal text.
				if (value != null) {
					if (highlight) {
						str.append ("<span foreground=\"#4696f8\">");
					}
					str.append (value);
					if (highlight) {
						str.append ("</span>");
					}
				}
				else {
					str.append (format);
				}
			}
			else {
				// strftime code, return the code itself.
				Template.append_template_code (str, code, args);
			}
			return false;
		}

		var highlight = preview && (code != 0) && (parent_code == 0) && !(TemplateFlags.NO_HIGHLIGHT in flags);
		if (highlight) {
			str.append ("<span foreground=\"#4696f8\">");
		}

		switch (code) {
		case '#':
			str.append (Template.replace_enumerator (args[0], preview ? 1 : enumerator_index));
			break;

		case Code.BASENAME:
			str.append (file_data.file.get_basename ());
			break;

		case Code.BASENAME_NO_EXTENSION:
			var basename = file_data.file.get_basename ();
			str.append (Util.remove_extension (basename));
			break;

		case Code.EXTENSION:
			var basename = file_data.file.get_basename ();
			str.append ("." + Util.get_extension (basename));
			break;

		case Code.ORIGINAL_ENUMERATOR:
			if (preview) {
				str.append ("123");
			}
			else {
				var basename = file_data.file.get_basename ();
				var value = Strings.get_number_in_name (basename);
				if (value != null) {
					str.append (value);
				}
			}
			break;

		case Code.CREATION_TIME:
			var time = file_data.get_creation_time ();
			var format = args[0];
			str.append (time.strftime (format ?? DEFAULT_STRFTIME_FORMAT));
			break;

		case Code.MODIFICATION_TIME:
			var time = file_data.get_modification_time ();
			var format = args[0];
			str.append (time.strftime (format ?? DEFAULT_STRFTIME_FORMAT));
			break;

		case Code.DIGITALIZATION_TIME:
			var time = file_data.get_digitalization_time ();
			var format = args[0];
			str.append (time.strftime (format ?? DEFAULT_STRFTIME_FORMAT));
			break;

		case Code.TIMESTAMP:
			var now = new GLib.DateTime.now_local ();
			var format = args[0];
			var timestamp = now.format (format ?? DEFAULT_STRFTIME_FORMAT);
			str.append (timestamp);
			break;

		case Code.FILE_ATTRIBUTE:
			if (preview) {
				str.append_printf ("{ %s }", args[0]);
			}
			else {
				var value = file_data.get_attribute_as_string (args[0]);
				if (value != null) {
					value = Strings.remove_newlines (value);
					str.append (value);
				}
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

	Script script;
	TemplateFlags flags;
	int last_parameter = 0;
	GenericArray<ScriptParameter> parameters;

	delegate string ValueFunc (FileData file_data);
}
