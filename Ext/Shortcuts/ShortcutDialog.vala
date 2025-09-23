public class Gth.ReadShortcut : Object {
	public async ShortcutKey? read_accelerator (Gtk.Window? parent, Job job) throws Error {
		callback = read_accelerator.callback;
		dialog = new ShortcutDialog ();
		dialog.saved.connect (() => {
			result = dialog.key;
			dialog.close ();
		});
		dialog.closed.connect (() => {
			if (callback != null) {
				Idle.add ((owned) callback);
				callback = null;
			}
		});
		cancelled_event = job.cancellable.cancelled.connect (() => {
			dialog.close ();
		});
		job.opens_dialog ();
		dialog.present (parent);
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
	ShortcutDialog dialog = null;
	ulong cancelled_event = 0;
	ShortcutKey? result = null;
}

[GtkTemplate (ui = "/app/gthumb/gthumb/ui/shortcut-dialog.ui")]
class Gth.ShortcutDialog : Adw.Dialog {
	public signal void saved ();
	public ShortcutKey key;

	public ShortcutDialog () {
		var key_events = new Gtk.EventControllerKey ();
		key_events.key_pressed.connect (on_key_pressed);
		main_view.add_controller (key_events);
		main_view.grab_focus ();
	}

	public bool on_key_pressed (Gtk.EventControllerKey controller, uint keyval, uint keycode, Gdk.ModifierType state) {
		if (keyval == Gdk.Key.Escape) {
			return false;
		}
		keyval = Shortcut.normalize_keycode (keyval);
		state = state & Gtk.accelerator_get_default_mod_mask ();
		if (!Shortcut.get_is_valid (keyval, state)) {
			return false;
		}
		key = ShortcutKey (keyval, state);
		saved ();
		return true;
	}

	[GtkChild] unowned Adw.ToolbarView main_view;
}
