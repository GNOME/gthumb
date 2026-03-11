public class Gth.MaskSelector : ImageSelector {
	public Image filtered;

	public ImageOperation filter_operation {
		get { return _filter_operation; }
		set {
			_filter_operation = value;
			queue_update_filter ();
		}
	}

	public override void set_view (ImageView? _view) {
		base.set_view (_view);
		queue_update_filter ();
	}

	public override void on_snapshot (Gtk.Snapshot snapshot) {
		if (view == null) {
			return;
		}
		view.snapshot_image (snapshot);
		snapshot.push_clip (view.texture_box);
		snapshot_filtered (snapshot);
		snapshot_selection_box (snapshot);
		snapshot.pop ();
	}

	public override void on_unrealize () {
		if (filter_cancellable != null) {
			filter_cancellable.cancel ();
		}
		base.on_unrealize ();
	}

	void snapshot_filtered (Gtk.Snapshot snapshot) {
		if ((filtered == null) || (_filter_operation == null)) {
			return;
		}

		Graphene.Rect filtered_box;
		image_box.intersection (selection, out filtered_box);
		if ((filtered_box.size.width == 0) || (filtered_box.size.height == 0)) {
			return;
		}

		var zoom = view.zoom;
		Graphene.Rect filtered_texture_box = {
			{ view.texture_box.origin.x + (filtered_box.origin.x * zoom) - view.viewport.origin.x,
			  view.texture_box.origin.y + (filtered_box.origin.y * zoom) - view.viewport.origin.y },
			{ filtered_box.size.width * zoom, filtered_box.size.height * zoom }
		};
		if (filtered_texture_box.origin.x < 0) {
			filtered_texture_box.size.width += filtered_texture_box.origin.x;
			filtered_texture_box.origin.x = 0;
		}
		if (filtered_texture_box.origin.y < 0) {
			filtered_texture_box.size.height += filtered_texture_box.origin.y;
			filtered_texture_box.origin.y = 0;
		}
		if (filtered_texture_box.origin.x + filtered_texture_box.size.width > view.viewport.size.width) {
			filtered_texture_box.size.width = view.viewport.size.width - filtered_texture_box.origin.x;
		}
		if (filtered_texture_box.origin.y + filtered_texture_box.size.height > view.viewport.size.height) {
			filtered_texture_box.size.height = view.viewport.size.height - filtered_texture_box.origin.y;
		}
		if ((filtered_texture_box.size.width == 0) || (filtered_texture_box.size.height == 0)) {
			return;
		}

		filtered_box = {
			{
				Math.floorf ((filtered_texture_box.origin.x - view.texture_box.origin.x + view.viewport.origin.x) / zoom),
				Math.floorf ((filtered_texture_box.origin.y - view.texture_box.origin.y + view.viewport.origin.y) / zoom)
			},
			{
				Math.floorf (filtered_texture_box.size.width / zoom),
				Math.floorf (filtered_texture_box.size.height / zoom)
			}
		};
		filtered_texture_box = {
			{ view.texture_box.origin.x + (filtered_box.origin.x * zoom) - view.viewport.origin.x,
			  view.texture_box.origin.y + (filtered_box.origin.y * zoom) - view.viewport.origin.y },
			{ filtered_box.size.width * zoom, filtered_box.size.height * zoom }
		};

		snapshot.save ();
		var texture = filtered.get_texture_for_rect (
			(uint) filtered_box.origin.x,
			(uint) filtered_box.origin.y,
			(uint) filtered_box.size.width,
			(uint) filtered_box.size.height);
		if (texture != null) {
			snapshot.append_scaled_texture (texture,
				Gsk.ScalingFilter.NEAREST,
				filtered_texture_box);
		}
		snapshot.restore ();
	}

	void queue_update_filter () {
		if (filter_id != 0) {
			return;
		}
		filter_id = Util.after_next_rearrange (() => {
			filter_id = 0;
			update_filter.begin ();
		});
	}

	async void update_filter () {
		if (filter_cancellable != null) {
			filter_cancellable.cancel ();
		}

		if ((view == null)
			|| (view.image == null)
			|| view.image.get_is_animated ())
		{
			return;
		}

		if (_filter_operation == null) {
			view.queue_draw ();
			return;
		}

		var local_cancellable = new Cancellable ();
		filter_cancellable = local_cancellable;
		try {
			filtered = yield app.image_editor.exec_operation (view.image, _filter_operation, local_cancellable);
			view.queue_draw ();
		}
		catch (Error error) {
		}
		if (filter_cancellable == local_cancellable) {
			filter_cancellable = null;
		}
	}

	construct {
		_filter_operation = null;
		filter_id = 0;
		filter_cancellable = null;
		view = null;
		filtered = null;
	}

	ImageOperation _filter_operation;
	uint filter_id;
	Cancellable filter_cancellable;
}
