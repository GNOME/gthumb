public interface Gth.ImageController : Object {
	public virtual void on_realize () {}
	public virtual void on_unrealize () {}
	public virtual void on_size_allocated () {}
	public abstract void set_view (ImageView? view);
	public abstract void on_snapshot (Gtk.Snapshot snapshot);
}
