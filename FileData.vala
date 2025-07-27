public class Gth.FileData : Object {
	public File file;
	public FileInfo info;
	public GenericList<FileData> children;
	public bool children_loaded;

	public FileData (File _file, FileInfo? _info = null) {
		file = _file;
		info = (_info != null) ? _info : new FileInfo ();
		children = null;
		children_loaded = false;
	}

	public FileData.copy (FileData other) {
		this (other.file, other.info);
	}

	public FileData.for_uri (string uri, string mime_type) {
		this (File.new_for_uri (uri), new FileInfo ());
		unowned var static_mime_type = Strings.get_static (mime_type);
		info.set_attribute_string (FileAttribute.STANDARD_CONTENT_TYPE, static_mime_type);
		info.set_attribute_string (FileAttribute.STANDARD_FAST_CONTENT_TYPE, static_mime_type);
	}

	public static uint hash (FileData file_data) {
		return file_data.file.hash ();
	}

	public static bool equal (FileData file1, FileData file2) {
		return file1.file.equal (file2.file);
	}

	public void set_file (File _file) {
		file = _file;
		sort_key = null;
	}

	public void update_info (FileInfo _info) {
		// Keep the STANDARD_CONTENT_TYPE if not included in the new info.
		string content_type = null;
		if (!_info.has_attribute (FileAttribute.STANDARD_CONTENT_TYPE)) {
			content_type = info.get_attribute_string (FileAttribute.STANDARD_CONTENT_TYPE);
		}
		info = _info;
		if (content_type != null) {
			info.set_attribute_string (FileAttribute.STANDARD_CONTENT_TYPE, content_type);
			info.set_attribute_string ("gth::file::content-type", Util.format_content_type (content_type));
		}
		info_changed ();
	}

	string sort_key = null;

	public unowned string get_filename_sort_key () {
		if (sort_key == null) {
			var name = info.get_display_name ();
			sort_key = (name != null) ? name.collate_key_for_filename () : "";
		}
		return sort_key;
	}

	public void info_changed () {
		// Invalidate cached data.
		mtime = null;
		ctime = null;
		dtime = null;
		embedded_otime = null;
		embedded_title = null;
		embedded_description = null;
		embedded_rating = -1;
		icon_name = null;
		app.monitor.metadata_changed (this);
	}

	Gth.DateTime mtime = null;

	public unowned Gth.DateTime get_modification_time () {
		if (mtime == null) {
			mtime = new Gth.DateTime ();
			var time = info.get_modification_date_time ();
			if (time != null)
				mtime.set_from_gdatetime (time);
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
				ctime.set_from_gdatetime (time);
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
				if (dtime.set_from_exif_date (metadata.get_raw ()))
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
				var time = Util.get_time_from_exif_date (metadata.get_raw ());
				if (time != null)
					embedded_otime.copy (time);
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
				embedded_title = metadata.get_formatted ();
			}
		}
		return embedded_title;
	}

	string embedded_description = null;

	public unowned string get_embedded_description () {
		if (embedded_description == null) {
			var metadata = info.get_attribute_object ("general::description") as Gth.Metadata;
			if (metadata != null) {
				embedded_description = metadata.get_formatted ();
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
				if (int.try_parse (metadata.get_raw (), out value, null, 10)) {
					embedded_rating = value;
				}
			}
			if (embedded_rating == -1) {
				embedded_rating = 0;
			}
		}
		return embedded_rating;
	}

	string get_first_available_attribute (string id_list) {
		if (info.has_attribute (id_list)) {
			return id_list;
		}
		if (id_list.index_of_char (',') < 0) {
			return null;
		}
		var list = id_list.split (",");
		foreach (unowned var id in list) {
			if (info.has_attribute (id)) {
				return id;
			}
		}
		return null;
	}

	public bool has_attribute (string id_list) {
		var id = get_first_available_attribute (id_list);
		return id != null;
	}

	public string get_attribute_as_string (string id_list) {
		var id = get_first_available_attribute (id_list);
		if (id == null) {
			return null;
		}
		string value = null;
		if (info.get_attribute_type (id) == FileAttributeType.OBJECT) {
			var obj = info.get_attribute_object (id);
			if (obj is Metadata) {
				var metadata = obj as Metadata;
				switch (metadata.get_data_type ()) {
				case MetadataType.STRING:
					if (id == "general::rating") {
						int n;
						if (int.try_parse (metadata.get_raw (), out n, null, 10)) {
							var str = new StringBuilder ();
							while (n > 0) {
								str.append ("⭐");
								n--;
							}
							value = str.str;
						}
					}
					if (value == null) {
						value = metadata.get_formatted ();
					}
					break;
				case MetadataType.STRING_LIST:
					value = metadata.get_string_list ().join (" ");
					break;

				case MetadataType.POINT:
					value = metadata.get_formatted ();
					break;
				}
			}
			else if (obj is StringList) {
				value = (obj as StringList).join (" ");
			}
		}
		if (value == null) {
			value = info.get_attribute_as_string (id);
		}
		return value;
	}

	public bool is_readable () {
		return info.get_attribute_boolean (FileAttribute.ACCESS_CAN_READ);
	}

	public void set_content_type (string type) {
		info.set_attribute_string (FileAttribute.STANDARD_CONTENT_TYPE, type);
		app.monitor.metadata_changed (this);
	}

	public unowned string get_content_type () {
		unowned var result = info.get_attribute_string (FileAttribute.STANDARD_CONTENT_TYPE);
		if (result == null) {
			result = info.get_attribute_string (FileAttribute.STANDARD_FAST_CONTENT_TYPE);
		}
		if (result == null) {
			var guessed_type = Util.guess_content_type_from_name (file.get_basename ());
			info.set_attribute_string (FileAttribute.STANDARD_CONTENT_TYPE, guessed_type);
			result = guessed_type;
		}
		return Strings.get_static (result);
	}

	public Gth.Image? thumbnail_image = null;

	public Gdk.Paintable? thumbnail_texture { get; set; default = null; }

	public void set_thumbnail (Gth.Image image) {
		thumbnail_image = image;
		thumbnail_texture = thumbnail_image.get_texture ();
		thumbnail_state = ThumbnailState.LOADED;
	}

	public void remove_thumbnail () {
		thumbnail_image = null;
		thumbnail_texture = null;
		thumbnail_state = ThumbnailState.ICON;
	}

	public ThumbnailState thumbnail_state { get; set; default = ThumbnailState.ICON; }

	public void set_thumbnail_loading () {
		thumbnail_state = ThumbnailState.LOADING;
	}

	public void set_thumbnail_broken () {
		thumbnail_state = ThumbnailState.BROKEN;
	}

	public GenericList<FileData> get_children_model () {
		if (children == null) {
			children = new GenericList<FileData>();
		}
		return children;
	}

	unowned string icon_name = null;

	public Icon get_symbolic_icon () {
		if (info.has_attribute (FileAttribute.STANDARD_SYMBOLIC_ICON)) {
			return info.get_symbolic_icon () as Icon;
		}
		if (icon_name == null) {
			icon_name = "text-x-generic-symbolic";
			if (Util.content_type_is_image (get_content_type ())) {
				icon_name = Strings.get_static ("image-x-generic-symbolic");
			}
			else if (Util.content_type_is_video (get_content_type ())) {
				icon_name = Strings.get_static ("video-x-generic-symbolic");
			}
			else if (Util.content_type_is_audio (get_content_type ())) {
				icon_name = Strings.get_static ("audio-x-generic-symbolic");
			}
		}
		return Util.get_themed_icon (icon_name) as Icon;
	}

	public unowned string? get_sort_name (string? default_value) {
		if (!info.has_attribute ("sort::type")) {
			return default_value;
		}
		return info.get_attribute_string ("sort::type");
	}

	public bool get_inverse_order (bool default_value) {
		if (!info.has_attribute ("sort::inverse")) {
			return default_value;
		}
		return info.get_attribute_boolean ("sort::inverse");
	}

	public unowned string? get_sort_attributes () {
		if (info.has_attribute ("sort::type")) {
			var sort_name = info.get_attribute_string ("sort::type");
			if (sort_name != null) {
				var sort_info = app.get_sorter_by_id (sort_name);
				if (sort_info != null) {
					if (!Strings.empty (sort_info.required_attributes)) {
						return sort_info.required_attributes;
					}
				}
			}
		}
		return null;
	}
}
