public class Gth.TestSelector : Gtk.Box {
	public TestSelector () {
		orientation = Gtk.Orientation.HORIZONTAL;
		spacing = HORIZONTAL_SPACING;

		// Test selector
		var test_name_expr = new Gtk.PropertyExpression (typeof (Gth.Test), null, "display-name");
		test_selector = new Gtk.DropDown (app.ordered_tests.model, test_name_expr);

		// Test options container
		options_container = new Gtk.Box (Gtk.Orientation.HORIZONTAL, 0);

		append (test_selector);
		append (options_container);
	}

	public void set_test (Test test) {
		var test_idx = -1;
		var idx = 0;
		var test_id = test.id;
		var iter = app.ordered_tests.iterator ();
		while (iter.next ()) {
			unowned var registered_test = iter.get ();
			if (test_id == registered_test.id) {
				test_idx = iter.index ();
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
			test_options.add_css_class ("test-options");
			options_container.append (test_options);
		}
	}

	public Test get_test () throws Error {
		current_test.update_from_options ();
		return current_test;
	}

	public void focus_options () {
		if (current_test != null)
			current_test.focus_options ();
	}

	Gtk.DropDown test_selector;
	Gtk.Box options_container;
	Gtk.Widget test_options;
	Test current_test;
}
