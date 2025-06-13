public class Gth.PropertyMetadata : Gth.PropertyFile {
	construct {
		show_details = true;
	}

	public override unowned string get_name () {
		return "metadata" ;
	}

	public override unowned string get_title () {
		return _("Embedded Metadata");
	}

	public override unowned string get_icon () {
		return "tag-symbolic";
	}
}
