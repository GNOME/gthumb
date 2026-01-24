public class Gth.ResizeImages : Object {
	public async FileOperation? resize (Gth.Window window, Job job) throws Error {
		callback = resize.callback;
		dialog = new ResizeDialog (window);
		dialog.saved.connect (() => {
			saved = true;
			dialog.close ();
		});
		dialog.closed.connect (() => {
			if (callback != null) {
				dialog.save_preferences ();
				Idle.add ((owned) callback);
				callback = null;
			}
		});
		cancelled_event = job.cancellable.cancelled.connect (() => {
			dialog.close ();
		});
		job.opens_dialog ();
		dialog.present (window);
		yield;
		job.dialog_closed ();
		if (cancelled_event != 0) {
			job.cancellable.disconnect (cancelled_event);
			cancelled_event = 0;
		}
		if (!saved) {
			throw new IOError.CANCELLED ("Cancelled");
		}
		return dialog.get_operation ();
	}

	SourceFunc callback = null;
	ResizeDialog dialog = null;
	ulong cancelled_event = 0;
	bool saved = false;
}

[GtkTemplate (ui = "/app/gthumb/gthumb/ui/resize-dialog.ui")]
public class Gth.ResizeDialog : Adw.Dialog {
	public signal void saved ();

	public ResizeDialog (Gth.Window window) throws Error {
		settings = new GLib.Settings (GTHUMB_RESIZE_SCHEMA);

		var format_names = new Gtk.StringList (null);
		format_names.append (_("Original"));

		var default_format = settings.get_string (PREF_RESIZE_FORMAT);
		var selected_idx = 0;
		var idx = 0;
		formats = new GenericArray<string>();
		saver_preferences = app.get_ordered_savers ();
		foreach (unowned var preferences in saver_preferences) {
			format_names.append (preferences.get_display_name ());
			formats.add (preferences.get_content_type ());
			idx++;
			if (default_format == preferences.get_content_type ()) {
				selected_idx = idx;
			}
		}
		format.model = format_names;
		format.selected = selected_idx;

		destination.folder = window.get_current_vfs_folder_or_default ();
		width.adjustment.value = settings.get_int (PREF_RESIZE_WIDTH);
		height.adjustment.value = settings.get_int (PREF_RESIZE_HEIGHT);
		unit_group.active_name = settings.get_string (PREF_RESIZE_UNIT);
		keep_aspect_ratio.active = settings.get_boolean (PREF_RESIZE_KEEP_ASPECT_RATIO);
	}

	[GtkCallback]
	void on_cancel (Gtk.Button button) {
		close ();
	}

	[GtkCallback]
	void on_save (Gtk.Button button) {
		saved ();
	}

	public void save_preferences () {
		settings.set_string (PREF_RESIZE_FORMAT, get_format () ?? "");
		settings.set_int (PREF_RESIZE_WIDTH, (int) width.adjustment.value);
		settings.set_int (PREF_RESIZE_HEIGHT, (int) height.adjustment.value);
		settings.set_string (PREF_RESIZE_UNIT, unit_group.active_name);
		settings.set_boolean (PREF_RESIZE_KEEP_ASPECT_RATIO, keep_aspect_ratio.active);
	}

	public FileOperation get_operation () {
		var resize_operation = new ResizeImageOperation ();
		resize_operation.unit = ResizeUnit.from_state (unit_group.active_name);
		resize_operation.max_width = width.adjustment.value;
		resize_operation.max_height = height.adjustment.value;
		resize_operation.keep_ratio = keep_aspect_ratio.active;

		var file_operation = new ImageFileOperation (resize_operation);
		file_operation.content_type = get_format ();
		file_operation.folder = destination.folder;
		return file_operation;
	}

	unowned string? get_format () {
		if (format.selected == 0) {
			return null;
		}
		else {
			var saver = saver_preferences[format.selected - 1];
			return saver.get_content_type ();
		}
	}

	GLib.Settings settings;
	GenericArray<string> formats;
	GenericArray<SaverPreferences> saver_preferences;

	[GtkChild] unowned Gtk.DropDown format;
	[GtkChild] unowned Gth.FolderRow destination;
	[GtkChild] unowned Adw.SpinRow width;
	[GtkChild] unowned Adw.SpinRow height;
	[GtkChild] unowned Adw.ToggleGroup unit_group;
	[GtkChild] unowned Adw.SwitchRow keep_aspect_ratio;
}

public class Gth.ResizeImageOperation : Gth.ImageOperation {
	public ResizeUnit unit;
	public double max_width;
	public double max_height;
	public bool keep_ratio = true;
	public bool allow_upscaling = true;
	public bool high_quality = true;

	public ResizeImageOperation () {
		unit = ResizeUnit.PERCENT;
		max_width = 100;
		max_height = 100;
		keep_ratio = true;
		allow_upscaling = true;
		high_quality = true;
	}

	public override Gth.Image? execute (Image input, Cancellable cancellable) {
		var width = input.width;
		var height = input.height;
		if (unit == ResizeUnit.PERCENT) {
			width = (uint) (width * (max_width / 100.0));
			height = (uint) (height * (max_height / 100.0));
		}
		else if (keep_ratio) {
			Lib.scale_keeping_ratio (ref width, ref height,
				(uint) max_width, (uint) max_height, allow_upscaling);
		}
		else {
			width = (uint) max_width;
			height = (uint) max_height;
		}
		return input.resize_to (width, height, high_quality ? ScaleFilter.BEST : ScaleFilter.POINT, cancellable);
	}
}
