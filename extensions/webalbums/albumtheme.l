%{
/*
 *  GThumb
 *
 *  Copyright (C) 2003, 2010 Free Software Foundation, Inc.
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
#include <gio/gio.h>

GFileInputStream *yy_istream;

#undef YY_INPUT
#define YY_INPUT(buf,result,max_size)									\
{													\
	gssize n = g_input_stream_read (G_INPUT_STREAM (yy_istream), buf, max_size, NULL, NULL);	\
	result = (n <= 0 ? YY_NULL : n);								\
}

%}

%option noinput nounput

number           [0-9]+
name             [a-zA-Z_][0-9a-zA-Z_:@]*
string           ([^'\n]|\\')*

%x TAG STR

%%

"<gthumb:theme-link"		{
					BEGIN (TAG);
					yylval.ivalue = GTH_TAG_THEME_LINK;
					return THEME_LINK;
				}
"<gthumb:image"			{
					BEGIN (TAG);
					yylval.ivalue = GTH_TAG_IMAGE;
					return IMAGE;
				}
"<gthumb:image-link"		{
					BEGIN (TAG);
					yylval.ivalue = GTH_TAG_IMAGE_LINK;
					return IMAGE_LINK;
				}
"<gthumb:image-index"		{
					BEGIN (TAG);
					yylval.ivalue = GTH_TAG_IMAGE_IDX;
					return IMAGE_IDX;
				}
"<gthumb:images"		{
					BEGIN (TAG);
					yylval.ivalue = GTH_TAG_IMAGES;
					return IMAGES;
				}
"<gthumb:page-link"		{
					BEGIN (TAG);
					yylval.ivalue = GTH_TAG_PAGE_LINK;
					return PAGE_LINK;
				}
"<gthumb:page-index"		{
					BEGIN (TAG);
					yylval.ivalue = GTH_TAG_PAGE_IDX;
					return PAGE_IDX;
				}
"<gthumb:table"			{
					BEGIN (TAG);
					yylval.ivalue = GTH_TAG_TABLE;
					return TABLE;
				}
"<gthumb:thumbs"		{
					BEGIN (TAG);
					yylval.ivalue = GTH_TAG_THUMBS;
					return THUMBS;
				}
"<gthumb:text"			{
					BEGIN (TAG);
					yylval.ivalue = GTH_TAG_TEXT;
					return TEXT;
				}
"</gthumb:text>"		{
					BEGIN (INITIAL);
					return TEXT_END;
				}
"<gthumb:set"			{
					BEGIN (TAG);
					yylval.ivalue = GTH_TAG_SET;
					return SET;
				}
"<gthumb:get"    	        {
					BEGIN (TAG);
					yylval.ivalue = GTH_TAG_GET;
					return GET;
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
<STR>{string}			{
					yylval.text = g_strdup (yytext);
					return STRING;
				}
<STR>"'"			{
					BEGIN (TAG);
					return '\'';
				}
<TAG>{number}			{
					yylval.ivalue = atoi (yytext);
					return NUMBER;
				}
<TAG>{name}			{
					yylval.text = g_strdup (yytext);
					return NAME;
				}
<TAG>"="			{
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

