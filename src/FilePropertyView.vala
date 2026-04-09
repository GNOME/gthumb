[GtkTemplate (ui = "/org/gnome/gthumb/ui/file-property-view.ui")]
public class Gth.FilePropertyView : Gtk.Box, Gth.PropertyView {
	public enum Filter {
		NONE,
		EXIF,
		IPTC,
		XMP,
		OTHER;

		public bool view_embedded () {
			return this != NONE;
		}

		public bool matches (MetadataInfo info) {
			if (this == NONE) {
				return !info.id.has_prefix ("Exif")
					&& !info.id.has_prefix ("Iptc")
					&& !info.id.has_prefix ("Xmp")
					&& (info.category != "Other");
			}
			else if (this == OTHER) {
				return info.category == "Other";
			}
			else {
				return info.id.has_prefix (PREFIX[this]);
			}
		}

		const string[] PREFIX = {
			"",
			"Exif",
			"Iptc",
			"Xmp"
		};
	}

	public Filter filter;

	construct {
		filter = Filter.NONE;
		filter_text = null;
		file_data = null;

		action_group = new SimpleActionGroup ();
		insert_action_group ("property", action_group);

		var action = new SimpleAction ("copy-value", null);
		action.activate.connect ((_action, param) => {
			if (current_property != null) {
				var win = app.active_window as Gth.MainWindow;
				if (win != null) {
					win.copy_text_to_clipboard (current_property.value);
				}
			}
		});
		action_group.add_action (action);

		action = new SimpleAction ("copy-details", null);
		action.activate.connect ((_action, param) => {
			if (current_property != null) {
				string text = null;
				var action_info = actions.get (current_property.id);
				if (action_info != null) {
					if (action_info.type == Action.Type.DETAILS) {
						text = file_data.get_attribute_as_string (action_info.details_attribute);
					}
				}
				if (text != null) {
					var win = app.active_window as Gth.MainWindow;
					if (win != null) {
						win.copy_text_to_clipboard (text);
					}
				}
			}
		});
		action_group.add_action (action);

		action = new SimpleAction ("copy-path", null);
		action.activate.connect ((_action, param) => {
			if (file_data != null) {
				var win = app.active_window as Gth.MainWindow;
				if (win != null) {
					win.copy_text_to_clipboard (file_data.file.get_path ());
				}
			}
		});
		action_group.add_action (action);

		action = new SimpleAction ("open-folder", null);
		action.activate.connect ((_action, param) => {
			if (file_data != null) {
				var win = app.active_window as Gth.MainWindow;
				if (win != null) {
					app.open_window (file_data.file.get_parent (), file_data.file, true);
				}
			}
		});
		action_group.add_action (action);

		action = new SimpleAction ("open-map", null);
		action.activate.connect ((_action, param) => {
			if (file_data != null) {
				var win = app.active_window as Gth.MainWindow;
				if (win != null) {
					var metadata = file_data.info.get_attribute_object ("Metadata::Coordinates") as Gth.Metadata;
					double lat, long;
					if ((metadata != null) && metadata.get_point (out lat, out long)) {
						try {
							var lat_s = Lib.format_double (lat, 6);
							var long_s = Lib.format_double (long, 6);
							var uri = "https://www.openstreetmap.org/?mlat=%s&mlon=%s&zoom=6".printf (lat_s, long_s);
							AppInfo.launch_default_for_uri (uri, null);
						}
						catch (Error error) {
							win.show_error (error);
						}
					}
				}
			}
		});
		action_group.add_action (action);

		actions = new HashTable<string, Action> (str_hash, str_equal);
		actions.set ("standard::display-name", new Action ("property.copy-path", "edit-copy-symbolic", _("Copy Path")));
		actions.set ("Private::File::Location", new Action ("property.open-folder", "folder-symbolic", _("Open")));
		actions.set ("Metadata::Coordinates", new Action ("property.open-map", "gth-map-marker-symbolic", _("View on OpenStreetMap")));
		actions.set ("Private::File::DisplaySize", new Action.with_details ("Private::File::Size"));
		actions.set ("Private::File::ContentType", new Action.with_details ("standard::content-type,standard::fast-content-type"));

		property_list = new GenericList<FileProperty> ();
		property_filter = new Gtk.CustomFilter ((obj) => {
			if (Strings.empty (filter_text))
				return true;
			unowned var prop = obj as FileProperty;
			return prop.name.casefold ().contains (filter_text) || prop.value.casefold ().contains (filter_text);
		});
		var filter_model = new Gtk.FilterListModel (property_list.model, property_filter);
		var sort_model = new Gtk.SortListModel (filter_model, null);
		sort_model.section_sorter = new Gtk.CustomSorter ((a, b) => {
			unowned var prop_a = a as FileProperty;
			unowned var prop_b = b as FileProperty;
			if ((prop_a.category == null) || (prop_b.category == null)) {
				return 0;
			}
			return Util.intcmp (prop_a.category.sort_order, prop_b.category.sort_order);
		});
		var selection_model = new Gtk.NoSelection (sort_model);
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
			//property_item.subtitle.wrap_mode = (property.id == "Metadata::Description") ? Pango.WrapMode.WORD_CHAR : Pango.WrapMode.CHAR;
			property_item.property = property;
			property_item.description.visible = false;
			var action_info = actions.get (property.id);
			if (action_info != null) {
				if (action_info.type == Action.Type.DETAILS) {
					property_item.description.label = file_data.get_attribute_as_string (action_info.details_attribute);
					property_item.description.visible = true;
					property_item.button.visible = false;
				}
				else {
					property_item.button.icon_name = action_info.icon_name;
					property_item.button.tooltip_text = action_info.tooltip;
					property_item.button.set_detailed_action_name (action_info.detailed_action_name);
					property_item.button.visible = true;
				}
			}
			else {
				property_item.button.visible = false;
				property_item.description.visible = false;
			}
			if (property_item.parent != null) {
				property_item.parent.add_css_class ("property-widget");
				if (property.id == "Metadata::Description") {
					property_item.parent.add_css_class ("description");
				}
				else {
					property_item.parent.remove_css_class ("description");
				}
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
			var property = list_header.item as FileProperty;
			label.label = (property.category != null) ? _(property.category.display_name) : "";
		});
		list_view.header_factory = header_factory;
	}

	public virtual unowned string get_id () {
		return "file-properties";
	}

	public virtual unowned string get_title () {
		return _("Properties");
	}

	public virtual unowned string get_icon () {
		return "gth-document-properties-symbolic";
	}

	public virtual bool can_view (Gth.FileData file_data) {
		if (filter == Filter.NONE) {
			return true;
		}
		var data_available = false;
		foreach (unowned var info in MetadataInfo.get_all ()) {
			if (info.id == null) {
				continue;
			}
			if (!(MetadataFlags.ALLOW_IN_PROPERTIES_VIEW in info.flags)) {
				continue;
			}
			if (filter.matches (info)) {
				var value = file_data.get_attribute_as_string (info.id);
				if (!Strings.empty (value)) {
					data_available = true;
					break;
				}
			}
		}
		return data_available;
	}

	public void set_file (Gth.FileData? _file_data) {
		property_list.model.remove_all ();
		file_data = _file_data;
		if (file_data == null)
			return;
		foreach (unowned var info in MetadataInfo.get_all ()) {
			if (info.id == null) {
				continue;
			}
			if (!(MetadataFlags.ALLOW_IN_PROPERTIES_VIEW in info.flags)) {
				continue;
			}
			if (!filter.matches (info)) {
				continue;
			}
			var value = file_data.get_attribute_as_string (info.id);
			if (Strings.empty (value)) {
				continue;
			}
			if (info.id != "Metadata::Description") {
				value = Strings.substring (value, MAX_VALUE_CHARACTERS, "…");
			}
			unowned var category = Gth.MetadataCategory.get (info.category);
			if (category == null) {
				continue;
			}
			var property = new FileProperty () {
				id = info.id,
				name = _(info.display_name),
				value = value,
				category = Gth.MetadataCategory.get (info.category),
			};
			property_list.model.append (property);
		}
	}

	FileProperty current_property = null;

	public void open_context_menu (FilePropertyItem item, int x, int y) {
		var has_details = false;
		var action_info = actions.get (item.property.id);
		if (action_info != null) {
			if (action_info.type == Action.Type.DETAILS) {
				has_details = file_data.has_attribute (action_info.details_attribute);
			}
		}
		Util.enable_action (action_group, "copy-details", has_details);

		current_property = item.property;
		Graphene.Point p = Graphene.Point.zero ();
		item.compute_point (this, Graphene.Point.zero (), out p);
		context_menu.pointing_to = { (int) p.x + x, (int) p.y + y, 1, 24 };
		context_menu.popup ();
	}

	public bool with_search () {
		return filter != Filter.NONE;
	}

	public void set_search (string? text) {
		filter_text = (text != null) ? text.casefold () : null;
		property_filter.changed (Gtk.FilterChange.DIFFERENT);
	}

	[GtkChild] protected unowned Gtk.ListView list_view;
	[GtkChild] unowned Gtk.PopoverMenu context_menu;

	GenericList<FileProperty> property_list;
	Gtk.CustomFilter property_filter;
	string filter_text;
	HashTable<string, Action> actions;
	Gth.FileData file_data;
	SimpleActionGroup action_group;

	class Action {
		public enum Type {
			ACTION,
			DETAILS,
		}

		public Type type;
		public string detailed_action_name;
		public string icon_name;
		public string tooltip;
		public string details_attribute;

		public Action (string _detailed_action_name, string _icon_name, string _tooltip) {
			type = Type.ACTION;
			detailed_action_name = _detailed_action_name;
			icon_name = _icon_name;
			tooltip = _tooltip;
		}

		public Action.with_details (string _details_attribute) {
			type = Type.DETAILS;
			details_attribute = _details_attribute;
		}
	}

	const int MAX_VALUE_CHARACTERS = 300;
}

public class Gth.FileProperty : Object {
	public string id;
	public string name;
	public string value;
	public unowned Gth.MetadataCategory? category;
}

[GtkTemplate (ui = "/org/gnome/gthumb/ui/file-property-item.ui")]
public class Gth.FilePropertyItem : Gtk.Box {
	public unowned Gth.FileProperty property = null;
	[GtkChild] public unowned Gtk.Label title;
	[GtkChild] public unowned Gtk.Label subtitle;
	[GtkChild] public unowned Gtk.Label description;
	[GtkChild] public unowned Gtk.Button button;

	weak Gth.PropertyView view;

	public FilePropertyItem (Gth.PropertyView _view) {
		view = _view;
		var click_events = new Gtk.GestureClick ();
		click_events.set_button (Gdk.BUTTON_SECONDARY);
		click_events.pressed.connect ((n_press, x, y) => {
			view.open_context_menu (this, (int) x, (int) y);
		});
		add_controller (click_events);
	}
}
