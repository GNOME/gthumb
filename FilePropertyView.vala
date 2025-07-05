[GtkTemplate (ui = "/app/gthumb/gthumb/ui/file-property-view.ui")]
public class Gth.FilePropertyView : Gtk.Box, Gth.PropertyView {
	public enum Filter {
		NONE,
		EXIF,
		IPTC,
		XMP;

		public bool view_embedded () {
			return this != NONE;
		}

		public bool matches (string property) {
			if (this == NONE) {
				return !property.has_prefix ("Exif")
					&& !property.has_prefix ("Iptc")
					&& !property.has_prefix ("Xmp");
			}
			else {
				return property.has_prefix (PREFIX[this]);
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

		var action_group = new SimpleActionGroup ();
		insert_action_group ("file", action_group);

		var action = new SimpleAction ("copy-path", null);
		action.activate.connect ((_action, param) => {
			if (file_data != null) {
				var win = app.active_window as Gth.Window;
				if (win != null) {
					win.copy_text (file_data.file.get_path ());
				}
			}
		});
		action_group.add_action (action);

		action = new SimpleAction ("open-folder", null);
		action.activate.connect ((_action, param) => {
			if (file_data != null) {
				var win = app.active_window as Gth.Window;
				if (win != null) {
					app.open_window (file_data.file.get_parent (), file_data.file, true);
				}
			}
		});
		action_group.add_action (action);

		action = new SimpleAction ("open-map", null);
		action.activate.connect ((_action, param) => {
			if (file_data != null) {
				var win = app.active_window as Gth.Window;
				if (win != null) {
					var metadata = file_data.info.get_attribute_object ("Embedded::Photo::Coordinates") as Gth.Metadata;
					double lat, long;
					if ((metadata != null) && metadata.get_point (out lat, out long)) {
						try {
							var uri = "https://www.openstreetmap.org/?mlat=%f&mlon=%f&zoom=6".printf (lat, long);
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
		actions.set ("standard::display-name", new Action ("file.copy-path", "edit-copy-symbolic", _("Copy Path")));
		actions.set ("gth::file::location", new Action ("file.open-folder", "folder-symbolic", _("Open")));
		actions.set ("Embedded::Photo::Coordinates", new Action ("file.open-map", "map-marker-symbolic", _("View on OpenStreetMap")));
		actions.set ("gth::file::display-size", new Action.with_description ("gth::file::size"));
		actions.set ("gth::file::content-type", new Action.with_description ("standard::fast-content-type"));

		property_list = new GenericList<Property> ();
		property_filter = new Gtk.CustomFilter ((obj) => {
			if (Strings.empty (filter_text))
				return true;
			unowned var prop = obj as Property;
			return prop.name.casefold ().contains (filter_text) || prop.value.casefold ().contains (filter_text);
		});
		var filter_model = new Gtk.FilterListModel (property_list.model, property_filter);
		var sort_model = new Gtk.SortListModel (filter_model, null);
		sort_model.section_sorter = new Gtk.CustomSorter ((a, b) => {
			unowned var prop_a = a as Property;
			unowned var prop_b = b as Property;
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
			list_item.child = new FilePropertyItem ();
		});
		factory.bind.connect ((item) => {
			var list_item = item as Gtk.ListItem;
			var property = list_item.item as Property;
			var property_item = list_item.child as FilePropertyItem;
			property_item.title.label = property.name;
			property_item.subtitle.label = property.value;
			property_item.property = property;
			property_item.description.visible = false;
			var action_info = actions.get (property.id);
			if (action_info != null) {
				if (action_info.type == Action.Type.DESCRIPTION) {
					property_item.description.label = file_data.get_attribute_as_string (action_info.tooltip_attribute);
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
			}
			if (property_item.parent != null) {
				property_item.parent.add_css_class ("property-widget");
				if (property.id == "general::description") {
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
			var property = list_header.item as Property;
			label.label = (property.category != null) ? property.category.display_name : "";
		});
		list_view.header_factory = header_factory;
	}

	public virtual unowned string get_name () {
		return "file-properties";
	}

	public virtual unowned string get_title () {
		return _("General");
	}

	public virtual unowned string get_icon () {
		return "document-properties-symbolic";
	}

	public virtual bool can_view (Gth.FileData file_data) {
		if (filter == Filter.NONE)
			return true;
		var data_available = false;
		foreach (unowned var info in MetadataInfo.get_all ()) {
			if (info.id == null) {
				continue;
			}
			if (!(MetadataFlags.ALLOW_IN_PROPERTIES_VIEW in info.flags)) {
				continue;
			}
			if (filter.matches (info.id)) {
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
			if (!filter.matches (info.id)) {
				continue;
			}
			var value = file_data.get_attribute_as_string (info.id);
			if (Strings.empty (value)) {
				continue;
			}
			if (info.id != "general::description") {
				value = Strings.substring (value, MAX_VALUE_CHARACTERS, "…");
			}
			unowned var category = Gth.MetadataCategory.get (info.category);
			if (category == null) {
				continue;
			}
			var property = new Property () {
				id = info.id,
				name = _(info.display_name),
				value = value,
				category = Gth.MetadataCategory.get (info.category),
			};
			property_list.model.append (property);
		}
	}

	[GtkCallback]
	void on_search_changed (Gtk.SearchEntry entry) {
		filter_text = entry.text.casefold ();
		property_filter.changed (Gtk.FilterChange.DIFFERENT);
	}

	[GtkChild] protected unowned Gtk.ListView list_view;
	[GtkChild] protected unowned Gtk.Box search_section;

	GenericList<Property> property_list;
	Gtk.CustomFilter property_filter;
	string filter_text;
	HashTable<string, Action> actions;
	Gth.FileData file_data;

	public class Property : Object {
		public string id;
		public string name;
		public string value;
		public unowned Gth.MetadataCategory? category;
	}

	class Action {
		public enum Type {
			ACTION,
			DESCRIPTION,
		}

		public Type type;
		public string detailed_action_name;
		public string icon_name;
		public string tooltip;
		public string tooltip_attribute;

		public Action (string _detailed_action_name, string _icon_name, string _tooltip) {
			type = Type.ACTION;
			detailed_action_name = _detailed_action_name;
			icon_name = _icon_name;
			tooltip = _tooltip;
		}

		public Action.with_description (string _tooltip_attribute) {
			type = Type.DESCRIPTION;
			tooltip_attribute = _tooltip_attribute;
		}
	}

	const int MAX_VALUE_CHARACTERS = 300;
}

[GtkTemplate (ui = "/app/gthumb/gthumb/ui/file-property-item.ui")]
class Gth.FilePropertyItem : Gtk.Box {
	public unowned Gth.FilePropertyView.Property property = null;

	public FilePropertyItem () {
		var action_group = new SimpleActionGroup ();
		insert_action_group ("property", action_group);

		var action = new SimpleAction ("copy-value", null);
		action.activate.connect ((_action, param) => {
			var win = app.active_window as Gth.Window;
			if (win != null) {
				win.copy_text (property.value);
			}
		});
		action_group.add_action (action);

		var click_events = new Gtk.GestureClick ();
		click_events.set_button (Gdk.BUTTON_SECONDARY);
		click_events.pressed.connect ((n_press, x, y) => {
			if (n_press == 1) {
				popover_menu.pointing_to = { (int) x, (int) y, 1, 1};
				popover_menu.popup ();
			}
		});
		add_controller (click_events);
	}

	[GtkChild] unowned Gtk.PopoverMenu popover_menu;
	[GtkChild] public unowned Gtk.Label title;
	[GtkChild] public unowned Gtk.Label subtitle;
	[GtkChild] public unowned Gtk.Label description;
	[GtkChild] public unowned Gtk.Button button;
}
