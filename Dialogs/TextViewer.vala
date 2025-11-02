public class Gth.TextViewer : Object {
	public async void view_buffer (Gtk.Window parent, Bytes buffer, Job job) throws Error {
		callback = view_buffer.callback;
		dialog = new TextViewerDialog (buffer);
		dialog.closed.connect (() => {
			if (callback != null) {
				Idle.add ((owned) callback);
				callback = null;
			}
		});
		if (job.cancellable != null) {
			cancelled_event = job.cancellable.cancelled.connect (() => {
				dialog.close ();
			});
		}
		job.opens_dialog ();
		dialog.present (parent);
		yield;
		job.dialog_closed ();
		if (cancelled_event != 0) {
			job.cancellable.disconnect (cancelled_event);
			cancelled_event = 0;
		}
	}

	SourceFunc callback = null;
	TextViewerDialog dialog = null;
	ulong cancelled_event = 0;
}

[GtkTemplate (ui = "/app/gthumb/gthumb/ui/text-viewer-dialog.ui")]
class Gth.TextViewerDialog : Adw.Dialog {
	public TextViewerDialog (Bytes buffer) {
		var text = (string) buffer.get_data ();
		text_view.buffer.set_text (text, (int) buffer.get_size ());
	}

	[GtkChild] private unowned Gtk.TextView text_view;
}
