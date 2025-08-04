public class Gth.RegisteredSignals {
	public RegisteredSignals () {
		registry = new GenericArray<Entry?> ();
	}

	public void add (Object obj, ulong handler_id) {
		registry.add (Entry () { obj = obj, handler_id = handler_id });
	}

	public void disconnect_all () {
		foreach (unowned var entry in registry) {
			entry.obj.disconnect (entry.handler_id);
		}
		registry.length = 0;
	}

	public void disconnect_object (Object obj) {
		var i = 0;
		while (i < registry.length) {
			unowned var entry = registry[i];
			if (entry.obj == obj) {
				entry.obj.disconnect (entry.handler_id);
				registry.remove_index (i);
			}
			else {
				i++;
			}
		}
	}

	struct Entry {
		Object obj;
		ulong handler_id;
	}

	GenericArray<Entry?> registry;
}
