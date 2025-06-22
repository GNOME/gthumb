[GtkTemplate (ui = "/app/gthumb/gthumb/ui/test-expr-editor-group.ui")]
public class Gth.TestExprEditorGroup : Adw.PreferencesGroup {
	public Gth.TestExpr expr;

	construct {
		var action_group = new SimpleActionGroup ();
		rule_page.insert_action_group ("expr", action_group);

		var action = new SimpleAction ("add-rule", GLib.VariantType.STRING);
		action.activate.connect ((_action, param) => {
			unowned var dialog = get_dialog ();
			if (dialog != null) {
				dialog.pop_subpage ();
			}
			add_rule (param.get_string ());
		});
		action_group.add_action (action);

		var empty_row = new Adw.ActionRow ();
		empty_row.title = _("No Rule");
		empty_row.sensitive = false;
		empty_row.halign = Gtk.Align.CENTER;
		test_list.set_placeholder (empty_row);
	}

	public void set_expr (Gth.TestExpr? _expr) {
		if (_expr != null) {
			expr = _expr;
		}
		else {
			expr = new Gth.TestExpr ();
		}
		test_list.bind_model (expr.tests.model, new_test_row);
		if (expr.operation == TestExpr.Operation.UNION) {
			match_any.active = true;
		}
		else {
			match_all.active = true;
		}
		update_visibility ();
		expr.tests.model.items_changed.connect (() => update_visibility ());
	}

	public void update_from_options () throws Error {
		foreach (unowned var test in expr.tests) {
			try {
				test.update_from_options ();
			}
			catch (Error error) {
				test.focus_options ();
				throw error;
			}
		}
		expr.operation = match_all.active ? TestExpr.Operation.INTERSECTION : TestExpr.Operation.UNION;
	}

	public void add_rule (string id) {
		var registered_test = app.get_test_by_id (id);
		if (registered_test != null) {
			var new_test = registered_test.duplicate ();
			expr.add (new_test);
			new_test.focus_options ();
		}
	}

	Gtk.Widget new_test_row (Object item) {
		var test = item as Gth.Test;

		var row = new Adw.ActionRow ();
		row.title = test.display_name;

		var icon = new Gtk.Image.from_icon_name ("filter-symbolic");
		row.add_prefix (icon);

		var test_options = test.create_options ();
		if (test_options != null) {
			test_options.valign = Gtk.Align.CENTER;
			row.add_suffix (test_options);
		}

		var delete_button = new Gtk.Button.from_icon_name ("list-delete-symbolic");
		delete_button.add_css_class ("flat");
		delete_button.valign = Gtk.Align.CENTER;
		delete_button.clicked.connect (() => {
			expr.remove (test);
		});
		row.add_suffix (delete_button);

		return row;
	}

	void update_visibility () {
		match_operation_group.visible = (expr != null) && (expr.tests.model.get_n_items () > 1);
	}

	bool rules_loaded = false;

	[GtkCallback]
	void on_add_rule (Gtk.Button source) {
		if (!rules_loaded) {
			var test_list = Widgets.new_boxed_list ();
			test_list.bind_model (app.ordered_tests.model, new_test_type_row);
			rule_types.add (test_list);
			rules_loaded = true;
		}
		// Show the rule chooser page.
		unowned var dialog = get_dialog ();
		if (dialog != null) {
			dialog.push_subpage (rule_page);
		}
	}

	unowned Adw.PreferencesDialog? get_dialog () {
		unowned var parent = this.parent;
		while (parent != null) {
			if (parent is Adw.PreferencesDialog) {
				return parent as Adw.PreferencesDialog;
			}
			parent = parent.parent;
		}
		return null;
	}

	Gtk.Widget new_test_type_row (Object item) {
		var test = item as Gth.Test;

		var row = new Adw.ActionRow ();
		row.title = test.display_name;
		row.activatable = true;
		row.action_name = "expr.add-rule";
		row.action_target = test.id;

		var icon = new Gtk.Image.from_icon_name ("list-add-symbolic");
		row.add_suffix (icon);

		return row;
	}

	[GtkChild] unowned Gtk.ListBox test_list;
	[GtkChild] unowned Gtk.ListBox match_operation_group;
	[GtkChild] unowned Gtk.CheckButton match_all;
	[GtkChild] unowned Gtk.CheckButton match_any;
	[GtkChild] unowned Adw.NavigationPage rule_page;
	[GtkChild] unowned Adw.PreferencesGroup rule_types;
}
