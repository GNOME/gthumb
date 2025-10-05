[GtkTemplate (ui = "/app/gthumb/gthumb/ui/preferences-dialog.ui")]
public class Gth.PreferencesDialog : Adw.Dialog {
	construct {
		pages = new GenericList<PageInfo?> ();
		pages.model.append (new PageInfo ("general", _("General")));
		pages.model.append (new PageInfo ("browser", _("Browser")));
		pages.model.append (new PageInfo ("viewer", _("Images")));
#if HAVE_GSTREAMER
		pages.model.append (new PageInfo ("video", _("Videos")));
		content_view.add (new Gth.VideoPreferences ());
#endif
		//pages.model.append (new PageInfo ("presentation", _("Presentation")));
		// Translators: section for the file saving options in the preferences dialog.
		pages.model.append (new PageInfo ("saving", _("Saving")));
		//pages.model.append (new PageInfo ("print", _("Print")));
		pages.model.append (new PageInfo ("shortcuts", _("Shortcuts")));
		page_list.bind_model (pages.model, new_page_row);
	}

	Gtk.Widget new_page_row (Object item) {
		var page_info = item as PageInfo;
		var row = new Adw.ActionRow ();
		row.activatable = true;
		row.set_title (page_info.display_name);
		return row;
	}

	[GtkCallback]
	void on_page_activated (Gtk.ListBoxRow row) {
		var idx = row.get_index ();
		var page_info = pages.model.get_item (idx) as PageInfo;
		content_view.replace_with_tags ({ page_info.tag });
	}

	class PageInfo : Object {
		public string tag;
		public string display_name;

		public PageInfo (string _tag, string _display_name) {
			tag = _tag;
			display_name = _display_name;
		}
	}

	[GtkChild] unowned Gtk.ListBox page_list;
	[GtkChild] unowned Adw.NavigationView content_view;
	//[GtkChild] unowned Adw.ToastOverlay toast_overlay;
	[GtkChild] unowned Gth.GeneralPreferences general_page;
	[GtkChild] unowned Gth.BrowserPreferences browser_page;
	[GtkChild] unowned Gth.SaversPreferences saving_page;
	[GtkChild] unowned Gth.ViewerPreferences viewer_page;
	[GtkChild] unowned Gth.ShortcutsPreferences shortcuts_page;

	GenericList<PageInfo?> pages;
}
