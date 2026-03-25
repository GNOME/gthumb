[GtkTemplate (ui = "/app/gthumb/gthumb/ui/main-window.ui")]
public class Gth.MainWindow : Gth.Window {
	public FileManager file_manager;
	public Page current_page;
	[GtkChild] public unowned Gtk.Stack stack;
	[GtkChild] public unowned Gth.Browser browser;
	[GtkChild] public unowned Gth.Viewer viewer;
	[GtkChild] public unowned Gth.Editor editor;
	public unowned Gth.SidebarResizer active_resizer;

	public enum Page {
		NONE = 0,
		BROWSER,
		VIEWER,
		EDITOR,
	}

	ulong clipboard_event = 0;

	public MainWindow () {
		Object (application: app);
	}

	public override void add_toast (Adw.Toast toast) {
		viewer.toasts++;
		toast_overlay.dismiss_all ();
		toast_overlay.add_toast (toast);

		var local_toast = toast;
		local_toast.dismissed.connect (() => {
			viewer.toasts--;
			local_toast = null;
		});
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
		if ((current_page == Page.VIEWER)
			&& viewer.current_file.get_is_modified ()
			&& (new_page != Page.EDITOR))
		{
			try {
				yield viewer.ask_whether_to_save ();
			}
			catch (Error error) {
				show_error (error);
				return;
			}
		}
		if (current_page == Page.EDITOR) {
			editor.before_close_page ();
		}
		var previuos_page = current_page;
		current_page = new_page;
		switch (current_page) {
		case Page.BROWSER:
			var from_viewer = !browser.never_loaded && (previuos_page == Page.VIEWER);
			if (from_viewer) {
				//if (viewer.main_view.show_sidebar) {
				//	browser.content_view.max_sidebar_width = viewer.main_view.max_sidebar_width;
				//}
				viewer.before_close_page ();
				browser.restore_window_size ();
			}
			stack.set_visible_child (browser);
			if (from_viewer) {
				if ((viewer.current_file != null)
					&& (browser.property_sidebar.current_file != null)
					&& viewer.current_file.file.equal (browser.property_sidebar.current_file.file))
				{
					browser.property_sidebar.update_view ();
				}
				else if (viewer.position >= 0) {
					browser.file_grid.select_position (viewer.position, SelectFile.SCROLL_TO_FILE);
				}
				else if (viewer.current_file != null) {
					browser.file_grid.select_file (viewer.current_file.file, SelectFile.SCROLL_TO_FILE);
				}
			}
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
			browser.after_open_page ();
			break;

		case Page.VIEWER:
			if (previuos_page == Page.BROWSER) {
				browser.before_close_page ();
				browser.save_window_size ();
				if (browser.content_view.show_sidebar) {
					viewer.main_view.max_sidebar_width = browser.content_view.max_sidebar_width;
				}
				viewer.status.set_list_info (browser.total_files);
			}
			else if (previuos_page == Page.EDITOR) {
				viewer.main_view.max_sidebar_width = editor.main_view.max_sidebar_width;
			}
			stack.set_visible_child (viewer);
			viewer.after_open_page ();
			if (previuos_page == Page.EDITOR) {
				var image_viewer = viewer.current_viewer as ImageViewer;
				if (image_viewer != null) {
					image_viewer.after_closing_editor ();
				}
			}
			break;

		case Page.EDITOR:
			if (previuos_page == Page.VIEWER) {
				if (viewer.main_view.show_sidebar) {
					editor.main_view.max_sidebar_width = viewer.main_view.max_sidebar_width;
				}
			}
			stack.set_visible_child (editor);
			break;

		case Page.NONE:
			break;
		}
	}

