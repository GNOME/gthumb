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
	FACEBOOK_VISIBILITY_EVERYONE,
	FACEBOOK_VISIBILITY_NETWORKS_FRIENDS,
	FACEBOOK_VISIBILITY_FRIENDS_OF_FRIENDS,
	FACEBOOK_VISIBILITY_ALL_FRIENDS,
	FACEBOOK_VISIBILITY_SELF,
	FACEBOOK_VISIBILITY_CUSTOM
} FacebookVisibility;


#endif /* FACEBOOK_TYPES_H */
