public class Gth.TestSize : Gth.Test {
	public uint64 number { get; set; default = 0; }
	public Test.Operation op { get; set; default = Test.Operation.EQUAL; }
	public bool negative { get; set; default = false; }

	public virtual uint64 get_file_value (FileData file) {
		return 0;
	}

	public override bool match (FileData file) {
		var matches = false;
		var value = get_file_value (file);
		switch (op) {
		case Test.Operation.EQUAL:
			matches = (value == number);
			break;
		case Test.Operation.LOWER:
			matches = (value < number);
			break;
		case Test.Operation.GREATER:
			matches = (value > number);
			break;
		default:
			break;
		}
		return matches;
	}

	public override Dom.Element create_element (Dom.Document doc) {
		var node = new Dom.Element.with_attributes ("test", "id", id);
		if (!visible)
			node.set_attribute ("display", "none");
		node.set_attribute ("op", op.to_xml_attribute ());
		if (op != Test.Operation.NONE) {
			if (negative) {
				node.set_attribute ("negative", negative ? "true" : "false");
			}
			node.set_attribute ("value", "%lld".printf (number));
		}
		return node;
	}

	public override void load_from_element (Dom.Element node) {
		id = node.get_attribute ("id");
		visible = node.get_attribute ("display") != "none";
		op = Test.Operation.from_xml_attribute (node.get_attribute ("op"));
		negative = node.get_attribute ("negative") == "true";
		int value;
		if (node.get_attribute_as_int ("value", out value)) {
			number = value;
		}
	}

	public override Gtk.Widget? create_options () {
		// Operation selector
		var names = new Gtk.StringList (null);
		var selected_operation_selector = 0;
		var idx = 0;
		foreach (unowned var info in Int_Operations) {
			names.append (_(info.name));
			if ((op == info.op) && (negative == info.negative))
				selected_operation_selector = idx;
			idx++;
		}
		operation_selector = new Gtk.DropDown (names, null);
		operation_selector.set_selected (selected_operation_selector);

		// Text entry
		value_entry = new Gtk.Entry ();
		value_entry.width_chars = 6;

		// Size selector
		names = new Gtk.StringList (null);
		var selected_unit = 0;
		idx = 0;
		var size_set = false;
		foreach (unowned var info in Unit_List) {
			names.append (_(info.display_name));
			if (!size_set
				&& ((idx == Unit_List.length - 1)
					|| (number < Unit_List[idx + 1].size)))
			{
				selected_unit = idx;
				var entry_value = "%.2f".printf (number / Unit_List[idx].size);
				value_entry.set_text (entry_value);
				size_set = true;
			}
			idx++;
		}
		unit_selector = new Gtk.DropDown (names, null);
		unit_selector.set_selected (selected_unit);

		// Horizontal Box
		var control = new Gtk.Box (Gtk.Orientation.HORIZONTAL, HORIZONTAL_SPACING);
		control.append (operation_selector);
		control.append (value_entry);
		control.append (unit_selector);
		return control;
	}

	public override void update_from_options () throws Error {
		var selected = operation_selector.get_selected ();
		if (selected >= Int_Operations.length)
			throw new IOError.FAILED ("The test definition is incomplete");
		unowned var op_info = Int_Operations[selected];
		op = op_info.op;
		negative = op_info.negative;
		double entry_value;
		if (double.try_parse (value_entry.get_text (), out entry_value, null)) {
			var selected_unit = Unit_List[unit_selector.get_selected ()].size;
			number = (uint64) (entry_value * selected_unit);
		}
	}

	public override void focus_options () {
		value_entry.grab_focus ();
	}

	Gtk.DropDown operation_selector;
	Gtk.Entry value_entry;
	Gtk.DropDown unit_selector;
}
