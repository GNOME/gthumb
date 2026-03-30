[GtkTemplate (ui = "/app/gthumb/gthumb/ui/template-page.ui")]
public class Gth.TemplatePage : Adw.NavigationPage {
	public GenericList<TemplateCodeInfo> allowed_codes;
	public unowned TemplatePreviewFunc get_preview_func = null;
	public unichar parent_code = 0;

	public signal void save ();

	public void set_template_text (string template, TemplateFlags _flags = TemplateFlags.DEFAULT) {
		flags = _flags;
		template_tokens = new GenericList<TemplateToken>();
		var tokens = Template.tokenize (template, flags);
		foreach (unowned var value in tokens) {
			var token = new TemplateToken (value);
			token.code = get_code_for_token (token);
			template_tokens.model.append (token);
		}
		token_list.bind_model (template_tokens.model, new_template_token_row);
		if (tokens.length == 0) {
			var token = new TemplateToken.with_code (allowed_codes[0]);
			template_tokens.model.insert (0, token);
		}
		update_preview ();
	}

	public string get_template () {
		var str = new StringBuilder ();
		var idx = 0;
		Gtk.ListBoxRow row = null;
		while ((row = token_list.get_row_at_index (idx)) != null) {
			if (row is TemplateTokenRow) {
				var template_row = row as TemplateTokenRow;
				template_row.append_value (str);
			}
			idx++;
		}
		return str.str;
	}

	[GtkCallback]
	void on_save (Gtk.Button source) {
		save ();
	}

	[GtkCallback]
	void on_add_token (Gtk.Button source) {
		add_token ();
	}

	TemplateCodeInfo? get_code_for_token (TemplateToken token) {
		TemplateCodeInfo text_info = null;
		TemplateCodeInfo space_info = null;
		foreach (var info in allowed_codes) {
			switch (info.type) {
			case TemplateCodeType.ENUMERATOR:
				if (token.value.has_prefix ("#")) {
					return info;
				}
				break;

			case TemplateCodeType.TEXT:
				text_info = info;
				break;

			case TemplateCodeType.SPACE:
				space_info = info;
				break;

			default:
				if (Template.token_has_code (token.value, info.code)) {
					//stdout.printf ("> token: '%s' (code: '%s') (description: '%s')\n",
					//	token.value,
					//	info.code.to_string (),
					//	info.description);
					return info;
				}
				break;
			}
		}
		return (token.value == " ") ? space_info : text_info;
	}

	Gtk.Widget new_template_token_row (Object item) {
		var token = item as TemplateToken;
		var row = new TemplateTokenRow (token);
		row.move_to_row.connect ((source_row, target_row) => {
			move_row_to_position (source_row, target_row.get_index ());
		});
		row.move_to_top.connect ((source_row) => {
			move_row_to_position (source_row, 0);
		});
		row.move_to_bottom.connect ((source_row) => {
			var last_pos = (int) template_tokens.length () - 1;
			move_row_to_position (source_row, last_pos);
		});
		row.delete_row.connect ((source_row) => {
			template_tokens.remove (token);
			changed ();
		});
		row.add_row.connect ((source_row) => {
			add_token (source_row.get_index () + 1);
		});
		row.edit_entry.connect ((source_row, title, entry) => {
			var template_page = new TemplatePage ();
			template_page.title = title;
			template_page.parent_code = source_row.token.code.code;
			// stdout.printf ("> template_page.parent_code: '%s'\n", template_page.parent_code.to_string ());
			if (source_row.token.code.type == TemplateCodeType.DATE) {
				// stdout.printf ("> DATE\n");
				if (date_codes == null) {
					date_codes = create_date_codes ();
				}
				template_page.allowed_codes = date_codes;
			}
			else {
				template_page.allowed_codes = allowed_codes;
			}
			template_page.save_button.label = _("_Apply");
			template_page.get_preview_func = get_preview_func;
			template_page.set_template_text (entry.text, flags | TemplateFlags.PARTIAL);
			template_page.save.connect ((page) => {
				entry.text = page.get_template ();
				Util.pop_page (this);
				changed ();
			});
			Util.push_page (this, template_page);
		});
		row.data_changed.connect (() => changed (true));
		return row;
	}

