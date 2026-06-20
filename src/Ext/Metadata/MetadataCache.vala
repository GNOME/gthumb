public class Gth.MetadataCache {
	public bool load (string provider_id, File file, FileInfo info, Cancellable cancellable) {
		try {
			var metadata_file = get_cache_file (provider_id, file, FileIntent.READ);
			// stdout.printf ("> file: %s => %s\n", file.get_uri (), metadata_file.get_uri ());
			var bytes = Files.load_file (metadata_file, cancellable);
			Serialized.Header header;
			var serialized = new Serialized.from_bytes (bytes, out header);
			if (!valid_timestamp (header.timestamp, info)) {
				return false;
			}
			// stdout.printf ("> cached:\n%s\n", serialized.to_debug ());
			if (deserialize_data (serialized, info)) {
				return true;
			}
		}
		catch (Error error) {
			// stderr.printf ("ERROR: MetadataCache.load: %s\n", error.message);
			return false;
		}
		return false;
	}

	public bool valid (string provider_id, File file, FileInfo info, Cancellable cancellable) {
		try {
			var metadata_file = get_cache_file (provider_id, file, FileIntent.READ);
			var stream = metadata_file.read (cancellable);
			uint8 buffer[Serialized.HEADER_SIZE];
			var size = stream.read (buffer, cancellable);
			if (size != Serialized.HEADER_SIZE) {
				return false;
			}
			var bytes = new Bytes.static (buffer);
			var header = Serialized.get_header (bytes);
			return valid_timestamp (header.timestamp, info);
		}
		catch (Error error) {
		}
		return false;
	}

	public void save (string provider_id, File file, FileInfo info, string[] attributes_to_save) {
		var time_changed = Files.get_changed_date_time (info);
		if (time_changed == null) {
			stderr.printf ("ERROR: MetadataCache.save: %s: time_changed == null\n", file.get_uri ());
			return;
		}

		var metadata = new Serialized.array ();
		foreach (unowned var attribute in info.list_attributes (null)) {
			if (Util.attribute_match_patterns (attribute, attributes_to_save)) {
				metadata.add_entry (serialize_attribute (info, attribute));
			}
		}
		var serialized = new Serialized.object ();
		serialized.set ("metadata", metadata);

		try {
			var metadata_file = get_cache_file (provider_id, file, FileIntent.WRITE);
			// stdout.printf ("> save metadata: %s => %s\n", file.get_uri (), metadata_file.get_uri ());
			Files.save_file (metadata_file, serialized.to_bytes (time_changed));
		}
		catch (Error error) {
			stderr.printf ("ERROR: MetadataCache.save: %s: %s\n", file.get_uri (), error.message);
		}
	}

	public bool print_diff (FileInfo file_info, FileInfo cache_info, string[] loaded_attributes) {
		var equal = true;
		foreach (unowned var attribute in file_info.list_attributes (null)) {
			if (Util.attribute_match_patterns (attribute, loaded_attributes)) {
				var valid = true;
				if (!cache_info.has_attribute (attribute)) {
					stderr.printf ("  %s: Not found\n", attribute);
					valid = false;
				}
				if (file_info.get_attribute_type (attribute) != cache_info.get_attribute_type (attribute)) {
					stderr.printf ("  %s: Wrong type\n", attribute);
					valid = false;
				}
				switch (file_info.get_attribute_type (attribute)) {
				case FileAttributeType.OBJECT:
					var obj = file_info.get_attribute_object (attribute);
					if (obj is Metadata) {
						var file_metadata = obj as Metadata;
						var cache_obj = cache_info.get_attribute_object (attribute);
						if (!(cache_obj is Metadata)) {
							stderr.printf ("  %s: Should be Metadata\n", attribute);
							valid = false;
						}
						else {
							var cache_metadata = cache_obj as Metadata;
							if (cache_metadata.get_data_type () != file_metadata.get_data_type ()) {
								stderr.printf ("  %s: Wrong Metadata data type\n", attribute);
								valid = false;
							}
							if (cache_metadata.value_type != file_metadata.value_type) {
								stderr.printf ("  %s: Wrong Metadata value type\n", attribute);
								valid = false;
							}

							switch (cache_metadata.get_data_type ()) {
							case MetadataType.STRING:
								if (cache_metadata.raw != file_metadata.raw) {
									stderr.printf ("  %s: Wrong Metadata raw value ('%s' <=> '%s')\n",
										attribute,
										cache_metadata.raw,
										file_metadata.raw);
									valid = false;
								}
								if (cache_metadata.formatted != file_metadata.formatted) {
									stderr.printf ("  %s: Wrong Metadata formatted value ('%s' <=> '%s')\n",
										attribute,
										cache_metadata.formatted,
										file_metadata.formatted);
									valid = false;
								}
								break;

							case MetadataType.STRING_LIST:
								var cache_list = cache_metadata.get_string_list ();
								var file_list = file_metadata.get_string_list ();
								if (!cache_list.equal (file_list)) {
									stderr.printf ("  %s: Wrong Metadata string list ('%s' <=> '%s')\n",
										attribute,
										cache_list.join (", "),
										file_list.join (", "));
									valid = false;
								}
								break;

							case MetadataType.POINT:
								double cache_x = 0, cache_y = 0;
								double file_x = 0, file_y = 0;
								if (cache_metadata.get_point (out cache_x, out cache_y)
									&& file_metadata.get_point (out file_x, out file_y)
									&& ((cache_x != file_x) || (cache_y != file_y)))
								{
									stderr.printf ("  %s: Wrong Metadata point ('%f, %f' <=> '%f, %f')\n",
										attribute,
										cache_x, cache_y,
										file_x, file_y);
									valid = false;
								}
								break;

							default:
								stderr.printf ("  %s: Unknown Metadata type\n",
									attribute);
								break;
							}
						}
					}
					else if (obj is StringList) {
						var cache_obj = cache_info.get_attribute_object (attribute);
						if (!(cache_obj is StringList)) {
							stderr.printf ("  %s: Should be StringList\n", attribute);
							valid = false;
						}
						else {
							var cache_list = cache_obj as StringList;
							var file_list = obj as StringList;
							if (!cache_list.equal (file_list)) {
								stderr.printf ("  %s: Wrong Metadata string list ('%s' <=> '%s')\n",
									attribute,
									cache_list.join (", "),
									file_list.join (", "));
								valid = false;
							}
						}
					}
					else {
						stderr.printf ("  %s: Unknown Object type\n", attribute);
						valid = false;
					}
					break;

				case FileAttributeType.STRING:
					var str = file_info.get_attribute_string (attribute);
					var cache_str = cache_info.get_attribute_string (attribute);
					if (cache_str != str) {
						stderr.printf ("  %s: Wrong value: ('%s' <=> '%s')\n",
							attribute,
							cache_str,
							str);
						valid = false;
					}
					break;

				case FileAttributeType.BOOLEAN:
					var file_value = file_info.get_attribute_boolean (attribute);
					var cache_value = cache_info.get_attribute_boolean (attribute);
					if (cache_value != file_value) {
						stderr.printf ("  %s: Wrong value: ('%s' <=> '%s')\n",
							attribute,
							cache_value.to_string (),
							file_value.to_string ());
						valid = false;
					}
					break;

				case FileAttributeType.INT32:
					var file_value = file_info.get_attribute_int32 (attribute);
					var cache_value = cache_info.get_attribute_int32 (attribute);
					if (cache_value != file_value) {
						stderr.printf ("  %s: Wrong value: ('%s' <=> '%s')\n",
							attribute,
							cache_value.to_string (),
							file_value.to_string ());
						valid = false;
					}
					break;

				case FileAttributeType.INT64:
					var file_value = file_info.get_attribute_int64 (attribute);
					var cache_value = cache_info.get_attribute_int64 (attribute);
					if (cache_value != file_value) {
						stderr.printf ("  %s: Wrong value: ('%s' <=> '%s')\n",
							attribute,
							cache_value.to_string (),
							file_value.to_string ());
						valid = false;
					}
					break;

				case FileAttributeType.UINT32:
					var file_value = file_info.get_attribute_uint32 (attribute);
					var cache_value = cache_info.get_attribute_uint32 (attribute);
					if (cache_value != file_value) {
						stderr.printf ("  %s: Wrong value: ('%s' <=> '%s')\n",
							attribute,
							cache_value.to_string (),
							file_value.to_string ());
						valid = false;
					}
					break;

				case FileAttributeType.UINT64:
					var file_value = file_info.get_attribute_uint64 (attribute);
					var cache_value = cache_info.get_attribute_uint64 (attribute);
					if (cache_value != file_value) {
						stderr.printf ("  %s: Wrong value: ('%s' <=> '%s')\n",
							attribute,
							cache_value.to_string (),
							file_value.to_string ());
						valid = false;
					}
					break;

				default:
					stderr.printf ("  %s: Unknown type %s\n",
						attribute,
						file_info.get_attribute_type (attribute).to_string ());
					valid = false;
					break;
				}
				if (valid) {
					// stderr.printf ("  %s: Ok\n", attribute);
				}
				else {
					equal = false;
				}
			}
		}
		return equal;
	}

	File get_cache_file (string provider_id, File file, FileIntent intent) {
		var dir = Files.build_directory (intent,
			File.new_for_path (Environment.get_user_cache_dir ()),
			"gthumb",
			"metadata");
		return dir.get_child (get_metadata_basename (provider_id, file));
	}

	string get_metadata_basename (string provider_id, File file) {
		var checksum = new Checksum (ChecksumType.MD5);
		var uri = file.get_uri ();
		checksum.update (uri.data, uri.length);
		return checksum.get_string () + "." + provider_id;
	}

	Serialized serialize_attribute (FileInfo info, string attribute) {
		Serialized item = null;
		if (info.get_attribute_type (attribute) == FileAttributeType.OBJECT) {
			var obj = info.get_attribute_object (attribute);
			if (obj is Metadata) {
				var metadata = obj as Metadata;
				switch (metadata.get_data_type ()) {
				case MetadataType.STRING:
					item = new Serialized.object ();
					item.set_string ("metadata_type", "string");
					item.set_string ("raw", metadata.raw);
					item.set_string ("formatted", metadata.formatted);
					break;

				case MetadataType.STRING_LIST:
					item = new Serialized.object ();
					item.set_string ("metadata_type", "string_list");
					var values = new Serialized.array ();
					var string_list = metadata.get_string_list ();
					foreach (unowned var str in string_list.get_list ()) {
						values.add_string (str);
					}
					item.set ("values", values);
					break;

				case MetadataType.POINT:
					double x, y;
					if (metadata.get_point (out x, out y)) {
						item = new Serialized.object ();
						item.set_string ("metadata_type", "point");
						item.set_string ("x", Util.format_double (x, "%f"));
						item.set_string ("y", Util.format_double (y, "%f"));
					}
					break;

				default:
					item = new Serialized.object ();
					item.set_string ("metadata_type", "other");
					item.set_string ("raw", metadata.raw);
					item.set_string ("formatted", metadata.formatted);
					break;
				}
				if (item != null) {
					item.set_string ("id", attribute);
					item.set_string ("type", "metadata");
					item.set_string ("value_type", metadata.value_type);
					item.set_string ("category", metadata.category);
					item.set_string ("description", metadata.description);
				}
			}
			else if (obj is StringList) {
				item = new Serialized.object ();
				item.set_string ("id", attribute);
				item.set_string ("type", "string_list");
				var values = new Serialized.array ();
				var string_list = obj as StringList;
				foreach (unowned var str in string_list.get_list ()) {
					values.add_entry (new Serialized.string (str));
				}
				item.set ("values", values);
			}
		}
		if (item == null) {
			item = new Serialized.object ();
			item.set_string ("id", attribute);

			switch (info.get_attribute_type (attribute)) {
			case FileAttributeType.STRING:
				item.set_string ("type", "string");
				item.set_string ("raw", info.get_attribute_string (attribute));
				break;

			case FileAttributeType.BOOLEAN:
				item.set_string ("type", "bool");
				item.set_string ("raw", info.get_attribute_boolean (attribute).to_string ());
				break;

			case FileAttributeType.INT32:
				item.set_string ("type", "int32");
				item.set_string ("raw", info.get_attribute_int32 (attribute).to_string ());
				break;

			case FileAttributeType.INT64:
				item.set_string ("type", "int64");
				item.set_string ("raw", info.get_attribute_int64 (attribute).to_string ());
				break;

			case FileAttributeType.UINT32:
				item.set_string ("type", "uint32");
				item.set_string ("raw", info.get_attribute_uint32 (attribute).to_string ());
				break;

			case FileAttributeType.UINT64:
				item.set_string ("type", "uint64");
				item.set_string ("raw", info.get_attribute_uint64 (attribute).to_string ());
				break;

			default:
				item.set_string ("type", "other");
				item.set_string ("raw", info.get_attribute_as_string (attribute));
				break;
			}
		}
		return item;
	}

	bool valid_timestamp (GLib.DateTime? timestamp, FileInfo info) {
		if (timestamp == null) {
			return false;
		}
		var time_changed = Files.get_changed_date_time (info);
		return (time_changed != null) && time_changed.equal (timestamp);
	}

	bool deserialize_data (Serialized data, FileInfo info) {
		unowned var attributes = data.get_item ("metadata");
		if (attributes == null) {
			// stderr.printf ("  deserialize_data [0]\n");
			return false;
		}
		// stdout.printf ("> TIMESTAMP VALID!\n");
		foreach (unowned var attribute in attributes) {
			unowned var id = attribute["id"];
			switch (attribute["type"]) {
			case "metadata":
				Metadata metadata = null;
				switch (attribute["metadata_type"]) {
				case "string":
					metadata = new Metadata ();
					metadata["raw"] = attribute["raw"];
					metadata["formatted"] = attribute["formatted"];
					break;

				case "string_list":
					var strings = new GenericArray<string>();
					foreach (unowned var str in attribute.get_item ("values")) {
						strings.add (str.to_string ());
					}
					var string_list = new StringList.from_array (strings);
					metadata = new Metadata.for_string_list (string_list);
					break;

				case "point":
					double x = 0, y = 0;
					if (double.try_parse (attribute["x"], out x, null)
						&& double.try_parse (attribute["y"], out y, null))
					{
						metadata = new Metadata.for_point (x, y);
					}
					break;

				case "other":
					// metadata["raw"] = attribute["raw"];
					// metadata["formatted"] = attribute["formatted"];
					break;

				default:
					break;
				}
				if (metadata == null) {
					// stderr.printf ("  deserialize_data [3] id: '%s', metadata_type: '%s'\n",
					// 	id,
					// 	attribute["metadata_type"]);
					return false;
				}
				metadata["value_type"] = attribute["value_type"];
				metadata["category"] = attribute["category"];
				metadata["description"] = attribute["description"];
				metadata["id"] = id;
				MetadataInfo.register_from_metadata (metadata);
				info.set_attribute_object (id, metadata);
				break;

			case "string_list":
				var strings = new GenericArray<string>();
				foreach (unowned var str in attribute) {
					strings.add (str.to_string ());
				}
				var string_list = new StringList.from_array (strings);
				info.set_attribute_object (id, string_list);
				break;

			case "string":
				var valid = false;
				unowned var raw = attribute["raw"];
				if (raw != null) {
					info.set_attribute_string (id, raw);
					valid = true;
				}
				if (!valid) {
					// stderr.printf ("  deserialize_data [4]\n");
					return false;
				}
				break;

			case "bool":
				var valid = false;
				unowned var raw = attribute["raw"];
				if (raw != null) {
					info.set_attribute_boolean (id, raw == "true");
					valid = true;
				}
				if (!valid) {
					// stderr.printf ("  deserialize_data [5]\n");
					return false;
				}
				break;

			case "int32":
				var valid = false;
				unowned var raw = attribute["raw"];
				if (raw != null) {
					int value = 0;
					if (int.try_parse (raw, out value, null)) {
						info.set_attribute_int32 (id, value);
						valid = true;
					}
				}
				if (!valid) {
					// stderr.printf ("  deserialize_data [6]\n");
					return false;
				}
				break;

			case "int64":
				var valid = false;
				unowned var raw = attribute["raw"];
				if (raw != null) {
					int64 value = 0;
					if (int64.try_parse (raw, out value, null)) {
						info.set_attribute_int64 (id, value);
						valid = true;
					}
				}
				if (!valid) {
					// stderr.printf ("  deserialize_data [7]\n");
					return false;
				}
				break;

			case "uint32":
				var valid = false;
				unowned var raw = attribute["raw"];
				if (raw != null) {
					uint value = 0;
					if (uint.try_parse (raw, out value, null)) {
						info.set_attribute_uint32 (id, value);
						valid = true;
					}
				}
				if (!valid) {
					// stderr.printf ("  deserialize_data [8]\n");
					return false;
				}
				break;

			case "uint64":
				var valid = false;
				unowned var raw = attribute["raw"];
				if (raw != null) {
					uint64 value = 0;
					if (uint64.try_parse (raw, out value, null)) {
						info.set_attribute_uint64 (id, value);
						valid = true;
					}
				}
				if (!valid) {
					// stderr.printf ("  deserialize_data [9]\n");
					return false;
				}
				break;

			default:
				// stderr.printf ("  deserialize_data [10]\n");
				return false;
			}

		}
		return true;
	}
}
