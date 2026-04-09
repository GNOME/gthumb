[GtkTemplate (ui = "/org/gnome/gthumb/ui/selection-view.ui")]
public class Gth.SelectionView : Gtk.Box, Gth.PropertyView {
	public unowned string get_id () {
		return "selection";
	}

	public unowned string get_title () {
		return _("Selection");
	}

	public unowned string get_icon () {
		return "gth-document-properties-symbolic";
	}

	public bool can_view (Gth.FileData file_data) {
		return false;
	}

	public void set_file (Gth.FileData? file_data) {
	}

	public bool with_search () {
		return false;
	}

	public void set_search (string? text) {
	}

	public void set_selection_info (uint files, uint64 size) {
		property_list.model.remove_all ();
		var property = new FileProperty () {
			id = "files",
			name = _("Files"),
			value = "%u".printf (files),
			category = null,
		};
		property_list.model.append (property);

		property = new FileProperty () {
			id = "bytes",
			name = _("Bytes"),
			value = GLib.format_size (size),
			category = null,
		};
		property_list.model.append (property);
	}

	public void open_context_menu (FilePropertyItem item, int x, int y) {
	}

	construct {
		property_list = new GenericList<FileProperty> ();
		var selection_model = new Gtk.NoSelection (property_list.model);
		list_view.model = selection_model;

		var factory = new Gtk.SignalListItemFactory ();
		factory.setup.connect ((item) => {
			var list_item = item as Gtk.ListItem;
			list_item.child = new FilePropertyItem (this);
		});
		factory.bind.connect ((item) => {
			var list_item = item as Gtk.ListItem;
			var property = list_item.item as FileProperty;
			var property_item = list_item.child as FilePropertyItem;
			property_item.title.label = property.name;
			property_item.subtitle.label = property.value;
			if (property_item.parent != null) {
				property_item.parent.add_css_class ("property-widget");
			}
		});
		list_view.factory = factory;

		var header_factory = new Gtk.SignalListItemFactory ();
		header_factory.setup.connect ((item) => {
			var list_header = item as Gtk.ListHeader;
			var label = new Gtk.Label ("");
			label.add_css_class ("property-header");
			label.halign = Gtk.Align.START;
			list_header.child = label;
		});
		header_factory.bind.connect ((item) => {
			var list_header = item as Gtk.ListHeader;
			var label = list_header.child as Gtk.Label;
			label.label = _("Selection");
		});
		list_view.header_factory = header_factory;
	}

	GenericList<FileProperty> property_list;
	[GtkChild] protected unowned Gtk.ListView list_view;
}
