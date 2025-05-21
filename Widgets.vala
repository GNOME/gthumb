public class Gth.Widgets {
	public static Gtk.ListBox new_list_box () {
		var list_box = new Gtk.ListBox ();
		list_box.add_css_class ("boxed-list");
		list_box.add_css_class ("rich-list");
		list_box.selection_mode = Gtk.SelectionMode.NONE;
		return list_box;
	}

	public static Gtk.Box new_list_box_header () {
		var header = new Gtk.Box (Gtk.Orientation.HORIZONTAL, 0);
		header.add_css_class ("gth-list-box-header");
		return header;
	}

	public static Gtk.Label new_list_box_label () {
		var label = new Gtk.Label (_("Filters"));
		label.add_css_class ("heading");
		label.add_css_class ("h4");
		label.hexpand = true;
		label.halign = Gtk.Align.START;
		return label;
	}

	[Flags]
	public enum ListBoxRowFlags {
		TITLE_BOX,
		PREFIXES,
		SUFFIXES,
	}

	public const ListBoxRowFlags LIST_BOX_ROW_ALL = ListBoxRowFlags.PREFIXES | ListBoxRowFlags.SUFFIXES;

	public static Gtk.Box new_list_box_row (out Gtk.Box title_box) {
		return new_list_box_row_full (out title_box, null, null, ListBoxRowFlags.TITLE_BOX);
	}

	public static Gtk.Box new_list_box_row_with_suffix (out Gtk.Box title_box, out Gtk.Box? suffixes) {
		return new_list_box_row_full (out title_box, null, out suffixes, ListBoxRowFlags.TITLE_BOX | ListBoxRowFlags.SUFFIXES);
	}

	public static Gtk.Box new_list_box_row_full (out Gtk.Box title_box, out Gtk.Box? prefixes = null, out Gtk.Box? suffixes = null, ListBoxRowFlags flags = LIST_BOX_ROW_ALL) {
		var row = new Gtk.Box (Gtk.Orientation.HORIZONTAL, HORIZONTAL_SPACING);
		row.add_css_class ("header");

		if (ListBoxRowFlags.PREFIXES in flags) {
			prefixes = new Gtk.Box (Gtk.Orientation.HORIZONTAL, HORIZONTAL_SPACING);
			prefixes.add_css_class ("prefixes");
			row.append (prefixes);
		}

		title_box = new Gtk.Box (Gtk.Orientation.HORIZONTAL, HORIZONTAL_SPACING);
		title_box.add_css_class ("title_box");
		title_box.hexpand = true;
		row.append (title_box);

		if (ListBoxRowFlags.SUFFIXES in flags) {
			suffixes = new Gtk.Box (Gtk.Orientation.HORIZONTAL, HORIZONTAL_SPACING);
			suffixes.vexpand = false;
			suffixes.valign = Gtk.Align.CENTER;
			suffixes.add_css_class ("suffixes");
			row.append (suffixes);
		}

		return row;
	}

	public static Gtk.ScrolledWindow new_scrollable_content () {
		var scrolled = new Gtk.ScrolledWindow ();
		scrolled.set_policy (Gtk.PolicyType.NEVER, Gtk.PolicyType.AUTOMATIC);
		scrolled.add_css_class ("undershoot-top");
		scrolled.vexpand = true;
		return scrolled;
	}

	public static Gtk.Box new_dialog_content () {
		var box = new Gtk.Box (Gtk.Orientation.VERTICAL, 12);
		box.add_css_class ("gth-dialog-content");
		return box;
	}

	public static Gtk.Box new_section () {
		var box = new Gtk.Box (Gtk.Orientation.VERTICAL, VERTICAL_SPACING);
		box.add_css_class ("gth-dialog-section");
		return box;
	}
}
