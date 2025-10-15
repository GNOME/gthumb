public class Gth.AppSelector : Object {
	public async AppInfo? select_app (Gtk.Window? parent, GenericArray<Gth.FileData> files, Cancellable? cancellable = null) throws Error {
		callback = select_app.callback;
		dialog = new AppSelectorDialog (files);
		if (!dialog.has_applications ()) {
			throw new IOError.FAILED (_("No application registered for this file type"));
		}
		dialog.selected.connect (() => {
			result = dialog.app_info;
			dialog.close ();
		});
		dialog.closed.connect (() => {
			if (callback != null) {
				Idle.add ((owned) callback);
				callback = null;
			}
		});
		if (cancellable != null) {
			cancelled_event = cancellable.cancelled.connect (() => {
				dialog.close ();
			});
		}
		dialog.present (parent);
		yield;
		if (cancelled_event != 0) {
			cancellable.disconnect (cancelled_event);
			cancelled_event = 0;
		}
		if (result == null) {
			throw new IOError.CANCELLED ("No application selected");
		}
		return result;
	}

	SourceFunc callback = null;
	AppSelectorDialog dialog = null;
	ulong cancelled_event = 0;
	AppInfo? result = null;
}

[GtkTemplate (ui = "/app/gthumb/gthumb/ui/app-selector-dialog.ui")]
class Gth.AppSelectorDialog : Adw.Dialog {
	public signal void selected ();
	public AppInfo app_info;

	public AppSelectorDialog (GenericArray<Gth.FileData> files) {
		var applications = new List<AppInfo>();
		var content_types = new GenericSet<string>(str_hash, str_equal);
		foreach (unowned var file in files) {
			unowned var content_type = file.get_content_type ();
			if ((content_type == null) || ContentType.is_unknown (content_type)) {
				continue;
			}
			if (content_type in content_types) {
				continue;
			}
			applications.concat (AppInfo.get_all_for_type (content_type));
			content_types.add (content_type);
		}
		applications.sort ((a, b) => a.get_display_name ().collate (b.get_display_name ()));

		apps = new GenericList<AppInfo>();
		var used_apps = new GenericSet<string>(str_hash, str_equal);
		foreach (unowned var app in applications) {
			if (Config.APP_ID in app.get_id ()) {
				continue;
			}
			if (app.get_id () in used_apps) {
				continue;
			}
			used_apps.add (app.get_id ());
			apps.model.append (app);
		}
		app_list.bind_model (apps.model, new_app_row);
	}

	public bool has_applications () {
		return apps.length () > 0;
	}

	Gtk.Widget new_app_row (Object item) {
		var app_info = item as AppInfo;
		var row = new Adw.ActionRow ();
		row.activatable = true;
		var icon = new Gtk.Image.from_gicon (app_info.get_icon ());
		icon.set_icon_size (Gtk.IconSize.LARGE);
		row.add_prefix (icon);
		row.set_title (app_info.get_name ());
		return row;
	}

	[GtkCallback]
	void on_cancel (Gtk.Button button) {
		close ();
	}

	[GtkCallback]
	void on_select (Gtk.Button button) {
		var row = app_list.get_selected_row ();
		if (row != null) {
			var idx = row.get_index ();
			app_info = apps.model.get_item (idx) as AppInfo;
			selected ();
		}
	}

	[GtkCallback]
	void on_row_activated (Gtk.ListBoxRow row) {
		var idx = row.get_index ();
		app_info = apps.model.get_item (idx) as AppInfo;
		selected ();
	}

	[GtkChild] unowned Gtk.ListBox app_list;
	GenericList<AppInfo> apps;
}
