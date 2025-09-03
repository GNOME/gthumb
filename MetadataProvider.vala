public abstract class Gth.MetadataProvider : Object {
	public abstract bool can_read (FileData file_data, string[] attribute_v);

	public abstract void read (FileData file_data, string[] attribute_v, Cancellable cancellable);

	public virtual void write (FileData file_data,  Cancellable cancellable) {
		// void
	}
}
