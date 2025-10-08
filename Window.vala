[GtkTemplate (ui = "/app/gthumb/gthumb/ui/window.ui")]
public class Gth.Window : Adw.ApplicationWindow {
	public Gth.JobQueue jobs;
	public bool closing;
	public SimpleActionGroup action_group;
	public FileManager file_manager;
	public Page current_page;
	[GtkChild] public unowned Gtk.Stack stack;
	[GtkChild] public unowned Gth.Browser browser;
	[GtkChild] public unowned Gth.Viewer viewer;
	public unowned Gth.SidebarResizer active_resizer;

	public enum Page {
		NONE = 0,
		BROWSER,
		VIEWER,
	}

	ulong clipboard_event = 0;

	public Window () {
		Object (application: app);
	}

	public void add_toast (Adw.Toast toast) {
		viewer.toasts++;
		viewer.reveal_overlay_controls (false);

		toast_overlay.dismiss_all ();
		toast_overlay.add_toast (toast);

		var local_toast = toast;
		local_toast.dismissed.connect (() => {
			viewer.toasts--;
			if (viewer.toasts == 0) {
				viewer.reveal_overlay_controls ();
			}
			local_toast = null;
		});
	}

	public void show_message (string message) {
		add_toast (Util.new_literal_toast (message));
	}

	public void show_error (Error error) {
		if ((error is IOError.CANCELLED)
			|| (error is Gtk.DialogError.DISMISSED)
			|| (error is Gtk.DialogError.CANCELLED))
		{
			return;
		}
		var toast = Util.new_literal_toast (error.message);
		toast.priority = Adw.ToastPriority.HIGH;
		add_toast (toast);
	}

	public void edge_reached (string? msg = null) {
		if (msg != null) {
			show_message (msg);
		}
		else {
			get_surface ().beep ();
		}
	}

	public void copy_text_to_clipboard (string text) {
		var clipboard = get_clipboard ();
		clipboard.set_text (text);
		show_message (_("Copied to Clipboard"));
	}

	bool file_not_saved () {
		return (current_page == Page.VIEWER) && viewer.current_file.get_is_modified ();
	}

	async void save_and_close () {
		try {
			yield viewer.ask_whether_to_save ();
		}
		catch (Error error) {
			show_error (error);
			return;
		}
		close ();
	}

	public async void set_page (Page new_page) {
		if (new_page == current_page) {
			return;
		}
		if (current_page == Page.NONE) {
			update_scripts_actions ();
		}
		if ((current_page == Page.VIEWER) && viewer.current_file.get_is_modified ()) {
			try {
				yield viewer.ask_whether_to_save ();
			}
			catch (Error error) {
				show_error (error);
				return;
			}
		}
		var previuos_page = current_page;
		current_page = new_page;
		switch (current_page) {
		case Page.BROWSER:
			if (!browser.never_loaded && (previuos_page == Page.VIEWER)) {
				//if (viewer.main_view.show_sidebar) {
				//	browser.content_view.max_sidebar_width = viewer.main_view.max_sidebar_width;
				//}
				viewer.before_close_page ();
				browser.restore_window_size ();
				if ((viewer.current_file != null)
					&& (browser.property_sidebar.current_file != null)
					&& viewer.current_file.file.equal (browser.property_sidebar.current_file.file))
				{
					browser.property_sidebar.update_view ();
				}
				else if (viewer.position >= 0) {
					browser.select_position (viewer.position);
				}
				else if (viewer.current_file != null) {
					browser.select_file (viewer.current_file.file);
				}
			}
			stack.set_visible_child (browser);
			if (browser.never_loaded) {
				yield browser.first_load ();
				if (previuos_page == Page.VIEWER) {
					if (viewer.current_file != null) {
						browser.open_location (
							viewer.current_file.file.get_parent (),
							LoadAction.OPEN,
							viewer.current_file.file);
					}
				}
			}
			browser.update_title ();
			browser.focus_thumbnail_list ();
			break;
		case Page.VIEWER:
			if (previuos_page == Page.BROWSER) {
				browser.save_window_size ();
				if (browser.content_view.show_sidebar) {
					viewer.main_view.max_sidebar_width = browser.content_view.max_sidebar_width;
				}
			}
			stack.set_visible_child (viewer);
			viewer.update_title ();
			viewer.focus_viewer ();
			break;
		case Page.NONE:
			break;
		}
		update_sensitivity ();
	}

