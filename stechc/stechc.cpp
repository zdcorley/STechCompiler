#include "stechc.h"

void stechc::ErrorAbort(const char* appmsg)
{
	printf(appmsg);

	abort();
}

bool stechc::StartsWith(std::string s, std::string prefix)
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

char* stechc::StringConcat(const char *s1, const char *s2)
{
	char *newBuf = new char[strlen(s2) + strlen(s1) + 1];
	strcpy(newBuf, s1);
	strcat(newBuf, s2);

	return newBuf;
}

char* stechc::StringConcatWithNewLine(const char *s1, const char *s2)
{
	// +2 for new lines, +1 for null term
	char *newBuf = new char[strlen(s2) + strlen(s1) + 2 + 1];
	strcpy(newBuf, s1);
	strcat(newBuf, "\n");
	strcat(newBuf, s2);

	return newBuf;
}

void stechc::OutputMatConstsAndRaw(std::ofstream& outFile, ConfigData& config, std::vector<MatConstant>& constArr, std::vector<RawDeclaration>& rawArr)
{
	int genBindingIndex = 0;
	std::vector<int> takenBindingIndices;
	for (size_t i = 0; i < constArr.size(); i++)
	{
		if (constArr[i].layoutTerm.bindingIndex != -1)
		{
			takenBindingIndices.push_back(constArr[i].layoutTerm.bindingIndex);
		}
	}

	// The raw and const arrays are pre sorted, so this is basically a merge algorithm
	size_t orderIdx = 0;
	size_t rawIdx = 0;
	size_t constIdx = 0;

	while (rawIdx < rawArr.size() || constIdx < constArr.size())
	{
		if (rawIdx < rawArr.size() && rawArr[rawIdx].order == orderIdx)
		{
			orderIdx++;
			rawIdx++;
			outFile << rawArr[rawIdx - 1].code << "\n";
		}
		else if (constIdx < constArr.size() && constArr[constIdx].order == orderIdx)
		{
			bool inlineType = false;
			for (size_t i = 0; i < constArr[constIdx].varType.size(); i++)
			{
				if (constArr[constIdx].varType[i] == '{')
				{
					inlineType = true;
				}
			}

			orderIdx++;
			constIdx++;
			int idx = constIdx - 1;
			if (constArr[idx].layoutTerm.bindingIndex != -1)
			{
				// Always use the cross platform standard data packing when using an inline uniform struct
				outFile << std::string("\nlayout(") + (inlineType ? "std140, " : "") + std::string("binding = ") << constArr[idx].layoutTerm.bindingIndex << ")";
			}
			else
			{
				while (find(takenBindingIndices.begin(), takenBindingIndices.end(), genBindingIndex) != takenBindingIndices.end())
				{
					// Loop until we find a binding index that isnt taken
					genBindingIndex++;
				}
				// Always use the cross platform standard data packing when using an inline uniform struct
				outFile << std::string("\nlayout(") + (inlineType ? "std140, " : "") + std::string("binding = ") << genBindingIndex << ")";
				takenBindingIndices.push_back(genBindingIndex);
			}
			outFile << " " << constArr[idx].resourceType << " " << constArr[idx].varType << " " << constArr[idx].varName << ";\n\n";
		}
	}
}

