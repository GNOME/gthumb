public class Gth.AppSelector : Object {
	public signal void trying (Gdk.RGBA new_color);

	public async AppInfo? select_app (Gtk.Window? parent, GenericArray<Gth.FileData> files, Cancellable? cancellable = null) throws Error {
		callback = select_app.callback;
		dialog = new AppSelectorDialog (parent, files);
		dialog.selected.connect (() => {
			result = dialog.app_info;
			dialog.close ();
		});
		dialog.close_request.connect (() => {
			if (callback != null) {
				Idle.add (callback);
				callback = null;
			}
			return false;
		});
		if (cancellable != null) {
			cancelled_event = cancellable.cancelled.connect (() => {
				dialog.close ();
			});
		}
		dialog.present ();
		yield;
		if (cancelled_event != 0) {
			cancellable.disconnect (cancelled_event);
			cancelled_event = 0;
		}
		if (result == null) {
			throw new IOError.FAILED ("No app selected");
		}
		return result;
	}

	SourceFunc callback = null;
	AppSelectorDialog dialog = null;
	ulong cancelled_event = 0;
	AppInfo? result = null;
}

[GtkTemplate (ui = "/app/gthumb/gthumb/ui/app-selector-dialog.ui")]
class Gth.AppSelectorDialog : Adw.ApplicationWindow {
	public signal void selected ();
	public AppInfo app_info;

	public AppSelectorDialog (Gtk.Window? _parent, GenericArray<Gth.FileData> files) {
		if (_parent != null) {
			transient_for = _parent;
		}

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
			if ("gthumb" in app.get_executable ()) {
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
