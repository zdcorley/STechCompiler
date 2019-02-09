%{
	#include "stechc.h"

	extern stechc::ShaderTechFileDeclarations gDecl;

	int gOrder = 0;

	using namespace stechc;
	using namespace std;
%}

%union {
	char *sval;
	void *pval;
	int ival;
}

%token IN
%token OUT
%token ARROW
%token IS
%token ENDCMD
%token SHADERTECH
%token VERT
%token FRAG
%token UNIFORM
%token STARTBRACKET
%token ENDBRACKET
%token COMMENT
%token RAWSTART
%token RAWEND
%token BINDING
%token EQUALS
%token OPENPAREN
%token CLOSEPAREN
%token LAYOUTPROP

%token <sval> STRINGLIT
%token <sval> IDENTIFIER
%token <sval> CODELINE
%token <sval> CODE
%token <sval> STAGEFUNCTIONSTART
%token <sval> CONST_ARRAY
%token <ival> INTEGER

// vector<SemanticVar*>*
%type <pval> semanticVarList
// SemanticVar*
%type <pval> semanticVar

// ShaderTechDeclaration*
%type <pval> shaderTechBindingList;

%type <sval> shaderTechBinding;

%type <sval> varType
%type <sval> varName
%type <sval> rawCodeBlock
%type <pval> layoutProp

%%

// the first rule defined is the highest-level rule, which 
// is just the concept of a whole "shadertech file":
shaderTechFile:
  declarationList 
  {
		cout << "done with a shadertech file!" << endl;
  }
  ;
declarationList:
  declarationList declaration
  | declaration
  {

  }
  ;
declaration:
  matConstDeclaration
  | stageTransferDeclaration
  | stageFunctionDeclaration
  | shaderTechDeclaration
  | rawDeclaration
  | COMMENT
  ;
matConstDeclaration:
  uniformDeclaration
  ;
uniformDeclaration:
  layoutProp UNIFORM varType varName ENDCMD
  {
		cout << "FOUND UNIFORM!" << endl;

		MatConstant matcon;
		matcon.order = gOrder++;
		matcon.layoutTerm = *((LayoutTerm*)$1);
		matcon.resourceType = "uniform";
		matcon.varType = string($3);
		matcon.varName = string($4);

		gDecl.constArr.push_back(matcon);

		delete $1;
  }
  | UNIFORM varType varName ENDCMD
  {
		cout << "FOUND UNIFORM!" << endl;

		MatConstant matcon;
		LayoutTerm term;
		term.bindingIndex = -1;
		matcon.order = gOrder++;
		matcon.layoutTerm = term;
		matcon.resourceType = "uniform";
		matcon.varType = string($2);
		matcon.varName = string($3);

		gDecl.constArr.push_back(matcon);
  }
  ;
varName:
  IDENTIFIER CONST_ARRAY
  {
		$$ = StringConcat($1, $2);
		delete $1;
		delete $2;
  }
  | IDENTIFIER
  {
		$$ = $1;
  }
  ;
layoutProp:
  LAYOUTPROP OPENPAREN BINDING EQUALS INTEGER CLOSEPAREN
  {
		LayoutTerm *term = new LayoutTerm();
		term->bindingIndex = $5;
		$$ = term;
  }
  ;
stageTransferDeclaration:
  IN ARROW IDENTIFIER STARTBRACKET semanticVarList ENDBRACKET
  {
		cout << "IN ARROW IDENTIFIER!" << endl;

		TransferDeclaration tDecl;
		tDecl.inName = "in";
		tDecl.outName = string($3);
		tDecl.varArray = (vector<SemanticVar*>*)$5;

		gDecl.transArr.push_back(tDecl);
  }
  | IDENTIFIER ARROW IDENTIFIER STARTBRACKET semanticVarList ENDBRACKET
  {
		cout << "IDENTIFIER ARROW IDENTIFIER!" << endl;

		TransferDeclaration tDecl;
		tDecl.inName = string($1);
		tDecl.outName = string($3);
		tDecl.varArray = (vector<SemanticVar*>*)$5;

		gDecl.transArr.push_back(tDecl);
  }
  | IDENTIFIER ARROW OUT STARTBRACKET semanticVarList ENDBRACKET
  {
		cout << "IDENTIFIER ARROW OUT!" << endl;

		TransferDeclaration tDecl;
		tDecl.inName = string($1);
		tDecl.outName = "out";
		tDecl.varArray = (vector<SemanticVar*>*)$5;

		gDecl.transArr.push_back(tDecl);
  }
  ;
semanticVarList:
  semanticVarList semanticVar
  {
		((vector<SemanticVar*>*)$1)->push_back((SemanticVar*)$2);
		$$ = ((vector<SemanticVar*>*)$1); 
  }
  | semanticVar
  {
		vector<SemanticVar*> *sVarArr = new vector<SemanticVar*>();
		sVarArr->push_back((SemanticVar*)$1);
		$$ = sVarArr;
  }
  ;
