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
	public string sort_name;
	public Gtk.SortType sort_type;

	Gth.TestChain _test;
	public Gth.TestChain test {
		get { return _test; }
		set {
			_test = value;
			attributes = _test.attributes;
		}
	}

	public Filter () {
		id = Strings.new_random (ID_LENGTH);
		_test = null;
		limit_type = LimitType.NONE;
		limit = 0;
		sort_name = null;
		sort_type = Gtk.SortType.ASCENDING;
	}

	public override void set_files (GenericArray<FileData> _files) {
		base.set_files (_files);
		if ((limit_type != LimitType.NONE) && !Strings.empty (sort_name)) {
			var sorter = app.get_sorter_by_id (sort_name);
			if ((sorter != null) && (sorter.cmp_func != null)) {
				files.sort (sorter.cmp_func);
				if (sort_type == Gtk.SortType.DESCENDING) {
					for (var i = 0; i < files.length / 2; i++) {
						var j = files.length - 1 - i;
						var tmp = files[i];
						files[i] = files[j];
						files[j] = tmp;
					}
				}
			}
		}
		tot_files = 0;
		tot_bytes = 0;
	}

	public virtual bool match (FileData file) {
		return false;
	}

	public override Dom.Element create_element (Dom.Document doc) {
		var node = new Dom.Element ("filter");
		node.set_attribute ("id", id);
		node.set_attribute ("name", display_name);
		if (!visible) {
			node.set_attribute ("display", "none");
		}
		if ((_test != null) && (_test.operation != Gth.TestChain.Operation.NONE)) {
			node.append_child (_test.create_element (doc));
		}
		if (limit_type != LimitType.NONE) {
			var limit_node = new Dom.Element ("limit");
			limit_node.set_attribute ("type", limit_type.to_xml_attribute ());
			limit_node.set_attribute ("value", limit.to_string ());
			limit_node.set_attribute ("selected_by", sort_name);
			limit_node.set_attribute ("direction", (sort_type == Gtk.SortType.ASCENDING) ? "ascending": "descending");
			node.append_child (limit_node);
		}
		return node;
	}

	public override void load_from_element (Dom.Element node) {
		id = node.get_attribute ("id");
		display_name = node.get_attribute ("name");
		visible = node.get_attribute ("display") != "none";

		foreach (unowned var child in node) {
			switch (child.tag_name) {
			case "tests":
				_test = new TestChain ();
				_test.load_from_element (child);
				break;

			case "limit":
				limit_type = LimitType.from_xml_attribute (child.get_attribute ("type"));
				child.get_attribute_as_int64 ("value", out limit);
				sort_name = child.get_attribute ("selected_by");
				sort_type = (child.get_attribute ("direction") == "ascending") ? Gtk.SortType.ASCENDING : Gtk.SortType.DESCENDING;
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
		spin_button.width_chars = 5;
		spin_button.value = limit;

		var label = new Gtk.Label.with_mnemonic (_("_Limit to"));
		label.mnemonic_widget = spin_button;

		var control = new Gtk.Box (Gtk.Orientation.HORIZONTAL, HORIZONTAL_SPACING);
		control.append (label);
		control.append (spin_button);
		control.append (new Gtk.Label (_("files")));
		return control;
	}

	public Gtk.Widget create_options_for_size () {
		spin_button = new Gtk.SpinButton.with_range (0, int.MAX, 1.0);
		spin_button.width_chars = 5;
		spin_button.value = limit;

		var label = new Gtk.Label.with_mnemonic (_("_Limit to"));
		label.mnemonic_widget = spin_button;

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
		control.append (label);
		control.append (spin_button);
		control.append (unit_selector);
		return control;
	}

	public override void update_from_options () throws Error {
		// TODO
	}

	public override void focus_options () {
		spin_button.grab_focus ();
	}

	int tot_files;
	int64 tot_bytes;
	Gtk.SpinButton spin_button;
	Gtk.DropDown unit_selector;

	const int ID_LENGTH = 8;
}
