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

		// Date and Time
		metadata = file_data.info.get_attribute_object ("Metadata::DateTime") as Metadata;
		if (metadata != null) {
			date.exif_date = metadata.raw;
			time.exif_date = metadata.raw;
		}
		else {
			date.date = Gth.Date ();
			time.time = Gth.Time ();
		}

		// Tags
		var tag_data = file_data.info.get_attribute_object ("Metadata::Tags") as Metadata;
		if (tag_data != null) {
			tags.entry.set_list (tag_data.string_list.get_list ());
		}

		set_entry_value (title_entry, "Metadata::Title");
		set_entry_value (place, "Metadata::Place");
		set_rating (rating.entry, "Metadata::Rating");

		// IPTC
		iptc_group.visible = app.settings.get_boolean (PREF_GENERAL_STORE_METADATA_IN_FILES) && Exiv2.can_write_metadata (file_data.get_content_type ());
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
		set_rating (urgency.entry, "Iptc::Application2::Urgency");
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

	void update_file_attributes () {
		var metadata = new Gth.Metadata.for_string (description.buffer.text);
		file_data.info.set_attribute_object ("Metadata::Description", metadata);

		set_string_attribute (title_entry, "Metadata::Title");
		set_string_attribute (place, "Metadata::Place");

		var date_time = new Gth.DateTime.from_date_time (date.date, time.time);
		if (date_time.date.is_valid ()) {
			metadata = new Gth.Metadata.for_string (date_time.to_exif_date ());
			file_data.info.set_attribute_object ("Metadata::DateTime", metadata);
		}
		else {
			file_data.info.remove_attribute ("Metadata::DateTime");
		}

		set_rating_attribute (rating, "Metadata::Rating");

		var tag_list = tags.entry.get_list ();
		if (tag_list.length > 0) {
			metadata = new Gth.Metadata.for_string_list (new Gth.StringList.from_array (tag_list));
			file_data.info.set_attribute_object ("Metadata::Tags", metadata);
		}
		else {
			file_data.info.remove_attribute ("Metadata::Tags");
		}

		set_string_attribute (copyright, "Iptc::Application2::Copyright");
		set_string_attribute (credit, "Iptc::Application2::Credit");
		set_string_attribute (byline, "Iptc::Application2::Byline");
		set_string_attribute (byline_title, "Iptc::Application2::BylineTitle");
		set_string_attribute (country, "Iptc::Application2::CountryName");
		set_string_attribute (country_code, "Iptc::Application2::CountryCode");
		set_string_attribute (state, "Iptc::Application2::ProvinceState");
		set_string_attribute (city, "Iptc::Application2::City");
		set_string_attribute (language, "Iptc::Application2::Language");
		set_string_attribute (object_name, "Iptc::Application2::ObjectName");
		set_string_attribute (source, "Iptc::Application2::Source");
		set_string_attribute (destination, "Iptc::Envelope::Destination");
		set_rating_attribute (urgency, "Iptc::Application2::Urgency");
	}

	void set_string_attribute (Adw.EntryRow row, string id) {
		var metadata = new Gth.Metadata.for_string (row.text);
		file_data.info.set_attribute_object (id, metadata);
	}

	void set_rating_attribute (Gth.RatingRow row, string id) {
		var value = row.entry.value;
		if (value > 0) {
			var metadata = new Gth.Metadata.for_string ("%d".printf (value));
			file_data.info.set_attribute_object (id, metadata);
		}
		else {
			file_data.info.remove_attribute (id);
		}
	}

	[GtkCallback]
	void on_cancel (Gtk.Button button) {
		close ();
	}

	[GtkCallback]
	void on_save (Gtk.Button button) {
		update_file_attributes ();
		saved ();
	}

	FileData file_data;
	[GtkChild] unowned Gtk.TextView description;
	[GtkChild] unowned Adw.EntryRow title_entry;
	[GtkChild] unowned Adw.EntryRow place;
	[GtkChild] unowned Gth.DateRow date;
	[GtkChild] unowned Gth.TimeRow time;
	[GtkChild] unowned Gth.RatingRow rating;
	[GtkChild] unowned Gth.TagsRow tags;
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
	[GtkChild] unowned Gth.RatingRow urgency;
	[GtkChild] unowned Adw.PreferencesGroup iptc_group;
}
