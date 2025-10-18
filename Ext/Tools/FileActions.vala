public class Gth.FileActions {
	public GenericList<FileAction> entries;
	public ActionCategory actions;
	public ActionCategory tools;
	public ActionCategory images;
	public ActionCategory metadata_tools;
	public ActionCategory scripts;

	public FileActions () {
		entries = new GenericList<FileAction> ();
		actions = new ActionCategory ("", 0);
		images = new ActionCategory (_("Images"), 1);
		metadata_tools = new ActionCategory (_("Metadata"), 2);
		tools = new ActionCategory (_("Tools"), 3);
		scripts = new ActionCategory (_("Scripts"), 4);
	}

	public void register (ToolCategory category, string action_name, string display_name, string? icon_name = null, string? default_shortcut = null) {
		var action = new FileAction ();
		action.category = category;
		action.action_name = action_name;
		action.display_name = display_name;
		action.icon_name = icon_name;
		action.default_shortcut = default_shortcut;
		entries.model.append (action);
	}
}
