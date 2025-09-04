public abstract class Gth.MetadataProvider : Object {
	public abstract bool can_read (File? file, FileInfo info, string[]? attribute_v = null);
	public abstract void read (File? file, Bytes? buffer, FileInfo info, Cancellable cancellable);
}
