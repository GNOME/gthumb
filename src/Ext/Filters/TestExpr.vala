public class Gth.TestExpr : Gth.Test {
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
	public GenericList<Gth.Test> tests;

	public TestExpr (Operation _type = Operation.NONE) {
		operation = _type;
		tests = new GenericList<Gth.Test>();
	}

	public void add (Gth.Test test) {
		test.visible = true;
		tests.model.append (test);
		update_attributes ();
	}

	public void remove (Gth.Test test) {
		uint pos;
		if (tests.model.find (test, out pos)) {
			tests.model.remove (pos);
			update_attributes ();
		}
	}

	public bool is_empty () {
		return tests.model.n_items == 0;
	}

	void update_attributes () {
		var new_attributes = new StringBuilder ();
		foreach (unowned var test in tests) {
			if (!Strings.empty (test.attributes)) {
				if (new_attributes.len > 0)
					new_attributes.append (",");
				new_attributes.append (test.attributes);
			}
		}
		attributes = new_attributes.str;
	}

	public bool contains_type_test () {
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

	public override void focus_options () {
		var test = tests.first ();
		if (test != null) {
			test.focus_options ();
		}
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
		tests.model.remove_all ();
		foreach (unowned var child in node) {
			switch (child.tag_name) {
			case "test":
				unowned var id = child.get_attribute ("id");
				if (id != null) {
					var registered_test = app.get_test_by_id (id);
					if (registered_test != null) {
						var test = registered_test.duplicate ();
						test.load_from_element (child);
						tests.model.append (test);
					}
				}
				break;

			case "tests":
				var subchain = new TestExpr ();
				subchain.load_from_element (child);
				tests.model.append (subchain);
				break;
			}
		}
		update_attributes ();
	}
}
