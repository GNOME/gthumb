[GtkTemplate (ui = "/app/gthumb/gthumb/ui/file-property-view.ui")]
public class Gth.FilePropertyView : Gtk.Box, Gth.PropertyView {
	public bool embedded_properties;

	construct {
		embedded_properties = false;
		filter_text = null;

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
		list_view.model = new Gtk.NoSelection (sort_model);

		var header_factory = list_view.header_factory as Gtk.SignalListItemFactory;
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

		var factory = list_view.factory as Gtk.SignalListItemFactory;
		factory.setup.connect ((item) => {
			var list_item = item as Gtk.ListItem;
			list_item.child = new PropertyListItem ();
		});
		factory.bind.connect ((item) => {
			var list_item = item as Gtk.ListItem;
			var property = list_item.item as Property;
			var property_item = list_item.child as PropertyListItem;
			property_item.title.label = property.name;
			property_item.subtitle.label = property.value;
		});
	}

	public virtual unowned string get_name () {
		return "file-properties";
	}

	public virtual unowned string get_title () {
		return _("File Properties");
	}

	public virtual unowned string get_icon () {
		return "document-properties-symbolic";
	}

	public virtual bool can_view (Gth.FileData file_data) {
		return (file_data != null);
	}

	public void set_file (Gth.FileData? file_data) {
		property_list.model.remove_all ();
		if (file_data == null)
			return;
		foreach (unowned var info in MetadataInfo.get_all ()) {
			if (info.id == null) {
				continue;
			}
			if (!(MetadataFlags.ALLOW_IN_PROPERTIES_VIEW in info.flags)) {
				continue;
			}
			var is_embedded = info.id.has_prefix ("Exif")
				|| info.id.has_prefix ("Iptc")
				|| info.id.has_prefix ("Xmp");
			if (is_embedded != embedded_properties) {
				continue;
			}
			var value = file_data.get_attribute_as_string (info.id);
			if (Strings.empty (value)) {
				continue;
			}
			unowned var category = Gth.MetadataCategory.get (info.category);
			if (category == null) {
				continue;
			}
			var property = new Property () {
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

	[GtkChild] unowned Gtk.ListView list_view;

	GenericList<Property> property_list;
	Gtk.CustomFilter property_filter;
	string filter_text;

	class Property : Object {
		public string name;
		public string value;
		public unowned Gth.MetadataCategory? category;
	}
}

class PropertyListItem : Gtk.Box {
	public Gtk.Label title;
	public Gtk.Label subtitle;

	public PropertyListItem () {
		orientation = Gtk.Orientation.VERTICAL;
		spacing = 6;
		add_css_class ("property-row");

		title = new Gtk.Label ("");
		title.add_css_class ("dim-label");
		//title.add_css_class ("smaller-text");
		title.halign = Gtk.Align.START;
		append (title);

		subtitle = new Gtk.Label ("");
		subtitle.halign = Gtk.Align.FILL;
		subtitle.xalign = 0.0f;
		subtitle.wrap = true;
		subtitle.wrap_mode = Pango.WrapMode.CHAR;
		append (subtitle);
	}
}
