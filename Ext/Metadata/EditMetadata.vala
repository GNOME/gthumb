public class Gth.EditMetadata : Object {
	public async void edit (Gtk.Window? parent, FileData file_data, Job job) throws Error {
		callback = edit.callback;
		dialog = new MetadataDialog (file_data);
		dialog.saved.connect (() => {
			dialog.close ();
			saved = true;
		});
		dialog.closed.connect (() => {
			if (callback != null) {
				Idle.add ((owned) callback);
				callback = null;
			}
		});
		cancelled_event = job.cancellable.cancelled.connect (() => {
			dialog.close ();
		});
		job.opens_dialog ();
		dialog.present (parent);
		yield;
		job.dialog_closed ();
		if (cancelled_event != 0) {
			job.cancellable.disconnect (cancelled_event);
			cancelled_event = 0;
		}
		if (!saved) {
			throw new IOError.CANCELLED ("Cancelled");
		}
	}

	SourceFunc callback = null;
	MetadataDialog dialog = null;
	ulong cancelled_event = 0;
	bool saved = false;
}

[GtkTemplate (ui = "/app/gthumb/gthumb/ui/metadata-dialog.ui")]
class Gth.MetadataDialog : Adw.Dialog {
	public signal void saved ();

	public MetadataDialog (FileData _file_data) {
		file_data = _file_data;

		// Description
		var metadata = file_data.info.get_attribute_object ("Metadata::Description") as Metadata;
		if (metadata != null) {
			description.buffer.text = metadata.formatted;
			Gtk.TextIter iter;
			description.buffer.get_iter_at_line (out iter, 0);
			description.buffer.place_cursor (iter);
		}
		else {
			description.buffer.text = "";
		}

		// Date TODO: add time
		metadata = file_data.info.get_attribute_object ("Metadata::DateTime") as Metadata;
		if (metadata != null) {
			date.exif_date = metadata.raw;
		}
		else {
			date.date = Gth.Date ();
		}

		// TODO add tags

		set_entry_value (title, "Metadata::Title");
		set_entry_value (place, "Metadata::Location");
		set_rating (rating, "Metadata::Rating");

		// IPTC
		set_entry_value (copyright, "Iptc::Application2::Copyright");
		set_entry_value (credit, "Iptc::Application2::Credit");
		set_entry_value (byline, "Iptc::Application2::Byline");
		set_entry_value (byline_title, "Iptc::Application2::BylineTitle");
		set_entry_value (country, "Iptc::Application2::CountryName");
		set_entry_value (country_code, "Iptc::Application2::CountryCode");
		set_entry_value (state, "Iptc::Application2::ProvinceState");
		set_entry_value (city, "Iptc::Application2::City");
		set_entry_value (language, "Iptc::Application2::Language");
		set_entry_value (object_name, "Iptc::Application2::ObjectName");
		set_entry_value (source, "Iptc::Application2::Source");
		set_entry_value (destination, "Iptc::Envelope::Destination");
		set_rating (urgency, "Iptc::Application2::Urgency");
	}

	void set_entry_value (Adw.EntryRow entry, string id) {
		var metadata = file_data.info.get_attribute_object (id) as Metadata;
		if (metadata != null) {
			entry.text = metadata.formatted;
		}
		else {
			entry.text = "";
		}
	}

	void set_adjustment_value (Gtk.Adjustment adj, string id) {
		var metadata = file_data.info.get_attribute_object (id) as Metadata;
		if (metadata != null) {
			int value;
			if (int.try_parse (metadata.formatted, out value, null, 10)) {
				adj.value = (double) value;
			}
			else {
				adj.value = 0;
			}
		}
		else {
			adj.value = 0;
		}
	}

	void set_rating (Gth.RatingEntry entry, string id) {
		var metadata = file_data.info.get_attribute_object (id) as Metadata;
		if (metadata != null) {
			int value;
			if (int.try_parse (metadata.formatted, out value, null, 10)) {
				entry.value = value;
			}
			else {
				entry.value = 0;
			}
		}
		else {
			entry.value = 0;
		}
	}

	[GtkCallback]
	void on_cancel (Gtk.Button button) {
		close ();
	}

	[GtkCallback]
	void on_save (Gtk.Button button) {
		saved ();
	}

	FileData file_data;
	[GtkChild] unowned Gtk.TextView description;
	[GtkChild] unowned Adw.EntryRow title;
	[GtkChild] unowned Adw.EntryRow place;
	[GtkChild] unowned Gth.DateRow date;
	[GtkChild] unowned Gth.TimeRow time;
	[GtkChild] unowned Gth.RatingEntry rating;
	[GtkChild] unowned Adw.EntryRow copyright;
	[GtkChild] unowned Adw.EntryRow credit;
	[GtkChild] unowned Adw.EntryRow byline;
	[GtkChild] unowned Adw.EntryRow byline_title;
	[GtkChild] unowned Adw.EntryRow country;
	[GtkChild] unowned Adw.EntryRow country_code;
	[GtkChild] unowned Adw.EntryRow state;
	[GtkChild] unowned Adw.EntryRow city;
	[GtkChild] unowned Adw.EntryRow language;
	[GtkChild] unowned Adw.EntryRow object_name;
	[GtkChild] unowned Adw.EntryRow source;
	[GtkChild] unowned Adw.EntryRow destination;
	[GtkChild] unowned Gth.RatingEntry urgency;
}
