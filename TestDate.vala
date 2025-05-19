public class Gth.TestDate : Gth.Test {
	public Gth.DateTime? date { get; set; default = null; }
	public Test.Operation op { get; set; default = Test.Operation.EQUAL; }
	public bool negative { get; set; default = false; }

	public virtual Gth.DateTime? get_file_value (FileData file) {
		return null;
	}

	public override bool match (FileData file) {
		var matches = false;
		var value = get_file_value (file);
		if ((date == null) || (value == null))
			return false;
		var result = date.compare (value);
		switch (op) {
		case Test.Operation.EQUAL:
			matches = (result == 0);
			break;
		case Test.Operation.BEFORE:
			matches = (result < 0);
			break;
		case Test.Operation.AFTER:
			matches = (result > 0);
			break;
		default:
			break;
		}
		return matches;
	}

	public override Dom.Element create_element (Dom.Document doc) {
		var node = new Dom.Element.with_attributes ("test", "id", id);
		if (!visible) {
			node.set_attribute ("display", "none");
		}
		if (op != Test.Operation.NONE) {
			node.set_attribute ("op", op.to_xml_attribute ());
			if (negative) {
				node.set_attribute ("negative", negative ? "true" : "false");
			}
			if (date != null) {
				node.set_attribute ("value", date.to_exif_date ());
			}
		}
		return node;
	}

	public override void load_from_element (Dom.Element node) {
		id = node.get_attribute ("id");
		visible = node.get_attribute ("display") != "none";
		if (node.has_attribute ("op")) {
			op = Test.Operation.from_xml_attribute (node.get_attribute ("op"));
		}
		negative = node.get_attribute ("negative") == "true";
		if (date == null) {
			date = new Gth.DateTime ();
		}
		date.set_from_exif_date (node.get_attribute ("value"));
	}

	public override Gtk.Widget? create_options () {
		// Operation
		var names = new Gtk.StringList (null);
		var selected = 0;
		var idx = 0;
		foreach (unowned var info in DateOperations) {
			names.append (_(info.name));
			if ((op == info.op) && (negative == info.negative))
				selected = idx;
			idx++;
		}
		operation = new Gtk.DropDown (names, null);
		operation.set_selected (selected);

		// Time selector
		time_selector = new Gth.TimeSelector (Gth.TimeSelector.Mode.WITHOUT_TIME);
		if (date != null)
			time_selector.set_time (date);

		// Horizontal Box
		var control = new Gtk.Box (Gtk.Orientation.HORIZONTAL, HORIZONTAL_SPACING);
		control.append (operation);
		control.append (time_selector);
		return control;
	}

	public override void update_from_options () throws Error {
		var selected_operation = operation.get_selected ();
		if (selected_operation >= DateOperations.length)
			throw new IOError.FAILED ("The test definition is incomplete");
		unowned var op_info = DateOperations[selected_operation];
		op = op_info.op;
		negative = op_info.negative;
		date = time_selector.get_time ();
		if (date == null)
			throw new IOError.FAILED ("The test definition is incomplete");
	}

	public override void focus_options () {
		time_selector.focus ();
	}

	static const Test.OperationInfo[] DateOperations = {
		{ N_("is before"), Test.Operation.BEFORE, false },
		{ N_("is after"), Test.Operation.AFTER, false },
		{ N_("is"), Test.Operation.EQUAL, false },
		{ N_("is not"), Test.Operation.EQUAL, true }
	};

	Gtk.DropDown operation;
	Gth.TimeSelector time_selector;
}