void stechc::OutputVertexShader(std::string filepath, ShaderTechDeclaration& stech, ConfigData& config, ShaderTechFileDeclarations& declarations)
{
	std::ofstream outFile;
	outFile.open(filepath);

	if (outFile.fail())
	{
		ErrorAbort("ERROR: failed to open output vert file\n");
	}

	outFile << "#version 450\n#extension GL_ARB_separate_shader_objects : enable\n";

	// First, output material constants and raw direct into the shader, filling in the binding ids
	OutputMatConstsAndRaw(outFile, config, declarations.constArr, declarations.rawArr);

	// Then, use transfer declarations/semantics to output correct
	// data transitions between stages/input/output

	std::string vertFunctionBody;

	for (size_t i = 0; i < declarations.funcArr.size(); i++)
	{
		if (declarations.funcArr[i].functionName.compare(stech.vertFunc) == 0)
		{
			vertFunctionBody = declarations.funcArr[i].functionBody;
		}
	}

	for (size_t i = 0; i < declarations.transArr.size(); i++)
	{
		if (declarations.transArr[i].inName.compare(stech.vertFunc) == 0)
		{
			if (declarations.transArr[i].outName.compare("out") == 0)
			{
				// Outputting from the vert shader is not supported
				ErrorAbort("ERROR: Outputting from the vert shader is not supported\n");
			}
			else if (declarations.transArr[i].outName.compare(stech.fragFunc) == 0)
			{
				// When the vert function is the input in a transfer declaration
				// it defines the output layout of the vert shader
				std::vector<SemanticVar*> varArr = *declarations.transArr[i].varArray;

				for (size_t i = 0; i < varArr.size(); i++)
				{
					// If there is not a semantic
					if (varArr[i]->varSemantic.compare("") == 0)
					{
						outFile << "layout(location = " << i << ") out " << varArr[i]->varType << " " << varArr[i]->varName << ";\n";
					}
					else
					{
						// Semantics BETWEEN shader stages are not supported
						ErrorAbort("ERROR: Semantics BETWEEN shader stages are not supported\n");
					}
				}
			}
		}
		else if (declarations.transArr[i].outName.compare(stech.vertFunc) == 0)
		{
			if (declarations.transArr[i].inName.compare("in") == 0)
			{
				// When the vert function is the output in a transfer declaration
				// it defines the vertex input layout of the vert shader
				std::vector<SemanticVar*> varArr = *declarations.transArr[i].varArray;

				for (size_t i = 0; i < varArr.size(); i++)
				{
					// If there is a not semantic
					if (varArr[i]->varSemantic.compare("") == 0)
					{
						// Inputs into the vert shader MUST have semantics
						ErrorAbort("ERROR: Inputs into the vert shader MUST have semantics\n");
					}
					else
					{
						int bindingIdx = config.semanticBindings[varArr[i]->varSemantic].bindingIdx;
						assert(bindingIdx != -1);
						outFile << "layout(location = " << bindingIdx << ") in " << varArr[i]->varType << " " << varArr[i]->varName << ";\n";
					}
				}
			}
			else
			{
				// Input into the vert shader from other shader stages is not supported
				ErrorAbort("ERROR: Input into the vert shader from other shader stages is not supported\n");
			}
		}
	}

	// Then, output raw function
	outFile << "\nvoid main()\n{\n" << vertFunctionBody << "\n}";

	outFile.flush();
	outFile.close();
}

