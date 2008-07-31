%{
/*
 *  GThumb
 *
 *  Copyright (C) 2003 Free Software Foundation, Inc.
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

#include <glib.h>

%}

number           [0-9]+
name             [a-zA-Z_][0-9a-zA-Z_:@]*
string           ([^'\n]|\\')*

%x TAG STAG STR SSTR

%%

"<gthumb:header"		{ 
					BEGIN (TAG);
					yylval.ivalue = GTH_TAG_HEADER;
					return HEADER;
				}
"<gthumb:footer"		{ 
					BEGIN (TAG);
					yylval.ivalue = GTH_TAG_FOOTER;
					return FOOTER;
				}
"<gthumb:language"		{ 
					BEGIN (TAG);
					yylval.ivalue = GTH_TAG_LANGUAGE;
					return LANGUAGE;
				}
"<gthumb:theme_link"		{ 
					BEGIN (TAG);
					yylval.ivalue = GTH_TAG_THEME_LINK;
					return THEME_LINK;
				}
"<gthumb:image"			{ 
					BEGIN (TAG);
					yylval.ivalue = GTH_TAG_IMAGE;
					return IMAGE;
				}
"<gthumb:image_link"		{ 
					BEGIN (TAG);
					yylval.ivalue = GTH_TAG_IMAGE_LINK;
					return IMAGE_LINK;
				}
"<gthumb:image_idx"		{ 
					BEGIN (TAG);
					yylval.ivalue = GTH_TAG_IMAGE_IDX;
					return IMAGE_IDX;
				}
"<gthumb:image_dim"		{ 
					BEGIN (TAG);
					yylval.ivalue = GTH_TAG_IMAGE_DIM;
					return IMAGE_DIM;
				}
"<gthumb:images"		{ 
					BEGIN (TAG);
					yylval.ivalue = GTH_TAG_IMAGES;
					return IMAGES;
				}
"<gthumb:file_name"		{ 
					BEGIN (TAG);
					yylval.ivalue = GTH_TAG_FILENAME;
					return FILENAME;
				}
"<gthumb:file_path"		{ 
					BEGIN (TAG);
					yylval.ivalue = GTH_TAG_FILEPATH;
					return FILEPATH;
				}
"<gthumb:file_size"		{ 
					BEGIN (TAG);
					yylval.ivalue = GTH_TAG_FILESIZE;
					return FILESIZE;
				}
"<gthumb:comment"		{ 
					BEGIN (TAG);
					yylval.ivalue = GTH_TAG_COMMENT;
					return COMMENT;
				}
"<gthumb:place"			{
					BEGIN (TAG);
					yylval.ivalue = GTH_TAG_PLACE;
					return PLACE;
				}
"<gthumb:date_time"			{
					BEGIN (TAG);
					yylval.ivalue = GTH_TAG_DATE_TIME;
					return DATE_TIME;
				}
"<gthumb:page_link"		{ 
					BEGIN (TAG);
					yylval.ivalue = GTH_TAG_PAGE_LINK;
					return PAGE_LINK;
				}
"<gthumb:page_idx"		{ 
					BEGIN (TAG);
					yylval.ivalue = GTH_TAG_PAGE_IDX;
					return PAGE_IDX;
				}
"<gthumb:page_rows"		{ 
					BEGIN (TAG);
					yylval.ivalue = GTH_TAG_PAGE_ROWS;
					return PAGE_ROWS;
				}
"<gthumb:page_cols"		{ 
					BEGIN (TAG);
					yylval.ivalue = GTH_TAG_PAGE_COLS;
					return PAGE_COLS;
				}
"<gthumb:pages"			{ 
					BEGIN (TAG);
					yylval.ivalue = GTH_TAG_PAGES;
					return PAGES;
				}
"<gthumb:table"			{ 
					BEGIN (TAG);
					yylval.ivalue = GTH_TAG_TABLE;
					return TABLE;
				}
"<gthumb:thumbs"			{ 
					BEGIN (TAG);
					yylval.ivalue = GTH_TAG_THUMBS;
					return THUMBS;
				}
"<gthumb:date"			{ 
					BEGIN (TAG);
					yylval.ivalue = GTH_TAG_DATE;
					return DATE;
				}
"<gthumb:text"			{ 
					BEGIN (STAG);
					yylval.ivalue = GTH_TAG_TEXT;
					return TEXT;
				}
"</gthumb:text>"		{
					BEGIN (INITIAL);
					return TEXT_END;
				}
"<gthumb:exif:exposure_time"	{ 
					BEGIN (TAG);
					yylval.ivalue = GTH_TAG_EXIF_EXPOSURE_TIME;
					return EXIF_EXPOSURE_TIME;
				}
"<gthumb:exif:exposure_mode"	{ 
					BEGIN (TAG);
					yylval.ivalue = GTH_TAG_EXIF_EXPOSURE_MODE;
					return EXIF_EXPOSURE_MODE;
				}
"<gthumb:exif:flash"		{ 
					BEGIN (TAG);
					yylval.ivalue = GTH_TAG_EXIF_FLASH;
					return EXIF_FLASH;
				}
"<gthumb:exif:shutter_speed"	{ 
					BEGIN (TAG);
					yylval.ivalue = GTH_TAG_EXIF_SHUTTER_SPEED;
					return EXIF_SHUTTER_SPEED;
				}
"<gthumb:exif:aperture_value"	{ 
					BEGIN (TAG);
					yylval.ivalue = GTH_TAG_EXIF_APERTURE_VALUE;
					return EXIF_APERTURE_VALUE;
				}
"<gthumb:exif:focal_length"	{ 
					BEGIN (TAG);
					yylval.ivalue = GTH_TAG_EXIF_FOCAL_LENGTH;
					return EXIF_FOCAL_LENGTH;
				}
"<gthumb:exif:date_time"	{ 
					BEGIN (TAG);
					yylval.ivalue = GTH_TAG_EXIF_DATE_TIME;
					return EXIF_DATE_TIME;
				}
"<gthumb:exif:camera_model"	{ 
					BEGIN (TAG);
					yylval.ivalue = GTH_TAG_EXIF_CAMERA_MODEL;
					return EXIF_CAMERA_MODEL;
				}
"<gthumb:set_var"		{ 
					BEGIN (TAG);
					yylval.ivalue = GTH_TAG_SET_VAR;
					return SET_VAR;
				}
"<gthumb:eval"			{
					BEGIN (TAG);
					yylval.ivalue = GTH_TAG_EVAL;
					return EVAL;
				}
"<gthumb:if"			{ 
					BEGIN (TAG);
					return IF;
				}
"<gthumb:else"			{ 
					BEGIN (TAG);
					return ELSE;
				}
"<gthumb:else_if"		{ 
					BEGIN (TAG);
					return ELSE_IF;
				}
"<gthumb:end"			{ 
					BEGIN (TAG);
					return END;
				}
<TAG>"'"			{
					BEGIN (STR);
					return '\'';
				}
<STAG>"'"			{
					BEGIN (SSTR);
					return '\'';
				}
<STR,SSTR>{string}		{
					yylval.text = g_strdup (yytext);
					return STRING;
				}
<STR>"'"			{
					BEGIN (TAG);
					return '\'';
				}
<SSTR>"'"			{
					BEGIN (STAG);
					return '\'';
				}
<TAG,STAG>{number}		{
					yylval.ivalue = atoi (yytext);
					return NUMBER;
				}
<TAG,STAG>{name}		{
					yylval.text = g_strdup (yytext);
					return NAME;
				}
<TAG,STAG>"="			{
					return '=';
				}
<TAG,STAG>"<"			{
					yylval.ivalue = GTH_OP_CMP_LT;
					return COMPARE;
				}
<TAG>">"			{
					yylval.ivalue = GTH_OP_CMP_GT;
					return COMPARE;
				}
<STAG>">"		        {
					BEGIN (INITIAL);
					return END_TAG;
				}
<TAG,STAG>"+"			{
					return '+';
				}
<TAG,STAG>"-"			{
					return '-';
				}
<TAG,STAG>"*"			{
					return '*';
				}
<TAG,STAG>"/"			{
					return '/';
				}
<TAG,STAG>"!"			{
					return '!';
				}
<TAG,STAG>"("			{
					return '(';
				}
<TAG,STAG>")"			{
					return ')';
				}
<TAG,STAG>\"			{
					return '"';
				}
<TAG,STAG>"&&"			{
					yylval.ivalue = GTH_OP_AND;
					return BOOL_OP;
				}
<TAG,STAG>"||"			{
					yylval.ivalue = GTH_OP_OR;
					return BOOL_OP;
				}
<TAG,STAG>"=="			{
					yylval.ivalue = GTH_OP_CMP_EQ;
					return COMPARE;
				}
<TAG,STAG>"!="			{
					yylval.ivalue = GTH_OP_CMP_NE;
					return COMPARE;
				}
<TAG,STAG>"<="			{
					yylval.ivalue = GTH_OP_CMP_LE;
					return COMPARE;
				}
<TAG,STAG>">="			{
					yylval.ivalue = GTH_OP_CMP_GE;
					return COMPARE;
				}
<TAG,STAG>[ \t\n]		{
					/* Eat spaces inside tag. */
				}
<TAG>"/>"		        {
					BEGIN (INITIAL);
					return END_TAG;
				}
"<"				{
					yylval.text = g_strdup (yytext);
					return HTML;
				}
[^<]{1,100}			{
					yylval.text = g_strdup (yytext);
					return HTML;
				}
<<EOF>>				{
					return 0;
				}

%%

