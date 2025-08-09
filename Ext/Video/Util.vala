using Gth;

public Gth.Image load_video_thumbnail (File file, uint requested_size, Cancellable cancellable) throws Error {
	var command = Path.build_filename (Config.PRIVEXECDIR, "video-thumbnailer");
	var tmp_output = Files.get_temp_file (".png");
	string[] argv = {
		command,
		"--size", "%u".printf (requested_size),
		file.get_path (),
		tmp_output.get_path ()
	};

	var proc = new Subprocess.newv (argv, SubprocessFlags.NONE);
	ulong id = 0;
	try {
		id = cancellable.connect (() => proc.force_exit ());
		proc.wait_check (null);
		if (proc.get_if_exited ()) {
			var bytes = Files.load_file (tmp_output, cancellable);
			var image = load_png (bytes, 0, cancellable);
			return image;
		}
	}
	catch (Error error) {
		throw error;
	}
	finally {
		if (id != 0) {
			cancellable.disconnect (id);
		}
		try { tmp_output.delete (null);	} catch (Error ignored) {}
	}
	return null;
}

Gth.Image generate_video_thumbnail (File file, Cancellable cancellable) throws Error {
	const Gst.ClockTime MAX_WAITING_TIME = Gst.SECOND * 10;

	var playbin = Gst.ElementFactory.make_full ("playbin",
		"uri", file.get_uri (),
		"audio-sink", Gst.ElementFactory.make ("fakesink"),
		"video-sink", Gst.ElementFactory.make ("fakesink"),
		"video-filter", Gst.ElementFactory.make_full ("videoflip",
			"video-direction", Gst.Video.OrientationMethod.AUTO, null),
		null);
	playbin.set_state (Gst.State.PAUSED);
	playbin.get_state (null, null, MAX_WAITING_TIME);

	// Get the video length.
	int64 duration;
	if (!playbin.query_duration (Gst.Format.TIME, out duration)) {
		throw new IOError.FAILED ("Could not get the media length");
	}

	// Seek to the thumbnail position.
	int64 thumbnail_position = duration / 3;
	if (!playbin.seek_simple (Gst.Format.TIME, Gst.SeekFlags.FLUSH | Gst.SeekFlags.ACCURATE, thumbnail_position))	{
		throw new IOError.FAILED ("Seek failed");
	}
	playbin.get_state (null, null, MAX_WAITING_TIME);

	// Get the sample.
	Gst.Sample sample;
	var caps = new Gst.Caps.empty ();
	caps.append_structure (new Gst.Structure ("video/x-raw", "format", Type.STRING, "RGB"));
	caps.append_structure (new Gst.Structure ("video/x-raw", "format", Type.STRING, "RGBA"));
	Signal.emit_by_name (playbin, "convert_sample", caps, out sample);
	if (sample == null) {
		throw new IOError.FAILED ("Could not get a sample");
	}

	var image = get_image_from_sample (sample);
	if (image == null) {
		throw new IOError.FAILED ("Could not get a sample");
	}

	playbin.set_state (Gst.State.NULL);
	playbin.get_state (null, null, -1);

	return image;
}

Gth.Image get_playbin_current_frame (Gst.Pipeline? playbin) throws Error {
	if (playbin == null) {
		throw new IOError.FAILED ("No playbin");
	}
	var sink = playbin.get_by_name ("sink");
	if (sink == null) {
		throw new IOError.FAILED ("Playbin has no 'sink' element");
	}
	Gst.Sample sample = null;
	sink.get ("last-sample", out sample);
	if (sample == null) {
		throw new IOError.FAILED ("Failed to retrieve the last sample");
	}
	return get_image_from_sample (sample);
}

Gth.Image get_image_from_sample (Gst.Sample sample) throws Error {
	// Check the sample format.
	unowned var sample_caps = sample.get_caps ();
	if (sample_caps == null) {
		throw new IOError.FAILED ("No caps on output buffer");
	}

	unowned var cap_struct = sample_caps.get_structure (0);
	unowned var format = cap_struct.get_string ("format");
	if ((format != "RGB") && (format != "RGBA")) {
		throw new IOError.FAILED ("Wrong sample format");
	}

	// Create the image from the sample data.
	int width = 0, height = 0;
	if (!cap_struct.get_int ("width", out width)
		|| !cap_struct.get_int ("height", out height))
	{
		throw new IOError.FAILED ("No size");
	}

	if ((width == 0) || (height == 0)) {
		throw new IOError.FAILED ("Wrong size");
	}

	Gth.Image image = null;
	var memory = sample.get_buffer ().get_memory (0);
	Gst.MapInfo info;
	if (memory.map (out info, Gst.MapFlags.READ)) {
		image = new Gth.Image (width, height);
		var with_alpha = (format == "RGBA");
		var sample_stride = width * (with_alpha ? 4 : 3);
		sample_stride = (sample_stride + 3) & ~3; // Round up to the next multiple of 4
		image.copy_from_rgba_big_endian (info.data, with_alpha, sample_stride);
	}
	memory.unmap (info);

	if (image == null) {
		throw new IOError.FAILED ("Could not create the image");
	}

	return image;
}
