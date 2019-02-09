%{
	#include <cstdio>
	#include <cstring>
	
	#include "stechc.tab.h"

	int gStageFunctionScopeCount = 0;
%}
%option noyywrap

%x SHADERTECHBLOCK
%x STAGEFUNCTION
%x RAWBLOCK
%x LAYOUTPARAMS

singleLineComment "//".*
multiLineComment "/*"([^*]|(\*+[^*/]))*\*+\/
identifier [a-zA-Z_]+[a-zA-Z_0-9]*
strLit \"[a-zA-Z_0-9]+\"
constArrayDecl (\[[0-9]+\])+

%%
<INITIAL>
{
	{singleLineComment}	{ return COMMENT; }
	{multiLineComment}	{ return COMMENT; }
	[ \t\n]				
	in					{ return IN; }
	out					{ return OUT; }
	\-\>				{ return ARROW; }
	:					{ return IS; }
	;					{ return ENDCMD; }
	shadertech			{ 
							BEGIN SHADERTECHBLOCK;
							return SHADERTECH; 
						}
	BEGINRAW			{
							BEGIN RAWBLOCK;
							return RAWSTART;
						}

	uniform				{ return UNIFORM; }
	\{					{ return STARTBRACKET; }
	\}					{ return ENDBRACKET; }
	layout				{ 
							BEGIN LAYOUTPARAMS;
							return LAYOUTPROP; 
						}
	{strLit}			{ yylval.sval = _strdup(yytext); return STRINGLIT; }
	{identifier}		{ yylval.sval = _strdup(yytext); return IDENTIFIER; }
	{identifier}:		{
							BEGIN STAGEFUNCTION; 
							yylval.sval = (char*)malloc(sizeof(char) * yyleng);
							memcpy(yylval.sval, yytext, yyleng-1);
							yylval.sval[yyleng-1] = '\0';
							return STAGEFUNCTIONSTART; 
						}
	{constArrayDecl}	{
							yylval.sval = _strdup(yytext); 
							return CONST_ARRAY; 
						}
}

<SHADERTECHBLOCK>
{
	{singleLineComment}	{ return COMMENT; }
	{multiLineComment}	{ return COMMENT; }
	[ \t\n]
	{strLit}			{ yylval.sval = _strdup(yytext); return STRINGLIT; }
	vert				{ return VERT; }
	frag				{ return FRAG; }
	:					{ return IS; }
	\{					{ return STARTBRACKET; }
	\}					{
							BEGIN INITIAL;
							return ENDBRACKET; 
						}
	{identifier}		{ yylval.sval = _strdup(yytext); return IDENTIFIER; }
	;					{ return ENDCMD; }
}

<STAGEFUNCTION>
{
	{singleLineComment}	{ yylval.sval = _strdup(yytext); return CODE; }
	{multiLineComment}	{ yylval.sval = _strdup(yytext); return CODE; }
	[ \t\n]*\{			{
							gStageFunctionScopeCount++;
							// The first bracket should return the STARTBRACKET SYMBOL, but any other bracket
							// should just be code
							if(gStageFunctionScopeCount == 1)
							{
								return STARTBRACKET;
							}
							else
							{
								// This is a "sub scope"
								yylval.sval = _strdup(yytext); 
								return CODE;
							}
						}
	\}					{
							gStageFunctionScopeCount--;
							if(gStageFunctionScopeCount == 0)
							{
								BEGIN INITIAL;
								return ENDBRACKET; 					
							}
							else
							{
								// This is a "sub scope"
								yylval.sval = _strdup(yytext); 
								return CODE;
							}
						}
	[^\{\}]				{ 
							if(gStageFunctionScopeCount > 0)
							{
								yylval.sval = _strdup(yytext); 
								return CODE;
							}
						}
}

<RAWBLOCK>
{
	[\n]
	[ \t]*ENDRAW[ \t]*	{
							BEGIN INITIAL;
							return RAWEND;
						}
	.*					{ yylval.sval = _strdup(yytext); return CODELINE; }

}

<LAYOUTPARAMS>
{
	[ ]
	binding			{ return BINDING; }
	=				{ return EQUALS; }
	[0-9]+			{ yylval.ival = atoi(yytext); return INTEGER; }
	\(				{ return OPENPAREN; }
	\)				{ BEGIN INITIAL; return CLOSEPAREN; }
}
%%