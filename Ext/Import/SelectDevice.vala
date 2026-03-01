public class Gth.SelectDevice : Object {
	public async ActionInfo select (Gth.Window window, Job job) throws Error {
		callback = select.callback;
		dialog = new DeviceDialog (window);
		dialog.selected.connect ((source) => {
			result = source;
			dialog.close ();
		});
		dialog.close_request.connect (() => {
			if (callback != null) {
				Idle.add ((owned) callback);
				callback = null;
			}
			return false;
		});
		cancelled_event = job.cancellable.cancelled.connect (() => {
			dialog.close ();
		});
		job.opens_dialog ();
		dialog.present ();
		yield;
		job.dialog_closed ();
		if (cancelled_event != 0) {
			job.cancellable.disconnect (cancelled_event);
			cancelled_event = 0;
		}
		if (result == null) {
			throw new IOError.CANCELLED ("Cancelled");
		}
		return result;
	}

	SourceFunc callback = null;
	DeviceDialog dialog = null;
	ulong cancelled_event = 0;
	ActionInfo result = null;
}

[GtkTemplate (ui = "/app/gthumb/gthumb/ui/device-dialog.ui")]
public class Gth.DeviceDialog : Adw.Window {
	public signal void selected (ActionInfo source);

	public DeviceDialog (Gth.Window _window) {
		window = _window;
		transient_for = _window;
		modal = true;

		devices = new GenericList<ActionInfo> ();
		update_mount_points ();
		device_group.bind_model (devices.model, new_source_row);
		app.events.mount_points_changed.connect (() => update_mount_points ());

		bookmark_group.bind_model (app.bookmarks.app_bookmarks.model, new_source_row);
		bookmark_group.visible = app.bookmarks.app_bookmarks.length () > 0;

		system_bookmark_group.bind_model (app.bookmarks.system_bookmarks.model, new_source_row);
		system_bookmark_group.visible = app.bookmarks.system_bookmarks.length () > 0;
	}

	void update_mount_points () {
		devices.model.remove_all ();
		devices.model.append (new ActionInfo.for_file ("", window.browser.folder_tree.current_folder.file));
		var mountable = new GenericList<FileData>();
		FileSourceVfs.add_mountable_volumes (mountable);
		foreach (var file_data in mountable) {
			var action = new ActionInfo (
				"",
				file_data.file.get_uri (),
				file_data.info.get_display_name (),
				file_data.info.get_symbolic_icon ()
			);
			devices.model.append (action);
		}
		device_group.visible = devices.length () > 0;
	}

	Gtk.Widget new_source_row (Object item) {
		var source = item as ActionInfo;
		var row = new Adw.ActionRow ();
		row.activatable = true;
		row.add_prefix (new Gtk.Image.from_gicon (source.icon));
		row.set_title (source.display_name);
		row.add_suffix (new Gtk.Image.from_icon_name ("go-next-symbolic"));
		row.activated.connect (() => {
			selected (source);
		});
		return row;
	}

	Gth.Window window;
	GenericList<ActionInfo> devices;
	[GtkChild] unowned Adw.PreferencesGroup device_group;
	[GtkChild] unowned Adw.PreferencesGroup bookmark_group;
	[GtkChild] unowned Adw.PreferencesGroup system_bookmark_group;
}
