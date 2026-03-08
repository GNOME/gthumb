public class Gth.WindowState {
	public Gth.Window.Page page;
	public File location;
	public File file;
	public GenericList<File> selected;
	public double vscroll;

	public WindowState (Gth.Window window) {
		page = window.current_page;
		if (window.browser.folder_tree.current_folder != null) {
			location = window.browser.folder_tree.current_folder.file;
		}
		if (window.viewer.current_file != null) {
			file = window.viewer.current_file.file;
		}
		selected = window.browser.file_grid.get_selected_files ();
		vscroll = window.browser.file_grid.view.vadjustment.value;
	}
}