	public void on_setting_change (string key) {
		switch (key) {
		case PREF_BROWSER_THUMBNAIL_SIZE:
			var size = app.settings.get_int (PREF_BROWSER_THUMBNAIL_SIZE);
			browser.file_grid.thumbnail_size = size;
			viewer.file_grid.thumbnail_size = size;
			break;
		case PREF_BROWSER_THUMBNAIL_CAPTION:
			browser.reload ();
			break;
		case PREF_BROWSER_OPEN_IN_FULLSCREEN:
			browser.open_in_fullscreen = app.settings.get_boolean (PREF_BROWSER_OPEN_IN_FULLSCREEN);
			break;
		}
	}

	public bool can_open_clipboard = false;
	public bool can_paste_from_clipboad = false;

	void update_sensitivity_for_clipboard () {
		unowned var clipboard = get_clipboard ();
		unowned var formats = clipboard.get_formats ();
		can_open_clipboard = formats.match (Gdk.ContentFormats.parse ("image/png"));
		can_paste_from_clipboad = formats.match (new Gdk.ContentFormats ({ "gthumb/cut-files", "text/uri-list" }));
		Util.enable_action (action_group, "open-clipboard", can_open_clipboard);
		browser.update_clipboard_actions ();
	}

	public void save_preferences () {
		if (get_realized ()) {
			browser.save_preferences (current_page == Page.BROWSER);
			viewer.save_preferences (current_page == Page.VIEWER);
		}
	}

	public FileData? get_current_file_data () {
		switch (current_page) {
		case Page.BROWSER:
			return browser.file_grid.get_selected_file_data ();
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
			return browser.file_grid.get_selected_files ();
		case Page.VIEWER:
			return viewer.get_selected_file ();
		default:
			return null;
		}
	}