	public Gth.Job new_job (string description, JobFlags flags = JobFlags.DEFAULT, string? icon_name = null) {
		return jobs.new_job (description, flags, icon_name);
	}

	bool cancel_jobs () {
		if (jobs.size () == 0) {
			return false;
		}
		jobs.cancel_all ();
		return true;
	}

	public void on_setting_change (string key) {
		switch (key) {
		case PREF_BROWSER_THUMBNAIL_SIZE:
			browser.set_thumbnail_size (app.settings.get_int (PREF_BROWSER_THUMBNAIL_SIZE));
			break;
		case PREF_BROWSER_THUMBNAIL_CAPTION:
			browser.reload ();
			break;
		case PREF_BROWSER_OPEN_IN_FULLSCREEN:
			browser.open_in_fullscreen = app.settings.get_boolean (PREF_BROWSER_OPEN_IN_FULLSCREEN);
			break;
		}
	}

	public string? get_monitor_name () {
		unowned var display = get_display ();
		unowned var monitor = display.get_monitor_at_surface (get_surface ());
		if (monitor == null) {
			return null;
		}
		return monitor.description;
	}

	public void get_monitor_geometry (out int width, out int height) {
		unowned var display = get_display ();
		unowned var monitor = display.get_monitor_at_surface (get_surface ());
		width = monitor.geometry.width;
		height = monitor.geometry.height;
	}

	IccProfile monitor_profile = null;
	bool no_monitor_profile = false;

	public async IccProfile? get_monitor_profile (Cancellable cancellable) {
		if (no_monitor_profile) {
			return null;
		}
		if (monitor_profile != null) {
			return monitor_profile;
		}
		unowned var display = get_display ();
		unowned var monitor = display.get_monitor_at_surface (get_surface ());
		if (monitor == null) {
			no_monitor_profile = true;
			return null;
		}
		try {
			monitor_profile = yield app.color_manager.get_profile_async (monitor.description, cancellable);
		}
		catch (Error error) {
			if (!(error is IOError.CANCELLED)) {
				no_monitor_profile = true;
			}
		}
		return monitor_profile;
	}

	void update_sensitivity () {
		// TODO
	}

	void update_sensitivity_for_clipboard () {
		unowned var clipboard = get_clipboard ();
		unowned var formats = clipboard.get_formats ();
		var can_open = formats.match (Gdk.ContentFormats.parse ("image/png"));
		Util.enable_action (action_group, "open-clipboard", can_open);
		var can_paste = formats.match (new Gdk.ContentFormats ({ "gthumb/cut-files", "text/uri-list" }));
		Util.enable_action (action_group, "paste-files", can_paste);
		Util.enable_action (browser.folder_actions, "paste", can_paste);
	}

	public void save_preferences () {
		if (get_realized ()) {
			browser.save_preferences (current_page == Page.BROWSER);
			viewer.save_preferences (current_page == Page.VIEWER);
		}
	}

	void before_closing () {
		var save_preferences = !app.quitting && app.one_window () && get_realized ();
		if (save_preferences) {
			browser.save_preferences (current_page == Page.BROWSER);
			viewer.save_preferences (current_page == Page.VIEWER);
		}
		viewer.release_resources ();
		browser.release_resources ();
		progress_dialog.force_close ();
	}

	public GenericArray<Gth.FileData>? get_selected () {
		switch (current_page) {
		case Page.BROWSER:
			return browser.get_selected ();
		case Page.VIEWER:
			return viewer.get_selected ();
		default:
			return null;
		}
	}

	public FileData? get_current_file_data () {
		switch (current_page) {
		case Page.BROWSER:
			return browser.get_selected_file_data ();
		case Page.VIEWER:
			return viewer.current_file;
		default:
			return null;
		}
	}

	public File? get_current_file () {
		var file_data = get_current_file_data ();
		return (file_data != null) ? file_data.file : null;
	}

	public GenericList<File>? get_selected_files () {
		switch (current_page) {
		case Page.BROWSER:
			return browser.get_selected_files ();
		case Page.VIEWER:
			return viewer.get_selected_file ();
		default:
			return null;
		}
	}

	public GenericList<FileData>? get_selected_file_data_list () {
		switch (current_page) {
		case Page.BROWSER:
			return browser.get_selected_file_data_list ();
		case Page.VIEWER:
			return viewer.get_selected_file_data_list ();
		default:
			return null;
		}
	}