semanticVar:
  varType varName IS IDENTIFIER ENDCMD
  {
		SemanticVar *sVar = new SemanticVar();
		sVar->varType = $1;
		sVar->varName = $2;
		sVar->varSemantic = $4;

		$$ = sVar;
  }
  | varType varName ENDCMD
  {
		SemanticVar *sVar = new SemanticVar();
		sVar->varType = $1;
		sVar->varName = $2;
		sVar->varSemantic = "";

		$$ = sVar;
  }
  ;
varType:
  IDENTIFIER STARTBRACKET semanticVarList ENDBRACKET 
  {
		char *first = StringConcat($1, "\n{");
		vector<SemanticVar*> varListArr = *((vector<SemanticVar*>*)$3);

		string varListConcat = "";
		for(size_t i = 0; i < varListArr.size(); i++)
		{
			varListConcat += "\n\t" + varListArr[i]->varType + " ";
			varListConcat += varListArr[i]->varName + ";";
			// ignore semantics here. Semantics must be defined at the
			// outer most type level.
		}
		varListConcat += "\n";

		char *second = StringConcat(first, varListConcat.c_str());
		$$ = StringConcat(second, "}");

		delete first;
		delete second;
  }
  | IDENTIFIER
  {
		$$ = $1;
  }
  ;
stageFunctionDeclaration:
  STAGEFUNCTIONSTART STARTBRACKET CODE ENDBRACKET
  {
		cout << "FUNCTION DECLARATION!" << endl;
		FunctionDeclaration funcDec;
		funcDec.functionName = string($1);
		funcDec.functionBody = string($3);

		gDecl.funcArr.push_back(funcDec);
  }
  ;
shaderTechDeclaration:
  SHADERTECH STRINGLIT STARTBRACKET shaderTechBindingList ENDBRACKET
  {
		cout << "SHADER TECH DECLARATION!" << endl;

		ShaderTechDeclaration *stechDecl = (ShaderTechDeclaration*)$4;
		string tName = string($2);
		stechDecl->techName = tName.substr(1, tName.size()-2);

		gDecl.stechArr.push_back(*stechDecl);
  }
  ;
shaderTechBindingList:
  shaderTechBindingList shaderTechBinding
  {
		cout << "SHADER TECH BINDING LIST!" << endl;
		
		ShaderTechDeclaration *decl = (ShaderTechDeclaration*)$1;

		string bstr = string($2);

		if(bstr[0] == 'v')
		{
			decl->vertFunc = bstr.substr(1, bstr.size()-1);
		}
		else if(bstr[0] == 'f')
		{
			decl->fragFunc = bstr.substr(1, bstr.size()-1);
		}

		delete $2;

		$$ = decl;
  }
  | shaderTechBinding
  {
		cout << "SHADER TECH BINDING LIST START!" << endl;
		
		ShaderTechDeclaration *decl = new ShaderTechDeclaration();
		decl->techName = "";
		decl->vertFunc = "";
		decl->fragFunc = "";

		string bstr = string($1);

		if(bstr[0] == 'v')
		{
			decl->vertFunc = bstr.substr(1, bstr.size()-1);
		}
		else if(bstr[0] == 'f')
		{
			decl->fragFunc = bstr.substr(1, bstr.size()-1);
		}

		delete $1;

		$$ = decl;
  }
  ;
shaderTechBinding:
  VERT IS IDENTIFIER ENDCMD
  {
		cout << "SHADER TECH BINDING!" << $3 << endl;

		$$ = StringConcat("v", $3);
  }
  | FRAG IS IDENTIFIER ENDCMD
  {
		cout << "SHADER TECH BINDING!" << $3 << endl;

		$$ = StringConcat("f", $3);
  }
  ;
rawDeclaration:
  RAWSTART rawCodeBlock RAWEND
  {
		cout << "RAW DECLARATION" << endl;
		RawDeclaration rawDec;
		rawDec.order = gOrder++;
		rawDec.code = string($2);

		gDecl.rawArr.push_back(rawDec);
  }
  ;
rawCodeBlock:
  rawCodeBlock CODELINE
  {
		$$ = StringConcatWithNewLine($1, $2);
		delete $1;
		delete $2;
  }
  | CODELINE
  {
		$$ = $1;
  }
  ;
%%

/*
int main(int argc, char** argv)
{
	if (argc < 2)
	{
		cout << "No input file provided!" << endl;
		system("pause");
		return -1;
	}

	stechc::CParsedArgs parsedArgs = stechc::ReadArgs(argc, argv);

	stechc::CompileShaderTech(std::string(argv[1]), parsedArgs);
}
*/


void yyerror(const char *s)
{
	cout << "PARSE ERROR! MESSAGE: " << s << endl;
	
	exit(-1);
}