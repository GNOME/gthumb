public class Gth.Filter : Gth.Test {
	public enum LimitType {
		NONE,
		FILES,
		BYTES;

		public string to_xml_attribute () {
			return XML_ATTRIBUTE[this];
		}

		public static LimitType from_xml_attribute (string? attr) {
			return (LimitType) Util.enum_index (XML_ATTRIBUTE, attr);
		}

		const string[] XML_ATTRIBUTE = { "none", "files", "size" };
	}

	public LimitType limit_type;
	public int64 limit;
	public Gth.Sort sort;
	public Gth.TestExpr tests {
		set {
			_tests = value;
			attributes = _tests.attributes;
		}
		get {
			return _tests;
		}
	}

	construct {
		id = Strings.new_random (ID_LENGTH);
		_tests = new TestExpr ();
		limit_type = LimitType.NONE;
		limit = 0;
		sort = { null, false };
	}

	public override TestIterator iterator (GenericList<FileData> files) {
		return new FilterIterator (this, files);
	}

	public override bool match (FileData file) {
		return (tests != null) ? tests.match (file) : true;
	}

	public override Dom.Element create_element (Dom.Document doc) {
		var node = new Dom.Element ("filter");
		node.set_attribute ("id", id);
		node.set_attribute ("name", display_name);
		if (!visible) {
			node.set_attribute ("display", "none");
		}
		if (_tests.operation != TestExpr.Operation.NONE) {
			node.append_child (_tests.create_element (doc));
		}
		if (limit_type != LimitType.NONE) {
			var limit_node = new Dom.Element ("limit");
			limit_node.set_attribute ("type", limit_type.to_xml_attribute ());
			limit_node.set_attribute ("value", limit.to_string ());
			limit_node.set_attribute ("selected_by", app.migration.sorter.get_old_key (sort.name));
			limit_node.set_attribute ("direction", sort.inverse ? "descending" : "ascending");
			node.append_child (limit_node);
		}
		return node;
	}

	public override void load_from_element (Dom.Element node) {
		base.load_from_element (node);
		display_name = node.get_attribute ("name");

		foreach (unowned var child in node) {
			switch (child.tag_name) {
			case "tests":
				_tests.load_from_element (child);
				break;

			case "limit":
				limit_type = LimitType.from_xml_attribute (child.get_attribute ("type"));
				child.get_attribute_as_int64 ("value", out limit);
				sort = {
					app.migration.sorter.get_new_key (child.get_attribute ("selected_by")),
					child.get_attribute ("direction") == "descending"
				};
				break;
			}
		}
	}

	public override Gtk.Widget? create_options () {
		switch (limit_type) {
		case LimitType.FILES:
			return create_options_for_files ();

		case LimitType.BYTES:
			return create_options_for_size ();

		default:
			return null;
		}
	}

	public Gtk.Widget create_options_for_files () {
		spin_button = new Gtk.SpinButton.with_range (0, int.MAX, 1.0);
		spin_button.width_chars = 4;
		spin_button.value = limit;

		var label = new Gtk.Label.with_mnemonic (_("files"));

		var control = new Gtk.Box (Gtk.Orientation.HORIZONTAL, HORIZONTAL_SPACING);
		control.valign = Gtk.Align.CENTER;
		control.append (spin_button);
		control.append (label);

		spin_button.value_changed.connect (() => options_changed ());

		return control;
	}

	public Gtk.Widget create_options_for_size () {
		spin_button = new Gtk.SpinButton.with_range (0, int.MAX, 1.0);
		spin_button.width_chars = 4;
		spin_button.value = limit;

		// Size selector
		var names = new Gtk.StringList (null);
		var selected_unit = 0;
		var idx = 0;
		var size_set = false;
		foreach (unowned var info in Test.Unit_List) {
			names.append (_(info.display_name));
			if (!size_set
				&& ((idx == Test.Unit_List.length - 1)
					|| (limit < Test.Unit_List[idx + 1].size)))
			{
				selected_unit = idx;
				spin_button.set_value (limit / Unit_List[idx].size);
				size_set = true;
			}
			idx++;
		}
		unit_selector = new Gtk.DropDown (names, null);
		unit_selector.set_selected (selected_unit);

		var control = new Gtk.Box (Gtk.Orientation.HORIZONTAL, HORIZONTAL_SPACING);
		control.valign = Gtk.Align.CENTER;
		control.append (spin_button);
		control.append (unit_selector);

		spin_button.value_changed.connect (() => options_changed ());
		unit_selector.notify["selected"].connect (() => options_changed ());

		return control;
	}

	public override void update_from_options () throws Error {
		switch (limit_type) {
		case LimitType.FILES:
			limit = (int64) spin_button.value;
			break;

		case LimitType.BYTES:
			var selected_unit = Unit_List[unit_selector.get_selected ()].size;
			limit = (int64) (spin_button.value * selected_unit);
			break;

		default:
			break;
		}
	}

	public override void focus_options () {
		spin_button.grab_focus ();
	}

	public override void copy (Gth.Test other) {
		var original_id = id;
		base.copy (other);
		id = original_id;
	}

	Gth.TestExpr _tests;
	Gtk.SpinButton spin_button = null;
	Gtk.DropDown unit_selector = null;

	const int ID_LENGTH = 8;
}

public class Gth.FilterIterator : Gth.TestIterator {
	public FilterIterator (Filter _test, GenericList<FileData> _files) {
		base (_test, _files);
		filter = _test as Filter;
		tot_files = 0;
		tot_bytes = 0;
		if (filter.limit_type != Filter.LimitType.NONE) {
			unowned var sort_info = app.get_sorter_by_id (filter.sort.name);
			if ((sort_info != null) && (sort_info.cmp_func != null)) {
				files.model.sort ((a, b) => {
					var result = sort_info.cmp_func ((FileData) a, (FileData) b);
					if (filter.sort.inverse)
						result *= -1;
					return result;
				});
			}
		}
	}

	public override bool next () {
		if (base.next () && (filter.limit_type != Filter.LimitType.NONE)) {
			tot_files += 1;
			tot_bytes += file.info.get_size ();
			if (filter.limit_type == Filter.LimitType.FILES) {
				if (tot_files > filter.limit) {
					file = null;
				}
			}
			else if (filter.limit_type == Filter.LimitType.BYTES) {
				if (tot_bytes > filter.limit) {
					file = null;
				}
			}
		}
		return file != null;
	}

	Filter filter;
	int tot_files;
	int64 tot_bytes;
}