	public File? get_current_vfs_folder () {
		if (current_page == Page.VIEWER) {
			return viewer.current_file.file.get_parent ();
		}
		var source = app.get_source_for_file (browser.folder_tree.current_folder.file);
		if (source is FileSourceVfs) {
			return browser.folder_tree.current_folder.file;
		}
		return null;
	}

	async void open_selected_files () {
		var local_job = new_job ("Choosing Application");
		try {
			var files = get_selected ();
			if (files.length == 0) {
				throw new IOError.FAILED (_("No file selected"));
			}
			var app_selector = new Gth.AppSelector ();
			var app_info = yield app_selector.select_app (this, files, local_job.cancellable);
			var uris = new List<string>();
			foreach (unowned var file_data in files) {
				uris.append (file_data.file.get_uri ());
			}
			var context = get_display ().get_app_launch_context ();
			context.set_timestamp (0);
			context.set_icon (app_info.get_icon ());
			app_info.launch_uris (uris, context);
		}
		catch (Error error) {
			show_error (error);
		}
		finally {
			local_job.done ();
		}
	}

	public async void open_clipboard () {
		var clipboard = get_clipboard ();
		var local_job = new_job (_("Reading Clipboard"), JobFlags.FOREGROUND);
		try {
			var texture = yield clipboard.read_texture_async (local_job.cancellable);
			var image = new Gth.Image.from_texture (texture);
			var timestamp = new GLib.DateTime.now_local ();
			var basename = _("Clipboard") + timestamp.format (" %Y-%m-%d %H-%M-%S.jpeg");
			var file = browser.folder_tree.current_folder.file.get_child (basename);
			var unsaved_file = new Gth.FileData (file);
			unsaved_file.info.set_display_name (basename);
			unsaved_file.info.set_edit_name (basename);
			unsaved_file.set_content_type ("image/jpeg");
			unsaved_file.set_is_modified (true);
			unsaved_file.info.set_attribute_boolean (PrivateAttribute.LOADED_IMAGE_FROM_CLIPBOARD, true);
			// Use this window if the viewer is ImageViewer and the file is
			// not modified.
			var window = new Gth.Window ();
			window.viewer.open_unsaved_image.begin (unsaved_file, image);
			window.present ();
		}
		catch (Error error) {
			show_error (error);
		}
		finally {
			local_job.done ();
		}
	}

	public void metadata_changed (FileData file_data) {
		viewer.metadata_changed (file_data);
		browser.metadata_changed (file_data);
	}

	async void edit_metadata (FileData file_data) {
		// TODO: load the metadata from file again?
		var local_job = new_job (_("Edit Comment"), JobFlags.FOREGROUND, "gth-note-symbolic");
		try {
			local_job.opens_dialog ();
			var dialog = new EditMetadata ();
			yield dialog.edit (this, file_data, local_job);
			yield app.metadata_writer.save (file_data, local_job.cancellable);
			local_job.dialog_closed ();
		}
		catch (Error error) {
			local_job.dialog_closed ();
			show_error (error);
		}
		finally {
			local_job.done ();
		}
	}

	public void update_scripts_actions () {
		ActionList[] menus = {
			browser.tools_popover.actions,
			viewer.tools_popover.actions,
		};
		foreach (var menu in menus) {
			menu.remove_all_actions ();

			// Actions
			var action = new ActionInfo ("win.edit-scripts", null, _("Personalize…"));
			action.category = tool_actions_category;
			menu.append_action (action);

			// Scripts
			foreach (unowned var script in app.scripts.entries) {
				if (!script.visible) {
					continue;
				}
				action = new ActionInfo ("win.exec-script",
					new Variant.string (script.id),
					script.display_name,
					new ThemedIcon ("gth-script-symbolic"));
				action.category = scripts_category;
				menu.append_action (action);
			}
		}
	}

	public void copy_files_to_clipboard (GenericList<File> files) {
		var text = new StringBuilder ();
		var uri_list = new StringBuilder ();
		foreach (unowned var file in files) {
			if (text.len > 0) {
				text.append ("\n");
			}
			if (file.get_uri_scheme () == "file") {
				text.append (file.get_path ());
			}
			else {
				text.append (file.get_uri ());
			}
			if (uri_list.len > 0) {
				uri_list.append ("\n");
			}
			uri_list.append (file.get_uri ());
		}
		unowned var clipboard = get_clipboard ();
		var text_provider = new Gdk.ContentProvider.for_bytes ("text/plain", new Bytes (text.str.data));
		var uri_provider = new Gdk.ContentProvider.for_bytes ("text/uri-list", new Bytes (uri_list.str.data));
		var provider = new Gdk.ContentProvider.union ({ text_provider, uri_provider });
		clipboard.set_content (provider);
	}

