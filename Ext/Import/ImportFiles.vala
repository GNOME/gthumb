public class Gth.ImportFiles : Object {
	public File destination;

	public async void import (Gth.Window window, ActionInfo folder_info, Job job) throws Error {
		callback = import.callback;
		destination = File.new_for_uri (folder_info.value.get_string ());
		var files = yield collect_files_from_folder (destination, job);
		dialog = new ImporterDialog (window, folder_info, files);
		dialog.saved.connect ((_importer) => {
			importer = _importer;
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
		if (importer == null) {
			throw new IOError.CANCELLED ("Cancelled");
		}
		yield importer.import_files (window, job);
	}

	async ListModel collect_files_from_folder (File folder, Job job) throws Error {
		var files = new GenericList<FileData>();
		var file_sorter = new Gth.FileSorter();
		file_sorter.set_order ("Time::Modified", true);
		var file_test = new Gth.TestFileTypeMedia ();
		var flags = ForEachFlags.RECURSIVE;
		var attributes = new StringBuilder ();
		attributes.append (FileAttribute.STANDARD_TYPE + "," +
			FileAttribute.STANDARD_IS_HIDDEN + "," +
			FileAttribute.STANDARD_NAME + "," +
			FileAttribute.STANDARD_DISPLAY_NAME + "," +
			FileAttribute.STANDARD_SIZE + "," +
			FileAttribute.STANDARD_ICON + "," +
			FileAttribute.STANDARD_SYMBOLIC_ICON + "," +
			FileAttribute.STANDARD_FAST_CONTENT_TYPE + "," +
			FileAttribute.TIME_MODIFIED + "," +
			FileAttribute.TIME_MODIFIED_USEC);
		if (!Strings.empty (file_test.attributes)) {
			attributes.append (",");
			attributes.append (file_test.attributes);
		}
		if (!Strings.empty (file_sorter.sort_info.required_attributes)) {
			attributes.append (",");
			attributes.append (file_sorter.sort_info.required_attributes);
		}
		var source = app.get_source_for_file (folder);
		yield source.foreach_child (folder, flags, attributes.str, job.cancellable, (child, is_parent) => {
			var action = ForEachAction.CONTINUE;
			if (is_parent) {
				if (child.info.get_is_hidden ()) {
					action = ForEachAction.SKIP;
				}
				else {
					job.subtitle = child.get_display_name ();
				}
			}
			else {
				if (file_test.match (child)) {
					files.model.append (child);
				}
			}
			return action;
		});
		return new Gtk.SortListModel (files.model, file_sorter);
	}

	SourceFunc callback = null;
	ImporterDialog dialog = null;
	ulong cancelled_event = 0;
	Importer importer = null;
}

[GtkTemplate (ui = "/app/gthumb/gthumb/ui/importer-dialog.ui")]
public class Gth.ImporterDialog : Adw.Window {
	public signal void saved (Importer importer);

	public ImporterDialog (Gth.Window _window, ActionInfo folder_info, ListModel model) {
		window = _window;
		transient_for = _window;
		modal = true;
		settings = new GLib.Settings (GTHUMB_IMPORT_SCHEMA);
		initializing = true;

		file_grid.set_model (model);
		update_selection_info ();
		file_grid.view.model.selection_changed.connect (() => update_selection_info ());

		var thumbnailer = new Thumbnailer.for_window (window);
		thumbnailer.save_to_cache = false;
		file_grid.thumbnailer = thumbnailer;
		file_grid.thumbnail_size = window.browser.file_grid.thumbnail_size; //Thumbnailer.Size.NORMAL.to_pixels ();

		var folder = Settings.get_file (settings, PREF_IMPORT_DESTINATION);
		if (folder == null) {
			var path = Environment.get_user_special_dir (UserDirectory.PICTURES);
			if (path != null) {
				folder = File.new_for_path (path);
			}
			else {
				folder = File.new_for_path (Environment.get_home_dir ());
			}
		}
		destination.folder = folder;
		import_page.title = folder_info.display_name;

		automatic_subfolder.enable_expansion = settings.get_boolean (PREF_IMPORT_AUTOMATIC_SUBFOLDER);

		var automatic_date = settings.get_string (PREF_IMPORT_AUTOMATIC_SUBFOLDER_DATE);
		automatic_subfolder_value.selected = (uint) AutomaticDate.from_state (automatic_date);

		var default_format = settings.get_string (PREF_IMPORT_AUTOMATIC_SUBFOLDER_FORMAT);
		var formats = new Gtk.StringList (null);
		var now = new GLib.DateTime.now_local ();
		var selected_format = 0u;
		var idx = 0u;
		foreach (unowned var format in SUBFOLDER_DATE_FORMATS) {
			formats.append (now.format (format));
			if (format == default_format) {
				selected_format = idx;
			}
			idx++;
		}
		automatic_subfolder_format.model = formats;
		automatic_subfolder_format.selected = selected_format;

		delete_imported.active = settings.get_boolean (PREF_IMPORT_DELETE_IMPORTED);

		update_destination_label ();
		initializing = false;
	}

	GenericArray<FileData> get_selected_files () {
		var files = new GenericArray<FileData>();
		var selected = file_grid.view.model.get_selection ();
		for (int64 idx = 0; idx < selected.get_size (); idx++) {
			var pos = selected.get_nth ((uint) idx);
			var file = file_grid.view.model.get_item (pos) as FileData;
			files.add (file);
		}
		if (selected.get_size () == 0) {
			// All the files.
			var iter = new ListIterator<FileData> (file_grid.view.model);
			while (iter.next ()) {
				files.add (iter.get ());
			}
		}
		return files;
	}

	void update_selection_info () {
		uint total_files = 0;
		uint64 total_size = 0;
		var selected_image = false;
		var selected = file_grid.view.model.get_selection ();
		for (int64 idx = 0; idx < selected.get_size (); idx++) {
			var pos = selected.get_nth ((uint) idx);
			var file = file_grid.view.model.get_item (pos) as FileData;
			if (file != null) {
				total_files++;
				total_size += file.info.get_size ();
			}
		}
		if (total_files == 0) {
			// All the files.
			total_files = file_grid.view.model.get_n_items ();
			var iter = new ListIterator<FileData> (file_grid.view.model);
			while (iter.next ()) {
				var file_data = iter.get ();
				total_size += file_data.info.get_size ();
			}
		}
		file_info.label = "%u".printf (total_files);
		size_info.label = GLib.format_size (total_size, FormatSizeFlags.DEFAULT);
	}

	[GtkCallback]
	void on_cancel (Gtk.Button button) {
		save_preferences ();
		close ();
	}

	[GtkCallback]
	void on_save (Gtk.Button button) {
		try {
			var importer = save_preferences ();
			importer.files = get_selected_files ();
			if (importer.files.length == 0) {
				return;
			}

			var info = importer.destination_folder.query_filesystem_info (
				FileAttribute.FILESYSTEM_FREE,
				null);
			var free_space = info.get_attribute_uint64 (FileAttribute.FILESYSTEM_FREE);

			uint64 total_size = 0;
			uint64 max_file_size = 0;
			foreach (var file_data in importer.files) {
				var file_size = file_data.info.get_size ();
				total_size += file_size;
				if (file_size > max_file_size) {
					max_file_size = file_size;
				}
			}
			var min_free_space = total_size +
				max_file_size + // Temporary file
				(total_size * 5 / 100); // 5% of file system fragmentation

			if (free_space <= min_free_space) {
				info = importer.destination_folder.query_info (FileAttribute.STANDARD_DISPLAY_NAME,
					FileQueryInfoFlags.NONE,
					null);
				var message = _("The operation requires %s of space, but only %s are available on “%s”.").printf (
					GLib.format_size (min_free_space, FormatSizeFlags.DEFAULT),
					GLib.format_size (free_space, FormatSizeFlags.DEFAULT),
					info.get_display_name ()
				);
				throw new IOError.NO_SPACE (message);
			}

			saved (importer);
			close ();
		}
		catch (Error error) {
			var title = "";
			if (error is IOError.NO_SPACE) {
				title = _("Not enough free space");
			}
			var dialog = new Adw.AlertDialog (title, error.message);
			dialog.add_response ("close", _("Close"));
			dialog.choose.begin (this, null);
		}
	}

	[GtkCallback]
	void on_automatic_subfolder_enabled (Object obj, ParamSpec param) {
		if (!initializing) {
			update_destination_label ();
		}
	}

	[GtkCallback]
	void on_subfolder_changed (Gtk.Editable editable) {
		if (!initializing) {
			update_destination_label ();
		}
	}

	[GtkCallback]
	void on_subfolder_format_changed (Object obj, ParamSpec param) {
		if (!initializing) {
			update_destination_label ();
		}
	}

	void update_destination_label () {
		if (!Strings.empty (subfolder.text)) {
			destination.subfolder = "/" + subfolder.text;
		}
		else {
			destination.subfolder = "";
		}
		if (automatic_subfolder.enable_expansion) {
			var string_list = automatic_subfolder_format.model as Gtk.StringList;
			var format = string_list.get_string (automatic_subfolder_format.selected);
			var now = new GLib.DateTime.now_local ();
			destination.automatic_subfolder = "/" + now.format (format);
		}
		else {
			destination.automatic_subfolder = "";
		}
	}

	Importer save_preferences () {
		var importer = new Importer ();
		importer.destination_folder = destination.folder;
		importer.automatic_subfolder = automatic_subfolder.enable_expansion;
		importer.automatic_subfolder_date = (AutomaticDate) automatic_subfolder_value.selected;
		importer.automatic_subfolder_format = SUBFOLDER_DATE_FORMATS[automatic_subfolder_format.selected];
		importer.delete_imported = delete_imported.active;

		settings.set_string (PREF_IMPORT_DESTINATION, importer.destination_folder.get_uri ());
		settings.set_boolean (PREF_IMPORT_AUTOMATIC_SUBFOLDER, importer.automatic_subfolder);
		settings.set_string (PREF_IMPORT_AUTOMATIC_SUBFOLDER_DATE, importer.automatic_subfolder_date.to_state ());
		settings.set_string (PREF_IMPORT_AUTOMATIC_SUBFOLDER_FORMAT, importer.automatic_subfolder_format);
		settings.set_boolean (PREF_IMPORT_DELETE_IMPORTED, importer.delete_imported);

		return importer;
	}

	GLib.Settings settings;
	Gth.Window window;
	bool initializing;
	[GtkChild] unowned Gth.FileGrid file_grid;
	[GtkChild] unowned Gtk.Label file_info;
	[GtkChild] unowned Gtk.Label size_info;
	[GtkChild] unowned Gth.DestinationRow destination;
	[GtkChild] unowned Adw.EntryRow subfolder;
	[GtkChild] unowned Adw.NavigationPage import_page;
	[GtkChild] unowned Adw.ExpanderRow automatic_subfolder;
	[GtkChild] unowned Adw.ComboRow automatic_subfolder_value;
	[GtkChild] unowned Adw.ComboRow automatic_subfolder_format;
	[GtkChild] unowned Adw.SwitchRow delete_imported;
	//[GtkChild] unowned Gth.TagsRow tags;
}

public const string[] SUBFOLDER_DATE_FORMATS = {
	"%Y",
	"%Y-%m",
	"%Y-%m-%d",
	"%Y/%m",
	"%Y/%m/%d",
};
