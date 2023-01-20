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
 *  along with this program; if not, see <http://www.gnu.org/licenses/>.
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

#undef YY_FATAL_ERROR
#define YY_FATAL_ERROR(msg) gth_albumtheme_yyfatalerror(msg)

static int before_string = 0; /* the start condition before entering in STRING */

%}

%option noinput nounput

number           [0-9]+
attribute_name   [a-zA-Z_][a-zA-Z_0-9:\.-]*
name             [a-zA-Z_][a-zA-Z_0-9:\.]*
string           [^']*

%x FUNCTION ATTRIBUTES CONDITION QUOTE STRING

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
"<% for_each"			{
					BEGIN (CONDITION);
					yylval.ivalue = GTH_TAG_FOR_EACH_IN_RANGE;
					return FOR_EACH;
				}
"<%="                           {
					BEGIN (FUNCTION);
					return PRINT;
				}
<FUNCTION>{attribute_name}	{
					if (g_str_equal (yytext, "eval") || g_str_equal (yytext, "translate"))
						BEGIN (CONDITION);
					else
						BEGIN (ATTRIBUTES);
					yylval.text = g_strdup (yytext);
					return FUNCTION_NAME;
				}
<ATTRIBUTES,QUOTE,CONDITION>"'"	{
					before_string = YY_START;
					BEGIN (STRING);
					return '\'';
				}
<STRING>"'"			{
					BEGIN (before_string);
					return '\'';
				}
<STRING>{string}		{
					yylval.text = g_strdup (yytext);
					return QUOTED_STRING;
				}
<ATTRIBUTES>"\""		{
					BEGIN (QUOTE);
					return '"';
				}
<QUOTE>"\""			{
					BEGIN (ATTRIBUTES);
					return '"';
				}
<ATTRIBUTES,CONDITION>"%>"	{
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
<QUOTE,CONDITION>","		{
					return ',';
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
<QUOTE,CONDITION>".."		{
					return RANGE;
				}
<QUOTE,CONDITION>"in"		{
					return IN;
				}
<QUOTE,CONDITION>{number}	{
					yylval.ivalue = atoi (yytext);
					return NUMBER;
				}
<QUOTE,CONDITION>{name}		{
					yylval.text = g_strdup (yytext);
					return VARIABLE;
				}
<ATTRIBUTES,QUOTE,CONDITION>[ \t\n]	{
					/* Eat spaces inside tag. */
				}
<INITIAL>"<"			{
					yylval.text = g_strdup (yytext);
					return HTML;
				}
<INITIAL>[^<]{1,100}		{
					yylval.text = g_strdup (yytext);
					return HTML;
				}
<<EOF>>				{
					YY_FLUSH_BUFFER;
					BEGIN(INITIAL);
					yyterminate ();
				}

%%
