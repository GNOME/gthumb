public class Gth.XmpPropertyView : Gth.FilePropertyView {
	construct {
		filter = FilePropertyView.Filter.XMP;
	}

	public override unowned string get_name () {
		return "xmp-properties";
	}

	public override unowned string get_title () {
		return _("XMP");
	}

	public override unowned string get_icon () {
		return "open-book-symbolic";
	}
}
