#pragma once

// so we dont have to use the _s version of cstring functions
#define _CRT_SECURE_NO_WARNINGS

#include <cstdio>
#include <iostream>
#include <sstream>
#include <fstream>
#include <string>
#include <cstring>
#include <vector>
#include <map>
#include <algorithm>
#include <assert.h>

// Declare stuff from Flex that Bison needs to know about:
extern int yylex();
extern int yyparse();
extern FILE *yyin;

void yyerror(const char *s);

namespace stechc
{

	enum ConfigState
	{
		CONFIGSTATE_UNDEFINED,
		CONFIGSTATE_IN,
		CONFIGSTATE_OUT
	};

	// structs for the config file
	struct ConfigSemanticBinding
	{
		std::string semanticName;
		std::string varName;
		int bindingIdx;
		ConfigState state;
	};

	struct ConfigData
	{
		std::map<std::string, ConfigSemanticBinding> semanticBindings;
	};

	// structs for the shadertech file
	struct RawDeclaration
	{
		int order;
		std::string code;
	};

	struct LayoutTerm
	{
		int bindingIndex;
	};

	struct MatConstant
	{
		int order;
		LayoutTerm layoutTerm;
		std::string resourceType;
		std::string varType;
		std::string varName;
	};

	struct SemanticVar
	{
		std::string varType;
		std::string varName;
		std::string varSemantic;
	};

	struct TransferDeclaration
	{
		std::string inName;
		std::string outName;
		std::vector<SemanticVar*> *varArray;
	};

	struct FunctionDeclaration
	{
		std::string functionName;
		std::string functionBody;
	};

	struct ShaderTechDeclaration
	{
		std::string techName;
		std::string vertFunc;
		std::string fragFunc;
	};

	struct ShaderTechFileDeclarations
	{
		std::vector<RawDeclaration> rawArr;
		std::vector<MatConstant> constArr;
		std::vector<TransferDeclaration> transArr;
		std::vector<FunctionDeclaration> funcArr;
		std::vector<ShaderTechDeclaration> stechArr;
	};

	struct CArgs
	{
		std::string outputDir;
	};

	void ErrorAbort(const char* appmsg);

	bool StartsWith(std::string s, std::string prefix);

	char* StringConcat(const char *s1, const char *s2);

	char* StringConcatWithNewLine(const char *s1, const char *s2);

	void OutputMatConstsAndRaw(std::ofstream& outFile, ConfigData& config, std::vector<MatConstant>& constArr, std::vector<RawDeclaration>& rawArr);

	void OutputVertexShader(std::string filepath, ShaderTechDeclaration& stech, ConfigData& config, ShaderTechFileDeclarations& declarations);

	void OutputFragmentShader(std::string filepath, ShaderTechDeclaration& stech, ConfigData& config, ShaderTechFileDeclarations& declarations);

	ConfigData ParseConfigFile(std::string filepath);

	std::vector<std::string> OutputFinalShaders(std::string outputExtraDir, ShaderTechFileDeclarations& declarations);

	CArgs ReadArgs(int argc, char** argv);

	std::vector<std::string> CompileShaderTech(std::string inFilepath, CArgs args);

}