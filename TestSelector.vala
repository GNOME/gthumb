public class Gth.TestSelector : Gtk.Box {
	public TestSelector () {
		orientation = Gtk.Orientation.HORIZONTAL;
		spacing = HORIZONTAL_SPACING;

		// Test selector
		var names = new Gtk.StringList (null);
		foreach (unowned var test_info in app.ordered_tests) {
			names.append (test_info.display_name);
		}
		test_selector = new Gtk.DropDown (names, null);

		// Test options container
		options_container = new Gtk.Box (Gtk.Orientation.HORIZONTAL, 0);

		append (test_selector);
		append (options_container);
	}

	public void set_test (Test test) {
		var test_idx = -1;
		var idx = 0;
		var test_type = test.get_class ().get_type ();
		foreach (unowned var test_info in app.ordered_tests) {
			if (test_type == test_info.test_type) {
				test_idx = idx;
				break;
			}
			idx++;
		}
		if (test_idx == -1)
			return;

		current_test = test;
		test_selector.set_selected (test_idx);
		if (test_options != null) {
			options_container.remove (test_options);
		}
		test_options = test.create_options ();
		if (test_options != null) {
			options_container.append (test_options);
		}
	}

	public Test get_test () throws Error {
		current_test.update_from_options ();
		return current_test;
	}

	public void focus () {
		if (current_test != null)
			current_test.focus_options ();
	}

	Gtk.DropDown test_selector;
	Gtk.Box options_container;
	Gtk.Widget test_options;
	Test current_test;
}
