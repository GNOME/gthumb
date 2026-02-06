[GtkTemplate (ui = "/app/gthumb/gthumb/ui/test-expr-editor-group.ui")]
public class Gth.TestExprEditorGroup : Adw.PreferencesGroup {
	public Gth.TestExpr expr;

	construct {
		var action_group = new SimpleActionGroup ();
		rule_page.insert_action_group ("expr", action_group);

		var action = new SimpleAction ("add-rule", GLib.VariantType.STRING);
		action.activate.connect ((_action, param) => {
			unowned var dialog = Util.get_preferences_dialog (this);
			if (dialog != null) {
				dialog.pop_subpage ();
			}
			else {
				if (add_rule_dialog != null) {
					add_rule_dialog.close ();
				}
			}
			add_rule (param.get_string ());
		});
		action_group.add_action (action);

		var empty_row = new Adw.ActionRow ();
		empty_row.title = _("Empty");
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

		Gtk.Widget row = null;

		var delete_button = new Gtk.Button.from_icon_name ("gth-list-delete-symbolic");
		delete_button.add_css_class ("flat");
		delete_button.add_css_class ("circular");
		delete_button.valign = Gtk.Align.CENTER;
		delete_button.margin_start = 6;
		delete_button.clicked.connect (() => {
			expr.remove (test);
		});

		var test_options = test.create_options ();
		if (test_options != null) {
			test_options.valign = Gtk.Align.CENTER;
			var vbox = new Gtk.Box (Gtk.Orientation.VERTICAL, 3);
			vbox.hexpand = true;
			var subtitle = new Gtk.Label (test.display_name);
			subtitle.halign = Gtk.Align.START;
			subtitle.add_css_class ("subtitle");
			vbox.append (subtitle);
			vbox.append (test_options);

			var hbox = new Gtk.Box (Gtk.Orientation.HORIZONTAL, 12);
			hbox.append (vbox);
			hbox.append (delete_button);

			var row_with_options = new Gtk.ListBoxRow ();
			row_with_options.add_css_class ("test-list-row");
			row_with_options.child = hbox;
			row = row_with_options;
		}
		else {
			var action_row = new Adw.ActionRow ();
			if (test.title != null) {
				action_row.title = test.title;
				action_row.subtitle = test.display_name;
				action_row.add_css_class ("property");
			}
			else {
				action_row.title = test.display_name;
			}
			action_row.add_suffix (delete_button);
			row = action_row;
		}

		return row;
	}

	void update_visibility () {
		match_operation_group.visible = (expr != null) && (expr.tests.model.get_n_items () > 1);
	}

	bool rules_loaded = false;

	[GtkCallback]
	void on_add_rule (Gtk.Button source) {
		if (!rules_loaded) {
			rule_list.bind_model (app.ordered_tests.model, new_test_type_row);
			rules_loaded = true;
		}
		// Show the rule chooser page.
		unowned var dialog = Util.get_preferences_dialog (this);
		if (dialog != null) {
			dialog.push_subpage (rule_page);
		}
		else {
			add_rule_dialog = new Gth.AddRuleDialog (this);
			add_rule_dialog.closed.connect (() => {
				add_rule_dialog = null;
			});
			add_rule_dialog.present (root as Gtk.Window);
		}
	}

	Gtk.Widget new_test_type_row (Object item) {
		var test = item as Gth.Test;

		var row = new Adw.ActionRow ();
		row.title = test.display_name;
		row.activatable = true;
		row.action_name = "expr.add-rule";
		row.action_target = test.id;

		var icon = new Gtk.Image.from_icon_name ("gth-list-add-symbolic");
		row.add_suffix (icon);

		return row;
	}

	[GtkChild] unowned Gtk.ListBox test_list;
	[GtkChild] unowned Gtk.ListBox match_operation_group;
	[GtkChild] unowned Gtk.CheckButton match_all;
	[GtkChild] unowned Gtk.CheckButton match_any;
	[GtkChild] public unowned Adw.NavigationPage rule_page;
	[GtkChild] unowned Gtk.ListBox rule_list;

	AddRuleDialog add_rule_dialog = null;
}
