public class Gth.ImageEditor {
	public ImageEditor () {
		factory = new Work.Factory (1);
	}

	public async Image? exec_async (Image image, Cancellable cancellable, EditorFunc editor_func) throws Error {
		var job = new Job ();
		job.callback = exec_async.callback;
		job.image = image;
		job.cancellable = cancellable;
		job.editor_func = editor_func;
		factory.add_job (job);
		yield;
		if (job.image == null) {
			throw new IOError.CANCELLED ("Cancelled");
		}
		return job.image;
	}

	class Job : Work.Job {
		public Image image;
		public Cancellable cancellable;
		public EditorFunc editor_func;

		public override void run (uint8[] tmp_buffer) {
			image = editor_func (image, cancellable);
		}
	}

	Work.Factory factory;
}

[CCode (has_target = false)]
public delegate Gth.Image? Gth.EditorFunc (Gth.Image image, Cancellable cancellable);
