public class Gth.FileAction : Object {
	public ToolCategory category;
	public string action_name;
	public string display_name;
	public string icon_name;
	public string default_shortcut;
	public bool visible;

	public FileAction () {
		category = ToolCategory.OTHER;
		action_name = null;
		display_name = null;
		icon_name = null;
		default_shortcut = null;
		visible = true;
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
}


public enum Gth.ToolCategory {
	METADATA,
	IMAGES,
	OTHER;
}
