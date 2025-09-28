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
				if (!shortcut.get_is_customizable ()) {
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

	public Shortcut? find_by_key (ShortcutContext context, uint keyval, Gdk.ModifierType modifiers) {
		if (keyval == 0) {
			return null;
		}

		// Remove flags not related to the window context.
		context = context & SHORTCUT_CONTEXT_ANY;
		keyval = Gdk.keyval_to_lower (keyval);

		foreach (unowned var shortcut in entries) {
			if (((shortcut.context & context) != 0)
				&& (shortcut.keyval == keyval)
				&& (shortcut.modifiers == modifiers))
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
		//register ("help", N_("Help"), SHORTCUT_CONTEXT_BROWSER_VIEWER, ShortcutCategory.GENERAL, "F1");
		register ("app.shortcuts", N_("Keyboard Shortcuts"), SHORTCUT_CONTEXT_BROWSER_VIEWER, ShortcutCategory.GENERAL, "<Primary>F1");
		register ("win.main-menu", N_("Main Menu"), SHORTCUT_CONTEXT_BROWSER_VIEWER, ShortcutCategory.GENERAL, "F10");

		register ("open-location", N_("Open Location"), ShortcutContext.BROWSER, ShortcutCategory.FILE_MANAGER, "<Primary>o");
		register ("win.load-home", N_("Home"), ShortcutContext.BROWSER, ShortcutCategory.FILE_MANAGER, "<Alt>Home");
		register ("win.load-parent", N_("Parent Folder"), ShortcutContext.BROWSER, ShortcutCategory.FILE_MANAGER, "<Alt>Up");
		register ("win.reload", N_("Reload"), ShortcutContext.BROWSER, ShortcutCategory.FILE_MANAGER, "<Primary>r");
		register ("win.load-previous", N_("Previous Visited"), ShortcutContext.BROWSER, ShortcutCategory.FILE_MANAGER, "<Alt>Left");
		register ("win.load-next", N_("Next Visited"), ShortcutContext.BROWSER, ShortcutCategory.FILE_MANAGER, "<Alt>Right");
		register ("win.load-next-folder", N_("Next Folder"), ShortcutContext.BROWSER, ShortcutCategory.FILE_MANAGER, "<Primary><Alt>n");
		register ("win.load-prev-folder", N_("Previous Folder"), ShortcutContext.BROWSER, ShortcutCategory.FILE_MANAGER, "<Primary><Alt>p");
		register ("win.hidden-files", N_("Hidden Files"), ShortcutContext.BROWSER, ShortcutCategory.FILE_MANAGER, "h");
		register ("win.sort-files", N_("Sort Files"), ShortcutContext.BROWSER, ShortcutCategory.FILE_MANAGER, "s");
		register ("win.sort-folders", N_("Sort Folders"), ShortcutContext.BROWSER, ShortcutCategory.FILE_MANAGER);
		register ("win.browser-sidebar", N_("Sidebar"), ShortcutContext.BROWSER, ShortcutCategory.FILE_MANAGER, "F9");
		register ("win.browser-properties", N_("File Properties"), ShortcutContext.BROWSER, ShortcutCategory.FILE_MANAGER, "i");
		register ("win.show-bookmarks", N_("Bookmarks"), ShortcutContext.BROWSER, ShortcutCategory.FILE_MANAGER, "<Primary>b");
		register ("win.show-history", N_("History"), ShortcutContext.BROWSER, ShortcutCategory.FILE_MANAGER, "<Primary>h");
		register ("win.show-catalogs", N_("Catalogs"), ShortcutContext.BROWSER, ShortcutCategory.FILE_MANAGER, "<Primary><Shift>c");
		register ("win.show-folders", N_("Folders"), ShortcutContext.BROWSER, ShortcutCategory.FILE_MANAGER, "<Primary><Shift>f");

		register ("win.find", N_("Find Files"), SHORTCUT_CONTEXT_BROWSER_VIEWER, ShortcutCategory.FILE_MANAGER, "<Primary>f");
		register ("win.cut-files", N_("Cut"), ShortcutContext.BROWSER | ShortcutContext.FIXED, ShortcutCategory.FILE_MANAGER, "<Primary>x");
		register ("win.copy-files", N_("Copy"), ShortcutContext.BROWSER | ShortcutContext.FIXED, ShortcutCategory.FILE_MANAGER, "<Primary>c");
		register ("win.paste-files", N_("Paste"), ShortcutContext.BROWSER | ShortcutContext.FIXED, ShortcutCategory.FILE_MANAGER, "<Primary>v");

		register ("rename", N_("Rename"), SHORTCUT_CONTEXT_BROWSER_VIEWER, ShortcutCategory.FILE_MANAGER, "F2");
		register ("win.duplicate-files", N_("Duplicate"), SHORTCUT_CONTEXT_BROWSER_VIEWER, ShortcutCategory.FILE_MANAGER, "<Primary>d");
		register ("remove-from-source", N_("Delete"), SHORTCUT_CONTEXT_BROWSER_VIEWER, ShortcutCategory.FILE_MANAGER, "Delete");
		register ("remove-from-source-permanently", N_("Delete Permanently"), SHORTCUT_CONTEXT_BROWSER_VIEWER, ShortcutCategory.FILE_MANAGER, "<Shift>Delete");
		register ("open-with-gimp", N_("Open with Gimp"), SHORTCUT_CONTEXT_BROWSER_VIEWER, ShortcutCategory.FILE_MANAGER, "g");
		register ("open-with-inkscape", N_("Open with Inkscape"), SHORTCUT_CONTEXT_BROWSER_VIEWER, ShortcutCategory.FILE_MANAGER, "k");
		register ("folder-context-open-in-terminal", N_("Open in Terminal"), SHORTCUT_CONTEXT_BROWSER_VIEWER, ShortcutCategory.FILE_MANAGER, "<Primary>t");
		register ("win.edit-metadata", N_("Edit Comment"), SHORTCUT_CONTEXT_BROWSER_VIEWER, ShortcutCategory.FILE_MANAGER, "c");
		//register ("edit-tags", N_("Edit Tags"), SHORTCUT_CONTEXT_BROWSER_VIEWER, ShortcutCategory.FILE_MANAGER, "t");
		register ("print", N_("Print"), SHORTCUT_CONTEXT_BROWSER_VIEWER, ShortcutCategory.FILE_MANAGER, "<Primary>p");
		register ("win.open-clipboard", N_("Open Clipboard"), SHORTCUT_CONTEXT_BROWSER_VIEWER, ShortcutCategory.FILE_MANAGER, "<Primary><Shift>v");
		register ("win.scripts", N_("Scripts"), SHORTCUT_CONTEXT_BROWSER_VIEWER, ShortcutCategory.FILE_MANAGER, "<Primary><Shift>x");

		register ("win.toggle-fullscreen", N_("Fullscreen"), SHORTCUT_CONTEXT_BROWSER_VIEWER, ShortcutCategory.VIEWER, "f");

		register ("file-tool-adjust-contrast", N_("Adjust Contrast"), SHORTCUT_CONTEXT_BROWSER_VIEWER, ShortcutCategory.VIEWER, "a");
		register ("file-tool-flip", N_("Flip"), SHORTCUT_CONTEXT_BROWSER_VIEWER, ShortcutCategory.VIEWER, "l");
		register ("file-tool-mirror", N_("Mirror"), SHORTCUT_CONTEXT_BROWSER_VIEWER, ShortcutCategory.VIEWER, "m");
		register ("file-tool-rotate-right", N_("Rotate Right"), SHORTCUT_CONTEXT_BROWSER_VIEWER, ShortcutCategory.VIEWER, "r");
		register ("file-tool-rotate-left", N_("Rotate Left"), SHORTCUT_CONTEXT_BROWSER_VIEWER, ShortcutCategory.VIEWER, "<Shift>r");
		register ("file-tool-crop", N_("Crop"), SHORTCUT_CONTEXT_BROWSER_VIEWER, ShortcutCategory.VIEWER, "<Shift>c");
		register ("file-tool-resize", N_("Resize"), SHORTCUT_CONTEXT_BROWSER_VIEWER, ShortcutCategory.VIEWER, "<Shift>s");

		register ("rotate-right", N_("Rotate Right"), SHORTCUT_CONTEXT_BROWSER_VIEWER, ShortcutCategory.SCRIPTS, "bracketright");
		register ("rotate-left", N_("Rotate Left"), SHORTCUT_CONTEXT_BROWSER_VIEWER, ShortcutCategory.SCRIPTS, "bracketleft");

		register ("add-to-selection-1", N_("Add to Selection"), SHORTCUT_CONTEXT_BROWSER_VIEWER | ShortcutContext.DOC, ShortcutCategory.HIDDEN, "<Alt>1");
		register ("add-to-selection-2", N_("Add to Selection"), SHORTCUT_CONTEXT_BROWSER_VIEWER | ShortcutContext.DOC, ShortcutCategory.HIDDEN, "<Alt>2");
		register ("add-to-selection-3", N_("Add to Selection"), SHORTCUT_CONTEXT_BROWSER_VIEWER | ShortcutContext.DOC, ShortcutCategory.HIDDEN, "<Alt>3");
		register ("remove-from-selection-1", N_("Remove from Selection"), SHORTCUT_CONTEXT_BROWSER_VIEWER | ShortcutContext.DOC, ShortcutCategory.HIDDEN, "<Shift><Alt>1");
		register ("remove-from-selection-2", N_("Remove from Selection"), SHORTCUT_CONTEXT_BROWSER_VIEWER | ShortcutContext.DOC, ShortcutCategory.HIDDEN, "<Shift><Alt>2");
		register ("remove-from-selection-3", N_("Remove from Selection"), SHORTCUT_CONTEXT_BROWSER_VIEWER | ShortcutContext.DOC, ShortcutCategory.HIDDEN, "<Shift><Alt>3");
		register ("go-to-selection-1", N_("Show Selection"), SHORTCUT_CONTEXT_BROWSER_VIEWER | ShortcutContext.DOC, ShortcutCategory.HIDDEN, "<Primary>1");
		register ("go-to-selection-2", N_("Show Selection"), SHORTCUT_CONTEXT_BROWSER_VIEWER | ShortcutContext.DOC, ShortcutCategory.HIDDEN, "<Primary>2");
		register ("go-to-selection-3", N_("Show Selection"), SHORTCUT_CONTEXT_BROWSER_VIEWER | ShortcutContext.DOC, ShortcutCategory.HIDDEN, "<Primary>3");

		register ("win.reset-filter", N_("View All"), ShortcutContext.BROWSER, ShortcutCategory.FILTERS);

		register ("win.pop-page", N_("Close"), SHORTCUT_CONTEXT_VIEWERS | ShortcutContext.FIXED, ShortcutCategory.VIEWER, "Escape");
		register ("win.view-next", N_("Next File"), SHORTCUT_CONTEXT_VIEWERS, ShortcutCategory.VIEWER, "Page_Down");
		register ("win.view-previous", N_("Previous File"), SHORTCUT_CONTEXT_VIEWERS, ShortcutCategory.VIEWER, "Page_Up");
		register ("win.view-first", N_("First File"), SHORTCUT_CONTEXT_VIEWERS, ShortcutCategory.VIEWER, "Home");
		register ("win.view-last", N_("Last File"), SHORTCUT_CONTEXT_VIEWERS, ShortcutCategory.VIEWER, "End");
		register ("win.viewer-controls", N_("Statusbar"), SHORTCUT_CONTEXT_VIEWERS, ShortcutCategory.VIEWER, "F7");
		register ("win.viewer-properties", N_("File Properties"), SHORTCUT_CONTEXT_VIEWERS, ShortcutCategory.VIEWER, "i");
		register ("toggle-thumbnail-list", N_("Thumbnails"), SHORTCUT_CONTEXT_VIEWERS, ShortcutCategory.VIEWER, "F8");
		register ("toggle-edit-file", N_("Image Tools"), ShortcutContext.IMAGE_VIEWER, ShortcutCategory.VIEWER, "e");

		register ("image.zoom-in", N_("Zoom In"), ShortcutContext.IMAGE_VIEWER, ShortcutCategory.VIEWER, "plus");
		register ("image.zoom-out", N_("Zoom Out"), ShortcutContext.IMAGE_VIEWER, ShortcutCategory.VIEWER, "minus");
		register ("image.zoom-in-fast", N_("Zoom In Fast"), ShortcutContext.IMAGE_VIEWER, ShortcutCategory.VIEWER, "<Alt>plus");
		register ("image.zoom-out-fast", N_("Zoom Out Fast"), ShortcutContext.IMAGE_VIEWER, ShortcutCategory.VIEWER, "<Alt>minus");
		register ("image.set-zoom('100')", N_("Zoom 100%"), ShortcutContext.IMAGE_VIEWER, ShortcutCategory.VIEWER, "1");
		register ("image.set-zoom('200')", N_("Zoom 200%"), ShortcutContext.IMAGE_VIEWER, ShortcutCategory.VIEWER, "2");
		register ("image.set-zoom('300')", N_("Zoom 300%"), ShortcutContext.IMAGE_VIEWER, ShortcutCategory.VIEWER, "3");
		register ("image.set-zoom('max-size')", N_("Zoom to Fit"), ShortcutContext.IMAGE_VIEWER, ShortcutCategory.VIEWER, "<Shift>x");
		register ("image.set-zoom('max-size-if-larger')", N_("Zoom to Fit if Larger"), ShortcutContext.IMAGE_VIEWER, ShortcutCategory.VIEWER, "x");
		register ("image.set-zoom('max-width')", N_("Zoom to Fit Width"), ShortcutContext.IMAGE_VIEWER, ShortcutCategory.VIEWER, "<Shift>w");
		register ("image.set-zoom('max-height')", N_("Zoom to Fit Height"), ShortcutContext.IMAGE_VIEWER, ShortcutCategory.VIEWER, "<Shift>h");
		register ("image.set-zoom('best-fit')", N_("Automatic Zoom"), ShortcutContext.IMAGE_VIEWER, ShortcutCategory.VIEWER, "0");

		register ("image.scroll-left", N_("Scroll Left"), ShortcutContext.IMAGE_VIEWER, ShortcutCategory.VIEWER, "Left");
		register ("image.scroll-right", N_("Scroll Right"), ShortcutContext.IMAGE_VIEWER, ShortcutCategory.VIEWER, "Right");
		register ("image.scroll-up", N_("Scroll Up"), ShortcutContext.IMAGE_VIEWER, ShortcutCategory.VIEWER, "Up");
		register ("image.scroll-down", N_("Scroll Down"), ShortcutContext.IMAGE_VIEWER, ShortcutCategory.VIEWER, "Down");
		register ("image.scroll-page-left", N_("Scroll Left Fast"), ShortcutContext.IMAGE_VIEWER, ShortcutCategory.VIEWER, "<Shift>Left");
		register ("image.scroll-page-right", N_("Scroll Right Fast"), ShortcutContext.IMAGE_VIEWER, ShortcutCategory.VIEWER, "<Shift>Right");
		register ("image.scroll-page-up", N_("Scroll Up Fast"), ShortcutContext.IMAGE_VIEWER, ShortcutCategory.VIEWER, "<Shift>Up");
		register ("image.scroll-page-down", N_("Scroll Down Fast"), ShortcutContext.IMAGE_VIEWER, ShortcutCategory.VIEWER, "<Shift>Down");
		register ("image.recenter", N_("Scroll to Center"), ShortcutContext.IMAGE_VIEWER, ShortcutCategory.VIEWER, "<Alt>Down");

		register ("save", N_("Save Image"), ShortcutContext.IMAGE_VIEWER, ShortcutCategory.VIEWER, "<Primary>s");
		register ("image.save-as", N_("Save Image As"), ShortcutContext.IMAGE_VIEWER, ShortcutCategory.VIEWER, "<Primary><Shift>s");
		register ("revert-to-saved", N_("Revert Image to Saved"), ShortcutContext.IMAGE_VIEWER, ShortcutCategory.VIEWER, "F4");

		register ("undo", N_("Undo Edit"), ShortcutContext.IMAGE_VIEWER, ShortcutCategory.VIEWER, "<Primary>z");
		register ("redo", N_("Redo Edit"), ShortcutContext.IMAGE_VIEWER, ShortcutCategory.VIEWER, "<Primary><Shift>z");
		register ("apply-editor-changes", null, ShortcutContext.IMAGE_EDITOR, ShortcutCategory.HIDDEN, "Return");

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
		register ("doc.add-to-selection", N_("Add to Selection"), SHORTCUT_CONTEXT_BROWSER_VIEWER | ShortcutContext.DOC, ShortcutCategory.SELECTIONS, "<Alt>1...3");
		register ("doc.remove-from-selection", N_("Remove from Selection"), SHORTCUT_CONTEXT_BROWSER_VIEWER | ShortcutContext.DOC, ShortcutCategory.SELECTIONS, "<Shift><Alt>1...3");
		register ("doc.go-to-selection", N_("Show Selection"), SHORTCUT_CONTEXT_BROWSER_VIEWER | ShortcutContext.DOC, ShortcutCategory.SELECTIONS, "<Primary>1...3");
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
