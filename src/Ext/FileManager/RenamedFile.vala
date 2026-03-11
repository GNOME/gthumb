public class RenamedFile : Object {
	public File old_file;
	public File new_file;

	public RenamedFile (File _old, File _new) {
		old_file = _old;
		new_file = _new;
	}
}
