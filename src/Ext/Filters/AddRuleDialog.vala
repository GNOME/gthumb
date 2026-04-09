[GtkTemplate (ui = "/org/gnome/gthumb/ui/add-rule-dialog.ui")]
public class Gth.AddRuleDialog : Adw.Dialog {
	public AddRuleDialog (TestExprEditorGroup expr_group) {
		set_child (expr_group.rule_page);
	}
}
