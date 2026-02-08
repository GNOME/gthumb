public class Gth.EditMetadata : Object {
	public async void edit (Gtk.Window? parent, GenericList<FileData> files, Job job) throws Error {
		callback = edit.callback;
		dialog = new MetadataDialog (files);
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

	public MetadataDialog (GenericList<FileData> _files) {
		files = _files;
		common_info = get_common_info ();

		if (files.length () > 1) {
			options_group.visible = true;
			only_modified_attributes_switch.active = true;
		}

		// Description
		var metadata = common_info.get_attribute_object ("Metadata::Description") as Metadata;
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
		metadata = common_info.get_attribute_object ("Metadata::DateTime") as Metadata;
		if (metadata != null) {
			date.exif_date = metadata.raw;
			time.exif_date = metadata.raw;
		}
		else {
			date.date = Gth.Date ();
			time.time = Gth.Time ();
		}

		// Tags
		var tag_data = common_info.get_attribute_object ("Metadata::Tags") as Metadata;
		if (tag_data != null) {
			tags.entry.set_list (tag_data.string_list.get_list ());
		}

		set_entry_value (title_entry, common_info, "Metadata::Title");
		set_entry_value (place, common_info, "Metadata::Place");
		set_rating (rating.entry, common_info, "Metadata::Rating");

		// IPTC
		var can_write_iptc = true;
		foreach (var file in files) {
			if (!Exiv2.can_write_metadata (file.get_content_type ())) {
				can_write_iptc = false;
				break;
			}
		}
		iptc_group.visible = can_write_iptc && app.settings.get_boolean (PREF_GENERAL_STORE_METADATA_IN_FILES);
		set_entry_value (copyright, common_info, "Iptc::Application2::Copyright");
		set_entry_value (credit, common_info, "Iptc::Application2::Credit");
		set_entry_value (byline, common_info, "Iptc::Application2::Byline");
		set_entry_value (byline_title, common_info, "Iptc::Application2::BylineTitle");
		set_entry_value (country, common_info, "Iptc::Application2::CountryName");
		set_entry_value (country_code, common_info, "Iptc::Application2::CountryCode");
		set_entry_value (state, common_info, "Iptc::Application2::ProvinceState");
		set_entry_value (city, common_info, "Iptc::Application2::City");
		set_entry_value (language, common_info, "Iptc::Application2::Language");
		set_entry_value (object_name, common_info, "Iptc::Application2::ObjectName");
		set_entry_value (source, common_info, "Iptc::Application2::Source");
		set_entry_value (destination, common_info, "Iptc::Envelope::Destination");
		set_rating (urgency.entry, common_info, "Iptc::Application2::Urgency");
	}

	const string[] ALL_ATTRIBUTES = {
		"Metadata::Description",
		"Metadata::DateTime",
		"Metadata::Tags",
		"Metadata::Title",
		"Metadata::Place",
		"Metadata::Rating",
		"Iptc::Application2::Copyright",
		"Iptc::Application2::Credit",
		"Iptc::Application2::Byline",
		"Iptc::Application2::BylineTitle",
		"Iptc::Application2::CountryName",
		"Iptc::Application2::CountryCode",
		"Iptc::Application2::ProvinceState",
		"Iptc::Application2::City",
		"Iptc::Application2::Language",
		"Iptc::Application2::ObjectName",
		"Iptc::Application2::Source",
		"Iptc::Envelope::Destination",
		"Iptc::Application2::Urgency",
	};

	FileInfo get_common_info () {
		var first_file = files.first ();
		if (files.length () == 1) {
			return first_file.info;
		}
		var info = new FileInfo ();
		first_file.info.copy_into (info);
		foreach (unowned string attribute in ALL_ATTRIBUTES) {
			var first_value = first_file.get_attribute_as_string (attribute);
			foreach (var file in files) {
				if (file == first_file) {
					continue;
				}
				var other_value = file.get_attribute_as_string (attribute);
				if (other_value != first_value) {
					info.remove_attribute (attribute);
					break;
				}
			}
		}
		return info;
	}

	void set_entry_value (Adw.EntryRow entry, FileInfo info, string id) {
		var metadata = info.get_attribute_object (id) as Metadata;
		if (metadata != null) {
			entry.text = metadata.formatted;
		}
		else {
			entry.text = "";
		}
	}

	void set_rating (Gth.RatingEntry entry, FileInfo info, string id) {
		var metadata = info.get_attribute_object (id) as Metadata;
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

	bool attribute_modified (string attribute, string? new_value) {
		var old_value = Util.get_attribute_as_string (common_info, attribute);
		if (Strings.empty (new_value) && Strings.empty (old_value)) {
			// null is equal to an empty string.
			return false;
		}
		if (Strings.empty (new_value) || Strings.empty (old_value)) {
			return true;
		}
		return old_value == new_value;
	}

	bool save_date;
	bool save_tags;

	void update_file_attributes (FileInfo info) {
		if (!only_modified_attributes || attribute_modified ("Metadata::Description", description.buffer.text)) {
			var metadata = new Gth.Metadata.for_string (description.buffer.text);
			info.set_attribute_object ("Metadata::Description", metadata);
		}

		set_string_attribute (info, title_entry, "Metadata::Title");
		set_string_attribute (info, place, "Metadata::Place");

		if (save_date) {
			var date_time = new Gth.DateTime.from_date_time (date.date, time.time);
			if (date_time.date.is_valid ()) {
				var metadata = new Gth.Metadata.for_string (date_time.to_exif_date ());
				info.set_attribute_object ("Metadata::DateTime", metadata);
			}
			else {
				info.remove_attribute ("Metadata::DateTime");
			}
		}

		set_rating_attribute (info, rating, "Metadata::Rating");

		if (save_tags) {
			var new_tag_list = new StringList.from_array (tags.entry.get_list ());
			if (!new_tag_list.is_empty ()) {
				var metadata = new Gth.Metadata.for_string_list (new_tag_list);
				info.set_attribute_object ("Metadata::Tags", metadata);
			}
			else {
				info.remove_attribute ("Metadata::Tags");
			}
		}

		set_string_attribute (info, copyright, "Iptc::Application2::Copyright");
		set_string_attribute (info, credit, "Iptc::Application2::Credit");
		set_string_attribute (info, byline, "Iptc::Application2::Byline");
		set_string_attribute (info, byline_title, "Iptc::Application2::BylineTitle");
		set_string_attribute (info, country, "Iptc::Application2::CountryName");
		set_string_attribute (info, country_code, "Iptc::Application2::CountryCode");
		set_string_attribute (info, state, "Iptc::Application2::ProvinceState");
		set_string_attribute (info, city, "Iptc::Application2::City");
		set_string_attribute (info, language, "Iptc::Application2::Language");
		set_string_attribute (info, object_name, "Iptc::Application2::ObjectName");
		set_string_attribute (info, source, "Iptc::Application2::Source");
		set_string_attribute (info, destination, "Iptc::Envelope::Destination");
		set_rating_attribute (info, urgency, "Iptc::Application2::Urgency");
	}

	void set_string_attribute (FileInfo info, Adw.EntryRow row, string id) {
		if (!only_modified_attributes || attribute_modified (id, row.text)) {
			var metadata = new Gth.Metadata.for_string (row.text);
			info.set_attribute_object (id, metadata);
		}
	}

	void set_rating_attribute (FileInfo info, Gth.RatingRow row, string id) {
		var value = row.entry.value;
		var value_as_string = "%d".printf (value);
		var save_rating = true;
		if (only_modified_attributes) {
			if (value == 0) {
				save_rating = common_info.has_attribute (id);
			}
			else {
				save_rating = attribute_modified (id, value_as_string);
			}
		}
		stdout.printf ("> rating '%s' -> '%s'\n", Util.get_attribute_as_string (common_info, id), value_as_string);
		stdout.printf ("  save_rating %s\n", save_rating.to_string ());
		if (save_rating) {
			if (value > 0) {
				var metadata = new Gth.Metadata.for_string (value_as_string);
				info.set_attribute_object (id, metadata);
			}
			else {
				info.remove_attribute (id);
			}
		}
	}

	[GtkCallback]
	void on_cancel (Gtk.Button button) {
		close ();
	}

	[GtkCallback]
	void on_save (Gtk.Button button) {
		only_modified_attributes = (files.length () > 1) && only_modified_attributes_switch.active;

		// save_date
		save_date = true;
		if (only_modified_attributes) {
			var date_time = new Gth.DateTime.from_date_time (date.date, time.time);
			var exif_date = date_time.to_exif_date ();
			if (date_time.date.is_valid ()) {
				save_date = attribute_modified ("Metadata::DateTime", exif_date);
			}
			else {
				save_date = common_info.has_attribute ("Metadata::DateTime");
			}
		}

		// save_tags
		save_tags = true;
		if (only_modified_attributes) {
			var new_tag_list = new StringList.from_array (tags.entry.get_list ());
			var tags_modified = true;
			var tag_list_was_empty = true;
			if (common_info.has_attribute ("Metadata::Tags")) {
				var old_tags = common_info.get_attribute_object ("Metadata::Tags");
				if (old_tags is Metadata) {
					var metadata = old_tags as Metadata;
					if (metadata.get_data_type () == MetadataType.STRING_LIST) {
						var old_tag_list = metadata.get_string_list ();
						tag_list_was_empty = old_tag_list.is_empty ();
						if (old_tag_list.equal (new_tag_list)) {
							tags_modified = false;
						}
					}
				}
			}
			if (tag_list_was_empty && new_tag_list.is_empty ()) {
				tags_modified = false;
			}
			save_tags = tags_modified;
		}

		foreach (var file in files) {
			update_file_attributes (file.info);
		}
		saved ();
	}

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
	[GtkChild] unowned Adw.SwitchRow only_modified_attributes_switch;
	[GtkChild] unowned Adw.PreferencesGroup options_group;
	GenericList<FileData> files;
	FileInfo common_info;
	bool only_modified_attributes;
}
