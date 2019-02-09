#pragma once

#include <iostream>
#include <fstream>
#include <string>
#include <cstring>
#include <assert.h>
#include <vector>
#include <stack>
#include <algorithm>
#include <sstream>
#include <cstdio>
#include <windows.h>

namespace stechpp
{

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

	std::string TrimWhitespace(std::string s)
	{
		if (s.size() == 0)
			return s;

		size_t startIndex = 0;
		int chkChar = s[startIndex];
		while (chkChar == ' ' || chkChar == '\t')
		{
			startIndex++;
			if (startIndex >= s.size() - 1)
				break;
			chkChar = s[startIndex];
		}

		int endIndex = s.size() - 1;
		chkChar = s[endIndex];
		while (chkChar == ' ' || chkChar == '\t')
		{
			endIndex--;
			if (endIndex <= 0)
				break;
			chkChar = s[endIndex];
		}

		// start to end, inclusive
		return s.substr(startIndex, (endIndex + 1) - startIndex);
	}

	std::string TrimLineComments(std::string s)
	{
		for (size_t i = 0; i < s.size(); i++)
		{
			if (s[i] == '/')
			{
				if (i + 1 < s.size() && s[i + 1] == '/')
				{
					return s.substr(0, i);
				}
			}
		}

		return s;
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

	// This is mainly in a seperate cpp so i can include windows.h
	// I'm using IN and OUT symbols in stechc.y which conflict with windows.h
	// Dont feel like refactoring atm
	void ErrorAbort(const char* appmsg)
	{
		printf(appmsg);

		abort();
	}

	struct PPArgs
	{
		std::string outFilepath;
		std::vector<std::string> defines;
	};

	PPArgs ReadArgs(int argc, char** argv)
	{
		PPArgs parsedArgs = {};

		// Ignore 0th arg (command), 1st arg (input file)
		for (int i = 2; i < argc; i++)
		{
			std::string strArg = std::string(argv[i]);
			if (StartsWith(strArg, "-d") || StartsWith(strArg, "-D"))
			{
				std::string define = strArg.substr(2);
				parsedArgs.defines.push_back(define);
			}
		}

		return parsedArgs;
	}

	void PreprocessFile(std::string inFilepath, PPArgs parsedArgs)
	{
		std::stack<std::ifstream*> streamStack;
		std::vector<std::string> alreadyIncl;
		std::vector<std::string> definedSymbols;

		alreadyIncl.push_back(inFilepath);

		std::ofstream outputFile;
		outputFile.open(parsedArgs.outFilepath);

		std::ifstream *inputFile = new std::ifstream();
		inputFile->open(inFilepath);

		streamStack.push(inputFile);

		while (!streamStack.empty())
		{
			std::ifstream *nextStream = streamStack.top();

			bool blockCommentActive = false;
			bool preprocessIfBlock = false;
			bool ignoreBlock = false;
			// loop until we either push a new stream on (due to an include)
			// OR we pop a stream off due to eof!
			while (nextStream == streamStack.top())
			{
				// was the previous line we looked at the last line of the file?
				if (nextStream->eof())
				{
					nextStream->close();
					delete nextStream;
					streamStack.pop();
					break;
				}

				std::string rawLine;
				getline(*nextStream, rawLine);

				std::string line = TrimLineComments(rawLine);

				// multi line comments are stateful across multiple lines
				if (blockCommentActive)
				{
					for (size_t i = 0; i < line.size(); i++)
					{
						// end block comment
						if (line[i] == '*')
						{
							if (i + 1 < line.size() && line[i + 1] == '/')
							{
								blockCommentActive = false;
								line = line.substr(i + 1, line.size() - (i + 1));
							}
						}
					}
					// if there wasnt an end block comment on this line, ignore this line
					if (blockCommentActive)
					{
						continue;
					}
				}
				else
				{
					size_t beginIdx = line.size();
					size_t endIdx = line.size();

					for (size_t i = 0; i < line.size(); i++)
					{
						// begin block comment
						if (line[i] == '/')
						{
							if (i + 1 < line.size() && line[i + 1] == '*')
							{
								blockCommentActive = true;
								beginIdx = i;
							}
						}
					}

					// check for block comment end on same line
					// MUST be unsigned here because we're counting down to 0
					for (int i = (int)line.size() - 1; i >= 0; i--)
					{
						// end block comment
						if (line[i] == '/')
						{
							if (i + 1 < (int)line.size() && line[i + 1] == '*')
							{
								blockCommentActive = false;
								endIdx = i;
							}
						}
					}

					// remove the middle portion that is commented out. If no comment,
					// this is still fine cause of default begin/endIdx values
					std::string newLine;
					for (size_t i = 0; i < line.size(); i++)
					{
						if (i < beginIdx || i > endIdx)
						{
							newLine.push_back(line[i]);
						}
					}
				}

				line = TrimWhitespace(line);

				if (preprocessIfBlock)
				{
					if (StartsWith(line, "#endif"))
					{
						ignoreBlock = false;
						outputFile << rawLine.substr(rawLine.find("#endif", 0) + 6) << "\n";
						continue;
					}
					else if (ignoreBlock)
					{
						continue;
					}
				}

				// Handle #include statements
				if (StartsWith(line, "#include"))
				{
					int firstQuote = -1;
					int lastQuote = -1;
					for (size_t i = 8; i < line.size(); i++)
					{
						if (line[i] == '"')
						{
							if (firstQuote == -1)
								firstQuote = i;
							else
							{
								lastQuote = i;
								break;
							}
						}
					}

					// must be a valid quoted string
					assert(firstQuote != -1 && lastQuote != -1);

					// now we have the include file! Open it, push it on the stack, then break this stream's loop so we can parse it.
					std::string inclFilepath = line.substr(firstQuote + 1, lastQuote - firstQuote - 1);

					if (find(alreadyIncl.begin(), alreadyIncl.end(), inclFilepath) != alreadyIncl.end())
					{
						// We've already included this file! files can only be included once!
						std::cout << "Tried to double include " << inclFilepath << std::endl;
						std::cout << "Aborting..." << std::endl;
						abort();
					}

					std::ifstream *inclFile = new std::ifstream();
					inclFile->open(inclFilepath);

					if (inclFile->fail())
					{
						std::cout << "Unable to open file " << inclFilepath << std::endl;
						std::cout << "Aborting..." << std::endl;
						abort();
					}

					alreadyIncl.push_back(inclFilepath);
					streamStack.push(inclFile);
					break;
				}
				else if (StartsWith(line, "#define"))
				{
					std::istringstream iss(line);

					std::string symbol;

					iss >> symbol;
					symbol.clear();
					iss >> symbol;

					if (symbol.size() == 0)
					{
						std::cout << "Couldn't find symbol for define" << std::endl;
						std::cout << "Aborting..." << std::endl;
						abort();
					}

					parsedArgs.defines.push_back(symbol);
				}
				else if (StartsWith(line, "#ifdef"))
				{
					std::istringstream iss(line);

					std::string symbol;

					iss >> symbol;
					symbol.clear();
					iss >> symbol;

					if (symbol.size() == 0)
					{
						std::cout << "Couldn't find symbol for ifdef" << std::endl;
						std::cout << "Aborting..." << std::endl;
						abort();
					}

					preprocessIfBlock = true;
					if (find(parsedArgs.defines.begin(), parsedArgs.defines.end(), symbol) == parsedArgs.defines.end())
					{
						// THIS SYMBOL WAS NOT DEFINED
						ignoreBlock = true;
					}
					else
					{
						// THIS SYMBOL WAS DEFINED
						// do nothing
					}
				}
				else
				{
					// Else its a non preprocessor line, so we just print to file!
					outputFile << rawLine << "\n";
				}
			}
		}

		outputFile.flush();
		outputFile.close();
	}
}