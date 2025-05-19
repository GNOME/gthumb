public class Gth.TestFileType : Gth.Test {
	// void
}

public class Gth.TestFileTypeRegular : Gth.TestFileType {
	construct {
		id = "file::type::is_file";
		display_name = _("All Files");
		attributes = null;
	}

	public override bool match (FileData file) {
		return true;
	}
}

public class Gth.TestFileTypeImage : Gth.TestFileType {
	construct {
		id = "file::type::is_image";
		display_name = _("All Images");
		attributes = null;
	}

	public override bool match (FileData file) {
		return Util.content_type_is_image (file.info.get_content_type ());
	}
}

public class Gth.TestFileTypeJpeg : Gth.TestFileType {
	construct {
		id = "file::type::is_jpeg";
		display_name = _("JPEG Images");
		attributes = null;
	}

	public override bool match (FileData file) {
		return ContentType.equals (file.info.get_content_type (), "image/jpeg");
	}
}

public class Gth.TestFileTypeRaw : Gth.TestFileType {
	construct {
		id = "file::type::is_raw";
		display_name = _("Raw Photos");
		attributes = null;
	}

	public override bool match (FileData file) {
		return Util.content_type_is_raw (file.info.get_content_type ());
	}
}

public class Gth.TestFileTypeVideo : Gth.TestFileType {
	construct {
		id = "file::type::is_video";
		display_name = _("Video Files");
		attributes = null;
	}

	public override bool match (FileData file) {
		return Util.content_type_is_video (file.info.get_content_type ());
	}
}

public class Gth.TestFileTypeAudio : Gth.TestFileType {
	construct {
		id = "file::type::is_audio";
		display_name = _("Audio Files");
		attributes = null;
	}

	public override bool match (FileData file) {
		return Util.content_type_is_audio (file.info.get_content_type ());
	}
}

public class Gth.TestFileTypeMedia : Gth.TestFileType {
	construct {
		id = "file::type::is_media";
		display_name = _("Media Files");
		attributes = null;
	}

	public override bool match (FileData file) {
		unowned var content_type = file.info.get_content_type ();
		return Util.content_type_is_image (content_type)
			 || Util.content_type_is_audio (content_type)
			 || Util.content_type_is_video (content_type);
	}
}

public class Gth.TestFileTypeText : Gth.TestFileType {
	construct {
		id = "file::type::is_text";
		display_name = _("Text Files");
		attributes = null;
	}

	public override bool match (FileData file) {
		return ContentType.is_a (file.info.get_content_type (), "text/*");
	}
}

public class Gth.TestFileName : Gth.TestString {
	construct {
		id = "file::name";
		display_name = _("Filename");
		attributes = "standard::display-name";
	}

	public override string get_file_value (FileData file) {
		return file.info.get_display_name ();
	}
}

public class Gth.TestFileSize : Gth.TestSize {
	construct {
		id = "file::size";
		display_name = _("Size");
		attributes = "gth::file::size";
	}

	public override uint64 get_file_value (FileData file) {
		return file.info.get_size ();
	}
}

public class Gth.TestFileModifiedTime : Gth.TestDate {
	construct {
		id = "file::mtime";
		display_name = _("File modified date");
		attributes = "time::modified,time::modified-usec";
	}

	public override Gth.DateTime? get_file_value (FileData file) {
		return file.get_modification_time ();
	}
}

public class Gth.TestTimeOriginal : Gth.TestDate {
	construct {
		id = "Embedded::Photo::DateTimeOriginal";
		display_name = _("Date photo was taken");
		attributes = "Embedded::Photo::DateTimeOriginal";
	}

	public override Gth.DateTime? get_file_value (FileData file) {
		return file.get_embedded_original_time ();
	}
}

public class Gth.TestTitleEmbedded : Gth.TestString {
	construct {
		id = "general::title";
		display_name = _("Title (embedded)");
		attributes = "general::title";
	}

	public override string get_file_value (FileData file) {
		return file.get_embedded_title ();
	}
}

public class Gth.TestDescriptionEmbedded : Gth.TestString {
	construct {
		id = "general::description";
		display_name = _("Description (embedded)");
		attributes = "general::description";
	}

	public override string get_file_value (FileData file) {
		return file.get_embedded_description ();
	}
}

public class Gth.TestRating : Gth.TestInt {
	construct {
		id = "general::rating";
		display_name = _("Rating");
		attributes = "general::rating";
	}

	public override int get_file_value (FileData file) {
		return file.get_embedded_rating ();
	}
}

public class Gth.TestTagEmbedded : Gth.Test {
	construct {
		id = "general::tags";
		display_name = _("Tag (embedded)");
		attributes = "general::tags";
	}
}

public class Gth.TestAspectRatio : Gth.Test {
	construct {
		id = "frame::aspect-ratio";
		display_name = _("Aspect ratio");
		attributes = "frame::width, frame::height";
	}
}
