#include <fstream>
#include <cerrno>
#include <queue>
#include <thread>
#include <time.h>  //clock

//3rd party headers
#include "easylogging++.h"//https://github.com/easylogging/easyloggingpp#logging
#include "dirent.h"

#include "utils.h"
#include "parser.h"
#include "basictypes.h"
#include "jofilelib.hpp"
#include "reservedsymbols.h"

_INITIALIZE_EASYLOGGINGPP

using namespace std;
using namespace game;

namespace par{

Parser::Parser(const std::string& _configFile)
	:
	m_gameData(lang::basicTypes),
	m_compiler(m_gameData),
	m_currentNamespace(nullptr),
	m_isCodeParsing(false)
{
	LOG(INFO) << "Parser created.";

	//read config
	LOG(INFO) << "Loading config " << _configFile << ".";
	Jo::Files::MetaFileWrapper config;
	try {
		Jo::Files::HDDFile hddFile(_configFile);
		config.Read(hddFile, Jo::Files::Format::JSON);
	}
	catch (std::string _message) {
		LOG(ERROR) << "Failed to load config with message: " << _message << ".";
		return;
	}
	m_config.m_sourceDir = config[string("sourceDir")];
	m_config.m_sourceDir += '\\';

	m_config.m_caseSensitive = config[string("caseSensitive")];
	m_config.m_alwaysSemikolon = config[string("alwaysSemicolon")];
	m_config.m_saveInOrder = config[string("saveInOrder")];
	m_config.m_showCodeSegmentOnError = config[string("showCodeSegmentOnError")];
}

// ***************************************************** //

void Parser::parse(const std::string& _fileName)
{
	//reserve some memory to reduce reallocations
	m_gameData.m_symbols.reserve(4096);
	std::vector< string > names;

	parseSource(_fileName, names);

	LOG(INFO) << "Starting tokenizing.";
	clock_t begin = clock();

	vector < unique_ptr < Lexer > > lexers;
	vector < vector< CodeToParse > > definitionsToParse; //one set of definitions to parse per lexer
	
	//now parse all retrieved names
	for (int i = 0; i < names.size(); ++i)
	{
		lexers.emplace_back(new Lexer(names[i]));//store file name to be able to display it in error messages
		tokenizeFile(m_config.m_sourceDir + names[i], lexers.back().get());
	}
	//make shure that all lexers have finished
	//for (auto& lexerThread : lexerThreads) lexerThread.join();
	clock_t end = clock();
	LOG(INFO) << "Finished tokenizing in " << double(end - begin) / CLOCKS_PER_SEC << "sec";

	LOG(INFO) << "Parsing code files.";
	definitionsToParse.resize(lexers.size());
	//the actual declaration parsing process begins
	for (int i = 0; i < (int)lexers.size(); ++i)
	{
		m_codeQue = &definitionsToParse[i];
		parseFile(*lexers[i]);
	}

	//finally parse function definitions
	LOG(INFO) << "Parsing function definitions.";

	for (int i = 0; i < (int)definitionsToParse.size(); ++i)
	{
		m_lexer = lexers[i].get();
		for (auto& codeToParse : definitionsToParse[i])
			parseCodeBlock(codeToParse);
	}

	LOG(INFO) << "Finished parsing.";
}

// ***************************************************** //

void Parser::parseSource(const std::string& _fileName, std::vector< string >& _names)
{
	std::ifstream in(m_config.m_sourceDir + _fileName.c_str(), std::ios::in | std::ios::binary);

	//check if file is valid
	if (!in)
	{
		LOG(ERROR) << "Source-File " << _fileName << " not found.";
		return;
	}

	LOG(INFO) << "Parsing Source-File " << _fileName;

	//extract the path
	size_t pathEnd = _fileName.find_last_of('\\');
	//_fileName.substr(0, _fileName.find_last_of('\\'));
	//string path = pathEnd == string::npos ? "" : string(_fileName.begin(), _fileName.begin() + pathEnd);

	vector< string > names;

	//parse the file line by line
	string line;
	while (getline(in, line))
	{
		//comments specefied throught "//" are ignored
		if (line[0] == '/' && line[1] == '/') continue;

		//precompiler directive
		if (line[0] == '#')
		{
			preDirective(line);
			continue;
		}

		//remove the '\r' from the end
		//'\n' is not copied by getline
		if (line.back() == '\r')
			line.resize(line.size() - 1);

		//build the complete path using the source dir
		string pathLocal;
		//extract the local path
		size_t localPathEnd = line.find_last_of('\\');

		if (localPathEnd != string::npos)
		{
			// +1 to include '\\'
			pathLocal += string(line.begin(), line.begin() + localPathEnd + 1);
			//remove the path from the filename
			line.erase(line.begin(), line.begin() + localPathEnd + 1);
		}

		//search for the '.' that is part of the file ending
		size_t endPos = line.find_last_of('.');

		//line is not relevant
		if (endPos == string::npos) continue;

		//namematching with wildcards '*' and '?'

		size_t wildcardM = line.find('*'); //multiple chars
		size_t wildcardS = line.find('?'); //single char

		//the name has a wildcard in it
		if (wildcardM != string::npos || wildcardS != string::npos)
		{
			//scan for every file in the target directionary
			DIR *dir;
			dirent *ent;

			auto begin = names.size();
			//scan the dir for containers
			if ((dir = opendir((m_config.m_sourceDir + pathLocal).c_str())) != NULL)
			{
				while ((ent = readdir(dir)) != NULL)
				{
					names.push_back(string(ent->d_name));
					utils::toLower(names.back());
				}
			}
			if (begin == names.size())
			{
				parserLog(par::LogLvl::Warning, "Entry has no matches.");
				continue;
			}
			//filesystem is not case sensitve while wildcardMatch() is
			utils::toLower(line);
			utils::wildcardMatch(line, names, begin);
			//the complete path has to be added again
			for (size_t i = begin; i < names.size(); ++i)
				if (names[i] != "") names[i] = pathLocal + names[i];
		}
		//the filename is correct
		else
		{
			names.push_back(pathLocal + line);
			utils::toLower(names.back());
		}

	}//end while
	
	//move valid names to _names
	for (auto& name : names)
	{
		if (name == "") continue;

		if (!memcmp((char*)&name[name.size() - 5], ".src", 4))
			parseSource(name, _names);
		else
			_names.push_back(std::move(name));
	}

}

// ***************************************************** //

void Parser::tokenizeFile(const std::string& _fileName, Lexer* _lexer)
{
	std::ifstream in(_fileName.c_str(), std::ios::in | std::ios::binary);

	//check if file is valid
	if (!in)
	{
		LOG(ERROR) << "Code-File " << _fileName << " not found.";
		return;
	}

	string fileContent;

	// put filecontent into a string
	in.seekg(0, std::ios::end);
	fileContent.reserve(in.tellg());
	in.seekg(0, std::ios::beg);

	fileContent.assign((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());

	//if (!m_caseSensitive) std::transform(m_currentFile.begin(), m_currentFile.end(), m_currentFile.begin(), ::tolower);


//	LOG(INFO) << "Parsing Code-File " << _fileName;

	// split into tokens
	// and convert to lowercase
	_lexer->analyse(std::move(fileContent)); //fileContent is invalid now

}

int Parser::parseFile(Lexer& _lexer)
{
	//set current lexer
	m_lexer = &_lexer;

	LOG(INFO) << "Parsing Code-File " << m_lexer->getDataName();

	Token* pToken;
	//translated part from zParser
	//ParseFile

	while (pToken = m_lexer->nextToken())
	{
		if (pToken->type != TokenType::Symbol)
		{
			PARSINGERROR("Word expected.", pToken);
		}

		int returnCode = 0;

		int len = 1 + pToken->end - pToken->begin;

		if (m_lexer->compare(*pToken, "const"))
		{
			returnCode = declareVar(1, m_gameData.m_symbols);
		}
		else if (m_lexer->compare(*pToken, "var"))
		{
			returnCode = declareVar(0, m_gameData.m_symbols);
		}
		else if (m_lexer->compare(*pToken, "func"))
		{
			returnCode = declareFunc();
		}
		else if (m_lexer->compare(*pToken, "class"))
		{
			returnCode = declareClass();
		}
		else if (m_lexer->compare(*pToken, "instance"))
		{
			returnCode = declareInstance();
		}
		else if (m_lexer->compare(*pToken, "prototype"))
		{
			returnCode = declarePrototype();
		}
		//not recognized?
		else
		{
			//gothic.exe checks against some kind of zero aswell
			//maybe for empty files
			//LOG(ERROR) << m_currentFile.substr(begin, m_pp - begin) << " is not a recognized symbol.";
			PARSINGERROR("Uknown symbol type.", pToken);
		}
		if (returnCode == -1) return -1;
	}

	//now all symbols are known -> parse function definitions

	return 0;
}



// ***************************************************** //

int Parser::declareVar(bool _const, utils::SymbolTable< game::Symbol >& _table)
{
	//gothic parser types:
	// 1 - float
	// 2 - int
	// 3 - string
	// 5 - func
	//(7 + index of type in the symboltable) - custom types
	//get type
	Token* token;

	TOKENEXT(Symbol, typeToken);

	string word(m_lexer->getWord(*typeToken));

	//validate type
	int index = utils::find(m_gameData.m_types, word);

	Token* nameToken;

	int arraySize = 1;

	if (index == -1) PARSINGERROR("Unknown type: " + word, typeToken);

	do
	{
		//get name
		nameToken = m_lexer->nextToken();
		if (!nameToken || nameToken->type != Symbol)
		{
			PARSINGERROR("Word expected.", nameToken);
		}

		//now filled with the name
		word = m_lexer->getWord(*nameToken);

		arraySize = 1;

		//array declaration
		//or end expression: ';'
		token = m_lexer->nextToken();
		NULLCHECK(token);

		//array declaration
		if (*token == SquareBracketLeft)
		{
			token = m_lexer->nextToken();
			NULLCHECK(token);

			if (*token != TokenType::ConstInt)
			{
				int constIndex = utils::find(m_gameData.m_constSymbols, m_lexer->getWord(*token));

				if (constIndex != -1) arraySize = ((ConstSymbol_Int&)m_gameData.m_constSymbols[constIndex]).value[0];
				else PARSINGERROR("Const int expected.", token);
			}
			else
				arraySize = m_lexer->getInt(*token);

			TOKEN(SquareBracketRight);

			//read next one
			token = m_lexer->nextToken();
			NULLCHECK(token);
		}

		//only non consts are added after knowing name and type
		if (!_const)
		{
			//type is saved as Symbol::type and as parent id
			bool check = _table.emplace(word, index, 0, arraySize, index >= g_atomTypeCount ? m_gameData.m_types[index].id : 0xFFFFFFFF);
			if (!check) parserLog(Warning, "Symbol redefinition.", nameToken);
		}

	} while (*token == Comma);

	//finished
	if (!_const && token->type == End)
	{
		return 0;
	}
	//constants need to be initialized instantly
	//todo make this faster 
	else if (m_lexer->getWord(*token) == "=")
	{
		//const arrays start with '{'
		if (arraySize > 1) TOKEN(CurlyBracketLeft);

		TokenType endChar = arraySize > 1 ? TokenType::CurlyBracketRight : TokenType::End;
		game::Symbol* symbol;
		//type decides how to parse
		if (index == 2)
		{
			ConstSymbol_Int* constSymbol = new game::ConstSymbol_Int(word, arraySize);
			symbol = constSymbol;

			for (int i = 0; i < arraySize - 1; ++i)
			{
				game::DummyInt result(0);
				if (Term(&result, TokenType::Comma)) return -1;
				constSymbol->value[i] = result.value;

				TOKEN(Comma)
			}

			game::DummyInt result(0);
			if (Term(&result, endChar)) return -1;
			constSymbol->value[arraySize - 1] = result.value;

			m_gameData.m_constSymbols.push_back(*symbol);
		}
		else if (index == 1)
		{
			ConstSymbol_Float* constSymbol = new game::ConstSymbol_Float(word, arraySize);
			symbol = constSymbol;

			for (int i = 0; i < arraySize - 1; ++i)
			{
				game::DummyFloat result(0);
				if (Term(&result, TokenType::Comma)) return -1;
				constSymbol->value[i] = result.value;

				TOKEN(Comma)
			}

			game::DummyFloat result(0);
			if (Term(&result, endChar)) return -1;
			constSymbol->value[arraySize-1] = result.value;

			m_gameData.m_constSymbols.push_back(*symbol);
		}
		else if (index == 3)
		{
			ConstSymbol_String* constSymbol = new game::ConstSymbol_String(word, arraySize);
			symbol = constSymbol;

			for (int i = 0; i < arraySize; ++i)
			{
				TOKENEXT(TokenType::ConstStr, strToken);
				constSymbol->value[i] = std::move(m_lexer->getWord(*strToken));
				if (i < arraySize - 1) TOKEN(Comma);
			}
		}
		else if (index == 5)
		{
			ConstSymbol_Func* constSymbol = new game::ConstSymbol_Func(word);
			symbol = constSymbol;

			TOKENEXT(TokenType::Symbol, functionSymbol);
			int symInd = m_gameData.m_symbols.find(m_lexer->getWord(*functionSymbol));


			constSymbol->value[0].ptr = &m_gameData.m_symbols[symInd];

			m_gameData.m_constVarFuncs.push_back(*constSymbol);
		}
		else PARSINGERROR("Only int, float, string and func can be const.", typeToken);

		if (arraySize > 1) TOKEN(CurlyBracketRight);

		TOKEN(End);

		m_gameData.m_symbols.add(symbol);

		return 0;
	}

	PARSINGERROR("Unexpected token.", token);
}

// ***************************************************** //

int Parser::declareFunc()
{
	TOKENEXT(Symbol, typeToken);
	TOKENEXT(Symbol, nameToken);

	string name = m_lexer->getWord(*nameToken);
	
	m_gameData.m_symbols.add(new Symbol_Function(name, getType(*typeToken)));
	game::Symbol_Function& functionSymbol = (Symbol_Function&)m_gameData.m_symbols.back();
	m_gameData.m_functions.push_back(functionSymbol);

	TOKEN(ParenthesisLeft);

	//required to make TOKENOPT work :(
	Token* tokenOpt;

	//optional param list
	if (!TOKENOPT(ParenthesisRight))
	{
		m_lexer->prev();
		do
		{
			//actually only 'var' is valid
			TOKEN(Symbol);
			//relevant information
			//parameter type
			TOKENEXT(Symbol, paramTypeToken);
			//parameter name
			TOKENEXT(Symbol, paramNameToken);

			//add and fix parent if necessary
			int type = getType(*paramTypeToken);
			functionSymbol.params.emplace(m_lexer->getWord(*paramNameToken), type);
			if (type >= g_atomTypeCount) functionSymbol.params.back().parent.ptr = &m_gameData.m_types[type];
			//todo: build uniform way to fix parent

		} while (TOKENOPT(Comma));

		//failed optional token
		m_lexer->prev();

		TOKEN(ParenthesisRight);
	}

	//externals have no body
	//instead there adress is in this format:
	// func void foo() 0xAB7541F0;
	if (TOKENOPT(ConstInt))
	{
		TOKEN(End);
		functionSymbol.addFlag(game::Flag::External);
		return 0;
	}

	m_lexer->prev();

	assignFunctionParam(functionSymbol);

	//code block
	codeBlock(functionSymbol);
	
/*	if (!TOKENOPT(End))
	{
		if (m_alwaysSemikolon)
		{
			PARSINGERROR("End';' expected.", tokenOpt);
		}
		else //read token is part of the next declaration
			m_lexer->prev();
	}*/

	//always end with a return
	//for some reason gothic even ends with two rets 
	//same happens here when a "return" is found
	//update: the compiler adds one return to every function including those without a body
	//functionSymbol.byteCode.emplace_back(game::Instruction::Ret);

	return 0;
}

// ***************************************************** //

int Parser::declareInstance()
{
	Token* tokenOpt;

	std::vector < string > names;

	do
	{
		TOKENEXT(Symbol, nameToken);

		names.push_back(m_lexer->getWord(*nameToken));

	} while (TOKENOPT(Comma));

	m_lexer->prev();

	TOKEN(ParenthesisLeft);

	//type
	TOKENEXT(Symbol, typeToken);

	int i = getType(*typeToken);

	//type does not matter when it is derivated from a prototype or not existent
	//thus the check may happen afterwards
	m_gameData.m_symbols.add(new Symbol_Instance(names[0], i));
	game::Symbol_Instance& instance = (Symbol_Instance&)m_gameData.m_symbols.back();

	if (i == -1)
	{
		i = utils::find(m_gameData.m_prototypes, m_lexer->getWord(*typeToken));

		if(i == -1) PARSINGERROR("Unknown type.", typeToken);

		//init function starts with a call to its prototype
		instance.byteCode.emplace_back(game::Instruction::call, m_gameData.m_prototypes[i].id);

		for (size_t j = 0; j < m_gameData.m_types.size(); ++j)
		{
			if (&m_gameData.m_types[j] == m_gameData.m_prototypes[i].parent.ptr)
			{
				instance.type = (unsigned int)j;
			}
		}
		instance.parent.ptr = &m_gameData.m_prototypes[i];
	}
	else
	{
		instance.parent.ptr = &m_gameData.m_types[i];
	}

	TOKEN(ParenthesisRight);

	if(!TOKENOPT(End))
	{
		m_lexer->prev();

		m_currentNamespace = &m_gameData.m_types[instance.type];
		m_thisInst = instance.parent.ptr;

		codeBlock(instance);

		m_currentNamespace = nullptr;

		//instances with a body have the const flag
		instance.addFlag(game::Flag::Const);

		//every use of self is subsituted with a this-reference
		for (auto& instr : instance.byteCode)
		{
			if (instr.instruction == game::Instruction::pushInst)
				instr.param = instance.id;
		}
	}

	if (names.size() > 1)
		for (int i = 1; i < names.size(); ++i)
		{
			m_gameData.m_symbols.add(new Symbol_Instance(names[i], instance));
		}

	return 0;
}

// ***************************************************** //

int Parser::declarePrototype()
{
	string name;

	TOKENEXT(Symbol, nameToken);

	name = m_lexer->getWord(*nameToken);

	TOKEN(ParenthesisLeft);

	//type
	TOKENEXT(Symbol, typeToken);

	int i = getType(*typeToken);

	if (i == -1) PARSINGERROR("Unknown type.", typeToken);

	TOKEN(ParenthesisRight);

	m_gameData.m_symbols.add(new Symbol_Instance(name, i));
	Symbol_Instance& prototype = (Symbol_Instance&)m_gameData.m_symbols.back();
	m_gameData.m_prototypes.push_back(prototype);

	prototype.type = 6; //type is prototype...
	prototype.parent.ptr = &m_gameData.m_types[i];

	// init the stack state
	m_currentNamespace = &m_gameData.m_types[i];
	m_thisInst = prototype.parent.ptr;

	codeBlock(prototype);

	m_currentNamespace = nullptr;

	return 0;
}

// ***************************************************** //

int Parser::declareClass()
{
	//get name
	Token* token = m_lexer->nextToken();
	if (!token || token->type != Symbol)
	{
		PARSINGERROR("Word expected.", token);
	}

	string word = m_lexer->getWord(*token);

	token = m_lexer->nextToken();

	if (!token || token->type != CurlyBracketLeft)
	{
		PARSINGERROR("Curly Bracket '{' expected.", token);
	}

	m_gameData.m_symbols.add(new Symbol_Type(word));
	game::Symbol_Type& type = (Symbol_Type&)m_gameData.m_symbols.back();
	m_gameData.m_types.push_back(type);

	int ret = 0;
	//parse all members
	while ((token = m_lexer->nextToken()) && ret != -1 && m_lexer->compare(*token, "var"))
		ret = declareVar(0, type.elem);

	for (size_t i = 0; i < type.elem.size(); ++i)
	{
		type.elem[i].setFlag(game::Flag::Classvar);
		type.elem[i].parent.ptr = &type;
	}

	if (ret == -1) return -1;

	//the next token is already retrieved by the loop

	NULLCHECK(token);

	if (token->type != CurlyBracketRight)
	{
		PARSINGERROR("Curly Bracket '}' expected.", token);
	}

	//gothic declarations always close with ';'
	
	token = m_lexer->nextToken();

	NULLCHECK(token);

	if (token->type != End)
	{
		if (m_config.m_alwaysSemikolon)
		{
			PARSINGERROR("End';' expected.", token);
		}
		else //read token is part of the next declaration
			m_lexer->prev();
	}

	return 0;

}

// ***************************************************** //

void Parser::preDirective(const std::string& _directive)
{
	if (_directive == "#INORDER")
		m_parseInOrder = true;
	else if (_directive == "#NOTINORDER")
		m_parseInOrder = false;
}

// ***************************************************** //

void Parser::parserLog(LogLvl _lvl, const std::string& _msg, Token* _token)
{
	string msg;
	//every file has atleast one line
	int lineCount = 1;
	if (_token)
	{
		for (unsigned int i = 0; i < _token->begin; ++i)
			if (m_lexer->getRawText()[i] == '\n') ++lineCount;

		msg = "Found: \"" + m_lexer->getWord(*_token) + "\" - ";
	}

	msg += _msg;

	//since ERROR and WARNING are forwarded by a macro runtime evaluation needs a switch
	if (_lvl == Error)
		LOG(ERROR) << msg << " [l." << lineCount << "] " << "[" << m_lexer->getDataName() << "]";
	else if (_lvl == Warning)
		LOG(WARNING) << msg << " [l." << lineCount << "] " << "[" << m_lexer->getDataName() << "]";

	//show +- 3 lines of the code where the error was found
	if (m_config.m_showCodeSegmentOnError && _token)
	{	
		std::cout << utils::strInterval(m_lexer->getRawText(), _token->begin, _token->end, 3) << endl << endl;
	}
}

int Parser::getType(Token& _token)
{
	return utils::find(m_gameData.m_types,m_lexer->getWord(_token));
}

}