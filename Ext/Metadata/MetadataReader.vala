public class Gth.MetadataReader {
	public MetadataReader (Work.Factory _factory) {
		factory = _factory;
	}

	public async void update (FileData file_data, string[] metadata_attributes_v, Cancellable cancellable) throws Error {
		var job = new Job ();
		job.callback = update.callback;
		job.file_data = file_data;
		job.metadata_attributes_v = metadata_attributes_v;
		job.cancellable = cancellable;
		factory.add_job (job);
		yield;
		if (job.error != null) {
			throw job.error;
		}
	}

	class Job : Work.Job {
		public FileData file_data;
		public string[] metadata_attributes_v;
		public Cancellable cancellable;

		public override void run (uint8[] tmp_buffer) throws Error {
			foreach (unowned var provider in app.metadata_providers) {
				if (provider.can_read (file_data.file, file_data.info, metadata_attributes_v)) {
					provider.read (file_data.file, null, file_data.info, cancellable);
				}
			}
		}
	}

	weak Work.Factory factory;
}
