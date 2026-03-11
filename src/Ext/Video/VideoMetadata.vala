using Gth;

namespace VideoMetadata {
	public void register_info () {
		MetadataInfo.register ("Media::Artist", N_("Artist"), "Metadata", METADATA_ALLOW_EVERYWHERE);
		MetadataInfo.register ("Media::Album", N_("Album"), "Metadata", METADATA_ALLOW_EVERYWHERE);
		//MetadataInfo.register ("Media::Bitrate", N_("Bitrate"), "Metadata", MetadataFlags.ALLOW_IN_PROPERTIES_VIEW);
		MetadataInfo.register ("Video::Codec", N_("Video Codec"), "Metadata", MetadataFlags.ALLOW_IN_PROPERTIES_VIEW);
		MetadataInfo.register ("Audio::Codec", N_("Audio Codec"), "Metadata", MetadataFlags.ALLOW_IN_PROPERTIES_VIEW);
		MetadataInfo.register ("Media::Encoder", N_("Encoder"), "Metadata", MetadataFlags.ALLOW_IN_PROPERTIES_VIEW);
		//MetadataInfo.register ("Media::Framerate", N_("Framerate"), "Metadata", MetadataFlags.ALLOW_IN_PROPERTIES_VIEW);
		//MetadataInfo.register ("Media::Channels", N_("Audio Channels"), "Metadata", MetadataFlags.ALLOW_IN_PROPERTIES_VIEW);
		//MetadataInfo.register ("Media::Samplerate", N_("Samplerate"), "Metadata", MetadataFlags.ALLOW_IN_PROPERTIES_VIEW);
	}
}
