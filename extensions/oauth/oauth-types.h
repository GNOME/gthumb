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

#ifndef OAUTH_TYPES_H
#define OAUTH_TYPES_H

typedef struct {
	const char *name;
	const char *url;
	const char *protocol;
	const char *request_token_url;
	const char *user_authorization_url;
	const char *access_token_url;
	const char *consumer_key;
	const char *consumer_secret;
	const char *login_request_uri;
	void      (* login_request_result) (SoupMessage        *msg,
					    SoupBuffer         *body,
					    GSimpleAsyncResult *result);
} OAuthConsumer;

#endif /* OAUTH_TYPES_H */
