public class Gth.RenameFiles : Object {
	public async GenericList<RenamedFile>? rename (Gth.MainWindow window, GenericList<File> files, Job job) throws Error {
		callback = rename.callback;
		dialog = new RenameDialog (window, files);
		dialog.saved.connect (() => {
			saved = true;
			dialog.close ();
		});
		dialog.close_request.connect (() => {
			if (callback != null) {
				dialog.save_preferences ();
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
		if (!saved) {
			throw new IOError.CANCELLED ("Cancelled");
		}
		return yield dialog.get_renamed_files (job);
	}

	SourceFunc callback = null;
	RenameDialog dialog = null;
	ulong cancelled_event = 0;
	bool saved = false;
}

[GtkTemplate (ui = "/app/gthumb/gthumb/ui/rename-dialog.ui")]
public class Gth.RenameDialog : Adw.Window {
	public signal void saved ();

	public RenameDialog (Gth.MainWindow _window, GenericList<File> _files) throws Error {
		window = _window;
		transient_for = _window;
		files = _files;
		single_file = files.length () == 1;
		if (single_file) {
			var file = files.first ();
			var info = file.query_info (FileAttribute.STANDARD_EDIT_NAME, FileQueryInfoFlags.NONE, null);
			name_entry.text = info.get_edit_name ();
			strategy_group.active_name = "simple";
		}
		name_group.visible = single_file;
		template_group.visible = !single_file;
		name_list_group.visible = !single_file;
		sort_group.visible = !single_file;
		strategy_group.visible = single_file;
		name_list = new GenericList<RenameEntry> ();
		if (!single_file) {
			construct_name_list ();
		}

		// Set values from the settings
		settings = new GLib.Settings (GTHUMB_RENAME_SCHEMA);
		enumerator_start.value = settings.get_int (PREF_RENAME_FIRST_INDEX);
		sort_direction.selected = settings.get_boolean (PREF_RENAME_REVERSE_ORDER) ? 1u : 0u;
		case_transform.selected = (uint) TextTransform.from_state (settings.get_string (PREF_RENAME_TEXT_TRANSFORM));

		var sort_id = settings.get_string (PREF_RENAME_SORT_NAME);
		var sort_names = new Gtk.StringList (null);
		var sort_idx = 0u;
		var idx = 0u;
		foreach (unowned var id in app.ordered_sorters) {
			unowned var info = app.sorters.get (id);
			sort_names.append (info.display_name);
			if (id == sort_id) {
				sort_idx = idx;
			}
			idx++;
		}
		sort_name.model = sort_names;
		sort_name.selected = sort_idx;

		// Signals
		template_page.save.connect (() => {
			set_name_template (template_page.get_template ());
			main_view.pop ();
		});
		enumerator_start.notify["value"].connect (() => update_file_list ());
		case_transform.notify["selected"].connect (() => update_file_list ());
		sort_name.notify["selected"].connect (() => {
			template_attributes = get_template_attributes ();
			attributes_changed = true;
			update_file_list ();
		});
		sort_direction.notify["selected"].connect (() => {
			template_attributes = get_template_attributes ();
			attributes_changed = true;
			update_file_list ();
		});
		if (single_file) {
			name_changed_id = name_entry.changed.connect (() => update_file_list ());
			strategy_group.notify["active-name"].connect (() => {
				if (strategy_group.active_name == "simple") {
					template_group.visible = false;
					reset_name.visible = true;
					focus_name ();
				}
				else {
					template_group.visible = true;
					reset_name.visible = false;
				}
				update_file_list ();
			});
		}

		set_name_template (settings.get_string (PREF_RENAME_TEMPLATE));

		if (single_file) {
			focus_name ();
		}
	}

	public async GenericList<RenamedFile> get_renamed_files (Job job) throws Error {
		if (single_file) {
			var result = new GenericList<RenamedFile> ();
			var file_data = file_data_list.first ();
			var old_file = file_data.file;
			var old_parent = old_file.get_parent ();
			var new_file = old_parent.get_child_for_display_name (name_entry.text);
			result.model.append (new RenamedFile (old_file, new_file));
			return result;
		}

		yield update_file_data_list (job);
		var result = new GenericList<RenamedFile> ();
		foreach (unowned var entry in name_list) {
			try {
				var old_file = entry.file_data.file;
				var old_parent = old_file.get_parent ();
				var new_file = old_parent.get_child_for_display_name (entry.new_name);
				result.model.append (new RenamedFile (old_file, new_file));
			}
			catch (Error error) {
			}
		}
		return result;
	}

	public void save_preferences () {
		settings.set_string (PREF_RENAME_TEMPLATE, template_text);
		settings.set_int (PREF_RENAME_FIRST_INDEX, (int) enumerator_start.value);
		var sort_info = app.sorters.get (app.ordered_sorters[sort_name.selected]);
		settings.set_string (PREF_RENAME_SORT_NAME, sort_info.id);
		settings.set_boolean (PREF_RENAME_REVERSE_ORDER, (sort_direction.selected == 1));
		settings.set_string (PREF_RENAME_TEXT_TRANSFORM, ((TextTransform) case_transform.selected).to_state ());
	}

	void set_name_template (string value) {
		template_text = value;
		var new_template_attributes = get_template_attributes ();
		if (new_template_attributes != template_attributes) {
			template_attributes = new_template_attributes;
			attributes_changed = true;
		}
		var rename_template = new RenameTemplate (template_text, TemplateFlags.PREVIEW | TemplateFlags.NO_HIGHLIGHT);
		template_preview.label = rename_template.get_preview ();
		update_file_list ();
	}

	[GtkCallback]
	void on_save (Gtk.Button button) {
		saved ();
	}

	[GtkCallback]
	void on_activated_template (Adw.ActionRow row) {
		template_page.set_template_text (template_text);
		main_view.push (template_page);
	}

	[GtkCallback]
	void on_undo_clicked (Gtk.Button button) {
		try {
			var file = files.first ();
			var info = file.query_info (FileAttribute.STANDARD_EDIT_NAME, FileQueryInfoFlags.NONE, null);
			update_name (info.get_edit_name ());
		}
		catch (Error error) {
		}
	}

	string get_template_attributes () {
		var attributes = new StringBuilder (REQUIRED_ATTRIBUTES);
		var sort_info = app.sorters.get (app.ordered_sorters[sort_name.selected]);
		if ((sort_info != null) && (sort_info.required_attributes != null)) {
			attributes.append_c (',');
			attributes.append (sort_info.required_attributes);
		}
		Template.for_each_token (template_text, TemplateFlags.DEFAULT, (parent_code, code, args) => {
			switch (code) {
			case RenameTemplate.Code.FILE_ATTRIBUTE:
				attributes.append_c (',');
				attributes.append (args[0]);
				break;

			case RenameTemplate.Code.DIGITALIZATION_TIME:
				foreach (unowned var tag in Exiv2.ORIGINAL_DATE_TAG_NAMES) {
					attributes.append_c (',');
					attributes.append (tag);
				}
				break;

			case RenameTemplate.Code.MODIFICATION_TIME:
				attributes.append_c (',');
				attributes.append (FileAttribute.TIME_MODIFIED);
				attributes.append_c (',');
				attributes.append (FileAttribute.TIME_MODIFIED_USEC);
				break;

			case RenameTemplate.Code.CREATION_TIME:
				attributes.append_c (',');
				attributes.append (FileAttribute.TIME_CREATED);
				attributes.append_c (',');
				attributes.append (FileAttribute.TIME_CREATED_USEC);
				break;

			default:
				break;
			}
			return false;
		});
		return attributes.str;
	}

	void construct_name_list () {
		name_list_column_view.model = new Gtk.SingleSelection (name_list.model);

		// Old name
		var factory = old_name_column.factory as Gtk.SignalListItemFactory;
		factory.setup.connect ((item) => {
			var list_item = item as Gtk.ListItem;
			var label = new Gtk.Inscription ("");
			label.text_overflow = Gtk.InscriptionOverflow.ELLIPSIZE_MIDDLE;
			label.xalign = 0.0f;
			list_item.child = label;
		});
		factory.bind.connect ((item) => {
			var list_item = item as Gtk.ListItem;
			var label = list_item.child as Gtk.Inscription;
			var entry = list_item.item as RenameEntry;
			var old_name = entry.file_data.get_display_name ();
			label.text = old_name;
			label.tooltip_text = old_name;
		});

		// New name
		factory = new_name_column.factory as Gtk.SignalListItemFactory;
		factory.setup.connect ((item) => {
			var list_item = item as Gtk.ListItem;
			var label = new Gtk.Inscription ("");
			label.text_overflow = Gtk.InscriptionOverflow.ELLIPSIZE_MIDDLE;
			label.xalign = 0.0f;
			list_item.child = label;
		});
		factory.bind.connect ((item) => {
			var list_item = item as Gtk.ListItem;
			var label = list_item.child as Gtk.Inscription;
			var entry = list_item.item as RenameEntry;
			label.text = entry.new_name;
			label.tooltip_text = entry.new_name;
		});
	}

	void update_file_list () {
		if (attributes_changed || (file_data_list == null)) {
			query_file_list_attributes.begin ();
		}
		else {
			update_entry_list ();
		}
	}

	async void query_file_list_attributes () {
		if (info_job != null) {
			info_job.cancel ();
		}
		var local_job = window.new_job ("Query Info");
		info_job = local_job;
		yield update_file_data_list (local_job);
		local_job.done ();
		if (info_job == local_job) {
			info_job = null;
		}
	}

	async void update_file_data_list (Job job) {
		try {
			file_data_list = yield FileManager.query_list_info (files, template_attributes, QueryListFlags.NOT_RECURSIVE, job.cancellable);
			attributes_changed = false;
			update_entry_list ();
		}
		catch (Error error) {
			toast_overlay.dismiss_all ();
			toast_overlay.add_toast (Util.new_literal_toast (error.message));
		}
	}

	void update_entry_list () {
		name_list.model.remove_all ();

		if (strategy_group.active_name == "simple") {
			var file_data = file_data_list.first ();
			var new_name = name_entry.text;
			name_list.model.append (new RenameEntry (file_data, new_name));
			return;
		}

		var rename_template = new RenameTemplate (template_text, TemplateFlags.DEFAULT);
		rename_template.enumerator_index = (int) enumerator_start.value;

		var text_transform = (TextTransform) case_transform.selected;
		var sorted_list = file_data_list.duplicate ();
		var sort_id = app.ordered_sorters[sort_name.selected];
		if (sort_id != "Private::Unsorted") {
			var sort_info = app.get_sorter_by_id (sort_id);
			sorted_list.sort ((a, b) => sort_info.cmp_func (a, b));
		}
		if (sort_direction.selected == 1) {
			sorted_list.reverse_order ();
		}
		foreach (var file_data in sorted_list) {
			var new_name = rename_template.get_new_name (file_data);
			if (text_transform == TextTransform.UPPERCASE) {
				new_name = new_name.up ();
			}
			else if (text_transform == TextTransform.LOWERCASE) {
				new_name = new_name.down ();
			}
			name_list.model.append (new RenameEntry (file_data, new_name));
			rename_template.enumerator_index++;

			if (single_file) {
				update_name (new_name);
				focus_name ();
			}
		}
	}

	void update_name (string value) {
		SignalHandler.block (name_entry, name_changed_id);
		name_entry.text = value;
		SignalHandler.unblock (name_entry, name_changed_id);
	}

	void focus_name () {
		name_entry.grab_focus ();
		Util.select_filename_without_ext (name_entry);
	}

	Gth.MainWindow window;
	GenericList<File> files;
	GenericList<RenameEntry> name_list;
	GenericList<FileData> file_data_list = null;
	Job info_job = null;
	bool attributes_changed = true;
	string template_text;
	string template_attributes = null;
	GLib.Settings settings;
	bool single_file;
	ulong name_changed_id;

	[GtkChild] unowned Adw.PreferencesGroup name_group;
	[GtkChild] unowned Adw.EntryRow name_entry;
	[GtkChild] unowned Adw.PreferencesGroup template_group;
	// [GtkChild] unowned Adw.ActionRow template;
	[GtkChild] unowned Adw.SpinRow enumerator_start;
	[GtkChild] unowned Adw.ComboRow case_transform;
	[GtkChild] unowned Adw.PreferencesGroup sort_group;
	[GtkChild] unowned Adw.ComboRow sort_name;
	[GtkChild] unowned Adw.ComboRow sort_direction;
	[GtkChild] unowned Gtk.ColumnViewColumn old_name_column;
	[GtkChild] unowned Gtk.ColumnViewColumn new_name_column;
	[GtkChild] unowned Gtk.ColumnView name_list_column_view;
	[GtkChild] unowned Adw.NavigationView main_view;
	[GtkChild] unowned Gth.RenameTemplatePage template_page;
	[GtkChild] unowned Gtk.Label template_preview;
	[GtkChild] unowned Adw.ToggleGroup strategy_group;
	[GtkChild] unowned Adw.PreferencesGroup name_list_group;
	[GtkChild] unowned Gtk.Button reset_name;
	[GtkChild] unowned Adw.ToastOverlay toast_overlay;

	const string REQUIRED_ATTRIBUTES =
		FileAttribute.STANDARD_TYPE + "," +
		FileAttribute.STANDARD_NAME + "," +
		FileAttribute.STANDARD_DISPLAY_NAME + "," +
		FileAttribute.STANDARD_EDIT_NAME;
}

class Gth.RenameEntry : Object {
	public FileData file_data;
	public string new_name;

	public RenameEntry (FileData _file_data, string _new) {
		file_data = _file_data;
		new_name = _new;
	}
}
