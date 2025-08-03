public class Gth.TestVisible : Gth.Test {
	construct {
		id = "Permission::Visible";
		display_name = _("Visible Files");
		attributes = FileAttribute.STANDARD_IS_HIDDEN;
	}

	public override bool match (FileData file) {
		return file.info.get_is_hidden () == false;
	}
}

public class Gth.TestFileType : Gth.Test {
	// void
}

public class Gth.TestFileTypeRegular : Gth.TestFileType {
	construct {
		id = "Type::File";
		display_name = _("All Files");
		attributes = null;
	}

	public override bool match (FileData file) {
		return file.info.get_file_type () == FileType.REGULAR;
	}
}

public class Gth.TestFileTypeImage : Gth.TestFileType {
	construct {
		id = "Type::Image";
		display_name = _("Images");
		attributes = null;
	}

	public override bool match (FileData file) {
		return Util.content_type_is_image (file.get_content_type ());
	}
}

public class Gth.TestFileTypeJpeg : Gth.TestFileType {
	construct {
		id = "Type::Jpeg";
		display_name = _("JPEG Images");
		attributes = null;
	}

	public override bool match (FileData file) {
		return ContentType.equals (file.get_content_type (), "image/jpeg");
	}
}

public class Gth.TestFileTypeRaw : Gth.TestFileType {
	construct {
		id = "Type::Raw";
		display_name = _("Raw Photos");
		attributes = null;
	}

	public override bool match (FileData file) {
		return Util.content_type_is_raw (file.get_content_type ());
	}
}

public class Gth.TestFileTypeVideo : Gth.TestFileType {
	construct {
		id = "Type::Video";
		display_name = _("Videos");
		attributes = null;
	}

	public override bool match (FileData file) {
		return Util.content_type_is_video (file.get_content_type ());
	}
}

public class Gth.TestFileTypeAudio : Gth.TestFileType {
	construct {
		id = "Type::Audio";
		display_name = _("Audio Files");
		attributes = null;
	}

	public override bool match (FileData file) {
		return Util.content_type_is_audio (file.get_content_type ());
	}
}

public class Gth.TestFileTypeMedia : Gth.TestFileType {
	construct {
		id = "Type::Media";
		display_name = _("Media Files");
		attributes = null;
	}

	public override bool match (FileData file) {
		unowned var content_type = file.get_content_type ();
		return Util.content_type_is_image (content_type)
			 || Util.content_type_is_audio (content_type)
			 || Util.content_type_is_video (content_type);
	}
}

public class Gth.TestFileTypeText : Gth.TestFileType {
	construct {
		id = "Type::Text";
		display_name = _("Text Files");
		attributes = null;
	}

	public override bool match (FileData file) {
		return ContentType.is_a (file.get_content_type (), "text/*");
	}
}

public class Gth.TestFileName : Gth.TestString {
	construct {
		id = "File::Name";
		display_name = _("Name");
		attributes = "standard::display-name";
	}

	public override string? get_file_value (FileData file) {
		return file.info.get_display_name ();
	}
}

public class Gth.TestFileSize : Gth.TestSize {
	construct {
		id = "File::Size";
		display_name = _("Size");
		attributes = "Private::File::Size";
	}

	public override uint64 get_file_value (FileData file) {
		return file.info.get_size ();
	}
}

public class Gth.TestFileCreatedTime : Gth.TestDate {
	construct {
		id = "Time::Created";
		display_name = _("Created");
		attributes = "time::created,time::created-usec";
	}

	public override Gth.DateTime? get_file_value (FileData file) {
		return file.get_creation_time ();
	}
}

public class Gth.TestFileModifiedTime : Gth.TestDate {
	construct {
		id = "Time::Modified";
		display_name = _("Modified");
		attributes = "time::modified,time::modified-usec";
	}

	public override Gth.DateTime? get_file_value (FileData file) {
		return file.get_modification_time ();
	}
}

public class Gth.TestTimeOriginal : Gth.TestDate {
	construct {
		id = "Embedded::Photo::DateTimeOriginal";
		display_name = _("Created (Original)");
		attributes = "Embedded::Photo::DateTimeOriginal";
	}

	public override Gth.DateTime? get_file_value (FileData file) {
		return file.get_embedded_original_time ();
	}
}

public class Gth.TestTitleEmbedded : Gth.TestString {
	construct {
		id = "Metadata::Title";
		display_name = _("Title (Embedded)");
		attributes = "Metadata::Title";
	}

	public override string? get_file_value (FileData file) {
		return file.get_embedded_title ();
	}
}

public class Gth.TestDescriptionEmbedded : Gth.TestString {
	construct {
		id = "Metadata::Description";
		display_name = _("Description (Embedded)");
		attributes = "Metadata::Description";
	}

	public override string? get_file_value (FileData file) {
		return file.get_embedded_description ();
	}
}

public class Gth.TestRating : Gth.TestInt {
	construct {
		id = "Metadata::Rating";
		display_name = _("Rating");
		attributes = "Metadata::Rating";
	}

	public override int get_file_value (FileData file) {
		return file.get_embedded_rating ();
	}
}

public class Gth.TestTagEmbedded : Gth.Test {
	construct {
		id = "Metadata::Tags";
		display_name = _("Tag (Embedded)");
		attributes = "Metadata::Tags";
	}
}

public class Gth.TestFrameAspectRatio : Gth.TestAspectRatio {
	construct {
		id = "Frame::AspectRatio";
		display_name = _("Aspect Ratio");
		attributes = "Frame::Width, Frame::Height";
	}

	public override float get_file_value (FileData file) {
		var value = 0.0f;
		if (file.info.has_attribute ("Frame::Width")
			&& file.info.has_attribute ("Frame::Height"))
		{
			var width = file.info.get_attribute_int32 ("Frame::Width");
			var height = file.info.get_attribute_int32 ("Frame::Height");
			value = (float) width / height;
		}
		return value;
	}
}
