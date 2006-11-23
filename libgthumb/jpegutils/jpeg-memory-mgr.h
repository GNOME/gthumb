/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 *  GThumb
 *
 *  Copyright (C) 2001, 2002 The Free Software Foundation, Inc.
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

#ifndef JPEG_MEMORY_MGR_H
#define JPEG_MEMORY_MGR_H

#include <stdio.h>
#include <jpeglib.h>

GLOBAL(void)
jpeg_memory_src (j_decompress_ptr cinfo, const void *data, size_t size);

GLOBAL(void)
jpeg_memory_dest (j_compress_ptr cinfo, void **data, size_t *size);

#endif /* JPEG_MEMORY_MGR_H */
