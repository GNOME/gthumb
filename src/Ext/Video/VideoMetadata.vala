using Gth;

namespace VideoMetadata {
	public void register_info () {
		MetadataInfo.register ("Media::Artist", N_("Artist"), "Metadata", METADATA_ALLOW_EVERYWHERE);
		MetadataInfo.register ("Media::Album", N_("Album"), "Metadata", METADATA_ALLOW_EVERYWHERE);
		MetadataInfo.register ("Video::Codec", N_("Video Codec"), "Metadata", MetadataFlags.ALLOW_IN_PROPERTIES_VIEW);
		MetadataInfo.register ("Audio::Codec", N_("Audio Codec"), "Metadata", MetadataFlags.ALLOW_IN_PROPERTIES_VIEW);
		MetadataInfo.register ("Media::Encoder", N_("Encoder"), "Metadata", MetadataFlags.ALLOW_IN_PROPERTIES_VIEW);
		MetadataInfo.register ("Video::FrameRate", N_("Framerate"), "Metadata", MetadataFlags.ALLOW_IN_PROPERTIES_VIEW);
		MetadataInfo.register ("Audio::Channels", N_("Channels"), "Metadata", MetadataFlags.ALLOW_IN_PROPERTIES_VIEW);
		MetadataInfo.register ("Audio::SampleRate", N_("Sample Rate"), "Metadata", MetadataFlags.ALLOW_IN_PROPERTIES_VIEW);
		// MetadataInfo.register ("Video::Bitrate", N_("Video Bitrate"), "Other", MetadataFlags.ALLOW_IN_PROPERTIES_VIEW);
		// MetadataInfo.register ("Audio::Bitrate", N_("Audio Bitrate"), "Other", MetadataFlags.ALLOW_IN_PROPERTIES_VIEW);
		// MetadataInfo.register ("Video::Width", N_("Width"), "Other", MetadataFlags.ALLOW_IN_PROPERTIES_VIEW);
		// MetadataInfo.register ("Video::Height", N_("Height"), "Other", MetadataFlags.ALLOW_IN_PROPERTIES_VIEW);
	}
}
