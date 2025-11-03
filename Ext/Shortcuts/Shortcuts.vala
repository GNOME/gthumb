public class Gth.Shortcuts {
	public GenericList<Shortcut> entries;
	public HashTable<string, Shortcut> by_action;

	public signal void changed () {
		save_to_file ();
		//app.monitor.shortcuts_changed ();
	}

	public Shortcuts () {
		entries = new GenericList<Shortcut> ();
		by_action = new HashTable<string, Shortcut> (str_hash, str_equal);
	}

	public bool load_from_file () {
		if (loaded) {
			return false;
		}
		var local_job = app.jobs.new_job ("Loading Shortcuts");
		try {
			var file = UserDir.get_config_file (FileIntent.READ, SHORTCUTS_FILE);
			var contents = Files.load_contents (file, local_job.cancellable);
			var doc = new Dom.Document ();
			doc.load_xml (contents);
			if ((doc.first_child != null) && (doc.first_child.tag_name == "shortcuts")) {
				foreach (unowned var node in doc.first_child) {
					if (node.tag_name == "shortcut") {
						var detailed_action = node.get_attribute ("action");
						if (detailed_action != null) {
							var shortcut = by_action.get (detailed_action);
							if (shortcut != null) {
								shortcut.set_accelerator (node.get_attribute ("accelerator"));
							}
						}
					}
				}
			}
		}
		catch (Error error) {
			// Ignored
			// stdout.printf (">> ERROR: %s\n", error.message);
		}
		local_job.done ();
		loaded = true;
		//app.monitor.shortcuts_changed ();
		return true;
	}

	void save_to_file () {
		try {
			var doc = new Dom.Document ();
			var root = new Dom.Element.with_attributes ("shortcuts", "version", SHORTCUT_FORMAT);
			doc.append_child (root);
			foreach (unowned var shortcut in entries) {
				if (!shortcut.get_is_customizable () || !shortcut.get_is_modified ()) {
					continue;
				}
				root.append_child (shortcut.create_element (doc));
			}
			var file = UserDir.get_config_file (FileIntent.WRITE, SHORTCUTS_FILE);
			Files.save_content (file, doc.to_xml ());
		}
		catch (Error error) {
		}
	}

	public Shortcut? find_by_key (ShortcutContext context, uint keyval, Gdk.ModifierType modifiers, Shortcut? other_shortcut = null) {
		if (keyval == 0) {
			return null;
		}

		// Remove flags not related to the window context.
		context = context & SHORTCUT_CONTEXT_ANY;
		keyval = Gdk.keyval_to_lower (keyval);

		foreach (unowned var shortcut in entries) {
			if (((shortcut.context & context) != 0)
				&& (shortcut.keyval == keyval)
				&& (shortcut.modifiers == modifiers)
				&& (shortcut != other_shortcut))
			{
				return shortcut;
			}
		}

		return null;
	}

	public Shortcut? find_by_action (string detailed_action) {
		return by_action.get (detailed_action);
	}

	public void remove_action (string action) {
		var iter = entries.iterator ();
		while (iter.next ()) {
			var shortcut = iter.get ();
			if (shortcut.action_name == action) {
				iter.remove ();
			}
		}
	}

	public bool remove_detailed_action (string detailed_action) {
		var iter = entries.iterator ();
		while (iter.next ()) {
			var shortcut = iter.get ();
			if (shortcut.detailed_action == detailed_action) {
				iter.remove ();
				return true;
			}
		}
		return false;
	}

	public void add (Shortcut shortcut) {
		entries.model.append (shortcut);
		by_action.set (shortcut.detailed_action, shortcut);
	}

	void register (string action, string? description, ShortcutContext context, ShortcutCategory category, string? default_value = null) {
		var shortcut = new Shortcut.from_detailed_action (action);
		shortcut.description = _(description);
		shortcut.context = context;
		shortcut.category = category;
		if (default_value != null) {
			shortcut.set_accelerator (default_value);
			shortcut.default_accelerator = shortcut.accelerator;
		}
		add (shortcut);
	}

	public void register_all () {
		register ("win.new-window", N_("New Window"), SHORTCUT_CONTEXT_BROWSER_VIEWER, ShortcutCategory.GENERAL, "<Primary>n");
		register ("win.close", N_("Close Window"), SHORTCUT_CONTEXT_BROWSER_VIEWER, ShortcutCategory.GENERAL, "<Primary>w");
		register ("app.quit", N_("Quit"), SHORTCUT_CONTEXT_BROWSER_VIEWER, ShortcutCategory.GENERAL, "<Primary>q");
		register ("app.preferences", N_("Preferences"), SHORTCUT_CONTEXT_BROWSER_VIEWER, ShortcutCategory.GENERAL, "<Primary>p");
		register ("app.shortcuts", N_("Keyboard Shortcuts"), SHORTCUT_CONTEXT_BROWSER_VIEWER, ShortcutCategory.GENERAL, "<Primary>F1");
		register ("win.main-menu", N_("Main Menu"), SHORTCUT_CONTEXT_BROWSER_VIEWER, ShortcutCategory.GENERAL, "F10");

		register ("win.ask-location", N_("Open Location"), ShortcutContext.BROWSER, ShortcutCategory.FILE_MANAGER, "<Primary>o");
		register ("win.load-home", N_("Home"), ShortcutContext.BROWSER, ShortcutCategory.FILE_MANAGER, "h");
		register ("win.load-parent", N_("Parent Folder"), ShortcutContext.BROWSER, ShortcutCategory.FILE_MANAGER, "<Alt>Up");
		register ("win.reload", N_("Reload"), ShortcutContext.BROWSER, ShortcutCategory.FILE_MANAGER, "<Primary>r");
		register ("win.load-previous", N_("Previous Visited"), ShortcutContext.BROWSER, ShortcutCategory.FILE_MANAGER, "<Alt>Left");
		register ("win.load-next", N_("Next Visited"), ShortcutContext.BROWSER, ShortcutCategory.FILE_MANAGER, "<Alt>Right");
		register ("win.load-next-folder", N_("Next Folder"), ShortcutContext.BROWSER, ShortcutCategory.FILE_MANAGER, "<Primary><Alt>n");
		register ("win.load-prev-folder", N_("Previous Folder"), ShortcutContext.BROWSER, ShortcutCategory.FILE_MANAGER, "<Primary><Alt>p");
		register ("win.open-file-manager-here", N_("Open with File Manager"), ShortcutContext.BROWSER, ShortcutCategory.FILE_MANAGER, "<Primary>m");
		register ("win.open-terminal-here", N_("Open in Terminal"), ShortcutContext.BROWSER, ShortcutCategory.FILE_MANAGER, "<Primary>t");
		register ("win.hidden-files", N_("Hidden Files"), ShortcutContext.BROWSER, ShortcutCategory.FILE_MANAGER, "<Primary>h");
		register ("win.sort-files", N_("Sort Files"), ShortcutContext.BROWSER, ShortcutCategory.FILE_MANAGER, "<Primary>s");
		register ("win.sort-folders", N_("Sort Folders"), ShortcutContext.BROWSER, ShortcutCategory.FILE_MANAGER, "<Primary><Shift>s");
		register ("win.browser-sidebar", N_("Sidebar"), ShortcutContext.BROWSER, ShortcutCategory.FILE_MANAGER, "F9");
		register ("win.browser-properties-pinned", N_("File Properties"), ShortcutContext.BROWSER, ShortcutCategory.FILE_MANAGER, "<Primary>i");
		register ("win.browser-properties-collapsed", N_("View Properties"), ShortcutContext.BROWSER, ShortcutCategory.FILE_MANAGER, "i");
		register ("win.edit-file", N_("Edit Image"), ShortcutContext.BROWSER, ShortcutCategory.FILE_MANAGER, "e");
		register ("win.show-bookmarks", N_("Bookmarks"), ShortcutContext.BROWSER, ShortcutCategory.FILE_MANAGER, "<Primary><Shift>b");
		register ("win.show-history", N_("History"), ShortcutContext.BROWSER, ShortcutCategory.FILE_MANAGER, "<Primary><Shift>h");
		register ("win.show-catalogs", N_("Catalogs"), ShortcutContext.BROWSER, ShortcutCategory.FILE_MANAGER, "<Primary><Shift>c");
		register ("win.show-folders", N_("Folders"), ShortcutContext.BROWSER, ShortcutCategory.FILE_MANAGER, "<Primary><Shift>f");
		register ("win.show-filters", N_("Filters"), ShortcutContext.BROWSER, ShortcutCategory.FILE_MANAGER, "<Primary><Shift>m");
		register ("win.show-path", N_("Location"), ShortcutContext.BROWSER, ShortcutCategory.FILE_MANAGER, "<Primary><Shift>p");
		register ("win.focus-thumbnail-list", N_("Browser"), ShortcutContext.BROWSER, ShortcutCategory.FILE_MANAGER, "<Primary>b");

		register ("win.cut-files", N_("Cut"), ShortcutContext.BROWSER | ShortcutContext.FIXED, ShortcutCategory.FILE_MANAGER, "<Primary>x");
		register ("win.copy-files", N_("Copy"), ShortcutContext.BROWSER | ShortcutContext.FIXED, ShortcutCategory.FILE_MANAGER, "<Primary>c");
		register ("win.paste-files", N_("Paste"), ShortcutContext.BROWSER | ShortcutContext.FIXED, ShortcutCategory.FILE_MANAGER, "<Primary>v");

		register ("win.find", N_("Find Files"), SHORTCUT_CONTEXT_BROWSER_VIEWER, ShortcutCategory.FILE_MANAGER, "<Primary>f");
		register ("rename", N_("Rename"), SHORTCUT_CONTEXT_BROWSER_VIEWER, ShortcutCategory.FILE_MANAGER, "F2");
		register ("win.copy-files-to", N_("Copy Files"), SHORTCUT_CONTEXT_BROWSER_VIEWER, ShortcutCategory.FILE_MANAGER);
		register ("win.move-files-to", N_("Move Files"), SHORTCUT_CONTEXT_BROWSER_VIEWER, ShortcutCategory.FILE_MANAGER);
		register ("win.duplicate-files", N_("Duplicate"), SHORTCUT_CONTEXT_BROWSER_VIEWER, ShortcutCategory.FILE_MANAGER, "<Primary>d");
		register ("win.remove-files", N_("Delete"), SHORTCUT_CONTEXT_BROWSER_VIEWER, ShortcutCategory.FILE_MANAGER, "Delete");
		register ("win.trash-files", N_("Move to Trash"), SHORTCUT_CONTEXT_BROWSER_VIEWER, ShortcutCategory.FILE_MANAGER);
		register ("win.delete-files-from-disk", N_("Delete Permanently"), SHORTCUT_CONTEXT_BROWSER_VIEWER, ShortcutCategory.FILE_MANAGER, "<Shift>Delete");
		register ("win.edit-metadata", N_("Edit Comment"), SHORTCUT_CONTEXT_BROWSER_VIEWER, ShortcutCategory.FILE_MANAGER, "c");
		register ("print", N_("Print"), SHORTCUT_CONTEXT_BROWSER_VIEWER, ShortcutCategory.FILE_MANAGER, "<Primary>p");
		register ("win.open-clipboard", N_("Open Clipboard"), SHORTCUT_CONTEXT_BROWSER_VIEWER, ShortcutCategory.FILE_MANAGER, "<Primary><Shift>v");
		register ("win.scripts", N_("Scripts"), SHORTCUT_CONTEXT_BROWSER_VIEWER, ShortcutCategory.FILE_MANAGER, "<Primary><Shift>x");
		register ("win.open-with-gimp", N_("Open with Gimp"), SHORTCUT_CONTEXT_BROWSER_VIEWER, ShortcutCategory.FILE_MANAGER, "g");
		register ("win.open-with-inkscape", N_("Open with Inkscape"), SHORTCUT_CONTEXT_BROWSER_VIEWER, ShortcutCategory.FILE_MANAGER, "k");
		register ("win.set-desktop-background", N_("Set as Desktop Background"), SHORTCUT_CONTEXT_BROWSER_VIEWER, ShortcutCategory.FILE_MANAGER);

		register ("win.toggle-fullscreen", N_("Fullscreen"), SHORTCUT_CONTEXT_BROWSER_VIEWER, ShortcutCategory.VIEWER, "f");

		foreach (var tool in app.tools.entries) {
			register (tool.action_name, tool.display_name,
				SHORTCUT_CONTEXT_BROWSER_VIEWER, ShortcutCategory.SCRIPTS,
				tool.default_shortcut);
		}

		register ("win.add-to-selection('1')", N_("Add to Selection 1"), SHORTCUT_CONTEXT_BROWSER_VIEWER, ShortcutCategory.SELECTIONS, "<Alt>1");
		register ("win.add-to-selection('2')", N_("Add to Selection 2"), SHORTCUT_CONTEXT_BROWSER_VIEWER, ShortcutCategory.SELECTIONS, "<Alt>2");
		register ("win.add-to-selection('3')", N_("Add to Selection 3"), SHORTCUT_CONTEXT_BROWSER_VIEWER, ShortcutCategory.SELECTIONS, "<Alt>3");
		register ("win.remove-from-selection(1)", N_("Remove from Selection 1"), SHORTCUT_CONTEXT_BROWSER_VIEWER, ShortcutCategory.SELECTIONS, "<Primary><Alt>1");
		register ("win.remove-from-selection(2)", N_("Remove from Selection 2"), SHORTCUT_CONTEXT_BROWSER_VIEWER, ShortcutCategory.SELECTIONS, "<Primary><Alt>2");
		register ("win.remove-from-selection(3)", N_("Remove from Selection 3"), SHORTCUT_CONTEXT_BROWSER_VIEWER, ShortcutCategory.SELECTIONS, "<Primary><Alt>3");
		register ("win.open-selection(1)", N_("Show Selection 1"), SHORTCUT_CONTEXT_BROWSER_VIEWER, ShortcutCategory.SELECTIONS, "<Primary>1");
		register ("win.open-selection(2)", N_("Show Selection 2"), SHORTCUT_CONTEXT_BROWSER_VIEWER, ShortcutCategory.SELECTIONS, "<Primary>2");
		register ("win.open-selection(3)", N_("Show Selection 3"), SHORTCUT_CONTEXT_BROWSER_VIEWER, ShortcutCategory.SELECTIONS, "<Primary>3");

		register ("win.reset-filter", N_("View All"), ShortcutContext.BROWSER, ShortcutCategory.FILTERS);

		register ("win.pop-page", N_("Close"), SHORTCUT_CONTEXT_VIEWERS | ShortcutContext.EDITOR | ShortcutContext.FIXED, ShortcutCategory.VIEWER, "Escape");
		register ("win.view-next", N_("Next File"), SHORTCUT_CONTEXT_VIEWERS, ShortcutCategory.VIEWER, "Page_Down");
		register ("win.view-previous", N_("Previous File"), SHORTCUT_CONTEXT_VIEWERS, ShortcutCategory.VIEWER, "Page_Up");
		register ("win.view-first", N_("First File"), SHORTCUT_CONTEXT_VIEWERS, ShortcutCategory.VIEWER, "Home");
		register ("win.view-last", N_("Last File"), SHORTCUT_CONTEXT_VIEWERS, ShortcutCategory.VIEWER, "End");
		register ("win.viewer-controls", N_("Statusbar"), SHORTCUT_CONTEXT_VIEWERS, ShortcutCategory.VIEWER, "F7");
		register ("win.viewer-properties", N_("File Properties"), SHORTCUT_CONTEXT_VIEWERS, ShortcutCategory.VIEWER, "i");
		register ("win.editor-tools", N_("Edit Image"), SHORTCUT_CONTEXT_VIEWERS, ShortcutCategory.VIEWER, "e");
		register ("toggle-thumbnail-list", N_("Thumbnails"), SHORTCUT_CONTEXT_VIEWERS, ShortcutCategory.VIEWER, "F8");

		register ("image.zoom-in", N_("Zoom In"), ShortcutContext.IMAGE_VIEWER, ShortcutCategory.VIEWER, "plus");
		register ("image.zoom-out", N_("Zoom Out"), ShortcutContext.IMAGE_VIEWER, ShortcutCategory.VIEWER, "minus");
		register ("image.zoom-in-fast", N_("Zoom In Fast"), ShortcutContext.IMAGE_VIEWER, ShortcutCategory.VIEWER, "<Alt>plus");
		register ("image.zoom-out-fast", N_("Zoom Out Fast"), ShortcutContext.IMAGE_VIEWER, ShortcutCategory.VIEWER, "<Alt>minus");
		register ("image.set-zoom('100')", N_("Zoom 100%"), ShortcutContext.IMAGE_VIEWER, ShortcutCategory.VIEWER, "1");
		register ("image.set-zoom('200')", N_("Zoom 200%"), ShortcutContext.IMAGE_VIEWER, ShortcutCategory.VIEWER, "2");
		register ("image.set-zoom('300')", N_("Zoom 300%"), ShortcutContext.IMAGE_VIEWER, ShortcutCategory.VIEWER, "3");
		register ("image.set-zoom('max-size')", N_("Zoom to Fit"), ShortcutContext.IMAGE_VIEWER, ShortcutCategory.VIEWER, "<Shift>x");
		register ("image.set-zoom('max-size-if-larger')", N_("Zoom to Fit if Larger"), ShortcutContext.IMAGE_VIEWER, ShortcutCategory.VIEWER, "x");
		register ("image.set-zoom('max-width')", N_("Zoom to Fit Width"), ShortcutContext.IMAGE_VIEWER, ShortcutCategory.VIEWER, "w");
		register ("image.set-zoom('max-height')", N_("Zoom to Fit Height"), ShortcutContext.IMAGE_VIEWER, ShortcutCategory.VIEWER, "h");
		register ("image.set-zoom('best-fit')", N_("Automatic Zoom"), ShortcutContext.IMAGE_VIEWER, ShortcutCategory.VIEWER, "0");
		register ("image.set-zoom('fill-space')", N_("Fill Space"), ShortcutContext.IMAGE_VIEWER, ShortcutCategory.VIEWER, "<Alt>f");

		register ("image.scroll-left", N_("Scroll Left"), ShortcutContext.IMAGE_VIEWER, ShortcutCategory.VIEWER, "Left");
		register ("image.scroll-right", N_("Scroll Right"), ShortcutContext.IMAGE_VIEWER, ShortcutCategory.VIEWER, "Right");
		register ("image.scroll-up", N_("Scroll Up"), ShortcutContext.IMAGE_VIEWER, ShortcutCategory.VIEWER, "Up");
		register ("image.scroll-down", N_("Scroll Down"), ShortcutContext.IMAGE_VIEWER, ShortcutCategory.VIEWER, "Down");
		register ("image.scroll-page-left", N_("Scroll Left Fast"), ShortcutContext.IMAGE_VIEWER, ShortcutCategory.VIEWER, "<Shift>Left");
		register ("image.scroll-page-right", N_("Scroll Right Fast"), ShortcutContext.IMAGE_VIEWER, ShortcutCategory.VIEWER, "<Shift>Right");
		register ("image.scroll-page-up", N_("Scroll Up Fast"), ShortcutContext.IMAGE_VIEWER, ShortcutCategory.VIEWER, "<Shift>Up");
		register ("image.scroll-page-down", N_("Scroll Down Fast"), ShortcutContext.IMAGE_VIEWER, ShortcutCategory.VIEWER, "<Shift>Down");
		register ("image.recenter", N_("Scroll to Center"), ShortcutContext.IMAGE_VIEWER, ShortcutCategory.VIEWER, "<Alt>Down");

		register ("image.save", N_("Save Image"), ShortcutContext.IMAGE_VIEWER, ShortcutCategory.VIEWER, "<Primary>s");
		register ("image.save-as", N_("Save Image As"), ShortcutContext.IMAGE_VIEWER, ShortcutCategory.VIEWER, "<Primary><Shift>s");
		register ("revert-to-saved", N_("Revert Image to Saved"), ShortcutContext.IMAGE_VIEWER, ShortcutCategory.VIEWER, "F4");
		register ("image.undo", N_("Undo Edit"), ShortcutContext.IMAGE_VIEWER, ShortcutCategory.VIEWER, "<Primary>z");
		register ("image.redo", N_("Redo Edit"), ShortcutContext.IMAGE_VIEWER, ShortcutCategory.VIEWER, "<Primary><Shift>z");
		register ("apply-editor-changes", null, ShortcutContext.EDITOR, ShortcutCategory.HIDDEN, "Return");

		register ("file-tool-adjust-contrast", N_("Adjust Contrast"), ShortcutContext.IMAGE_VIEWER, ShortcutCategory.VIEWER, "a");
		register ("file-tool-flip", N_("Flip"), ShortcutContext.IMAGE_VIEWER, ShortcutCategory.VIEWER, "l");
		register ("file-tool-mirror", N_("Mirror"), ShortcutContext.IMAGE_VIEWER, ShortcutCategory.VIEWER, "m");
		register ("file-tool-rotate-right", N_("Rotate Right"), ShortcutContext.IMAGE_VIEWER, ShortcutCategory.VIEWER, "r");
		register ("file-tool-rotate-left", N_("Rotate Left"), ShortcutContext.IMAGE_VIEWER, ShortcutCategory.VIEWER, "<Shift>r");
		register ("file-tool-crop", N_("Crop"), ShortcutContext.IMAGE_VIEWER, ShortcutCategory.VIEWER, "<Shift>c");
		register ("file-tool-resize", N_("Resize"), ShortcutContext.IMAGE_VIEWER, ShortcutCategory.VIEWER, "<Shift>s");
		register ("image.color-picker", N_("Color Picker"), ShortcutContext.IMAGE_VIEWER, ShortcutCategory.VIEWER, "p");

		register ("slideshow", N_("Start Presentation"), SHORTCUT_CONTEXT_BROWSER_VIEWER, ShortcutCategory.VIEWER, "F5");
		register ("slideshow-close", N_("Terminate Presentation"), ShortcutContext.SLIDESHOW, ShortcutCategory.SLIDESHOW, "Escape");
		register ("slideshow-toggle-pause", N_("Pause/Resume Presentation"), ShortcutContext.SLIDESHOW, ShortcutCategory.SLIDESHOW, "p");
		register ("slideshow-next-image", N_("Show Next File"), ShortcutContext.SLIDESHOW, ShortcutCategory.SLIDESHOW, "space");
		register ("slideshow-previous-image", N_("Show Previous File"), ShortcutContext.SLIDESHOW, ShortcutCategory.SLIDESHOW, "b");

		register ("video.toggle-play", N_("Play/Pause"), ShortcutContext.MEDIA_VIEWER, ShortcutCategory.MEDIA_VIEWER, "space");
		register ("video.save-screenshot", N_("Screenshot"), ShortcutContext.MEDIA_VIEWER, ShortcutCategory.MEDIA_VIEWER, "<Alt>s");
		register ("video.mute", N_("Mute"), ShortcutContext.MEDIA_VIEWER, ShortcutCategory.MEDIA_VIEWER, "m");
		register ("video.increase-speed", N_("Play Faster"), ShortcutContext.MEDIA_VIEWER, ShortcutCategory.MEDIA_VIEWER, "plus");
		register ("video.decrease-speed", N_("Play Slower"), ShortcutContext.MEDIA_VIEWER, ShortcutCategory.MEDIA_VIEWER, "minus");
		register ("video.normal-speed", N_("Normal Speed"), ShortcutContext.MEDIA_VIEWER, ShortcutCategory.MEDIA_VIEWER, "equal");
		register ("video.next-frame", N_("Next Frame"), ShortcutContext.MEDIA_VIEWER, ShortcutCategory.MEDIA_VIEWER, "period");
		register ("video.skip(1)", N_("Go Forward 1 Second"), ShortcutContext.MEDIA_VIEWER, ShortcutCategory.MEDIA_VIEWER, null);
		register ("video.skip(5)", N_("Go Forward 5 Seconds"), ShortcutContext.MEDIA_VIEWER, ShortcutCategory.MEDIA_VIEWER, "<Shift>Right");
		register ("video.skip(10)", N_("Go Forward 10 Seconds"), ShortcutContext.MEDIA_VIEWER, ShortcutCategory.MEDIA_VIEWER, "<Alt>Right");
		register ("video.skip(60)", N_("Go Forward 1 Minute"), ShortcutContext.MEDIA_VIEWER, ShortcutCategory.MEDIA_VIEWER, "<Primary>Right");
		register ("video.skip(300)", N_("Go Forward 5 Minutes"), ShortcutContext.MEDIA_VIEWER, ShortcutCategory.MEDIA_VIEWER, "<Primary><Alt>Right");
		register ("video.skip(-1)", N_("Go Backward 1 Second"), ShortcutContext.MEDIA_VIEWER, ShortcutCategory.MEDIA_VIEWER, null);
		register ("video.skip(-5)", N_("Go Backward 5 Seconds"), ShortcutContext.MEDIA_VIEWER, ShortcutCategory.MEDIA_VIEWER, "<Shift>Left");
		register ("video.skip(-10)", N_("Go Backward 10 Seconds"), ShortcutContext.MEDIA_VIEWER, ShortcutCategory.MEDIA_VIEWER, "<Alt>Left");
		register ("video.skip(-60)", N_("Go Backward 1 Minute"), ShortcutContext.MEDIA_VIEWER, ShortcutCategory.MEDIA_VIEWER, "<Primary>Left");
		register ("video.skip(-300)", N_("Go Backward 5 Minutes"), ShortcutContext.MEDIA_VIEWER, ShortcutCategory.MEDIA_VIEWER, "<Primary><Alt>Left");
		register ("video.copy-frame", N_("Copy Frame"), ShortcutContext.MEDIA_VIEWER, ShortcutCategory.MEDIA_VIEWER, "<Ctrl><Shift>c");

		// Not real actions, used for documentation only.

		register ("doc.file-list-select-all", N_("Select All"), ShortcutContext.BROWSER | ShortcutContext.DOC, ShortcutCategory.FILE_MANAGER, "<Primary>a");
		register ("doc.file-list-unselect-all", N_("Select None"), ShortcutContext.BROWSER | ShortcutContext.DOC, ShortcutCategory.FILE_MANAGER, "<Primary><Shift>a");
	}

	public void show_help () {
		var dialog = new Adw.ShortcutsDialog ();
		var sorted_shortcuts = app.shortcuts.entries.duplicate ();
		sorted_shortcuts.sort ((a, b) => Util.intcmp (a.category, b.category));
		var last_category = -1;
		Adw.ShortcutsSection section = null;
		foreach (var shortcut in sorted_shortcuts) {
			if (shortcut.category == ShortcutCategory.HIDDEN) {
				continue;
			}
			if (Strings.empty (shortcut.accelerator)) {
				continue;
			}
			if (shortcut.category != last_category) {
				section = new Adw.ShortcutsSection (_(shortcut.category.to_title ()));
				dialog.add (section);
				last_category = shortcut.category;
			}
			section.add (new Adw.ShortcutsItem (shortcut.description, shortcut.accelerator));
		}
		dialog.present (app.active_window);
	}

	bool loaded = false;

	const string SHORTCUT_FORMAT = "1.0";
}