	void move_row_to_position (TemplateTokenRow row, int target_pos) {
		var source_pos = row.get_index ();
		if ((target_pos >= 0) && (target_pos != source_pos)) {
			template_tokens.model.remove (source_pos);
			template_tokens.model.insert (target_pos, row.token);
			changed ();
		}
	}

	void add_token (int pos = -1) {
		if (code_chooser == null) {
			code_chooser = new TemplateCodeChooser (this);
		}
		code_chooser.pos = (pos == -1) ? (int) template_tokens.length () : pos;
		Util.push_page (this, code_chooser);
	}

	public void add_token_for_code (TemplateCodeInfo code, int pos) {
		var token = new TemplateToken.with_code (code);
		template_tokens.model.insert (pos, token);
		Util.pop_page (this);
		changed ();
	}

	GenericList<TemplateCodeInfo> create_date_codes () {
		var codes = new GenericList<TemplateCodeInfo> ();
		codes.model.append (new TemplateCodeInfo (TemplateCodeType.TEXT, _("Text")));
		codes.model.append (new TemplateCodeInfo (TemplateCodeType.SPACE, _("Space")));
		codes.model.append (new TemplateCodeInfo (TemplateCodeType.SIMPLE, _("Year"), 'Y'));
		codes.model.append (new TemplateCodeInfo (TemplateCodeType.SIMPLE, _("Month"), 'm'));
		codes.model.append (new TemplateCodeInfo (TemplateCodeType.SIMPLE, _("Day"), 'd'));
		codes.model.append (new TemplateCodeInfo (TemplateCodeType.SIMPLE, _("Hour"), 'H'));
		codes.model.append (new TemplateCodeInfo (TemplateCodeType.SIMPLE, _("Minute"), 'M'));
		// Translators: the time second, not the second place.
		codes.model.append (new TemplateCodeInfo (TemplateCodeType.SIMPLE, _("Second"), 'S'));
		return codes;
	}

	construct {
		var empty_row = new Adw.ActionRow ();
		empty_row.title = _("Empty");
		empty_row.sensitive = false;
		empty_row.halign = Gtk.Align.CENTER;
		token_list.set_placeholder (empty_row);
	}

	void update_preview () {
		if (get_preview_func != null) {
			preview.subtitle = get_preview_func (get_template (), flags, parent_code);
		}
	}

	void cancel_preview () {
		if (update_id != 0) {
			Source.remove (update_id);
		}
		update_id = 0;
	}

	void changed (bool after_delay = false) {
		cancel_preview ();
		if (after_delay) {
			update_id = Util.after_timeout (PREVIEW_DELAY, () => {
				update_preview ();
				update_id = 0;
			});
		}
		else {
			update_preview ();
		}
	}

	~TemplatePage() {
		cancel_preview ();
	}

	GenericList<TemplateToken> template_tokens;
	[GtkChild] unowned Gtk.ListBox token_list;
	[GtkChild] public unowned Gtk.Button save_button;
	[GtkChild] public unowned Adw.ActionRow preview;
	TemplateCodeChooser code_chooser = null;
	TemplateFlags flags;
	GenericList<TemplateCodeInfo> date_codes;
	uint update_id = 0;

	const uint PREVIEW_DELAY = 200;
}

public class Gth.TemplateToken : Object {
	public TemplateCodeInfo code;
	public string value;

	public TemplateToken (string _value) {
		value = _value;
		code = null;
	}

	public TemplateToken.with_code (TemplateCodeInfo _code) {
		code = _code;
		value = "";
	}
}
