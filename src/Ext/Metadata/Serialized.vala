public class Gth.Serialized : Object {
	public const int HEADER_SIZE =
		1 + // format
		1 + // byte order
		1 + // timestamp type
		2 + // timestamp size
		27; // timestamp data

	public struct Header {
		uint8 format;
		ByteOrder byte_order;
		GLib.DateTime timestamp;

		public Header () {
			format = 0;
			byte_order = ByteOrder.HOST;
			timestamp = null;
		}
	}

	public Serialized.object () {
		data_type = DataType.OBJECT;
	}

	public Serialized.array () {
		data_type = DataType.ARRAY;
	}

	public Serialized.string (string _str) {
		data_type = DataType.STRING;
		str = _str;
	}

	public Serialized.from_bytes (Bytes bytes, out Header header = null) {
		try {
			var data = Serialized.get_data_stream_for_bytes (bytes);

			header = Serialized.read_header (data);
			if (header.format != FORMAT) {
				throw new IOError.FAILED ("Wrong format");
			}
			if (header.byte_order != BYTE_ORDER) {
				throw new IOError.FAILED ("Wrong endianness");
			}

			read (data);
		}
		catch (Error error) {
			data_type = DataType.ERROR;
			header = Header ();
		}
	}

	public static Header get_header (Bytes bytes) {
		try {
			var data = Serialized.get_data_stream_for_bytes (bytes);
			return Serialized.read_header (data);
		}
		catch (Error error) {
			return Header ();
		}
	}

	public void set_string (string id, string? value) {
		if (value != null) {
			set (id, new Serialized.string (value));
		}
		else if (obj != null) {
			obj.remove (id);
		}
	}

	public new void set (string id, Serialized value) {
		return_if_fail (data_type == DataType.OBJECT);
		if (obj == null) {
			obj = new HashTable<string, Serialized>(str_hash, str_equal);
		}
		obj[id] = value;
	}

	public void add_entry (Serialized value) {
		return_if_fail (data_type == DataType.ARRAY);
		if (arr == null) {
			arr = new GenericArray<Serialized>();
		}
		arr.add (value);
	}

	public void add_string (string value) {
		add_entry (new Serialized.string (value));
	}

	public Bytes? to_bytes (GLib.DateTime? timestamp = null) {
		try {
			var mem = new MemoryOutputStream.resizable ();
			var data = new DataOutputStream (mem);
			data.set_byte_order (DataStreamByteOrder.HOST_ENDIAN);
			data.put_byte (FORMAT);
			data.put_byte ((BYTE_ORDER == ByteOrder.BIG_ENDIAN) ? EndiannessCode.BIG : EndiannessCode.LITTLE);
			var local_timestamp = (timestamp == null) ? new GLib.DateTime.now () : timestamp;
			add_string_to_data (data, local_timestamp.format_iso8601 ());
			add_to_data (data);
			mem.close ();
			return mem.steal_as_bytes ();
		}
		catch (Error error) {
			stderr.printf ("*ERROR* Serialized.to_bytes: %s\n", error.message);
			return null;
		}
	}

	public unowned Serialized? get_item (string id) {
		return (obj != null) ? obj[id] : null;
	}

	public new unowned string? get (string id) {
		var value = get_item (id);
		return (value != null) ? value.to_string () : null;
	}

	public Iterator iterator () {
		if (data_type == DataType.ARRAY) {
			return new ArrayIterator (this);
		}
		return new Iterator ();
	}

	public bool equal (Serialized other) {
		if (other.data_type != data_type) {
			return false;
		}

		switch (data_type) {
		case DataType.STRING:
			return str == other.str;

		case DataType.ARRAY:
			if ((arr == null) && (other.arr == null)) {
				return true;
			}
			if ((arr == null) || (other.arr == null)) {
				return false;
			}
			if (arr.length != other.arr.length) {
				return false;
			}
			for (var i = 0; i < arr.length; i++) {
				if (!arr[i].equal (other.arr[i])) {
					return false;
				}
			}
			return true;

		case DataType.OBJECT:
			if ((obj == null) && (other.obj == null)) {
				return true;
			}
			if ((obj == null) || (other.obj == null)) {
				return false;
			}
			if (obj.length != other.obj.length) {
				return false;
			}
			var iter = HashTableIter<string, Serialized> (obj);
			unowned string key;
			unowned Serialized item;
			while (iter.next (out key, out item)) {
				var other_item = other.obj[key];
				if (other_item == null) {
					return false;
				}
				if (!other_item.equal (item)) {
					return false;
				}
			}
			return true;

		default:
			break;
		}
		return false;
	}

	public unowned string? to_string () {
		return (data_type == DataType.STRING) ? str : null;
	}

	public string to_debug () {
		var text = new StringBuilder ();
		build (text);
		return text.str;
	}

	void build (StringBuilder text, string prefix = "  ") {
		switch (data_type) {
		case DataType.STRING:
			text.append_printf ("%s'%s'\n", prefix, str ?? "(null)");
			break;

		case DataType.ARRAY:
			text.append_printf ("%sarr:\n", prefix);
			if (arr != null) {
				var item_prefix = prefix + "  ";
				for (var i = 0; i < arr.length; i++) {
					arr[i].build (text, item_prefix);
				}
			}
			break;

		case DataType.OBJECT:
			text.append_printf ("%sobj:\n", prefix);
			if (obj != null) {
				var iter = HashTableIter<string, Serialized> (obj);
				unowned string key;
				unowned Serialized item;
				var key_prefix = prefix + "  ";
				var item_prefix = prefix + "    ";
				while (iter.next (out key, out item)) {
					text.append_printf ("%s'%s' =>\n", key_prefix, key);
					item.build (text, item_prefix);
				}
			}
			break;

		default:
			text.append_printf ("%serror\n", prefix);
			break;
		}
	}

	Serialized () {
		data_type = DataType.ERROR;
	}

	void add_string_to_data (DataOutputStream data, string value) throws Error {
		var size = value.length;
		if (size <= uint16.MAX) {
			data.put_byte (TypeCode.STRING16);
			data.put_uint16 ((uint16) size);
			data.put_string (value);
		}
		else if (size <= uint32.MAX) {
			data.put_byte (TypeCode.STRING32);
			data.put_uint32 ((uint32) size);
			data.put_string (value);
		}
		else {
			throw new IOError.FAILED ("String too big");
		}
	}

	void add_to_data (DataOutputStream data) throws Error {
		switch (data_type) {
		case DataType.STRING:
			add_string_to_data (data, str);
			break;

		case DataType.ARRAY:
			data.put_byte (TypeCode.ARRAY);
			if (arr == null) {
				data.put_uint16 (0);
				return;
			}

			// Array length
			var len = arr.length;
			if (len > uint16.MAX) {
				throw new IOError.FAILED ("Array too big");
			}
			data.put_uint16 ((uint16) len);

			// Array items
			foreach (var item in arr) {
				item.add_to_data (data);
			}
			break;

		case DataType.OBJECT:
			data.put_byte (TypeCode.OBJECT);
			if (obj == null) {
				data.put_uint16 (0);
				return;
			}

			// Object length
			var len = obj.length;
			if (len > uint16.MAX) {
				throw new IOError.FAILED ("Object too big");
			}
			data.put_uint16 ((uint16) len);

			// Object items
			var iter = HashTableIter<string, Serialized> (obj);
			unowned string key;
			unowned Serialized item;
			while (iter.next (out key, out item)) {
				// Key
				data.put_uint16 ((uint16) key.length);
				data.put_string (key);

				// Item
				item.add_to_data (data);
			}
			break;

		default:
			break;
		}
	}

	static string read_string (DataInputStream data) throws Error {
		var type_code = data.read_byte ();
		return Serialized.read_string_with_type (data, type_code);
	}

	void read (DataInputStream data) throws Error {
		var type_code = data.read_byte ();
		switch (type_code) {
		case TypeCode.STRING16, TypeCode.STRING32:
			str = Serialized.read_string_with_type (data, type_code);
			data_type = DataType.STRING;
			break;

		case TypeCode.ARRAY:
			var len = data.read_uint16 ();
			if (len > 0) {
				arr = new GenericArray<Serialized>();
				for (var i = 0; i < len; i++) {
					var item = new Serialized ();
					item.read (data);
					arr.add (item);
				}
			}
			else {
				arr = null;
			}
			data_type = DataType.ARRAY;
			break;

		case TypeCode.OBJECT:
			var len = data.read_uint16 ();
			if (len > 0) {
				obj = new HashTable<string, Serialized>(str_hash, str_equal);
				for (var i = 0; i < len; i++) {
					var key_len = (size_t) data.read_uint16 ();
					var key = Serialized.read_string_with_len (data, key_len);

					var item = new Serialized ();
					item.read (data);

					obj[key] = item;
				}
			}
			else {
				obj = null;
			}
			data_type = DataType.OBJECT;
			break;

		default:
			throw new IOError.FAILED ("Unknown type");
		}
	}

	static string read_string_with_type (DataInputStream data, TypeCode type_code) throws Error {
		size_t len = 0;
		switch (type_code) {
		case TypeCode.STRING16:
			len = (size_t) data.read_uint16 ();
			break;
		case TypeCode.STRING32:
			len = (size_t) data.read_uint32 ();
			break;
		default:
			throw new IOError.FAILED ("Wrong type (expected string)");
		}
		return Serialized.read_string_with_len (data, len);
	}

	static string read_string_with_len (DataInputStream data, size_t size) throws Error {
		var buffer = new uint8[size + 1];
		for (var i = 0; i < size; i++) {
			buffer[i] = data.read_byte ();
		}
		buffer[size] = 0;
		return (string) buffer;
	}

	static Header read_header (DataInputStream data) throws Error {
		var format = data.read_byte ();

		var byte_order = ByteOrder.HOST;
		var endianness_code = data.read_byte ();
		switch (endianness_code) {
		case EndiannessCode.LITTLE:
			byte_order = ByteOrder.LITTLE_ENDIAN;
			break;
		case EndiannessCode.BIG:
			byte_order = ByteOrder.BIG_ENDIAN;
			break;
		default:
			break;
		}

		var timestamp_s = Serialized.read_string (data);
		var timestamp = new GLib.DateTime.from_iso8601 (timestamp_s, null);

		return Header () {
			format = format,
			byte_order = byte_order,
			timestamp = timestamp
		};
	}

	static DataInputStream get_data_stream_for_bytes (Bytes bytes) {
		var mem = new MemoryInputStream.from_bytes (bytes);
		var data = new DataInputStream (mem);
		data.set_byte_order (DataStreamByteOrder.HOST_ENDIAN);
		return data;
	}

	public class Iterator {
		public virtual bool next () {
			return false;
		}

		public virtual unowned Serialized? get () {
			return null;
		}
	}

	public class ArrayIterator : Iterator {
		public ArrayIterator (Serialized data) {
			arr = data.arr;
			current = -1;
		}

		public override bool next () {
			if (arr == null) {
				return false;
			}
			current++;
			return current < arr.length;
		}

		public override unowned Serialized? get () {
			return (arr != null) ? arr[current] : null;
		}

		GenericArray<Serialized> arr;
		int current;
	}

	DataType data_type;
	HashTable<string, Serialized> obj;
	protected GenericArray<Serialized> arr;
	string str;

	enum DataType {
		ERROR,
		OBJECT,
		ARRAY,
		STRING,
	}

	enum EndiannessCode {
		LITTLE = 0,
		BIG = 1,
	}

	enum TypeCode {
		STRING16 = 0,
		STRING32 = 1,
		ARRAY = 2,
		OBJECT = 3,
	}

	const uint8 FORMAT = 1;
}
