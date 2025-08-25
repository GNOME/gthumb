public abstract class Gth.MetadataProvider : Object {
	public abstract bool can_read (FileData file_data, string content_type, string[] attribute_v);
	public abstract void read (FileData file_data, string[] attribute_v, Cancellable cancellable);
	public abstract bool can_write (FileData file_data, string content_type, string[] attribute_v);
	public abstract void write (FileData file_data, string[] attribute_v, Cancellable cancellable, Gth.MetadataWriteFlags flags = MetadataWriteFlags.DEFAULT);
}

[Flags]
public enum Gth.MetadataWriteFlags {
	DEFAULT,
	FORCE_EMBEDDED,
}
