public class Gth.EmbeddedPropertyView : Gth.FilePropertyView {
	construct {
		embedded_properties = true;
	}

	public override unowned string get_name () {
		return "embedded-properties";
	}

	public override unowned string get_title () {
		return _("Embedded Properties");
	}

	public override unowned string get_icon () {
		return "open-book-symbolic";
	}

	public override bool can_view (Gth.FileData file_data) {
		var data_available = false;
		foreach (unowned var info in MetadataInfo.get_all ()) {
			if (info.id == null) {
				continue;
			}
			if (!(MetadataFlags.ALLOW_IN_PROPERTIES_VIEW in info.flags)) {
				continue;
			}
			if (info.id.has_prefix ("Exif")
				|| info.id.has_prefix ("Iptc")
				|| info.id.has_prefix ("Xmp"))
			{
				var value = file_data.get_attribute_as_string (info.id);
				if (!Strings.empty (value)) {
					data_available = true;
					break;
				}
			}
		}
		return data_available;
	}
}