	public void cut_files_to_clipboard (GenericList<File> files) {
		var uri_list = new StringBuilder ();
		foreach (unowned var file in files) {
			if (uri_list.len > 0) {
				uri_list.append ("\n");
			}
			uri_list.append (file.get_uri ());
		}
		unowned var clipboard = get_clipboard ();
		var provider = new Gdk.ContentProvider.for_bytes ("gthumb/cut-files", new Bytes (uri_list.str.data));
		clipboard.set_content (provider);
	}

	public void remove_files (GenericList<File> files) {
		var location = browser.folder_tree.current_folder.file;
		var source = app.get_source_for_file (location);
		var local_job = new_job (_("Deleting Files"), JobFlags.FOREGROUND);
		source.remove_files.begin (this, location, files, local_job, (_obj, res) => {
			try {
				source.remove_files.end (res);
			}
			catch (Error error) {
				show_error (error);
			}
			finally {
				local_job.done ();
			}
		});
	}

	public void delete_files_from_disk (GenericList<FileData> files) {
		var local_job = new_job (_("Deleting Files"), JobFlags.FOREGROUND);
		file_manager.delete_files_from_disk.begin (files, local_job, (_obj, res) => {
			try {
				file_manager.delete_files_from_disk.end (res);
			}
			catch (Error error) {
				show_error (error);
			}
			finally {
				local_job.done ();
			}
		});
	}

	public void trash_files (GenericList<File> files) {
		var local_job = new_job (_("Deleting Files"), JobFlags.FOREGROUND);
		file_manager.trash_files.begin (files, local_job, (_obj, res) => {
			try {
				file_manager.trash_files.end (res);
			}
			catch (Error error) {
				show_error (error);
			}
			finally {
				local_job.done ();
			}
		});
	}

	public void copy_files_ask_destination (GenericList<File> files) {
		var local_job = new_job (_("Copying Files"), JobFlags.FOREGROUND);
		file_manager.copy_files_ask_destination.begin (files, local_job, (_obj, res) => {
			try {
				file_manager.copy_files_ask_destination.end (res);
			}
			catch (Error error) {
				show_error (error);
			}
			finally {
				local_job.done ();
			}
		});
	}

	public void move_files_ask_destination (GenericList<File> files) {
		var local_job = new_job (_("Moving Files"), JobFlags.FOREGROUND);
		file_manager.move_files_ask_destination.begin (files, local_job, (_obj, res) => {
			try {
				file_manager.move_files_ask_destination.end (res);
			}
			catch (Error error) {
				show_error (error);
			}
			finally {
				local_job.done ();
			}
		});
	}