void stechc::OutputFragmentShader(std::string filepath, ShaderTechDeclaration& stech, ConfigData& config, ShaderTechFileDeclarations& declarations)
{
	std::ofstream outFile;
	outFile.open(filepath);

	if (outFile.fail())
	{
		ErrorAbort("ERROR: failed to open output frag file\n");
	}

	outFile << "#version 450\n#extension GL_ARB_separate_shader_objects : enable\n";

	// First, output material constants and raw direct into the shader, filling in the binding ids
	OutputMatConstsAndRaw(outFile, config, declarations.constArr, declarations.rawArr);

	// Then, use transfer declarations/semantics to output correct
	// data transitions between stages/input/output

	std::string fragFunctionBody;

	for (size_t i = 0; i < declarations.funcArr.size(); i++)
	{
		if (declarations.funcArr[i].functionName.compare(stech.fragFunc) == 0)
		{
			fragFunctionBody = declarations.funcArr[i].functionBody;
		}
	}

	for (size_t i = 0; i < declarations.transArr.size(); i++)
	{
		if (declarations.transArr[i].inName.compare(stech.fragFunc) == 0)
		{
			if (declarations.transArr[i].outName.compare("out") == 0)
			{
				// When the frag function is the input in a transfer declaration to out
				// it defines the output layout of the frag shader
				std::vector<SemanticVar*> varArr = *declarations.transArr[i].varArray;

				for (size_t i = 0; i < varArr.size(); i++)
				{
					// If there is not a semantic
					if (varArr[i]->varSemantic.compare("") == 0)
					{
						outFile << "layout(location = " << i << ") out " << varArr[i]->varType << " " << varArr[i]->varName << ";\n";
					}
					else
					{
						assert(config.semanticBindings.find(varArr[i]->varSemantic) != config.semanticBindings.end());
						assert(config.semanticBindings[varArr[i]->varSemantic].bindingIdx != -1);

						// output semantics here determine the location index of the output var
						outFile << "layout(location = " << config.semanticBindings[varArr[i]->varSemantic].bindingIdx << ") out " << varArr[i]->varType << " " << varArr[i]->varName << ";\n";
					}
				}
			}
			else
			{
				// Outputting from the frag shader into another stage is not supported
				ErrorAbort("ERROR: Outputting from the frag shader into another stage is not supported\n");
			}
		}
		else if (declarations.transArr[i].outName.compare(stech.fragFunc) == 0)
		{
			if (declarations.transArr[i].inName.compare(stech.vertFunc) == 0)
			{
				// When the frag function is the output in a transfer declaration
				// it defines the input layout of the frag shader
				std::vector<SemanticVar*> varArr = *declarations.transArr[i].varArray;

				for (size_t i = 0; i < varArr.size(); i++)
				{
					// If there is not a semantic
					if (varArr[i]->varSemantic.compare("") == 0)
					{
						outFile << "layout(location = " << i << ") in " << varArr[i]->varType << " " << varArr[i]->varName << ";\n";
					}
					else
					{
						// Semantics BETWEEN shader stages are not supported
						ErrorAbort("ERROR: Semantics BETWEEN shader stages are not supported\n");
					}
				}
			}
			else
			{
				// Input into the vert shader from other shader stages is not supported
				ErrorAbort("ERROR: Input into the frag shader from other shader stages is not supported\n");
			}
		}
	}

	// Then, output raw function
	outFile << "\nvoid main()\n{\n" << fragFunctionBody << "\n}";

	outFile.flush();
	outFile.close();
}

enum SemanticReadState
{
	SEMANTICREAD_UNDEFINED,
	SEMANTICREAD_VARNAME,
	SEMANTICREAD_INDEX
};

stechc::ConfigData stechc::ParseConfigFile(std::string filepath)
{
	std::ifstream inFile;
	inFile.open(filepath);

	ConfigData data;

	int bufSize = 1024;
	char *buffer = new char[bufSize];

	ConfigState readState = CONFIGSTATE_UNDEFINED;

	while (!inFile.eof())
	{
		inFile.getline(buffer, bufSize);
		// check first char for comment
		if (buffer[0] == '#')
		{
			// ignore
			continue;
		}

		ConfigSemanticBinding binding;
		binding.bindingIdx = -1;
		binding.semanticName = "";
		binding.varName = "";
		binding.state = CONFIGSTATE_UNDEFINED;
		SemanticReadState semReadState = SEMANTICREAD_UNDEFINED;

		int bufIdx = 0;
		// accumulation buffer. We need to clear this on a : or = and push them into the proper
		// fields of the ConfigSemanticBinding structs
		std::string accBuf = "";
		while (buffer[bufIdx] != '\0')
		{
			if (buffer[bufIdx] != ' ' && buffer[bufIdx] != '\t')
			{
				if (buffer[bufIdx] == ':')
				{
					assert(readState != CONFIGSTATE_UNDEFINED);
					semReadState = SEMANTICREAD_INDEX;
					binding.semanticName = accBuf;
					accBuf.clear();
				}
				else if (buffer[bufIdx] == '=')
				{
					assert(readState != CONFIGSTATE_UNDEFINED);
					semReadState = SEMANTICREAD_VARNAME;
					binding.semanticName = accBuf;
					accBuf.clear();
				}
				else
				{
					accBuf.push_back(buffer[bufIdx]);
				}
			}
			bufIdx++;
		}

		// We're at END OF LINE here

		// If we still have data inside our accumulationBuffer we need to check for in/out, or throw an error
		if (accBuf.length() != 0)
		{
			if (accBuf.compare("in") == 0)
			{
				readState = CONFIGSTATE_IN;
			}
			else if (accBuf.compare("out") == 0)
			{
				readState = CONFIGSTATE_OUT;
			}
			else if (semReadState != SEMANTICREAD_UNDEFINED)
			{
				assert(readState != CONFIGSTATE_UNDEFINED);

				// if we found our semantic name in the above loop, then the accBuf should contain the var name or index
				switch (semReadState)
				{
				case SEMANTICREAD_INDEX:
					binding.bindingIdx = stoi(accBuf);
					break;
				case SEMANTICREAD_VARNAME:
					binding.varName = accBuf;
					break;
				}

				binding.state = readState;
				data.semanticBindings[binding.semanticName] = binding;
			}
			else
			{
				// then this is not a keyword. Syntax error
				std::cout << "Config Error: Unrecognized keyword '" << accBuf << "'" << std::endl;
			}
		}
	}

	delete buffer;

	inFile.close();

	return data;
}

