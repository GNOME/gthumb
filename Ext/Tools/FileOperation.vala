public abstract class Gth.FileOperation {
	public OverwriteResponse overwrite_response;
	public bool single_file;
	public abstract async void execute (Gth.MainWindow window, File file, Gth.Job job) throws Error;
}
