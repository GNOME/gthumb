public class Gth.TestStringList : Gth.Test {
	string _text;
	public string text {
		get { return _text; }
		set {
			_text = value;
			lowercase_text = null;
			pattern = null;
		}
	}
	public Test.Operation op { get; set; default = Test.Operation.CONTAINS; }
	public bool negative { get; set; default = false; }

	public virtual Gth.StringList? get_file_value (FileData file) {
		return null;
	}

	public override bool match (FileData file) {
		var matches = false;
		var string_list = get_file_value (file);
		if (text == null) {
			return false;
		}
		if (string_list == null) {
			return negative;
		}
		if (lowercase_text == null) {
			lowercase_text = text.casefold ();
		}
		switch (op) {
		case Test.Operation.CONTAINS, Test.Operation.CONTAINS_ONLY:
			unowned var list = string_list.get_list ();
			var one_element = (list.next == null);
			foreach (unowned var str in list) {
				if ((str != null) && (lowercase_text.collate (str.casefold ()) == 0)) {
					matches = true;
					break;
				}
			}
			matches = matches && ((op == Test.Operation.CONTAINS) || one_element);
			break;

		case Test.Operation.MATCHES:
			if (pattern == null) {
				pattern = new PatternSpec (text);
			}
			unowned var list = string_list.get_list ();
			foreach (unowned var str in list) {
				if ((str != null) && pattern.match_string (str)) {
					matches = true;
					break;
				}
			}
			break;

		default:
			break;
		}
		return negative ? !matches : matches;
	}

	public override Dom.Element create_element (Dom.Document doc) {
		var node = base.create_element (doc);
		node.set_attribute ("op", op.to_xml_attribute ());
		if (op != Test.Operation.NONE) {
			if (negative) {
				node.set_attribute ("negative", negative ? "true" : "false");
			}
			node.set_attribute ("value", text);
		}
		return node;
	}

	public override void load_from_element (Dom.Element node) {
		base.load_from_element (node);
		var op_attribute = node.get_attribute ("op");
		if (op_attribute != null) {
			op = Test.Operation.from_xml_attribute (op_attribute);
			// Convert EQUAL to CONTAINS for backward compatibility.
			if (op == Test.Operation.EQUAL) {
				op = Test.Operation.CONTAINS;
			}
		}
		negative = node.get_attribute ("negative") == "true";
		text = node.get_attribute ("value");
	}

	public override Gtk.Widget? create_options () {
		// Operation
		var names = new Gtk.StringList (null);
		var selected = 0;
		var idx = 0;
		foreach (unowned var info in Operations) {
			names.append (_(info.name));
			if ((op == info.op) && (negative == info.negative))
				selected = idx;
			idx++;
		}
		operation = new Gtk.DropDown (names, null);
		operation.set_selected (selected);

		// Entry
		entry = new Gtk.Entry ();
		entry.set_size_request (150, -1);
		if (text != null) {
			entry.set_text (text);
		}

		// Horizontal Box
		var control = new Gtk.Box (Gtk.Orientation.HORIZONTAL, HORIZONTAL_SPACING);
		control.append (operation);
		control.append (entry);

		operation.notify["selected"].connect (() => options_changed ());
		entry.changed.connect (() => options_changed ());

		return control;
	}

	public override void update_from_options () throws Error {
		var selected = operation.get_selected ();
		if (selected >= Operations.length)
			throw new IOError.FAILED ("The test definition is incomplete");
		unowned var op_info = Operations[selected];
		op = op_info.op;
		negative = op_info.negative;
		text = entry.get_text ();
		if (Strings.empty (text))
			throw new IOError.FAILED ("The test definition is incomplete");
	}

	public override void focus_options () {
		entry.grab_focus ();
	}

	const Test.OperationInfo[] Operations = {
		{ N_("Includes"), Test.Operation.CONTAINS, false },
		{ N_("Includes Only"), Test.Operation.CONTAINS_ONLY, false },
		{ N_("Not Includes"), Test.Operation.CONTAINS, true },
		{ N_("Matches Pattern"), Test.Operation.MATCHES, false }
	};

	Gtk.DropDown operation;
	Gtk.Entry entry;
	PatternSpec pattern = null;
	string lowercase_text = null;
}
