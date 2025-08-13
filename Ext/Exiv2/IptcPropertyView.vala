public class Gth.IptcPropertyView : Gth.FilePropertyView {
	construct {
		filter = FilePropertyView.Filter.IPTC;
	}

	public override unowned string get_id () {
		return "iptc-properties";
	}

	public override unowned string get_title () {
		return _("IPTC");
	}

	public override unowned string get_icon () {
		return "open-book-symbolic";
	}
}
