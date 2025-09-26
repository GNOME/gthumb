public class Gth.FileSet {
	public signal void added (File file);
	public signal void removed (File file);

	public FileSet () {
		counted_files = new HashTable<File, Counter> (Files.hash, Files.equal);
	}

	public void add (File file) {
		var counter = counted_files.get (file);
		if (counter == null) {
			counter = new Counter ();
			counted_files.set (file, counter);
			added (file);
		}
		else {
			counter.increase ();
		}
	}

	public void remove (File file) {
		var counter = counted_files.get (file);
		if (counter != null) {
			if (counter.decrease () == 0) {
				counted_files.remove (file);
				removed (file);
			}
		}
	}

	public void remove_all () {
		foreach (unowned var file in counted_files.get_keys ()) {
			removed (file);
		}
		counted_files.remove_all ();
	}

	HashTable<File, Counter> counted_files;

	class Counter {
		public int count;

		public Counter () {
			count = 1;
		}

		public void increase () {
			count++;
		}

		public int decrease () {
			count--;
			return count;
		}
	}
}
