public interface Gth.FileViewer : Object {
	public abstract void activate (Gth.Window window);
	public abstract async void load (FileData file) throws Error;
	public abstract void deactivate ();
	public abstract void save_preferences ();
	public abstract void release_resources ();
	public abstract void show ();
	public abstract void hide ();
	public abstract bool on_scroll (double x, double y, double dx, double dy);
	public abstract bool get_pixel_size (out uint width, out uint height);
	//public bool can_save ();
	//public async void save_async () throws Error;
}
