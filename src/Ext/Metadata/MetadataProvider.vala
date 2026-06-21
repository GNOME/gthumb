public abstract class Gth.MetadataProvider : Object {
	public string[] supported_attributes { get; set; default = {}; }

	public bool cachable { get; set; default = false; }

	public string id { get; set; default = null; }

	public abstract bool can_read (File? file, FileInfo info, string[]? attribute_v = null);

	public abstract bool read (File? file, Bytes? buffer, FileInfo info, Cancellable cancellable);

	public bool read_with_cache (File? file, Bytes? buffer, FileInfo info, Cancellable cancellable) {
		var use_cache = cachable && (file != null);
		// stdout.printf ("> read_with_cache: %s\n", (file != null) ? file.get_uri () : "(null)");
		// stdout.printf ("  provider: %s\n", id);
		// stdout.printf ("  use_cache: %s\n", use_cache.to_string ());
#if DEBUG_METADATA_CACHE
		var test_info = info.dup ();
		if (!read (file, buffer, info, cancellable)) {
			// stderr.printf ("> read_with_cache(%s) READ ERROR - %s\n", id, file.get_uri ());
			return false;
		}
		if (use_cache) {
			if (cache.load (id, file, test_info, cancellable)) {
				if (!cache.print_diff (info, test_info, supported_attributes)) {
					stderr.printf ("> read_with_cache(%s) CACHE ERROR - %s\n", id, file.get_uri ());
					assert_not_reached ();
				}
				else {
					stdout.printf ("> read_with_cache(%s) CACHE OK - %s\n", id, file.get_uri ());
				}
			}
			else {
				// stdout.printf ("> read_with_cache(%s) SAVE TO CACHE - %s\n", id, file.get_uri ());
				cache.save (id, file, info, supported_attributes);
			}
		}
#else
		var update_cache = use_cache;
		if (use_cache) {
			if (buffer == null) {
				if (cache.load (id, file, info, cancellable)) {
					// stdout.printf ("> read_with_cache(%s) FROM CACHE - %s\n", id, file.get_uri ());
					return true;
				}
			}
			else {
				// Always read from the buffer, update the cache if not valid.
				update_cache = !cache.valid (id, file, info, cancellable);
				// stdout.printf ("> read_with_cache(%s) FROM BUFFER - %s\n", id, file.get_uri ());
				// stdout.printf ("> read_with_cache(%s) VALID: %s\n", id, (!update_cache).to_string ());
			}
		}
		if (!read (file, buffer, info, cancellable)) {
			// stdout.printf ("> read_with_cache(%s) READ ERROR - %s\n", id, file.get_uri ());
			return false;
		}
		if (update_cache) {
			// stdout.printf ("> read_with_cache(%s) SAVE TO CACHE - %s\n", id, file.get_uri ());
			cache.save (id, file, info, supported_attributes);
		}
#endif
		return true;
	}

	construct {
		cache = new MetadataCache ();
	}

	MetadataCache cache;
}
