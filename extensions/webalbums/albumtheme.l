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
attribute_name   [a-zA-Z_][a-zA-Z_0-9:\.-]*
name             [a-zA-Z_][^'" \t\n]*
quote_sign       ['"]

%x FUNCTION ATTRIBUTES CONDITION QUOTE

%%

"<% if"                         {
					BEGIN (CONDITION);
					return IF;
				}
"<% elsif"                      {
					BEGIN (CONDITION);
					return ELSE_IF;
				}
"<% else"                       {
					BEGIN (ATTRIBUTES);
					return ELSE;
				}
"<% end"                        {
					BEGIN (ATTRIBUTES);
					return END;
				}
"<% set_var"		 	{
					BEGIN (ATTRIBUTES);
					return SET_VAR;
				}	
"<% for_each_thumbnail_caption"	{
					BEGIN (ATTRIBUTES);
					yylval.ivalue = GTH_TAG_FOR_EACH_THUMBNAIL_CAPTION;
					return FOR_EACH;
				}
"<%="                           {
					BEGIN (FUNCTION);
					return BEGIN_TAG;
				}
<FUNCTION>{attribute_name}	{
					BEGIN (ATTRIBUTES);
					yylval.text = g_strdup (yytext);
					return FUNCTION_NAME;
				}
"<%text"                        {
					BEGIN (ATTRIBUTES);
					return BEGIN_TEXT_TAG;
				}
"<%/text>"		        {
					BEGIN (INITIAL);
					return END_TEXT_TAG;
				}
<ATTRIBUTES>{quote_sign} 	{
					BEGIN (QUOTE);
					return '"';
				}
<QUOTE>{quote_sign}		{
					BEGIN (ATTRIBUTES);
					return '"';
				}
<ATTRIBUTES,QUOTE,CONDITION>"%>"	{
					BEGIN (INITIAL);
					return END_TAG;
				}
<ATTRIBUTES>{attribute_name}	{
					yylval.text = g_strdup (yytext);
					return ATTRIBUTE_NAME;
				}
<ATTRIBUTES>"="			{
					return '=';
				}
<QUOTE,CONDITION>{number}	{
					yylval.ivalue = atoi (yytext);
					return NUMBER;
				}
<QUOTE,CONDITION>{name}		{
					yylval.text = g_strdup (yytext);
					return QUOTED_NAME;
				}
<QUOTE,CONDITION>"="		{
					return '=';
				}
<QUOTE,CONDITION>"<"		{
					yylval.ivalue = GTH_OP_CMP_LT;
					return COMPARE;
				}
<QUOTE,CONDITION>">"		{
					yylval.ivalue = GTH_OP_CMP_GT;
					return COMPARE;
				}
<QUOTE,CONDITION>"+"		{
					return '+';
				}
<QUOTE,CONDITION>"-"		{
					return '-';
				}
<QUOTE,CONDITION>"*"		{
					return '*';
				}
<QUOTE,CONDITION>"/"		{
					return '/';
				}
<QUOTE,CONDITION>"!"		{
					return '!';
				}
<QUOTE,CONDITION>"("		{
					return '(';
				}
<QUOTE,CONDITION>")"		{
					return ')';
				}
<QUOTE,CONDITION>"&&"		{
					yylval.ivalue = GTH_OP_AND;
					return BOOL_OP;
				}
<QUOTE,CONDITION>"||"		{
					yylval.ivalue = GTH_OP_OR;
					return BOOL_OP;
				}
<QUOTE,CONDITION>"=="		{
					yylval.ivalue = GTH_OP_CMP_EQ;
					return COMPARE;
				}
<QUOTE,CONDITION>"!="		{
					yylval.ivalue = GTH_OP_CMP_NE;
					return COMPARE;
				}
<QUOTE,CONDITION>"<="		{
					yylval.ivalue = GTH_OP_CMP_LE;
					return COMPARE;
				}
<QUOTE,CONDITION>">="		{
					yylval.ivalue = GTH_OP_CMP_GE;
					return COMPARE;
				}
<ATTRIBUTES,QUOTE,CONDITION>[ \t\n]	{
					/* Eat spaces inside tag. */
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
					YY_FLUSH_BUFFER;
					BEGIN(INITIAL);
					yyterminate ();
				}

%%
