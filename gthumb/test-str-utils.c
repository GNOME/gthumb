/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2008 Free Software Foundation, Inc.
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

#include <config.h>
#include <string.h>
#include <locale.h>
#include "glib-utils.h"


static void
test_g_convert_double_to_c_format (void) {
	g_assert_cmpstr (_g_convert_double_to_c_format (0.0, 1), ==, "0.0");
	g_assert_cmpstr (_g_convert_double_to_c_format (0.0, 2), ==, "0.00");
	g_assert_cmpstr (_g_convert_double_to_c_format (0.0, 3), ==, "0.000");
	g_assert_cmpstr (_g_convert_double_to_c_format (1.0, 1), ==, "1.0");
	g_assert_cmpstr (_g_convert_double_to_c_format (10.0, 1), ==, "10.0");
	g_assert_cmpstr (_g_convert_double_to_c_format (100.0, 1), ==, "100.0");
	g_assert_cmpstr (_g_convert_double_to_c_format (1000.0, 1), ==, "1000.0");
	g_assert_cmpstr (_g_convert_double_to_c_format (1000.01, 2), ==, "1000.01");
	g_assert_cmpstr (_g_convert_double_to_c_format (1000.001, 3), ==, "1000.001");
	g_assert_cmpstr (_g_convert_double_to_c_format (1000.0001, 4), ==, "1000.0001");
	g_assert_cmpstr (_g_convert_double_to_c_format (9999.9999, 4), ==, "9999.9999");
	g_assert_cmpstr (_g_convert_double_to_c_format (9999.9999, 5), ==, "9999.99990");
	g_assert_cmpstr (_g_convert_double_to_c_format (9999.9999, 3), ==, "9999.999");
	g_assert_cmpstr (_g_convert_double_to_c_format (9999.9999, 2), ==, "9999.99");
	g_assert_cmpstr (_g_convert_double_to_c_format (9999.9999, 1), ==, "9999.9");
	g_assert_cmpstr (_g_convert_double_to_c_format (-1.0, 1), ==, "-1.0");
	g_assert_cmpstr (_g_convert_double_to_c_format (-1000.0, 1), ==, "-1000.0");
	g_assert_cmpstr (_g_convert_double_to_c_format (-1000.0005, 4), ==, "-1000.0005");
}


int main (int argc, char *argv[]) {
	//setlocale (LC_ALL, "it_IT.UTF-8");
	g_test_init (&argc, &argv, NULL);
	g_test_add_func ("/str-utils/_g_convert_double_to_c_format", test_g_convert_double_to_c_format);
	return g_test_run ();
}
