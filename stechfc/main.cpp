// so we dont have to use the _s version of cstring functions
#define _CRT_SECURE_NO_WARNINGS

#include <iostream>
#include <fstream>
#include <string>
#include <cstring>
#include <assert.h>
#include <vector>
#include <stack>
#include <algorithm>
#include <sstream>
#include <filesystem>

#include "../stechc/stechc.h"
#include "../stechpp/stechpp.h"

void ErrorAbort(const char* appmsg)
{
	printf(appmsg);

	abort();
}

bool StartsWith(std::string s, std::string prefix)
{
	for (size_t i = 0; i < prefix.size(); i++)
	{
		if (i >= s.size() || prefix[i] != s[i])
		{
			return false;
		}
	}
	return true;
}

std::string ConstructOutputPath(std::string odir, std::string filename)
{
	std::string compiledPath;
	if (odir.compare("") != 0)
	{
		compiledPath = odir + "/" + filename + "spv";
	}
	else
	{
		compiledPath = filename + "spv";
	}
	return compiledPath;
}

std::string GetFilenameFromPath(std::string filepath)
{
	std::string filename;
	for (int i = filepath.size() - 1; i >= 0; i--)
	{
		if (filepath[i] != '\\' && filepath[i] != '/')
			filename.push_back(filepath[i]);
		else
			break;
	}
	std::reverse(filename.begin(), filename.end());
	return filename;
}

int main(int argc, char **argv)
{
	// Long namespace, lets just give it a nickname
	namespace fs = std::experimental::filesystem;

	if (argc == 1)
	{
		std::cout << "Usage: stechfc.exe <Input Filepath> [-idir <intermediateDirPath>] [-odir <ouputDirPath>] [-d<symbol>]..." << std::endl;
		return 0;
	}

	// first arg is path of exe
	// second arg is file path of our source stech file to compile
	std::string filepath(argv[1]);

	// Preprocessor args
	stechpp::PPArgs ppArgs;
	// stech compiler args
	stechc::CArgs cArgs;

	// Default preprocessor output filepath, can be overridden below
	ppArgs.outFilepath = GetFilenameFromPath(filepath) + ".stechpp";

	std::string finalODir = "compiled";

	// Ignore 0th arg (command), 1st arg (input file)
	for (int i = 2; i < argc; i++)
	{
		std::string strArg = std::string(argv[i]);
		// Specify a final output dir for glslc output
		if (strArg.compare("-ODIR") == 0 || strArg.compare("-odir") == 0)
		{
			// Error case. They didnt provide an output folder at all!
			if (i + 1 >= argc)
			{
				ErrorAbort("-odir wasn't provided a output folder string!");
			}
			else
			{
				finalODir = std::string(argv[i + 1]);
				i++;
				continue;
			}
		}
		// Specify a intermediate dir for stechpp and stechc output
		else if (strArg.compare("-IDIR") == 0 || strArg.compare("-idir") == 0)
		{
			// Error case. They didnt provide an output folder at all!
			if (i + 1 >= argc)
			{
				ErrorAbort("-idir wasn't provided a output folder string!");
			}
			else
			{
				ppArgs.outFilepath = std::string(argv[i + 1]) + "/" + GetFilenameFromPath(filepath) + ".stechpp";
				cArgs.outputDir = std::string(argv[i + 1]);
				i++;
				continue;
			}
		}
		// Defines a symbol for the preprocessor
		else if (StartsWith(strArg, "-d") || StartsWith(strArg, "-D"))
		{
			std::string define = strArg.substr(2);
			ppArgs.defines.push_back(define);
		}
	}

	if(cArgs.outputDir.compare("") != 0)
	{ 
		// Create intermediate dir
		fs::create_directory(cArgs.outputDir);
	}
	if(finalODir.compare("") != 0)
	{ 
		// Create output dir
		fs::create_directory(finalODir);
	}


	// First, execute preprocessor (stechpp) on the file.
	stechpp::PreprocessFile(filepath, ppArgs);

	// Second, execute the shader tech compiler.
	std::vector<std::string> files = stechc::CompileShaderTech(ppArgs.outFilepath, cArgs);

	// Third, execute the glsl compiler in the VulkanSDK for all the outputs
	std::string glslCompilerPath = "%VULKAN_SDK%/Bin/glslc.exe";

	for (std::string rawPath : files)
	{
		auto p = fs::path(rawPath);
		// if this is any of the file extensions we support, compile!
		if (p.extension().compare(".vert") == 0)
		{
			std::string compiledPath = ConstructOutputPath(finalODir, p.filename().string());
			system((glslCompilerPath + " " + p.string() + " -DVERT --target-env=vulkan1.0 -o " + compiledPath).c_str());
		}
		else if (p.extension().compare(".frag") == 0)
		{
			std::string compiledPath = ConstructOutputPath(finalODir, p.filename().string());
			system((glslCompilerPath + " " + p.string() + " -DFRAG --target-env=vulkan1.0 -o " + compiledPath).c_str());
		}
	}

	return 0;
}