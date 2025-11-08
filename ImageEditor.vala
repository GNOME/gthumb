public class Gth.ImageEditor {
	public ImageEditor () {
		factory = new Work.Factory (Util.get_workers ());
	}

	public async Image? exec_operation (Image input, ImageOperation operation, Cancellable cancellable) throws Error {
		var job = new Job ();
		job.callback = exec_operation.callback;
		job.input = input;
		job.operation = operation;
		job.cancellable = cancellable;
		factory.add_job (job);
		yield;
		if (job.output == null) {
			throw new IOError.CANCELLED ("Cancelled");
		}
		return job.output;
	}

	class Job : Work.Job {
		public Image input;
		public ImageOperation operation;
		public Cancellable cancellable;
		public Image output;

		public override void run (uint8[] tmp_buffer) {
			output = operation.execute (input, cancellable);
		}
	}

	Work.Factory factory;
}
