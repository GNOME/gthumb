[GtkTemplate (ui = "/org/gnome/gthumb/ui/template-code-chooser.ui")]
public class Gth.TemplateCodeChooser : Adw.NavigationPage {
	public int pos;

	public TemplateCodeChooser (TemplatePage _main_page) {
		main_page = _main_page;
		code_list.bind_model (main_page.allowed_codes.model, new_template_code_row);
	}

	Gtk.Widget new_template_code_row (Object item) {
		var info = item as TemplateCodeInfo;

		var row = new Adw.ActionRow ();
		row.title = _(info.description);
		row.activatable = true;
		row.activated.connect (() => main_page.add_token_for_code (info, pos));

		var icon = new Gtk.Image.from_icon_name ("gth-list-add-symbolic");
		row.add_suffix (icon);

		return row;
	}

	weak TemplatePage main_page;
	[GtkChild] unowned Gtk.ListBox code_list;
}
