public class Gth.RatingRow : Adw.ActionRow {
	public Gth.RatingEntry entry;

	public string rating_title { construct set; get; }
	public bool stars { construct set; get; }

	construct {
		label = new Gtk.Label (rating_title);
		label.add_css_class ("dimmed");
		add_prefix (label);

		entry = new Gth.RatingEntry ();
		if (stars) {
			entry.unchecked_icon_name = "gth-star-outline-symbolic";
			entry.checked_icon_name = "gth-star-symbolic";
		}
		entry.notify["value"].connect (() => {
			if (entry.value == 0) {
				label.add_css_class ("dimmed");
			}
			else {
				label.remove_css_class ("dimmed");
			}
		});
		entry.valign = Gtk.Align.CENTER;
		add_suffix (entry);
	}

	Gtk.Label label;
}
