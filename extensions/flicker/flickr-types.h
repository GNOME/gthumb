/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2010 Free Software Foundation, Inc.
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
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef FLICKR_TYPES_H
#define FLICKR_TYPES_H

typedef enum  {
	FLICKR_SAFETY_LEVEL_SAFE = 1,
	FLICKR_SAFETY_LEVEL_MODERATE = 2,
	FLICKR_SAFETY_LEVEL_RESTRICTED = 3
} FlickrSafetyLevel;

typedef enum  {
	FLICKR_CONTENT_TYPE_PHOTO = 1,
	FLICKR_CONTENT_TYPE_SCREENSHOT = 2,
	FLICKR_CONTENT_TYPE_OTHER = 3
} FlickrContentType;

typedef enum  {
	FLICKR_HIDDEN_PUBLIC = 1,
	FLICKR_HIDDEN_HIDDEN = 2,
} FlickrHiddenType;

typedef enum {
	FLICKR_SIZE_SMALL_SQUARE = 75,
	FLICKR_SIZE_THUMBNAIL = 100,
	FLICKR_SIZE_SMALL = 240,
	FLICKR_SIZE_MEDIUM = 500,
	FLICKR_SIZE_LARGE = 1024
} FlickrSize;

typedef struct {
	const char *name;
	const char *url;
	const char *authentication_url;
	const char *rest_url;
	const char *upload_url;
	const char *api_key;
	const char *shared_secret;
} FlickrServer;

#endif /* FLICKR_TYPES_H */
