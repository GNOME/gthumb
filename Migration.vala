public class Gth.Migration {
	public MigrationMap sorter;
	public MigrationMap metadata;
	public MigrationMap test;

	public Migration () {
		sorter = new MigrationMap ();
		sorter.add ("file::name", "File::Name");
		sorter.add ("file::mtime", "Time::Modified");
		sorter.add ("file::size", "File::Size");
		sorter.add ("file::path", "File::Path");
		sorter.add ("general::dimensions", "Frame::Pixels");
		sorter.add ("frame::aspect-ratio", "Frame::AspectRatio");
		sorter.add ("general::unsorted", "Private::Unsorted");

		metadata = new MigrationMap ();
		metadata.add ("gth::file::content-type", "Private::File::ContentType");
		metadata.add ("gth::file::display-size", "Private::File::DisplaySize");
		metadata.add ("gth::file::display-mtime", "Private::File::DisplayMtime");
		metadata.add ("gth::file::location", "Private::File::Location");
		metadata.add ("gth::file::is-modified", "Loaded::Image::IsModified");
		metadata.add ("general::dimensions", "Frame::Pixels");
		metadata.add ("general::duration", "Metadata::Duration");
		metadata.add ("general::title", "Metadata::Title");
		metadata.add ("general::description", "Metadata::Description");
		metadata.add ("general::location", "Metadata::Location");
		metadata.add ("general::datetime", "Metadata::DateTime");
		metadata.add ("general::tags", "Metadata::Tags");
		metadata.add ("general::rating", "Metadata::Rating");
		metadata.add ("gth::file::emblems", "Private::File::Emblems");
		metadata.add ("gth::standard::secondary-sort-order", "Private::SecondarySortOrder");
		metadata.add ("image::width", "Frame::Width");
		metadata.add ("image::height", "Frame::Height");
		metadata.add ("frame::width", "Frame::Width");
		metadata.add ("frame::height", "Frame::Height");
		metadata.add ("Embedded::Image::Orientation", "Embedded::Photo::Orientation");

		test = new MigrationMap ();
		test.add ("file::is_visible", "Permission::Visible");
		test.add ("file::type::is_file", "Type::File");
		test.add ("file::type::is_image", "Type::Image");
		test.add ("file::type::is_jpeg", "Type::Jpeg");
		test.add ("file::type::is_raw", "Type::Raw");
		test.add ("file::type::is_video", "Type::Video");
		test.add ("file::type::is_audio", "Type::Audio");
		test.add ("file::type::is_media", "Type::Media");
		test.add ("file::type::is_text", "Type::Text");
		test.add ("file::name", "File::Name");
		test.add ("file::size", "File::Size");
		test.add ("file::ctime", "Time::Created");
		test.add ("file::mtime", "Time::Modified");
		test.add ("general::title", "Metadata::Title");
		test.add ("general::description", "Metadata::Description");
		test.add ("general::rating", "Metadata::Rating");
		test.add ("general::tags", "Metadata::Tags");
		test.add ("frame::aspect-ratio", "Frame::AspectRatio");
	}
}

public class Gth.MigrationMap {
	public MigrationMap () {
		old_to_new = new HashTable<string, string> (str_hash, str_equal);
		new_to_old = new HashTable<string, string> (str_hash, str_equal);
	}

	public void add (string old_key, string new_key) {
		old_to_new[old_key] = new_key;
		new_to_old[new_key] = old_key;
	}

	public unowned string get_new_key (string key) {
		unowned var other_key = old_to_new[key];
		if (other_key == null) {
			other_key = key;
		}
		return other_key;
	}

	public unowned string get_old_key (string key) {
		unowned var other_key = new_to_old[key];
		if (other_key == null) {
			other_key = key;
		}
		return other_key;
	}

	public string[] get_new_key_v (string? keys) {
		if (keys == null) {
			return {};
		}
		string[] result = {};
		var key_v = keys.split (",");
		foreach (unowned var key in key_v) {
			result += app.migration.metadata.get_new_key (key);
		}
		return result;
	}

	HashTable<string, string> old_to_new;
	HashTable<string, string> new_to_old;
}
