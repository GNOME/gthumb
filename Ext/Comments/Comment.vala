public class Gth.Comment : Object {
	public string caption;
	public string note;
	public string place;
	public int rating;
	public GenericArray<string> categories;
	public Gth.DateTime time;

	public Comment () {
		reset ();
	}

	public Comment.from_info (FileInfo info) {
		reset ();

		var metadata = info.get_attribute_object ("Metadata::Title") as Gth.Metadata;
		if (metadata != null) {
			caption = metadata.raw;
		}

		metadata = info.get_attribute_object ("Metadata::Description") as Gth.Metadata;
		if (metadata != null) {
			note = metadata.raw;
		}

		metadata = info.get_attribute_object ("Metadata::Location") as Gth.Metadata;
		if (metadata != null) {
			place = metadata.raw;
		}

		metadata = info.get_attribute_object ("Metadata::DateTime") as Gth.Metadata;
		if (metadata != null) {
			time.set_from_exif_date (metadata.raw);
		}

		metadata = info.get_attribute_object ("Metadata::Tags") as Gth.Metadata;
		if (metadata != null) {
			categories = new GenericArray<string>();
			unowned var list = metadata.string_list.get_list ();
			foreach (unowned var str in list) {
				categories.add (str);
			}
		}

		metadata = info.get_attribute_object ("Metadata::Rating") as Gth.Metadata;
		if (metadata != null) {
			int.try_parse (metadata.raw, out rating, null, 10);
		}
	}

	public void load_doc (Dom.Document doc) {
		reset ();

		var root = doc.first_child;
		if (root.tag_name != "comment")
			return;

		unowned var version = root.get_attribute ("version");
		if (version == "2.0") {
			// TODO
		}
		else if (version == "3.0") {
			foreach (unowned var child in root) {
				switch (child.tag_name) {
				case "caption":
					caption = child.get_inner_text ();
					break;

				case "note":
					note = child.get_inner_text ();
					break;

				case "place":
					place = child.get_inner_text ();
					break;

				case "time":
					var exif_time = DateTime.get_from_exif_date (child.get_attribute ("value"));
					if (exif_time != null) {
						time.copy (exif_time);
					}
					break;

				case "rating":
					int value;
					if (int.try_parse (child.get_attribute ("value"), out value, null, 10)) {
						rating = value.clamp (0, 5);
					}
					break;

				case "categories":
					foreach (unowned var category in child) {
						categories.add (category.get_attribute ("value"));
					}
					break;
				}
			}
		}
	}

	public static File? get_comment_file (File file) {
		var parent = file.get_parent ();
		if (parent == null)
			return null;
		var comment_dir = Comment.get_comment_destination (parent);
		return comment_dir.get_child (file.get_basename () + ".xml");
	}

	public static File get_comment_destination (File destination) {
		return destination.get_child (".comments");
	}

	public void load_bytes (Bytes bytes) throws Error {
		unowned var buffer = bytes.get_data ();
		uint8[] unzipped_buffer;
		if (buffer[0] != '<') {
			if (!Zip.decompress (buffer, out unzipped_buffer)) {
				throw new IOError.INVALID_DATA ("Could not unzip the content");
			}
			buffer = unzipped_buffer;
		}
		var doc = new Dom.Document ();
		doc.load_xml ((string) buffer);
		load_doc (doc);
	}

	public string to_xml () {
		var doc = new Dom.Document ();
		var root = new Dom.Element.with_attributes ("comment", "version", COMMENT_VERSION);
		doc.append_child (root);
		root.append_child (new Dom.Element.with_text ("caption", caption));
		root.append_child (new Dom.Element.with_text ("note", note));
		root.append_child (new Dom.Element.with_text ("place", place));
		if (rating > 0) {
			root.append_child (new Dom.Element.with_attributes ("rating", "value", "%d".printf (rating)));
		}
		if (time.date_is_valid ()) {
			root.append_child (new Dom.Element.with_attributes ("time", "value", time.to_exif_date ()));
		}
		var categories_node = new Dom.Element ("categories");
		root.append_child (categories_node);
		foreach (unowned var category in categories) {
			categories_node.append_child (new Dom.Element.with_attributes ("category", "value", category));
		}
		return doc.to_xml ();
	}

	public void update_info (FileInfo info) {
		if (!Strings.empty (note)) {
			info.set_attribute_object ("Metadata::Description", new Metadata.for_string (note));
		}

		if (!Strings.empty (caption)) {
			info.set_attribute_object ("Metadata::Title", new Metadata.for_string (caption));
		}

		if (!Strings.empty (place)) {
			info.set_attribute_object ("Metadata::Location", new Metadata.for_string (place));
		}

		if (rating > 0) {
			info.set_attribute_object ("Metadata::Rating", new Metadata.for_string ("%d".printf (rating)));
		}
		else {
			info.remove_attribute ("Metadata::Rating");
		}

		if (categories.length > 0) {
			var list = new StringList.from_array (categories);
			var metadata = new Metadata.for_string_list (list);
			info.set_attribute_object ("Metadata::Tags", metadata);
		}
		else {
			info.remove_attribute ("Metadata::Tags");
		}

		if (time.date_is_valid ()) {
			var raw = time.to_exif_date ();
			var formatted = time.to_display_string ();
			var metadata = new Metadata.for_string (raw, formatted);
			info.set_attribute_object ("Metadata::DateTime", metadata);
		}
		else {
			info.remove_attribute ("Metadata::DateTime");
		}
	}

	void reset () {
		caption = null;
		note = null;
		place = null;
		rating = 0;
		categories = new GenericArray<string>();
		time = new Gth.DateTime ();
	}

	const string COMMENT_VERSION = "3.0";
}
