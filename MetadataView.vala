[GtkTemplate (ui = "/app/gthumb/gthumb/ui/metadata-view.ui")]
public class Gth.MetadataView : Gtk.Box, Gth.PropertyView {
	construct {
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
		column_view.model = new Gtk.SingleSelection (sort_model);

		var header_factory = column_view.header_factory as Gtk.SignalListItemFactory;
		header_factory.setup.connect ((item) => {
			var list_header = item as Gtk.ListHeader;
			var label = new Gtk.Inscription ("");
			label.text_overflow = Gtk.InscriptionOverflow.ELLIPSIZE_MIDDLE;
			label.xalign = 0.0f;
			label.add_css_class ("smaller-text");
			list_header.child = label;
		});
		header_factory.bind.connect ((item) => {
			var list_header = item as Gtk.ListHeader;
			var label = list_header.child as Gtk.Inscription;
			var property = list_header.item as Property;
			label.text = (property.category != null) ? property.category.display_name : "";
		});

		var name_factory = name_column.factory as Gtk.SignalListItemFactory;
		name_factory.setup.connect ((item) => {
			var list_item = item as Gtk.ListItem;
			var label = new Gtk.Inscription ("");
			label.text_overflow = Gtk.InscriptionOverflow.ELLIPSIZE_MIDDLE;
			label.xalign = 0.0f;
			list_item.child = label;
		});
		name_factory.bind.connect ((item) => {
			var list_item = item as Gtk.ListItem;
			var label = list_item.child as Gtk.Inscription;
			var property = list_item.item as Property;
			label.text = property.name;
			label.tooltip_text = property.name;
			label.add_css_class ("smaller-text");
		});

		var value_factory = value_column.factory as Gtk.SignalListItemFactory;
		value_factory.setup.connect ((item) => {
			var list_item = item as Gtk.ListItem;
			var label = new Gtk.Inscription ("");
			label.text_overflow = Gtk.InscriptionOverflow.ELLIPSIZE_MIDDLE;
			label.xalign = 0.0f;
			label.add_css_class ("smaller-text");
			list_item.child = label;
		});
		value_factory.bind.connect ((item) => {
			var list_item = item as Gtk.ListItem;
			var label = list_item.child as Gtk.Inscription;
			var property = list_item.item as Property;
			label.text = property.value;
			label.tooltip_text = property.value;
		});
	}

	public unowned string get_name () {
		return "metadata-table";
	}

	public unowned string get_title () {
		return _("Metadata");
	}

	public unowned string get_icon () {
		return "table-symbolic";
	}

	public bool can_view (Gth.FileData file_data) {
		return true;/*
		var data_available = false;
		foreach (unowned var info in MetadataInfo.get_all ()) {
			if (info.id == null) {
				continue;
			}
			if (!(MetadataFlags.ALLOW_IN_PROPERTIES_VIEW in info.flags)) {
				continue;
			}
			if (info.id.has_prefix ("Exif")
				|| info.id.has_prefix ("Iptc")
				|| info.id.has_prefix ("Xmp"))
			{
				var value = file_data.get_attribute_as_string (info.id);
				if (!Strings.empty (value)) {
					data_available = true;
					break;
				}
			}
		}
		return data_available;*/
	}

	[GtkCallback]
	void on_search_changed (Gtk.SearchEntry entry) {
		filter_text = entry.text.casefold ();
		property_filter.changed (Gtk.FilterChange.DIFFERENT);
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
			/*if (!info.id.has_prefix ("Exif")
				&& !info.id.has_prefix ("Iptc")
				&& !info.id.has_prefix ("Xmp"))
			{
				continue;
			}*/
			var value = file_data.get_attribute_as_string (info.id);
			if (Strings.empty (value)) {
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

	[GtkChild] unowned Gtk.ColumnView column_view;
	[GtkChild] unowned Gtk.ColumnViewColumn name_column;
	[GtkChild] unowned Gtk.ColumnViewColumn value_column;

	GenericList<Property> property_list;
	Gtk.CustomFilter property_filter;
	string filter_text;

	class Property : Object {
		public string name;
		public string value;
		public unowned Gth.MetadataCategory? category;
	}
}
