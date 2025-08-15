public class Gth.OtherPropertyView : Gth.FilePropertyView {
	construct {
		filter = FilePropertyView.Filter.OTHER;
	}

	public override unowned string get_id () {
		return "other-properties";
	}

	public override unowned string get_title () {
		return _("Other");
	}

	public override unowned string get_icon () {
		return "open-book-symbolic";
	}
}
