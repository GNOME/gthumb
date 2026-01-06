public class Gth.MetadataWriter {
	public MetadataWriter (Work.Factory _factory) {
		factory = _factory;
	}

	public async void save (FileData file_data, Cancellable cancellable) throws Error {
		var flags = Flags.DEFAULT;
		if (app.settings.get_boolean (PREF_GENERAL_STORE_METADATA_IN_FILES)) {
			flags |= Flags.PREFER_EMBEDDED;
		}
		var job = new Job ();
		job.callback = save.callback;
		job.file_data = file_data;
		job.flags = flags;
		job.cancellable = cancellable;
		factory.add_job (job);
		yield;
		if (job.error != null) {
			throw job.error;
		}
		app.monitor.metadata_changed (job.file_data.file);
	}

	class Job : Work.Job {
		public FileData file_data;
		public Flags flags;
		public Cancellable cancellable;

		public override void run (uint8[] tmp_buffer) throws Error {
			var saved = false;
			if (Flags.PREFER_EMBEDDED in flags) {
				if (Exiv2.can_write_metadata (file_data.get_content_type ())) {
					var bytes = Files.load_file (file_data.file, cancellable);
					bytes = Exiv2.write_metadata_to_buffer (bytes, file_data.info);
					Files.save_file (file_data.file, bytes, SaveFileFlags.CONTENT_NOT_CHANGED, cancellable);
					saved = true;
				}
			}
			if (!saved) {
				var comment = new Comment.from_info (file_data.info);
				var comment_file = Comment.get_comment_file (file_data.file);
				var comment_dir = comment_file.get_parent ();
				Files.ensure_directory_exists (comment_dir);
				Files.save_content (comment_file, comment.to_xml (), cancellable);
			}
		}
	}

	weak Work.Factory factory;

	[Flags]
	enum Flags {
		DEFAULT,
		PREFER_EMBEDDED,
	}
}
