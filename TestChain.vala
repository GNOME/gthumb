public class Gth.TestChain : Gth.Test {
	public enum Operation {
		NONE,
		INTERSECTION,
		UNION;

		public string to_xml_attribute () {
			return XML_ATTRIBUTE[this];
		}

		public static Operation from_xml_attribute (string attr) {
			return (Operation) Util.enum_index (XML_ATTRIBUTE, attr);
		}

		const string[] XML_ATTRIBUTE = { "none", "all", "any" };
	}

	public Operation operation { get; set; default = Operation.NONE; }
	public GenericArray<Gth.Test> tests;

	public TestChain (Operation _type = Operation.NONE) {
		operation = _type;
		tests = new GenericArray<Gth.Test>();
	}

	public void add_test (Gth.Test test) {
		test.visible = true;
		tests.add (test);

		// Add the test attributes to this test.
		var new_attributes = new StringBuilder ();
		if (!Strings.empty (attributes)) {
			new_attributes.append (attributes);
		}
		if (!Strings.empty (test.attributes)) {
			if (new_attributes.len > 0)
				new_attributes.append (",");
			new_attributes.append (test.attributes);
		}
		attributes = new_attributes.str;
	}

	public bool has_type_test () {
		foreach (unowned var test in tests) {
			if (test is TestFileType)
				return true;
		}
		return false;
	}

	public override bool match (FileData file) {
		if (operation == Operation.NONE)
			return false;
		var result = (operation == Operation.INTERSECTION) ? true : false;
		foreach (unowned var test in tests) {
			if (test.match (file)) {
				if (operation == Operation.UNION) {
					result = true;
					break;
				}
			}
			else if (operation == Operation.INTERSECTION) {
				result = false;
				break;
			}
		}
		return result;
	}

	public override Gtk.Widget? create_options () {
		// TODO
		return null;
	}

	public override void update_from_options () throws Error {
		// TODO
	}

	public override void focus_options () {
		// TODO
	}

	public override Dom.Element create_element (Dom.Document doc) {
		var node = new Dom.Element ("tests");
		node.set_attribute ("match", operation.to_xml_attribute ());
		foreach (unowned var test in tests) {
			node.append_child (test.create_element (doc));
		}
		return node;
	}

	public override void load_from_element (Dom.Element node) {
		var match = node.get_attribute ("match");
		if (match != null) {
			operation = Operation.from_xml_attribute (match);
		}
		tests = new GenericArray<Gth.Test>();
		foreach (unowned var child in node) {
			switch (child.tag_name) {
			case "test":
				unowned var id = child.get_attribute ("id");
				if (id != null) {
					var test_info = app.get_test_by_id (id);
					if (test_info != null) {
						var test = Object.new (test_info.test_type) as Gth.Test;
						if (test != null) {
							test.load_from_element (child);
							tests.add (test);
						}
					}
				}
				break;

			case "tests":
				var subchain = new TestChain ();
				subchain.load_from_element (child);
				tests.add (subchain);
				break;
			}
		}
	}
}