	public GenericList<FileData>? get_selected_file_data_list () {
		switch (current_page) {
		case Page.BROWSER:
			return browser.file_grid.get_selected_file_data_list ();
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
		if (browser.folder_tree.current_source is FileSourceVfs) {
			return browser.folder_tree.current_folder.file;
		}
		return null;
	}

	public File get_current_vfs_folder_or_default () {
		var folder = get_current_vfs_folder ();
		if (folder == null) {
			var path = Environment.get_user_special_dir (UserDirectory.PICTURES);
			if (path != null) {
				folder = File.new_for_path (path);
			}
			else {
				folder = File.new_for_path (Environment.get_home_dir ());
			}
		}
		return folder;
	}

	async void open_selected_files () {
		var local_job = new_job ("Choosing Application");
		try {
			var files = get_selected_file_data_list ();
			if (files.is_empty ()) {
				throw new IOError.FAILED (_("No files selected"));
			}
			var app_selector = new Gth.AppSelector ();
			var app_info = yield app_selector.select_app (this, files, local_job);
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
			var clipboard_type = app.settings.get_string (PREF_BROWSER_CLIPBOARD_TYPE);
			var type_preferences = app.get_saver_preferences (clipboard_type);
			if (type_preferences == null) {
				throw new IOError.FAILED (_("Invalid filename"));
			}
			var texture = yield clipboard.read_texture_async (local_job.cancellable);
			var image = new Gth.Image.from_texture (texture);

			var timestamp = new GLib.DateTime.now_local ();
			var basename = _("Clipboard") + timestamp.format (" %Y-%m-%d %H-%M-%S.") + type_preferences.get_default_extension ();
			var file = browser.folder_tree.current_folder.file.get_child (basename);
			var unsaved_file = new Gth.FileData (file);
			unsaved_file.info.set_display_name (basename);
			unsaved_file.info.set_edit_name (basename);
			unsaved_file.set_content_type (type_preferences.get_content_type ());
			unsaved_file.set_is_modified (true);
			Lib.set_frame_size (unsaved_file.info, (int) image.width, (int) image.height);
			unsaved_file.info.set_attribute_boolean (PrivateAttribute.ASK_FILENAME_WHEN_SAVING, true);

			yield monitor_profile.apply_color_profile (image, unsaved_file.info, local_job.cancellable, false);

			// Use this window if in browser mode or in viewer mode and the image was not modified
			MainWindow window = (current_page == Page.BROWSER) ? this : null;
			if (window == null) {
				if ((current_page == Page.VIEWER) && (viewer.current_viewer is ImageViewer)) {
					if ((viewer.current_file == null) || !viewer.current_file.get_is_modified ()) {
						window = this;
					}
				}
			}
			if (window == null) {
				window = new Gth.MainWindow ();
			}
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

	public void set_desktop_background () {
		var file_data = get_current_file_data ();
		if (file_data == null) {
			show_error (new IOError.FAILED (_("No files selected")));
			return;
		}
		var desktop_background = new DesktopBackground ();
		var local_job = new_job (_("Set as Desktop Background"), JobFlags.FOREGROUND);
		desktop_background.set_file.begin (this, file_data, local_job.cancellable, (_obj, res) => {
			try {
				desktop_background.set_file.end (res);
			}
			catch (Error error) {
				show_error (error);
			}
			finally {
				local_job.done ();
				desktop_background = null;
			}
		});
	}

	async void edit_metadata (GenericList<FileData> files) {
		var local_job = new_job (_("Edit Comment"), JobFlags.FOREGROUND, "gth-note-symbolic");
		try {
			var total_files = files.length ();
			var current_file = 0;

			// Read all the metadata attributes.
			foreach (var file_data in files) {
				local_job.subtitle = file_data.get_display_name ();
				local_job.progress = Util.calc_progress (current_file++, total_files);
				var result = yield FileManager.read_metadata (file_data.file, "*", local_job.cancellable);
				file_data.update_info (result.info);
			}

			// Edit
			var dialog = new EditMetadata ();
			yield dialog.edit (this, files, local_job);

			// Save
			current_file = 0;
			foreach (var file_data in files) {
				local_job.subtitle = file_data.get_display_name ();
				local_job.progress = Util.calc_progress (current_file++, total_files);
				yield app.metadata_writer.save (file_data, local_job.cancellable);
			}
		}
		catch (Error error) {
			show_error (error);
		}
		finally {
			local_job.done ();
		}
	}

	async void rename_files (GenericList<File> files) {
		var local_job = new_job (_("Rename"), JobFlags.FOREGROUND);
		try {
			var dialog = new Gth.RenameFiles ();
			var renamed_files = yield dialog.rename (this, files, local_job);
			yield file_manager.rename_files (renamed_files, local_job);
		}
		catch (Error error) {
			show_error (error);
		}
		finally {
			local_job.done ();
		}
	}

	async void resize_images (GenericList<File> files) {
		var local_job = new_job (_("Resize Images"), JobFlags.FOREGROUND);
		try {
			var dialog = new Gth.ResizeImages ();
			var operation = yield dialog.resize (this, local_job);
			yield exec_file_operation_with_job (operation, files, local_job);
		}
		catch (Error error) {
			show_error (error);
		}
		finally {
			local_job.done ();
		}
	}

	async void convert_format (GenericList<File> files) {
		var local_job = new_job (_("Convert Format"), JobFlags.FOREGROUND);
		try {
			var dialog = new Gth.ConvertFormat ();
			var operation = yield dialog.convert (this, local_job);
			yield exec_file_operation_with_job (operation, files, local_job);
		}
		catch (Error error) {
			show_error (error);
		}
		finally {
			local_job.done ();
		}
	}

	async void print_files (GenericList<File> files) {
		var local_job = new_job (_("Print"), JobFlags.FOREGROUND);
		try {
			var dialog = new Gth.PrintImages ();
			if ((current_page == Page.VIEWER) && (viewer.current_viewer is ImageViewer)) {
				File file = null;
				if (viewer.current_file != null) {
					file = viewer.current_file.file;
				}
				var image_viewer = viewer.current_viewer as ImageViewer;
				yield dialog.print_image (this, image_viewer.image_view.image, file, local_job);
			}
			else {
				yield dialog.print_files (this, files, local_job);
			}
		}
		catch (Error error) {
			show_error (error);
		}
		finally {
			local_job.done ();
		}
	}

	async void import_files () {
		var local_job = new_job (_("Import Files"), JobFlags.FOREGROUND);
		try {
			var selector = new Gth.SelectDevice ();
			var source = yield selector.select (this, local_job);
			var importer = new Gth.ImportFiles ();
			yield importer.import (this, source, local_job);
			browser.open_location (importer.destination);
		}
		catch (Error error) {
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
			action.category = app.tools.actions;
			menu.append_action (action);

			// Tools

			foreach (var tool in app.tools.entries) {
				if (!tool.visible) {
					continue;
				}
				action = new ActionInfo (tool.action_name,
					null,
					tool.display_name,
					(tool.icon_name != null) ? new ThemedIcon (tool.icon_name) : null);
				action.category = tool.get_action_category ();
				menu.append_action (action);
			}

			// Scripts

			foreach (unowned var script in app.scripts.entries) {
				if (!script.visible) {
					continue;
				}
				action = new ActionInfo ("win.exec-script",
					new Variant.string (script.id),
					script.display_name,
					null /*new ThemedIcon ("gth-script-symbolic")*/);
				action.category = app.tools.scripts;
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
		var text_provider = new Gdk.ContentProvider.for_value (text.str);
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

	public override void init_actions () {
		base.init_actions ();

		var action = new SimpleAction ("new-window", null);
		action.activate.connect (() => {
			var new_window = new Gth.MainWindow ();
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
			else if (current_page == Page.EDITOR) {
				set_page.begin (Page.VIEWER);
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
				editor.after_fullscreen ();
			}
			else {
				viewer.after_unfullscreen ();
				editor.after_unfullscreen ();
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
		action.activate.connect (() => set_desktop_background ());
		action_group.add_action (action);

		action = new SimpleAction ("view-new-window", null);
		action.activate.connect (() => {
			var file_data = get_current_file_data ();
			if (file_data == null) {
				show_error (new IOError.FAILED (_("No files selected")));
				return;
			}
			var new_window = new Gth.MainWindow ();
			new_window.viewer.view_file (file_data);
			new_window.present ();
		});
		action_group.add_action (action);

		action = new SimpleAction ("edit-metadata", null);
		action.activate.connect (() => {
			var files = get_selected_file_data_list ();
			if (files.is_empty ()) {
				show_message (_("No files selected"));
				return;
			}
			edit_metadata.begin (files);
		});
		action_group.add_action (action);

		action = new SimpleAction ("copy-files-to", null);
		action.activate.connect (() => {
			var files = get_selected_files ();
			if (files.is_empty ()) {
				return;
			}
			copy_files_ask_destination (files);
		});
		action_group.add_action (action);

		action = new SimpleAction ("move-files-to", null);
		action.activate.connect (() => {
			var files = get_selected_files ();
			if (files.is_empty ()) {
				return;
			}
			move_files_ask_destination (files);
		});
		action_group.add_action (action);

		action = new SimpleAction ("remove-files", null);
		action.activate.connect (() => {
			var files = get_selected_files ();
			if (files.is_empty ()) {
				return;
			}
			remove_files (files);
		});
		action_group.add_action (action);

		action = new SimpleAction ("delete-files-from-disk", null);
		action.activate.connect (() => {
			var files = get_selected_file_data_list ();
			if (files.is_empty ()) {
				return;
			}
			delete_files_from_disk (files);
		});
		action_group.add_action (action);

		action = new SimpleAction ("trash-files", null);
		action.activate.connect (() => {
			var files = get_selected_files ();
			if (files.is_empty ()) {
				return;
			}
			trash_files (files);
		});
		action_group.add_action (action);

		action = new SimpleAction ("exec-script", GLib.VariantType.STRING);
		action.activate.connect ((_action, param) => {
			var files = get_selected_files ();
			if (files.is_empty ()) {
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

		action = new SimpleAction ("rotate-left", null);
		action.activate.connect ((_action, param) => {
			var files = get_selected_files ();
			if (files.is_empty ()) {
				show_message (_("No files selected"));
				return;
			}
			var operation = new ImageRotation (Transform.ROTATE_270);
			exec_file_operation.begin (_("Rotate Left"), operation, files);
		});
		action_group.add_action (action);

		action = new SimpleAction ("rotate-right", null);
		action.activate.connect ((_action, param) => {
			var files = get_selected_files ();
			if (files.is_empty ()) {
				show_message (_("No files selected"));
				return;
			}
			var operation = new ImageRotation (Transform.ROTATE_90);
			exec_file_operation.begin (_("Rotate Right"), operation, files);
		});
		action_group.add_action (action);

		action = new SimpleAction ("apply-orientation", null);
		action.activate.connect ((_action, param) => {
			var files = get_selected_files ();
			if (files.is_empty ()) {
				show_message (_("No files selected"));
				return;
			}
			var operation = new ImageRotation (Transform.NONE, TransformFlags.CHANGE_IMAGE | TransformFlags.ALWAYS_SAVE);
			exec_file_operation.begin (_("Rotate Physically"), operation, files);
		});
		action_group.add_action (action);

		action = new SimpleAction ("reset-orientation", null);
		action.activate.connect ((_action, param) => {
			var files = get_selected_files ();
			if (files.is_empty ()) {
				show_message (_("No files selected"));
				return;
			}
			var operation = new ImageRotation (Transform.NONE, TransformFlags.RESET);
			exec_file_operation.begin (_("Clear EXIF Rotation"), operation, files);
		});
		action_group.add_action (action);

		action = new SimpleAction ("clear-metadata", null);
		action.activate.connect ((_action, param) => {
			var files = get_selected_files ();
			if (files.is_empty ()) {
				show_message (_("No files selected"));
				return;
			}
			var operation = new ClearMetadata ();
			exec_file_operation.begin (_("Clear Metadata"), operation, files);
		});
		action_group.add_action (action);

		action = new SimpleAction ("update-thumbnail", null);
		action.activate.connect ((_action, param) => {
			var files = get_selected_files ();
			if (files.is_empty ()) {
				show_message (_("No files selected"));
				return;
			}
			var operation = new UpdateThumbnail ();
			exec_file_operation.begin (_("Update Thumbnail"), operation, files);
		});
		action_group.add_action (action);

		action = new SimpleAction ("open-with-gimp", null);
		action.activate.connect ((_action, param) => {
			var files = get_selected_files ();
			if (files.is_empty ()) {
				show_message (_("No files selected"));
				return;
			}
			open_with_command ({ "gimp", "org.gimp.GIMP" }, "Gimp", files);
		});
		action_group.add_action (action);

		action = new SimpleAction ("open-with-inkscape", null);
		action.activate.connect ((_action, param) => {
			var files = get_selected_files ();
			if (files.is_empty ()) {
				show_message (_("No files selected"));
				return;
			}
			open_with_command ({ "inkscape", "org.inkscape.Inkscape" }, "Inkscape", files);
		});
		action_group.add_action (action);

		action = new SimpleAction ("copy-files", null);
		action.activate.connect (() => {
			var files = get_selected_files ();
			copy_files_to_clipboard (files);
		});
		action_group.add_action (action);

		action = new SimpleAction ("view-script-output", null);
		action.activate.connect (() => {
			if (script_output != null) {
				var job = new_job ("Script Output");
				var viewer = new TextViewer ();
				viewer.view_buffer.begin (this, script_output, job, (_obj, res) => {
					try {
						viewer.view_buffer.end (res);
					}
					catch (Error error) {
						show_error (error);
					}
					finally {
						job.done ();
					}
				});
			}
		});
		action_group.add_action (action);

		action = new SimpleAction ("rename-files", null);
		action.activate.connect (() => {
			var files = get_selected_files ();
			if (files.is_empty ()) {
				show_message (_("No files selected"));
				return;
			}
			rename_files.begin (files);
		});
		action_group.add_action (action);

		action = new SimpleAction ("resize-images", null);
		action.activate.connect ((_action, param) => {
			var files = get_selected_files ();
			if (files.is_empty ()) {
				show_message (_("No files selected"));
				return;
			}
			resize_images.begin (files);
		});
		action_group.add_action (action);

		action = new SimpleAction ("convert-format", null);
		action.activate.connect ((_action, param) => {
			var files = get_selected_files ();
			if (files.is_empty ()) {
				show_message (_("No files selected"));
				return;
			}
			convert_format.begin (files);
		});
		action_group.add_action (action);

		action = new SimpleAction ("print", null);
		action.activate.connect ((_action, param) => {
			var files = get_selected_files ();
			if (files.is_empty ()) {
				show_message (_("No files selected"));
				return;
			}
			print_files.begin (files);
		});
		action_group.add_action (action);

		action = new SimpleAction ("import-files", null);
		action.activate.connect (() => import_files.begin ());
		action_group.add_action (action);

		action = new SimpleAction ("slideshow", null);
		action.activate.connect ((_action, param) => {
			var files = get_selected_file_data_list ();
			if (files.length () <= 1) {
				files = browser.file_grid.get_file_data_list ();
			}
			if (files.is_empty ()) {
				show_message (_("No images selected"));
				return;
			}
			var slideshow = new Gth.Slideshow ();
			slideshow.files = files;
			if (slideshow.files.is_empty ()) {
				show_message (_("No images selected"));
				return;
			}
			slideshow.fullscreened = true;
			slideshow.present ();
		});
		action_group.add_action (action);
	}

	async void exec_file_operation (string name, FileOperation operation, GenericList<File> files, Gth.Job? external_job = null) {
		var local_job = new_job (name, JobFlags.FOREGROUND);
		try {
			yield exec_file_operation_with_job (operation, files, local_job);
		}
		catch (Error error) {
			show_error (error);
		}
		local_job.done ();
	}

	async void exec_file_operation_with_job (FileOperation operation, GenericList<File> files, Gth.Job? external_job = null) throws Error {
		var overwrite_response = OverwriteResponse.NONE;
		foreach (var file in files) {
			operation.overwrite_response = overwrite_response;
			operation.single_file = (files.length () == 1);
			yield operation.execute (this, file, external_job);
			overwrite_response = operation.overwrite_response;
		}
	}

	void open_with_command (string[] try_commands, string display_name, GenericList<File>? files) {
		var launched = false;
		foreach (unowned var command in try_commands) {
			try {
				var context = display.get_app_launch_context ();
				context.set_timestamp (0);
				var app_info = AppInfo.create_from_commandline ("%s %%U".printf (command), display_name, AppInfoCreateFlags.SUPPORTS_URIS);
				app_info.launch (files.to_glist (), context);
				launched = true;
				break;
			}
			catch (Error error) {
			}
		}
		if (!launched) {
			var error = new IOError.FAILED (_("Application '%s' not found").printf (display_name));
			show_error (error);
		}
	}

	public bool on_key_pressed (Gtk.EventControllerKey controller, uint keyval, uint keycode, Gdk.ModifierType state) {
		var focused = get_focus ();
		if (focused is Gtk.Editable) {
			return false;
		}
		//stdout.printf ("> ON KEY PRESSED keyval: %u, keycode: %u, state: %b\n", keyval, keycode, state);
		var context = ShortcutContext.NONE;
		if (current_page == Page.BROWSER) {
			context |= ShortcutContext.BROWSER;
		}
		else if ((current_page == Page.VIEWER) && (viewer.current_viewer != null)) {
			context |= viewer.current_viewer.shortcut_context;
			// stdout.printf ("> viewer.current_viewer.shortcut_context: %s\n", viewer.current_viewer.shortcut_context.to_string ());
		}
		else if (current_page == Page.EDITOR) {
			context |= ShortcutContext.EDITOR | editor.current_editor.shortcut_context;
			// stdout.printf ("> editor.current_editor.shortcut_context: %s\n", editor.current_editor.shortcut_context.to_string ());
		}

		var shortcut = app.shortcuts.find_by_key (context, keyval, state);
		if ((shortcut == null) || (ShortcutContext.DOC in shortcut.context)) {
			// stdout.printf ("> NULL\n");
			return false;
		}
		// stdout.printf ("> shortcut: '%s'\n", shortcut.detailed_action);
		activate_action_variant (shortcut.action_name, shortcut.action_parameter);
		return true;
	}

	public async void open (File file, ViewFlags flags = ViewFlags.DEFAULT) {
		var local_job = new_job ("Open");
		try {
			var source = app.get_source_for_file (file);
			var type_info = source.get_display_info (file);
			switch (type_info.get_file_type ()) {
			case FileType.DIRECTORY:
				yield browser.open_location_async (file, LoadAction.OPEN, local_job);
				break;
			case FileType.REGULAR:
				if (ViewFlags.FULLSCREEN in flags) {
					yield set_page (Page.VIEWER);
					fullscreened = true;
				}
				yield viewer.open_file_async (file, ViewFlags.FOCUS | ViewFlags.LOAD_PARENT_DIRECTORY, local_job);
				break;
			default:
				throw new IOError.FAILED (_("File type not supported"));
			}
		}
		catch (Error error) {
			show_error (error);
			if (browser.never_loaded) {
				browser.open_home ();
			}
		}
		finally {
			local_job.done ();
		}
	}

	public async void restore_state (WindowState state) {
		var local_job = new_job ("Restore State");
		if (state.page == Page.BROWSER) {
			try {
				yield browser.open_location_async (state.location, LoadAction.OPEN, local_job);
				browser.file_grid.select_files (state.selected);
			}
			catch (Error error) {
				show_error (error);
			}
		}
		else if (state.page == Page.VIEWER) {
			try {
				yield viewer.open_file_async (state.file, ViewFlags.FOCUS, local_job);
			}
			catch (Error error) {
				show_error (error);
			}
		}
		local_job.done ();
	}

	public void update_selection_status (uint number) {
		var selection = app.selections.get_selection (number);
		var size = selection.files.length;
		browser.selections_status.set_selection_size (number, size);
	}

	public override bool on_close () {
		if (file_not_saved ()) {
			save_and_close.begin ();
			return true;
		}
		if (base.on_close ()) {
			return true;
		}
		before_closing ();
		return false;
	}

	public override void before_closing () {
		browser.file_grid.stop_thumbnailer ();
		viewer.file_grid.stop_thumbnailer ();
		var save_preferences = app.one_window () && get_realized ();
		if (save_preferences) {
			browser.save_preferences (current_page == Page.BROWSER);
			viewer.save_preferences (current_page == Page.VIEWER);
			app.selections.save_to_file ();
		}
		viewer.release_resources ();
		browser.release_resources ();
		base.before_closing ();
	}

	public override void on_jobs_changed () {
		browser.status.set_n_jobs (jobs.size ());
		viewer.status.set_n_jobs (jobs.size ());
		base.on_jobs_changed ();
	}

	construct {
		title = Config.APP_NAME;
		current_page = Page.NONE;

		file_manager = new FileManager (this);
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

		browser.window = this;
		viewer.window = this;
		editor.window = this;

		var key_events = new Gtk.EventControllerKey ();
		key_events.key_pressed.connect (on_key_pressed);
		stack.add_controller (key_events);

		update_selection_status (1);
		update_selection_status (2);
		update_selection_status (3);
	}

	~MainWindow () {
		stdout.printf ("~MainWindow\n");
	}

	public Bytes script_output = null;
	[GtkChild] unowned Adw.ToastOverlay toast_overlay;
}
