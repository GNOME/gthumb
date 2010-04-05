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
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Street #330, Boston, MA 02111-1307, USA.
 */

#ifndef FACEBOOK_TYPES_H
#define FACEBOOK_TYPES_H

typedef enum  {
	FACEBOOK_SAFETY_LEVEL_SAFE = 1,
	FACEBOOK_SAFETY_LEVEL_MODERATE = 2,
	FACEBOOK_SAFETY_LEVEL_RESTRICTED = 3
} FacebookSafetyLevel;

typedef enum  {
	FACEBOOK_CONTENT_TYPE_PHOTO = 1,
	FACEBOOK_CONTENT_TYPE_SCREENSHOT = 2,
	FACEBOOK_CONTENT_TYPE_OTHER = 3
} FacebookContentType;

typedef enum  {
	FACEBOOK_HIDDEN_PUBLIC = 1,
	FACEBOOK_HIDDEN_HIDDEN = 2,
} FacebookHiddenType;

typedef enum {
	FACEBOOK_SIZE_SMALL_SQUARE = 75,
	FACEBOOK_SIZE_THUMBNAIL = 100,
	FACEBOOK_SIZE_SMALL = 240,
	FACEBOOK_SIZE_MEDIUM = 500,
	FACEBOOK_SIZE_LARGE = 1024
} FacebookSize;

#endif /* FACEBOOK_TYPES_H */
