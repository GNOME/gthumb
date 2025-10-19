public class Gth.ImageEditor {
	public ImageEditor () {
		factory = new Work.Factory (Util.get_workers ());
	}

	public async Image? exec_operation (ImageOperation operation, Cancellable cancellable) throws Error {
		var job = new Job ();
		job.callback = exec_operation.callback;
		job.operation = operation;
		job.cancellable = cancellable;
		factory.add_job (job);
		yield;
		if (job.result == null) {
			throw new IOError.CANCELLED ("Cancelled");
		}
		return job.result;
	}

	class Job : Work.Job {
		public ImageOperation operation;
		public Cancellable cancellable;
		public Image result;

		public override void run (uint8[] tmp_buffer) {
			result = operation.get_image (cancellable);
		}
	}

	Work.Factory factory;
}