	void init_actions () {
		var action = new SimpleAction ("new-window", null);
		action.activate.connect (() => {
			var new_window = new Gth.Window ();
			new_window.browser.open_location (browser.folder_tree.current_folder.file, LoadAction.OPEN, get_current_file ());
			new_window.present ();
		});
		action_group.add_action (action);

		action = new SimpleAction ("pop-page", null);
		action.activate.connect (() => {
			if (fullscreened) {
				fullscreened = false;
			}
			else if (current_page == Page.VIEWER) {
				set_page.begin (Page.BROWSER);
			}
		});
		action_group.add_action (action);

		action = new SimpleAction ("toggle-fullscreen", null);
		action.activate.connect (() => {
			if ((current_page == Page.BROWSER) && !fullscreened) {
				browser.view_fullscreen ();
			}
			else {
				fullscreened = !fullscreened;
			}
		});
		action_group.add_action (action);

		notify["fullscreened"].connect (() => {
			if (fullscreened) {
				viewer.after_fullscreen ();
			}
			else {
				viewer.after_unfullscreen ();
			}
		});

		action = new SimpleAction ("open-with", null);
		action.activate.connect ((_action, param) => {
			open_selected_files.begin ();
		});
		action_group.add_action (action);

		action = new SimpleAction ("open-clipboard", null);
		action.activate.connect (() => open_clipboard.begin ());
		action_group.add_action (action);

		action = new SimpleAction ("set-desktop-background", null);
		action.activate.connect (() => {
			var file_data = get_current_file_data ();
			if (file_data == null) {
				show_error (new IOError.FAILED (_("No file selected")));
				return;
			}
			desktop_background = new DesktopBackground (this);
			var local_job = new_job (_("Setting Background"), JobFlags.FOREGROUND);
			desktop_background.set_file.begin (file_data, local_job.cancellable, (_obj, res) => {
				try {
					desktop_background.set_file.end (res);
				}
				catch (Error error) {
					show_error (error);
				}
				finally {
					local_job.done ();
				}
			});
		});
		action_group.add_action (action);

		action = new SimpleAction ("undo-desktop-background", null);
		action.activate.connect (() => {
			if (desktop_background == null) {
				return;
			}
			var local_job = new_job (_("Setting Background"), JobFlags.FOREGROUND);
			desktop_background.undo.begin (local_job.cancellable, (_obj, res) => {
				try {
					desktop_background.undo.end (res);
				}
				catch (Error error) {
					show_error (error);
				}
				finally {
					local_job.done ();
				}
			});
		});
		action_group.add_action (action);

		action = new SimpleAction ("job-queue", null);
		action.activate.connect ((_action, param) => {
			progress_dialog.show_dialog (true);
		});
		action_group.add_action (action);

		action = new SimpleAction ("fake-job", null);
		action.activate.connect ((_action, param) => {
			add_fake_job ();
			Util.after_timeout (1500, () => add_fake_job ());
		});
		action_group.add_action (action);

		action = new SimpleAction ("cancel-all-jobs", null);
		action.activate.connect ((_action, param) => {
			cancel_jobs ();
		});
		action_group.add_action (action);

		action = new SimpleAction ("view-new-window", null);
		action.activate.connect (() => {
			var file_data = get_current_file_data ();
			if (file_data == null) {
				show_error (new IOError.FAILED (_("No file selected")));
				return;
			}
			var new_window = new Gth.Window ();
			new_window.viewer.open_file.begin (file_data);
			new_window.present ();
		});
		action_group.add_action (action);

		action = new SimpleAction ("edit-metadata", null);
		action.activate.connect (() => {
			var file_data = get_current_file_data ();
			if (file_data == null) {
				show_message (_("No file selected"));
				return;
			}
			edit_metadata.begin (file_data);
		});
		action_group.add_action (action);

		action = new SimpleAction ("copy-files-to", null);
		action.activate.connect (() => {
			var files = get_selected_files ();
			if ((files == null) || files.is_empty ()) {
				return;
			}
			copy_files_ask_destination (files);
		});
		action_group.add_action (action);

		action = new SimpleAction ("move-files-to", null);
		action.activate.connect (() => {
			var files = get_selected_files ();
			if ((files == null) || files.is_empty ()) {
				return;
			}
			move_files_ask_destination (files);
		});
		action_group.add_action (action);

		action = new SimpleAction ("remove-files", null);
		action.activate.connect (() => {
			var files = get_selected_files ();
			if ((files == null) || files.is_empty ()) {
				return;
			}
			remove_files (files);
		});
		action_group.add_action (action);

		action = new SimpleAction ("delete-files-from-disk", null);
		action.activate.connect (() => {
			var files = get_selected_file_data_list ();
			if ((files == null) || files.is_empty ()) {
				return;
			}
			delete_files_from_disk (files);
		});
		action_group.add_action (action);

		action = new SimpleAction ("trash-files", null);
		action.activate.connect (() => {
			var files = get_selected_files ();
			if ((files == null) || files.is_empty ()) {
				return;
			}
			trash_files (files);
		});
		action_group.add_action (action);

		action = new SimpleAction ("exec-script", GLib.VariantType.STRING);
		action.activate.connect ((_action, param) => {
			var files = get_selected_files ();
			if ((files == null) || files.is_empty ()) {
				return;
			}
			var id = param.get_string ();
			var script = app.scripts.get_script (id);
			script.execute.begin (this, files);
		});
		action_group.add_action (action);

		action = new SimpleAction ("edit-scripts", null);
		action.activate.connect (() => {
			var dialog = new Gth.ScriptsDialog ();
			dialog.present (this);
		});
		action_group.add_action (action);

		action = new SimpleAction ("close", null);
		action.activate.connect (() => {
			close ();
		});
		action_group.add_action (action);

		action = new SimpleAction ("main-menu", null);
		action.activate.connect (() => {
			if (current_page == Page.BROWSER) {
				browser.app_menu_button.popup ();
			}
			else if (current_page == Page.VIEWER) {
				viewer.app_menu_button.popup ();
			}
		});
		action_group.add_action (action);

		action = new SimpleAction ("scripts", null);
		action.activate.connect (() => {
			if (current_page == Page.BROWSER) {
				browser.scripts_menu_button.popup ();
			}
			else if (current_page == Page.VIEWER) {
				viewer.scripts_menu_button.popup ();
			}
		});
		action_group.add_action (action);
	}

