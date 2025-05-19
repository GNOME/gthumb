public class Gth.FileData {
	public File file;
	public FileInfo info;

	public FileData (File _file, FileInfo? _info = null) {
		file = _file;
		info = (_info != null) ? _info : new FileInfo ();
	}

	public FileData.for_uri (string uri, string mime_type) {
		file = File.new_for_uri (uri);
		info = new FileInfo ();
		unowned var static_mime_type = Strings.get_static (mime_type);
		info.set_attribute_string (FileAttribute.STANDARD_CONTENT_TYPE, static_mime_type);
		info.set_attribute_string (FileAttribute.STANDARD_FAST_CONTENT_TYPE, static_mime_type);
	}

	string sort_key = null;

	public unowned string get_filename_sort_key () {
		if (sort_key == null) {
			var name = info.get_display_name ();
			sort_key = (name != null) ? name.collate_key_for_filename () : "";
		}
		return sort_key;
	}

	Gth.DateTime mtime = null;

	public unowned Gth.DateTime get_modification_time () {
		if (mtime == null) {
			mtime = new Gth.DateTime ();
			var time = info.get_modification_date_time ();
			if (time != null)
				mtime.set_from_gtime (time);
			else
				mtime.clear ();
		}
		return mtime;
	}

	Gth.DateTime ctime = null;

	public unowned Gth.DateTime get_creation_time () {
		if (ctime == null) {
			ctime = new Gth.DateTime ();
			var time = info.get_creation_date_time ();
			if (time != null)
				ctime.set_from_gtime (time);
			else
				ctime.clear ();
		}
		return ctime;
	}

	const string[] DIGITALIZATION_TIME_TAGS = {
		"Exif::Photo::DateTimeOriginal",
		"Xmp::exif::DateTimeOriginal",
		"Exif::Photo::DateTimeDigitized",
		"Xmp::exif::DateTimeDigitized",
		"Xmp::xmp::CreateDate",
		"Xmp::photoshop::DateCreated",
		"Xmp::xmp::ModifyDate",
		"Xmp::xmp::MetadataDate"
	};

	Gth.DateTime dtime = null;

	public unowned Gth.DateTime get_digitalization_time () {
		if (dtime == null) {
			dtime = new Gth.DateTime ();
			foreach (unowned var tag in DIGITALIZATION_TIME_TAGS) {
				var metadata = info.get_attribute_object (tag) as Gth.Metadata;
				if (metadata == null)
					continue;
				if (dtime.set_from_exif_date (metadata.raw))
					break;
			}
		}
		return dtime;
	}

	Gth.DateTime embedded_otime = null;

	public unowned Gth.DateTime get_embedded_original_time () {
		if (embedded_otime == null) {
			embedded_otime = new Gth.DateTime ();
			var metadata = info.get_attribute_object ("Embedded::Photo::DateTimeOriginal") as Gth.Metadata;
			if (metadata != null) {
				var time = Util.get_time_from_exif_date (metadata.raw);
				if (time != null)
					embedded_otime.set_from_gtime (time);
				else
					embedded_otime.clear ();
			}
		}
		return embedded_otime;
	}

	string embedded_title = null;

	public unowned string get_embedded_title () {
		if (embedded_title == null) {
			var metadata = info.get_attribute_object ("general::title") as Gth.Metadata;
			if (metadata != null) {
				embedded_title = metadata.formatted;
			}
		}
		return embedded_title;
	}

	string embedded_description = null;

	public unowned string get_embedded_description () {
		if (embedded_description == null) {
			var metadata = info.get_attribute_object ("general::description") as Gth.Metadata;
			if (metadata != null) {
				embedded_description = metadata.formatted;
			}
		}
		return embedded_description;
	}

	int embedded_rating = -1;

	public int get_embedded_rating () {
		if (embedded_rating == -1) {
			var metadata = info.get_attribute_object ("general::rating") as Gth.Metadata;
			if (metadata != null) {
				int value;
				if (int.try_parse (metadata.raw, out value, null, 10)) {
					embedded_rating = value;
				}
			}
			if (embedded_rating == -1) {
				embedded_rating = 0;
			}
		}
		return embedded_rating;
	}

	public bool is_readable () {
		return info.get_attribute_boolean (FileAttribute.ACCESS_CAN_READ);
	}
}
