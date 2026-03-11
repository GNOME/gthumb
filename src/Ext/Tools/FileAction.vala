public class Gth.FileAction : Object {
	public ToolCategory category;
	public string action_name;
	public string display_name;
	public string icon_name;
	public string default_shortcut;
	public bool visible;
	public uint position;

	public FileAction () {
		category = ToolCategory.OTHER;
		action_name = null;
		display_name = null;
		icon_name = null;
		default_shortcut = null;
		visible = true;
		position = 0;
	}

	public ActionCategory get_action_category () {
		switch (category) {
		case ToolCategory.METADATA:
			return app.tools.metadata_tools;
		case ToolCategory.IMAGES:
			return app.tools.images;
		default:
			return app.tools.tools;
		}
	}

	public Dom.Element create_element (Dom.Document doc) {
		var node = new Dom.Element ("tool");
		node.set_attribute ("id", action_name);
		if (!visible) {
			node.set_attribute ("display", "none");
		}
		return node;
	}

	public void load_from_element (Dom.Element node) {
		visible = node.get_attribute ("display") != "none";
	}
}

public enum Gth.ToolCategory {
	METADATA,
	IMAGES,
	OTHER;
}
