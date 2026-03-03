public interface Gth.FileViewer : Object {
	public abstract Gth.ShortcutContext shortcut_context { get; }
	public abstract void activate (Gth.Window window);
	public abstract async bool load (FileData file, Job job) throws Error;
	public abstract void deactivate ();
	public abstract void save_preferences ();
	public abstract void release_resources ();
	public abstract bool on_scroll (double dx, double dy, Gdk.ModifierType state);
	public abstract bool get_pixel_size (out uint width, out uint height);
	public virtual void adapt_content_to_window () {}
	public virtual async void save () throws Error {}
	public virtual void update_sensitivity () {}
	public abstract void focus ();
	public virtual bool same_etag (FileInfo info) {
		return false;
	}
	public virtual void preload_some_files () { /* void */ }
}