	void add_fake_job () {
		fake_job_id++;
		var local_job = new_job ("Fake Job %u".printf (fake_job_id),
			JobFlags.FOREGROUND,
			"applications-science-symbolic");
		local_job.subtitle = Strings.new_random (50);
		local_job.cancellable.cancelled.connect (() => {
			local_job.done ();
		});
		//local_job.progress = 0.3f;
	}

	public bool on_key_pressed (Gtk.EventControllerKey controller, uint keyval, uint keycode, Gdk.ModifierType state) {
		var focused = get_focus ();
		if (focused is Gtk.Editable) {
			return false;
		}
		//stdout.printf ("> ON KEY PRESSED keyval: %u, keycode: %u, state: %b\n", keyval, keycode, state);
		//stdout.printf ("> ON KEY PRESSED current_modifiers: %b\n", current_modifiers);
		var context = ShortcutContext.NONE;
		if (current_page == Page.BROWSER) {
			context |= ShortcutContext.BROWSER;
		}
		else if ((current_page == Page.VIEWER) && (viewer.current_viewer != null)) {
			context |= viewer.current_viewer.shortcut_context;
			//stdout.printf ("> viewer.current_viewer.shortcut_context: %s\n", viewer.current_viewer.shortcut_context.to_string ());
		}
		var shortcut = app.shortcuts.find_by_key (context, keyval, state);
		if ((shortcut == null)
			|| (ShortcutContext.DOC in shortcut.context))
		{
			//stdout.printf ("> NULL\n");
			return false;
		}
		//stdout.printf ("> shortcut: '%s'\n", shortcut.detailed_action);
		activate_action_variant (shortcut.action_name, shortcut.action_parameter);
		return true;
	}

	construct {
		title = Config.APP_NAME;
		current_page = Page.NONE;

		jobs = new Gth.JobQueue ();
		jobs.size_changed.connect (() => {
			if (closing && (jobs.size () == 0)) {
				Util.next_tick (() => {
					before_closing ();
					close ();
				});
				return;
			}
			if (progress_dialog == null) {
				progress_dialog = new ProgressDialog (this);
				progress_dialog.set_queue (jobs);
			}
			browser.status.set_n_jobs (jobs.size ());
			viewer.status.set_n_jobs (jobs.size ());
		});
		file_manager = new FileManager (this);
		closing = false;
		action_group = new SimpleActionGroup ();
		insert_action_group ("win", action_group);

		close_request.connect (() => {
			if (file_not_saved ()) {
				save_and_close.begin ();
				return true;
			}
			closing = true;
			if (cancel_jobs ()) {
				return true;
			}
			before_closing ();
			return false;
		});

		active_resizer = null;

		var motion_events = new Gtk.EventControllerMotion ();
		motion_events.motion.connect ((x, y) => {
			if (active_resizer != null) {
				active_resizer.update_width (x);
			}
		});
		child.add_controller (motion_events);

		unowned var clipboard = get_clipboard ();
		clipboard_event = clipboard.changed.connect (() => {
			update_sensitivity_for_clipboard ();
		});

		init_actions ();
		browser.window = this;
		viewer.window = this;

		tool_actions_category = new ActionCategory ("", -1);
		scripts_category = new ActionCategory (_("Scripts"), 1);

		var key_events = new Gtk.EventControllerKey ();
		key_events.key_pressed.connect (on_key_pressed);
		stack.add_controller (key_events);
	}

	~Window () {
		stdout.printf ("~Window\n");
	}

	uint fake_job_id = 0;
	DesktopBackground desktop_background = null;
	ActionCategory tool_actions_category;
	ActionCategory scripts_category;
	ProgressDialog progress_dialog = null;
	[GtkChild] unowned Adw.ToastOverlay toast_overlay;
}