std::vector<std::string> stechc::OutputFinalShaders(std::string outputExtraDir, ShaderTechFileDeclarations& declarations)
{
	// our first step is to determine how many shaders we need to create
	printf("Beginning Deferred Shader Tech Compile...\n");

	std::vector<std::string> outputFiles;

	ConfigData config = ParseConfigFile("stechc.config");

	for (size_t i = 0; i < declarations.stechArr.size(); i++)
	{
		if (declarations.stechArr[i].vertFunc.compare("") != 0)
		{
			std::cout << "Generating Vertex Shader for " + declarations.stechArr[i].techName + "..." << std::endl;
			std::string outputFilename;
			if(outputExtraDir.compare("") != 0)
				outputFilename = outputExtraDir + "\\" + declarations.stechArr[i].techName + ".vert";
			else
				outputFilename = declarations.stechArr[i].techName + ".vert";
		
			OutputVertexShader(outputFilename, declarations.stechArr[i], config, declarations);
			outputFiles.push_back(outputFilename);
		}
		if (declarations.stechArr[i].fragFunc.compare("") != 0)
		{
			std::cout << "Generating Fragment Shader for " + declarations.stechArr[i].techName + "..." << std::endl;
			std::string outputFilename;
			if (outputExtraDir.compare("") != 0)
				outputFilename = outputExtraDir + "\\" + declarations.stechArr[i].techName + ".frag";
			else
				outputFilename = declarations.stechArr[i].techName + ".frag";

			OutputFragmentShader(outputFilename, declarations.stechArr[i], config, declarations);
			outputFiles.push_back(outputFilename);
		}
	}

	printf("Completed Deferred Shader Tech Compile...\n");

	return outputFiles;
}


stechc::CArgs stechc::ReadArgs(int argc, char** argv)
{
	CArgs parsedArgs = {};

	// Ignore 0th arg (command), 1st arg (input file)
	for (int i = 2; i < argc; i++)
	{
		std::string strArg = std::string(argv[i]);
		if (strArg.compare("-ODIR") == 0 || strArg.compare("-odir") == 0)
		{
			// Error case. They didnt provide an output folder at all!
			if (i + 1 >= argc)
			{
				ErrorAbort("-odir wasn't provided a output folder string!");
			}
			else
			{
				parsedArgs.outputDir = argv[i + 1];
				i++;
				continue;
			}
		}
	}

	return parsedArgs;
}

stechc::ShaderTechFileDeclarations gDecl;

std::vector<std::string> stechc::CompileShaderTech(std::string inFilepath, CArgs args)
{
	FILE *inFile;
	fopen_s(&inFile, inFilepath.c_str(), "r");

	if (!inFile)
	{
		std::cout << "I cant open " << inFilepath << std::endl;
		abort();
	}

	yyin = inFile;

	yyparse();

	// gDecl is populated by yyparse execution
	auto output = OutputFinalShaders(args.outputDir, gDecl);

	fclose(inFile);

	return output;
}