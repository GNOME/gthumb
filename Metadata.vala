public class Gth.Metadata {
	public enum Type {
		STRING,
		STRING_LIST,
	}

	public Type data_type;
	public string id;
	public string description;
	public string raw;
	public string formatted;
	public string[] list;
	public string value_type;

	public Metadata () {
		data_type = Type.STRING;
		id = null;
		description = null;
		raw = null;
		formatted = null;
		list = null;
		value_type = null;
	}
}
