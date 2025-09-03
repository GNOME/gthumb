public class Gth.TagEntry : Gtk.Box {
	public signal void changed ();

	public void set_list (List<string> list) {
		foreach (unowned var text in list) {
			add_tag (text);
		}
	}

	public GenericArray<string> get_list () {
		var arr = new GenericArray<string>();
		foreach (unowned var tag in tags) {
			arr.add (tag.text);
		}
		return arr;
	}

	public void add_tag (string? text) {
		if (Strings.empty (text)) {
			return;
		}
		var box = new Gtk.Box (Gtk.Orientation.HORIZONTAL, 3);
		box.add_css_class ("tag");
		box.append (new Gtk.Label (text));
		var button = new Gtk.Button.from_icon_name ("gth-cross-small-symbolic");
		button.add_css_class ("flat");
		button.add_css_class ("small");
		button.clicked.connect (() => {
			remove_tag (text);
		});
		box.append (button);
		tags.add (TagInfo () { box = box, text = text });

		wrap_box.append (box);
		title.remove_css_class ("dimmed");
		changed ();
	}

	void remove_tag (string text) {
		var idx = 0;
		while (idx < tags.length) {
			unowned var tag = tags[idx];
			if (tag.text == text) {
				wrap_box.remove (tag.box);
				tags.remove_index (idx);
				break;
			}
			idx++;
		}
		if (tags.length == 0) {
			title.add_css_class ("dimmed");
		}
		changed ();
	}

	public bool get_is_empty () {
		return tags.length == 0;
	}

	construct {
		spacing = 12;
		margin_top = 12;
		margin_bottom = 12;
		tags = new GenericArray<TagInfo?>();

		wrap_box = new Adw.WrapBox ();
		wrap_box.child_spacing = 12;
		wrap_box.line_spacing = 12;
		append (wrap_box);

		title = new Gtk.Label (_("Tags"));
		title.add_css_class ("dimmed");
		title.margin_end = 12;
		wrap_box.append (title);
	}

	Adw.WrapBox wrap_box;
	GenericArray<TagInfo?> tags;
	Gtk.Label title;

	struct TagInfo {
		weak Gtk.Box box;
		string text;
	}
}
