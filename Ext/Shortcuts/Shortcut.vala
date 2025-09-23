public class Gth.Shortcut : Object {
	public signal void changed ();

	public string action_name;
	public string description;
	public string detailed_action;
	public ShortcutContext context;
	public ShortcutCategory category;
	public string default_accelerator;
	public uint keyval;
	public Gdk.ModifierType modifiers;
	public Variant action_parameter;
	public string accelerator;
	public string accelerator_label;

	public Shortcut (string _action_name, Variant? _action_parameter = null) {
		action_name = _action_name;
		action_parameter = _action_parameter;
		detailed_action = Action.print_detailed_name (action_name, action_parameter);
	}

	public Shortcut.from_detailed_action (string _detailed_action) {
		try {
			detailed_action = _detailed_action;
			Action.parse_detailed_name (detailed_action, out action_name, out action_parameter);
		}
		catch (Error error) {
			action_name = null;
			action_parameter = null;
		}
	}

	public void set_accelerator (string? value) {
		if (value == null) {
			keyval = 0;
			modifiers = 0;
			accelerator = null;
			accelerator_label = "";
		}
		else if (!(ShortcutContext.DOC in context)) {
			if (value != null) {
				Gtk.accelerator_parse (value, out keyval, out modifiers);
				keyval = Shortcut.normalize_keycode (keyval);
			}
			else {
				keyval = 0;
				modifiers = 0;
			}
			accelerator = Gtk.accelerator_name (keyval, modifiers);
			accelerator_label = Gtk.accelerator_get_label (keyval, modifiers);
		}
		else {
			keyval = 0;
			modifiers = 0;
			accelerator = value;
			accelerator_label = accelerator;
		}
		changed ();
	}

	public void set_key (ShortcutKey key) {
		keyval = key.keyval;
		modifiers = key.state;
		accelerator = Gtk.accelerator_name (keyval, modifiers);
		accelerator_label = Gtk.accelerator_get_label (keyval, modifiers);
		changed ();
	}

	public bool get_is_customizable () {
		return ((context & ShortcutContext.FIXED) == 0)
			&& ((context & ShortcutContext.DOC) == 0);
	}

	public bool get_is_modified () {
		return accelerator != default_accelerator;
	}

	public bool match_filter (string? text) {
		if (text == null) {
			return true;
		}
		return description.down ().index_of (text) >= 0;
	}

	public static bool get_is_valid (uint keycode, Gdk.ModifierType modifiers) {
		switch (keycode) {
		case Gdk.Key.Escape:
		case Gdk.Key.Tab:
		case Gdk.Key.KP_Tab:
			return false;

		// These shortcuts are valid for us but Gtk.accelerator_valid
		// considers them not valid, hence they are added here
		// explicitly.

		case Gdk.Key.Left:
		case Gdk.Key.Right:
		case Gdk.Key.Up:
		case Gdk.Key.Down:
		case Gdk.Key.KP_Left:
		case Gdk.Key.KP_Right:
		case Gdk.Key.KP_Up:
		case Gdk.Key.KP_Down:
			return true;

		default:
			break;
		}

		return Gtk.accelerator_valid (keycode, modifiers);
	}

	public static uint normalize_keycode (uint keycode) {
		switch (keycode) {
		case Gdk.Key.KP_Page_Up:
			keycode = Gdk.Key.Page_Up;
			break;

		case Gdk.Key.KP_Page_Down:
			keycode = Gdk.Key.Page_Down;
			break;

		case Gdk.Key.KP_Begin:
			keycode = Gdk.Key.Begin;
			break;

		case Gdk.Key.KP_End:
			keycode = Gdk.Key.End;
			break;

		case Gdk.Key.KP_Subtract:
			keycode = Gdk.Key.minus;
			break;

		case Gdk.Key.KP_Add:
			keycode = Gdk.Key.plus;
			break;

		case Gdk.Key.KP_Divide:
			keycode = Gdk.Key.slash;
			break;

		case Gdk.Key.KP_Multiply:
			keycode = Gdk.Key.asterisk;
			break;

		case Gdk.Key.KP_Decimal:
			keycode = Gdk.Key.decimalpoint;
			break;

		case Gdk.Key.KP_Left:
			keycode = Gdk.Key.Left;
			break;

		case Gdk.Key.KP_Right:
			keycode = Gdk.Key.Right;
			break;

		case Gdk.Key.KP_Up:
			keycode = Gdk.Key.Up;
			break;

		case Gdk.Key.KP_Down:
			keycode = Gdk.Key.Down;
			break;

		case Gdk.Key.KP_Enter:
			keycode = Gdk.Key.Return;
			break;

		case Gdk.Key.KP_Insert:
			keycode = Gdk.Key.Insert;
			break;

		case Gdk.Key.KP_Delete:
			keycode = Gdk.Key.Delete;
			break;
		}
		return keycode;
	}
}

[Flags]
public enum Gth.ShortcutContext {
	NONE,
	DOC, // Not handled by the application, specified for documentation only.
	FIXED, // Not customizable.
	BROWSER,
	IMAGE_VIEWER,
	MEDIA_VIEWER,
	IMAGE_EDITOR,
	SLIDESHOW,
}

const Gth.ShortcutContext SHORTCUT_CONTEXT_BROWSER_VIEWER = (
	Gth.ShortcutContext.BROWSER |
	Gth.ShortcutContext.IMAGE_VIEWER |
	Gth.ShortcutContext.MEDIA_VIEWER);

const Gth.ShortcutContext SHORTCUT_CONTEXT_VIEWERS = (
	Gth.ShortcutContext.IMAGE_VIEWER |
	Gth.ShortcutContext.MEDIA_VIEWER);

const Gth.ShortcutContext SHORTCUT_CONTEXT_ANY = (
	Gth.ShortcutContext.BROWSER |
	Gth.ShortcutContext.IMAGE_VIEWER |
	Gth.ShortcutContext.MEDIA_VIEWER |
	Gth.ShortcutContext.IMAGE_EDITOR |
	Gth.ShortcutContext.SLIDESHOW);

public enum Gth.ShortcutCategory {
	HIDDEN,
	GENERAL,
	VIEWER,
	IMAGE_VIEWER,
	SCROLL_IMAGE,
	SLIDESHOW,
	MEDIA_VIEWER,
	IMAGE_EDITOR,
	UI,
	NAVIGATION,
	FILE_MANAGER,
	FILTERS,
	SCRIPTS,
	SELECTIONS;

	public string to_title () {
		return TITLE[this];
	}

	public static const string[] TITLE = {
		N_("Hidden"),
		N_("General"),
		N_("Viewer"),
		N_("Image Viewer"),
		N_("Image Scrolling"),
		N_("Slideshow"),
		N_("Media Viewer"),
		N_("Image Editor"),
		N_("UI"),
		N_("Navigation"),
		N_("File Manager"),
		N_("Filters"),
		N_("Scripts"),
		N_("Selections"),
	};
}

public struct Gth.ShortcutKey {
	uint keyval;
	Gdk.ModifierType state;

	public ShortcutKey (uint _keyval, Gdk.ModifierType _state) {
		keyval = _keyval;
		state = _state;
	}
}
