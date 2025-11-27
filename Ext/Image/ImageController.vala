public interface Gth.ImageController : Object {
	public abstract void on_realize ();
	public abstract void on_unrealize ();
	public abstract void on_size_allocated ();
	public abstract void on_snapshot (Gtk.Snapshot snapshot);
}
