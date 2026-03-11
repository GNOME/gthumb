public class Gth.ExifPropertyView : Gth.FilePropertyView {
	construct {
		filter = FilePropertyView.Filter.EXIF;
	}

	public override unowned string get_id () {
		return "exif-properties";
	}

	public override unowned string get_title () {
		return _("EXIF");
	}

	public override unowned string get_icon () {
		return "gth-open-book-symbolic";
	}
}
