public class Gth.Shortcuts {
	public GenericList<Shortcut> entries;
	public HashTable<string, Shortcut> by_action;

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

	public Shortcut? find_by_key (ShortcutContext context, uint keyval, Gdk.ModifierType modifiers) {
		if (keyval == 0) {
			return null;
		}

		// Remove flags not related to the window mode.
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

	public Shortcut? find_by_accel (ShortcutContext context, string accelerator) {
		if (accelerator == null) {
			return null;
		}

		// Remove flags not related to the window mode.
		context = context & SHORTCUT_CONTEXT_ANY;

		foreach (unowned var shortcut in entries) {
			if (((shortcut.context & context) != 0)
				&& (shortcut.accelerator == accelerator))
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
		shortcut.description = description;
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
		register ("help", N_("Help"), SHORTCUT_CONTEXT_BROWSER_VIEWER, ShortcutCategory.GENERAL, "F1");
		register ("shortcuts", N_("Keyboard Shortcuts"), SHORTCUT_CONTEXT_BROWSER_VIEWER, ShortcutCategory.GENERAL, "<Primary>F1");
		register ("app.quit", N_("Quit"), SHORTCUT_CONTEXT_BROWSER_VIEWER, ShortcutCategory.GENERAL, "<Primary>q");
		register ("show-menu", N_("Main Menu"), SHORTCUT_CONTEXT_BROWSER_VIEWER, ShortcutCategory.GENERAL, "F10");
		register ("preferences", N_("Preferences"), SHORTCUT_CONTEXT_BROWSER_VIEWER, ShortcutCategory.GENERAL, "<Primary>p");
		register ("win.close", N_("Close Window"), SHORTCUT_CONTEXT_BROWSER_VIEWER, ShortcutCategory.GENERAL, "<Primary>w");
		register ("win.open-clipboard", N_("Paste Image"), SHORTCUT_CONTEXT_VIEWERS, ShortcutCategory.GENERAL, "<Primary><Shift>v");

		register ("win.edit-metadata", N_("Edit Comment"), SHORTCUT_CONTEXT_BROWSER_VIEWER, ShortcutCategory.FILE_MANAGER, "c");
		//register ("edit-tags", N_("Edit Tags"), SHORTCUT_CONTEXT_BROWSER_VIEWER, ShortcutCategory.FILE_MANAGER, "t");
		register ("rename", N_("Rename"), SHORTCUT_CONTEXT_BROWSER_VIEWER, ShortcutCategory.FILE_MANAGER, "F2");
		register ("duplicate", N_("Duplicate"), SHORTCUT_CONTEXT_BROWSER_VIEWER, ShortcutCategory.FILE_MANAGER, "<Primary>d");
		register ("remove-from-source", N_("Delete"), SHORTCUT_CONTEXT_BROWSER_VIEWER, ShortcutCategory.FILE_MANAGER, "Delete");
		register ("remove-from-source-permanently", N_("Delete Permanently"), SHORTCUT_CONTEXT_BROWSER_VIEWER, ShortcutCategory.FILE_MANAGER, "<Shift>Delete");
		register ("open-with-gimp", N_("Open with Gimp"), SHORTCUT_CONTEXT_BROWSER_VIEWER, ShortcutCategory.FILE_MANAGER, "g");
		register ("print", N_("Print"), SHORTCUT_CONTEXT_BROWSER_VIEWER, ShortcutCategory.FILE_MANAGER, "<Primary>p");
		register ("find", N_("Find files"), SHORTCUT_CONTEXT_BROWSER_VIEWER, ShortcutCategory.FILE_MANAGER, "<Primary>f");
		register ("edit-cut", N_("Cut"), ShortcutContext.BROWSER | ShortcutContext.FIXED, ShortcutCategory.FILE_MANAGER, "<Primary>x");
		register ("edit-copy", N_("Copy"), ShortcutContext.BROWSER | ShortcutContext.FIXED, ShortcutCategory.FILE_MANAGER, "<Primary>c");
		register ("edit-paste", N_("Paste"), ShortcutContext.BROWSER | ShortcutContext.FIXED, ShortcutCategory.FILE_MANAGER, "<Primary>v");
		register ("folder-context-open-in-terminal", N_("Open in Terminal"), SHORTCUT_CONTEXT_BROWSER_VIEWER, ShortcutCategory.FILE_MANAGER, "<Primary>t");

		register ("win.toggle-fullscreen", N_("Fullscreen"), SHORTCUT_CONTEXT_BROWSER_VIEWER, ShortcutCategory.VIEWER, "f");

		register ("file-tool-adjust-contrast", N_("Adjust Contrast"), SHORTCUT_CONTEXT_BROWSER_VIEWER, ShortcutCategory.IMAGE_EDITOR, "a");
		register ("file-tool-flip", N_("Flip"), SHORTCUT_CONTEXT_BROWSER_VIEWER, ShortcutCategory.IMAGE_EDITOR, "l");
		register ("file-tool-mirror", N_("Mirror"), SHORTCUT_CONTEXT_BROWSER_VIEWER, ShortcutCategory.IMAGE_EDITOR, "m");
		register ("file-tool-rotate-right", N_("Rotate Right"), SHORTCUT_CONTEXT_BROWSER_VIEWER, ShortcutCategory.IMAGE_EDITOR, "r");
		register ("file-tool-rotate-left", N_("Rotate Left"), SHORTCUT_CONTEXT_BROWSER_VIEWER, ShortcutCategory.IMAGE_EDITOR, "<Shift>r");
		register ("file-tool-crop", N_("Crop"), SHORTCUT_CONTEXT_BROWSER_VIEWER, ShortcutCategory.IMAGE_EDITOR, "<Shift>c");
		register ("file-tool-resize", N_("Resize"), SHORTCUT_CONTEXT_BROWSER_VIEWER, ShortcutCategory.IMAGE_EDITOR, "<Shift>s");

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

		register ("open-location", N_("Open Location"), ShortcutContext.BROWSER, ShortcutCategory.NAVIGATION, "<Primary>o");
		register ("go-back", N_("Load Previous Location"), ShortcutContext.BROWSER, ShortcutCategory.NAVIGATION, "<Alt>Left");
		register ("go-forward", N_("Load Next Location"), ShortcutContext.BROWSER, ShortcutCategory.NAVIGATION, "<Alt>Right");
		register ("go-up", N_("Load Parent Folder"), ShortcutContext.BROWSER, ShortcutCategory.NAVIGATION, "<Alt>Up");
		register ("go-home", N_("Load Home"), ShortcutContext.BROWSER, ShortcutCategory.NAVIGATION, "<Alt>Home");
		register ("reload", N_("Reload Location"), ShortcutContext.BROWSER, ShortcutCategory.NAVIGATION, "<Primary>r");
		register ("show-hidden-files", N_("Show/Hide Hidden Files"), ShortcutContext.BROWSER, ShortcutCategory.NAVIGATION, "h");
		register ("sort-by", N_("Change Sorting Order"), ShortcutContext.BROWSER, ShortcutCategory.NAVIGATION, "s");
		register ("toggle-sidebar", N_("Sidebar"), ShortcutContext.BROWSER, ShortcutCategory.UI, "F9");

		register ("set-filter-all", N_("View All"), ShortcutContext.BROWSER, ShortcutCategory.FILTERS);

		register ("file-list-select-all", N_("Select All"), ShortcutContext.BROWSER | ShortcutContext.DOC, ShortcutCategory.NAVIGATION, "<Primary>a");
		register ("file-list-unselect-all", N_("Select None"), ShortcutContext.BROWSER | ShortcutContext.DOC, ShortcutCategory.NAVIGATION, "<Primary><Shift>a");

		register ("win.pop-page", N_("Show Browser"), SHORTCUT_CONTEXT_VIEWERS | ShortcutContext.FIXED, ShortcutCategory.VIEWER, "Escape");
		register ("show-next-image", N_("Show Next File"), SHORTCUT_CONTEXT_VIEWERS, ShortcutCategory.VIEWER, "Page_Down");
		register ("show-previous-image", N_("Show Previous File"), SHORTCUT_CONTEXT_VIEWERS, ShortcutCategory.VIEWER, "Page_Up");
		register ("show-first-image", N_("Show First File"), SHORTCUT_CONTEXT_VIEWERS, ShortcutCategory.VIEWER, "Home");
		register ("show-last-image", N_("Show Last File"), SHORTCUT_CONTEXT_VIEWERS, ShortcutCategory.VIEWER, "End");
		register ("toggle-statusbar", N_("Statusbar"), SHORTCUT_CONTEXT_VIEWERS, ShortcutCategory.UI, "F7");
		register ("toggle-thumbnail-list", N_("Thumbnails List"), SHORTCUT_CONTEXT_VIEWERS, ShortcutCategory.UI, "F8");
		register ("toggle-edit-file", N_("Image Tools"), ShortcutContext.IMAGE_VIEWER, ShortcutCategory.UI, "e");
		register ("toggle-file-properties", N_("File Properties"), SHORTCUT_CONTEXT_VIEWERS, ShortcutCategory.UI, "i");

		register ("video.save-screenshot", N_("Screenshot"), ShortcutContext.MEDIA_VIEWER, ShortcutCategory.MEDIA_VIEWER, "<Alt>s");
		register ("video.toggle-play", N_("Play/Pause"), ShortcutContext.MEDIA_VIEWER, ShortcutCategory.MEDIA_VIEWER, "space");
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

		register ("image.zoom-in", N_("Zoom In"), ShortcutContext.IMAGE_VIEWER, ShortcutCategory.IMAGE_VIEWER, "plus");
		register ("image.zoom-out", N_("Zoom Out"), ShortcutContext.IMAGE_VIEWER, ShortcutCategory.IMAGE_VIEWER, "minus");
		register ("image.set-zoom('100')", N_("Zoom 100%"), ShortcutContext.IMAGE_VIEWER, ShortcutCategory.IMAGE_VIEWER, "1");
		register ("image.set-zoom('200')", N_("Zoom 200%"), ShortcutContext.IMAGE_VIEWER, ShortcutCategory.IMAGE_VIEWER, "2");
		register ("image.set-zoom('300')", N_("Zoom 300%"), ShortcutContext.IMAGE_VIEWER, ShortcutCategory.IMAGE_VIEWER, "3");
		register ("image.set-zoom('max-size')", N_("Zoom to Fit"), ShortcutContext.IMAGE_VIEWER, ShortcutCategory.IMAGE_VIEWER, "<Shift>x");
		register ("image.set-zoom('max-size-if-larger')", N_("Zoom to Fit if Larger"), ShortcutContext.IMAGE_VIEWER, ShortcutCategory.IMAGE_VIEWER, "x");
		register ("image.set-zoom('max-width')", N_("Zoom to Fit Width"), ShortcutContext.IMAGE_VIEWER, ShortcutCategory.IMAGE_VIEWER, "<Shift>w");
		register ("image.set-zoom('max-height')", N_("Zoom to Fit Height"), ShortcutContext.IMAGE_VIEWER, ShortcutCategory.IMAGE_VIEWER, "<Shift>h");
		register ("image.set-zoom('best-fit')", N_("Automatic Zoom"), ShortcutContext.IMAGE_VIEWER, ShortcutCategory.IMAGE_VIEWER, "equal");

		register ("save", N_("Save Image"), ShortcutContext.IMAGE_VIEWER, ShortcutCategory.IMAGE_VIEWER, "<Primary>s");
		register ("image.save-as", N_("Save Image As"), ShortcutContext.IMAGE_VIEWER, ShortcutCategory.IMAGE_VIEWER, "<Primary><Shift>s");
		register ("revert-to-saved", N_("Revert Image to Saved"), ShortcutContext.IMAGE_VIEWER, ShortcutCategory.IMAGE_VIEWER, "F4");

		register ("undo", N_("Undo Edit"), ShortcutContext.IMAGE_VIEWER, ShortcutCategory.IMAGE_EDITOR, "<Primary>z");
		register ("redo", N_("Redo Edit"), ShortcutContext.IMAGE_VIEWER, ShortcutCategory.IMAGE_EDITOR, "<Primary><Shift>z");
		register ("apply-editor-changes", null, ShortcutContext.IMAGE_EDITOR, ShortcutCategory.HIDDEN, "Return");

		register ("scroll-step-left", N_("Scroll Left"), ShortcutContext.IMAGE_VIEWER, ShortcutCategory.SCROLL_IMAGE, "Left");
		register ("scroll-step-right", N_("Scroll Right"), ShortcutContext.IMAGE_VIEWER, ShortcutCategory.SCROLL_IMAGE, "Right");
		register ("scroll-step-up", N_("Scroll Up"), ShortcutContext.IMAGE_VIEWER, ShortcutCategory.SCROLL_IMAGE, "Up");
		register ("scroll-step-down", N_("Scroll Down"), ShortcutContext.IMAGE_VIEWER, ShortcutCategory.SCROLL_IMAGE, "Down");
		register ("scroll-page-left", N_("Scroll Left Fast"), ShortcutContext.IMAGE_VIEWER, ShortcutCategory.SCROLL_IMAGE, "<Shift>Left");
		register ("scroll-page-right", N_("Scroll Right Fast"), ShortcutContext.IMAGE_VIEWER, ShortcutCategory.SCROLL_IMAGE, "<Shift>Right");
		register ("scroll-page-up", N_("Scroll Up Fast"), ShortcutContext.IMAGE_VIEWER, ShortcutCategory.SCROLL_IMAGE, "<Shift>Up");
		register ("scroll-page-down", N_("Scroll Down Fast"), ShortcutContext.IMAGE_VIEWER, ShortcutCategory.SCROLL_IMAGE, "<Shift>Down");
		register ("scroll-to-center", N_("Scroll to Center"), ShortcutContext.IMAGE_VIEWER, ShortcutCategory.SCROLL_IMAGE, "<Alt>Down");

		register ("slideshow", N_("Start Presentation"), SHORTCUT_CONTEXT_BROWSER_VIEWER, ShortcutCategory.VIEWER, "F5");
		register ("slideshow-close", N_("Terminate Presentation"), ShortcutContext.SLIDESHOW, ShortcutCategory.SLIDESHOW, "Escape");
		register ("slideshow-toggle-pause", N_("Pause/Resume Presentation"), ShortcutContext.SLIDESHOW, ShortcutCategory.SLIDESHOW, "p");
		register ("slideshow-next-image", N_("Show Next File"), ShortcutContext.SLIDESHOW, ShortcutCategory.SLIDESHOW, "space");
		register ("slideshow-previous-image", N_("Show Previous File"), ShortcutContext.SLIDESHOW, ShortcutCategory.SLIDESHOW, "b");

		// Not real actions, used in the shorcut window for documentation.

		register ("add-to-selection-doc", N_("Add to Selection"), SHORTCUT_CONTEXT_BROWSER_VIEWER | ShortcutContext.DOC, ShortcutCategory.SELECTIONS, "<Alt>1...3");
		register ("remove-from-selection-doc", N_("Remove from Selection"), SHORTCUT_CONTEXT_BROWSER_VIEWER | ShortcutContext.DOC, ShortcutCategory.SELECTIONS, "<Shift><Alt>1...3");
		register ("go-to-selection-doc", N_("Show Selection"), SHORTCUT_CONTEXT_BROWSER_VIEWER | ShortcutContext.DOC, ShortcutCategory.SELECTIONS, "<Primary>1...3");
	}

	bool loaded = false;
}
