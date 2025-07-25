public interface Gth.FileViewer : Object {
	public abstract void activate (Gth.Window window);
	public abstract async void load (FileData file) throws Error;
	public abstract void deactivate ();
	public abstract void show ();
	public abstract void hide ();
	//public bool can_save ();
	//public async void save_async () throws Error;
}
