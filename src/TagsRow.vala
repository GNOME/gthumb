public class Gth.TagsRow : Adw.ActionRow {
	public Gth.TagEntry entry;

	construct {
		entry = new Gth.TagEntry ();
		add_prefix (entry);

		var tag_editor = new Gtk.Box (Gtk.Orientation.VERTICAL, 12);
		tag_editor.margin_top = 6;
		tag_editor.margin_bottom = 6;
		tag_editor.margin_start = 6;
		tag_editor.margin_end = 6;

		tag_entry = new Gtk.Entry ();
		tag_entry.max_width_chars = 20;
		tag_entry.activate.connect (() => add_tag ());
		tag_editor.append (tag_entry);

		var ok_button = new Gtk.Button.with_label (_("Add"));
		ok_button.add_css_class ("suggested-action");
		ok_button.halign = Gtk.Align.END;
		ok_button.clicked.connect (() => add_tag ());
		tag_editor.append (ok_button);

		tag_popup = new Gtk.Popover ();
		tag_popup.child = tag_editor;

		var add_button = new Gtk.MenuButton ();
		add_button.icon_name = "list-add-symbolic";
		add_button.popover = tag_popup;
		add_button.valign = Gtk.Align.CENTER;
		add_button.add_css_class ("flat");
		add_button.notify["active"].connect (() => {
			tag_entry.text = "";
		});
		add_suffix (add_button);
	}

	void add_tag () {
		if (!Strings.empty (tag_entry.text)) {
			entry.add_tag (tag_entry.text);
		}
		tag_popup.popdown ();
	}

	Gtk.Entry tag_entry;
	Gtk.Popover tag_popup;
}
