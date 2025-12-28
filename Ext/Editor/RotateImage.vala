public class Gth.RotateImage : ImageTool {
	public override void after_activate () {
		builder = new Gtk.Builder.from_resource ("/app/gthumb/gthumb/ui/rotate-image.ui");
		window.editor.set_options (builder.get_object ("options") as Gtk.Widget);
		window.editor.sidebar.insert_action_group ("rotate", action_group);
	}

	public override void before_deactivate () {
		window.editor.sidebar.insert_action_group ("rotate", null);
		builder = null;
	}

	construct {
		title = _("Rotate");
		icon_name = "gth-rotate-symbolic";
		size_type = SizeType.ORIGINAL;

		action_group = new SimpleActionGroup ();
		var action = new SimpleAction.stateful ("size-type", VariantType.STRING, new Variant.string (size_type.to_state ()));
		action.activate.connect ((_action, param) => {
			var method = param.get_string ();
			action.set_state (method);
			size_type = SizeType.from_state (method);
			// TODO
		});
		action_group.add_action (action);
	}

	enum SizeType {
		ORIGINAL,
		BOUNDING_BOX,
		CROP_BORDERS;

		public unowned string to_state () {
			return STATE[this];
		}

		public static SizeType from_state (string? state) {
			var idx = Util.enum_index (STATE, state);
			return (SizeType) idx;
		}

		const string[] STATE = { "original", "bounding-box", "crop-borders" };
	}

	Gtk.Builder builder;
	SimpleActionGroup action_group;
	SizeType size_type;
}
