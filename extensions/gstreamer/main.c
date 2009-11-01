/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2009 Free Software Foundation, Inc.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Street #330, Boston, MA 02111-1307, USA.
 */


#include <config.h>
#include <gtk/gtk.h>
#include <gthumb.h>
#include "gth-metadata-provider-gstreamer.h"


GthMetadataCategory gstreamer_metadata_category[] = {
	{ "audio-video::general", N_("General"), 30 },
	{ "audio-video::video", N_("Video"), 40 },
	{ "audio-video::audio", N_("Audio"), 50 },
	{ "audio-video::other", N_("Other"), 60 },
	{ NULL, NULL, 0 }
};


GthMetadataInfo gstreamer_metadata_info[] = {
	{ "audio-video::general::title", N_("Title"), "audio-video::general", 1, GTH_METADATA_ALLOW_EVERYWHERE },
	{ "audio-video::general::artist", N_("Artist"), "audio-video::general", 2, GTH_METADATA_ALLOW_EVERYWHERE },
	{ "audio-video::general::album", N_("Album"), "audio-video::general", 3, GTH_METADATA_ALLOW_EVERYWHERE },
	{ "audio-video::general::duration", N_("Duration"), "audio-video::general", 4, GTH_METADATA_ALLOW_EVERYWHERE },
	{ "audio-video::general::container-format", N_("Container Format"), "audio-video::general", 5, GTH_METADATA_ALLOW_IN_PROPERTIES_VIEW },
	{ "audio-video::general::encoder", N_("Encoder"), "audio-video::general", 6, GTH_METADATA_ALLOW_IN_PROPERTIES_VIEW },

	{ "audio-video::video::size", N_("Dimensions"), "audio-video::video", 1, GTH_METADATA_ALLOW_EVERYWHERE },
	{ "audio-video::video::codec", N_("Codec"), "audio-video::video", 2, GTH_METADATA_ALLOW_EVERYWHERE },
	{ "audio-video::video::framerate", N_("Framerate"), "audio-video::video", 3, GTH_METADATA_ALLOW_IN_PROPERTIES_VIEW },
	{ "audio-video::video::bitrate", N_("Bitrate"), "audio-video::video", 4, GTH_METADATA_ALLOW_IN_PROPERTIES_VIEW },
	{ "audio-video::video::width", N_("Width"), "audio-video::video", 0, GTH_METADATA_ALLOW_NOWHERE },
	{ "audio-video::video::height", N_("Height"), "audio-video::video", 0, GTH_METADATA_ALLOW_NOWHERE },

	{ "audio-video::audio::codec", N_("Codec"), "audio-video::audio", 1, GTH_METADATA_ALLOW_IN_PROPERTIES_VIEW },
	{ "audio-video::audio::channels", N_("Channels"), "audio-video::audio", 2, GTH_METADATA_ALLOW_IN_PROPERTIES_VIEW },
	{ "audio-video::audio::samplerate", N_("Sample rate"), "audio-video::audio", 3, GTH_METADATA_ALLOW_IN_PROPERTIES_VIEW },
	{ "audio-video::audio::bitrate", N_("Bitrate"), "audio-video::audio", 4, GTH_METADATA_ALLOW_IN_PROPERTIES_VIEW },

	{ NULL, NULL, NULL, 0, 0 }
};


G_MODULE_EXPORT void
gthumb_extension_activate (void)
{
	gth_main_register_metadata_category (gstreamer_metadata_category);
	gth_main_register_metadata_info_v (gstreamer_metadata_info);
	gth_main_register_metadata_provider (GTH_TYPE_METADATA_PROVIDER_GSTREAMER);
}


G_MODULE_EXPORT void
gthumb_extension_deactivate (void)
{
}


G_MODULE_EXPORT gboolean
gthumb_extension_is_configurable (void)
{
	return FALSE;
}


G_MODULE_EXPORT void
gthumb_extension_configure (GtkWindow *parent)
{
}
