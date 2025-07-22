public class Gth.TestAspectRatio : Gth.Test {
	public float number { get; set; default = 1.0f; }
	public Test.Operation op { get; set; default = Test.Operation.EQUAL; }
	public bool negative { get; set; default = false; }

	public virtual float get_file_value (FileData file) {
		return 0.0f;
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
			node.set_attribute ("value", "%f".printf (number));
		}
		return node;
	}

	public override void load_from_element (Dom.Element node) {
		id = node.get_attribute ("id");
		visible = node.get_attribute ("display") != "none";
		op = Test.Operation.from_xml_attribute (node.get_attribute ("op"));
		negative = node.get_attribute ("negative") == "true";
		float value;
		if (node.get_attribute_as_float ("value", out value)) {
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

		// Value
		spin_button = new Gtk.SpinButton.with_range (0, int.MAX, 1.0);
		spin_button.digits = 2;
		spin_button.width_chars = 5;
		spin_button.visible = false;
		spin_button.value = number;

		undo_button = new Gtk.Button.from_icon_name ("edit-undo");
		undo_button.visible = false;
		undo_button.clicked.connect (() => show_custom_value (false));

		// Ratio selector
		names = new Gtk.StringList (null);
		var selected_size = -1;
		idx = 0;
		foreach (unowned var info in Size_List) {
			names.append (_(info.display_name));
			if (number == Size_List[idx].value) {
				selected_size = idx;
				spin_button.value = Size_List[idx].value;
			}
			idx++;
		}
		// Translators: choose another value
		names.append (_("Other…"));
		size_selector = new Gtk.DropDown (names, null);
		if (selected_size >= 0) {
			size_selector.set_selected (selected_size);
		}
		else {
			show_custom_value (true);
		}

		// Horizontal Box
		var control = new Gtk.Box (Gtk.Orientation.HORIZONTAL, HORIZONTAL_SPACING);
		control.append (operation_selector);
		control.append (spin_button);
		control.append (undo_button);
		control.append (size_selector);

		operation_selector.notify["selected"].connect (() => options_changed ());
		size_selector.notify["selected"].connect (() => {
			var selected_idx = size_selector.get_selected ();
			if (selected_idx >= Size_List.length) {
				show_custom_value (true);
			}
			else {
				spin_button.value = Size_List[selected_idx].value;
			}
			options_changed ();
		});
		spin_button.value_changed.connect (() => options_changed ());

		return control;
	}

	public override void update_from_options () throws Error {
		var selected = operation_selector.get_selected ();
		if (selected >= Int_Operations.length)
			throw new IOError.FAILED ("The test definition is incomplete");
		unowned var op_info = Int_Operations[selected];
		op = op_info.op;
		negative = op_info.negative;
		var idx = size_selector.get_selected ();
		if (idx < Size_List.length) {
			number = Size_List[idx].value;
		}
		else {
			number = (float) spin_button.value;
		}
	}

	public override void focus_options () {
		//spin_button.grab_focus ();
	}

	void show_custom_value (bool visible) {
		spin_button.visible = visible;
		undo_button.visible = visible;
		size_selector.visible = !visible;
		if (!visible) {
			var value = (float) spin_button.value;
			var idx = 0;
			foreach (unowned var info in Size_List) {
				if (value == Size_List[idx].value) {
					break;
				}
				idx++;
			}
			if (idx >= Size_List.length)
				idx = 0;
			size_selector.set_selected (idx);
		}
	}

	struct SizeInfo {
		string display_name;
		float value;
	}

	const SizeInfo[] Size_List = {
		// Translators: aspect ratio
		{ N_("Square"), 1f },
		// Translators: aspect ratio
		{ N_("5∶4"), 5f / 4f },
		// Translators: aspect ratio
		{ N_("4∶3"), 4f / 3f },
		// Translators: aspect ratio
		{ N_("7∶5"), 7f / 5f },
		// Translators: aspect ratio
		{ N_("3∶2"), 3f / 2f },
		// Translators: aspect ratio
		{ N_("16∶10"), 16f / 10f },
		// Translators: aspect ratio
		{ N_("16∶9"), 16f / 9f },
		// Translators: aspect ratio
		{ N_("1.85∶1"), 1.85f },
		// Translators: aspect ratio
		{ N_("2.39∶1"), 2.39f },
	};

	Gtk.DropDown operation_selector;
	Gtk.DropDown size_selector;
	Gtk.SpinButton spin_button;
	Gtk.Button undo_button;
}
