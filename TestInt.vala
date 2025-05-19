public class Gth.TestInt : Gth.Test {
	public int number { get; set; default = 0; }
	public int max_value { get; set; default = 0; }
	public Test.Operation op { get; set; default = Test.Operation.EQUAL; }
	public bool negative { get; set; default = false; }

	public virtual int get_file_value (FileData file) {
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
			node.set_attribute ("value", "%d".printf (number));
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
		// Operation
		var names = new Gtk.StringList (null);
		var selected = 0;
		var idx = 0;
		foreach (unowned var info in Int_Operations) {
			names.append (_(info.name));
			if ((op == info.op) && (negative == info.negative))
				selected = idx;
			idx++;
		}
		operation = new Gtk.DropDown (names, null);
		operation.set_selected (selected);

		// SpinButton
		spin_button = new Gtk.SpinButton.with_range (0, max_value, 1);
		spin_button.value = number;

		// Horizontal Box
		var control = new Gtk.Box (Gtk.Orientation.HORIZONTAL, HORIZONTAL_SPACING);
		control.append (operation);
		control.append (spin_button);
		return control;
	}

	public override void update_from_options () throws Error {
		var selected = operation.get_selected ();
		if (selected >= Int_Operations.length)
			throw new IOError.FAILED ("The test definition is incomplete");
		unowned var op_info = Int_Operations[selected];
		op = op_info.op;
		negative = op_info.negative;
		number = spin_button.get_value_as_int ();
	}

	public override void focus_options () {
		spin_button.grab_focus ();
	}

	Gtk.DropDown operation;
	Gtk.SpinButton spin_button;
}
