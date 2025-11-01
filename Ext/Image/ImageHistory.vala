public class Gth.ImageHistory {
	public signal void changed ();

	public bool can_undo {
		get {
			return current > 0;
		}
	}

	public bool can_redo {
		get {
			return current < images.length - 1;
		}
	}

	public ImageHistory () {
		images = new GenericArray<Image>();
		saved = -1;
		current = -1;
	}

	public void add (Image image, bool is_modified) {
		if (current >= 0) {
			images.length = current + 1;
		}
		if (saved >= images.length) {
			saved = -1;
		}
		if (images.length == MAX_LENGTH - 1) {
			images.remove_index (0);
			current--;
			saved--;
		}
		images.add (image);
		current = images.length - 1;
		if (!is_modified) {
			saved = images.length - 1;
		}
		changed ();
	}

	public Image? undo (out bool is_modified) {
		if (current <= 0) {
			is_modified = false;
			return null;
		}
		current--;
		is_modified = current != saved;
		changed ();
		return images[current];
	}

	public Image? redo (out bool is_modified) {
		if (current >= images.length - 1) {
			is_modified = false;
			return null;
		}
		current++;
		is_modified = current != saved;
		changed ();
		return images[current];
	}

	public void clear () {
		images.length = 0;
		current = -1;
		saved = -1;
	}

	public void current_was_saved () {
		saved = current;
		changed ();
	}

	public Image revert () {
		var saved_image = ((saved >= 0) && (saved < images.length - 1)) ? images[saved] : null;
		clear ();
		return saved_image;
	}

	public Image? last () {
		return (images.length > 0) ? images[images.length - 1] : null;
	}

	GenericArray<Image> images;
	int saved;
	int current;

	const uint MAX_LENGTH = 5;
}
