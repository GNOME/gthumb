public class Gth.TestString : Gth.Test {
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

	public virtual string get_file_value (FileData file) {
		return null;
	}

	public override bool match (FileData file) {
		var matches = false;
		var value = get_file_value (file);
		if (value == null) {
			return false;
		}
		if (lowercase_text == null) {
			lowercase_text = text.casefold ();
		}
		unowned var a = lowercase_text;
		var b = value.casefold ();
		switch (op) {
		case Test.Operation.EQUAL:
			matches = a.collate (b) == 0;
			break;

		case Test.Operation.LOWER:
			matches = a.collate (b) < 0;
			break;

		case Test.Operation.GREATER:
			matches = a.collate (b) > 0;
			break;

		case Test.Operation.CONTAINS:
			matches = b.contains (a);
			break;

		case Test.Operation.STARTS_WITH:
			matches = b.has_prefix (a);
			break;

		case Test.Operation.ENDS_WITH:
			matches = b.has_suffix (a);
			break;

		case Test.Operation.MATCHES:
			if (pattern == null) {
				pattern = new PatternSpec (text);
			}
			matches = pattern.match_string (value);
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
			if (text != null) {
				node.set_attribute ("value", text);
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
		{ N_("Contains"), Test.Operation.CONTAINS, false },
		{ N_("Starts With"), Test.Operation.STARTS_WITH, false },
		{ N_("Ends With"), Test.Operation.ENDS_WITH, false },
		{ N_("Equal To"), Test.Operation.EQUAL, false },
		{ N_("Different From"), Test.Operation.EQUAL, true },
		{ N_("Does not Contain"), Test.Operation.CONTAINS, true },
		{ N_("Matches"), Test.Operation.MATCHES, false }
	};

	Gtk.DropDown operation;
	Gtk.Entry entry;
	PatternSpec pattern = null;
	string lowercase_text = null;
}

public delegate string Gth.FileDataGetStringFunc (Gth.FileData file);
