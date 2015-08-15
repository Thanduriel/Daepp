#include <fstream>
#include <cerrno>
#include <queue>

//3rd party headers
#include "easylogging++.h"//https://github.com/easylogging/easyloggingpp#logging
#include "dirent.h"

#include "parser.h"
#include "parserintern.h"
#include "basictypes.h"
#include "jofilelib.hpp"
#include "reservedsymbols.h"

//stupid intelli, there is nothing wrong here
_INITIALIZE_EASYLOGGINGPP

using namespace std;

namespace par{

Parser::Parser(const std::string& _configFile)
	:m_lexer(m_currentFile),
	m_gameData(lang::basicTypes),
	m_compiler(m_gameData)
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
	m_sourceDir = config[string("sourceDir")];
	m_sourceDir += '\\';

	m_caseSensitive = config[string("caseSensitive")];
	m_alwaysSemikolon = config[string("alwaysSemikolon")];
}


Parser::~Parser()
{
}

void Parser::parseSource(const std::string& _fileName)
{

	std::ifstream in(m_sourceDir + _fileName.c_str(), std::ios::in | std::ios::binary);

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

	//parse the file line by line
	string line;
	while (getline(in, line))
	{
		//comments specefied throught "//" are ignored
		if (line[0] == '/' && line[1] == '/') continue;

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


		std::vector<string> names;

		//namematching with wildcards '*' and '?'

		size_t wildcardM = line.find('*'); //multiple chars
		size_t wildcardS = line.find('?'); //single char

		//the name has a wildcard in it
		if (wildcardM != string::npos || wildcardS != string::npos)
		{
			//scan for every file in the target directionary
			DIR *dir;
			dirent *ent;

			//scan the dir for containers
			if ((dir = opendir((m_sourceDir + pathLocal).c_str())) != NULL)
			{
				while ((ent = readdir(dir)) != NULL)
				{
					names.push_back(string(ent->d_name));
				}
			}
			//match the filename with every file in the directionary
			for (int i = 0; i < names.size(); ++i)
			{
				bool fits = true;
				int offset = 0;
				unsigned int j;
				for (j = 0; j < line.size(); ++j)
				{
					// '?' => ignore the next char
					if (line[j] == '?') offset++;
					// '*'
					else if (line[j] == '*')
					{
						//the char afterwards is where the '*' part ends
						//'*' is never at the end since there are only two types of significant endings that should never overlap
						j++;
						//todo find all possible points to continiue so that this works: a*b -> abab
						bool match = false;
						for (offset; j + offset < names[i].size(); ++offset)
							//found where the '*' ends
							if (::tolower(names[i][j + offset]) == ::tolower(line[j]))
							{
								match = true;
								break;
							}
						//file does not match
						if (!match)
						{
							fits = false;
							break;
						}
					}
					//check char by char
					//file does not match
					if (::tolower(line[j]) != ::tolower(names[i][j + offset]))
					{
						fits = false;
						break;
					}
				}
				//char by char comparisation iterates only throught line; names could be even longer
				if (j + offset < names[i].size()) fits = false;
				//mark string instead of removing him to prevent movement in the container
				if (!fits) names[i] = "";
			}
		}
		//the filename is correct
		else names.push_back(line);

		//convert the ending to lowercase for easier comparison
		std::transform(line.begin() + endPos, line.end(), line.begin() + endPos, ::tolower);

		//check the file ending; since only two chars are checked other endings like ".dpp" are valid aswell
		if (!memcmp((char*)&line[endPos], ".d", 2))
		{
			for (auto& fileName : names)
				if (fileName.size()) parseFile(pathLocal + fileName); //parse a code file
		}
		else if (!memcmp((char*)&line[endPos], ".src", 4))
		{
			for (auto& fileName : names)
				if (fileName.size()) parseSource(pathLocal + fileName); //parse a source-file
		}

//		cout << line << endl;
	}
}

// ***************************************************** //

int Parser::parseFile(const std::string& _fileName)
{
	std::ifstream in(m_sourceDir + _fileName.c_str(), std::ios::in | std::ios::binary);

	//check if file is valid
	if (!in)
	{
		LOG(ERROR) << "Code-File " << _fileName << " not found.";
		return -1;
	}

	m_currentFileName = _fileName;

	// put filecontent into a string
	in.seekg(0, std::ios::end);
	m_currentFile.reserve(in.tellg());
	in.seekg(0, std::ios::beg);

	m_currentFile.assign((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());

	if (!m_caseSensitive) std::transform(m_currentFile.begin(), m_currentFile.end(), m_currentFile.begin(), ::tolower);


	LOG(INFO) << "Parsing Code-File " << _fileName;

	// split into tokens
	m_lexer.analyse();
	
	//reset file specific vars
	//useless now?
	m_pp = 0;
	m_lineCount = 0;

	Token* pToken;

	//translated part from zParser
	//ParseFile

	while (pToken = m_lexer.nextToken())
	{
		if (pToken->type != TokenType::Symbol)
		{
			PARSINGERROR("Word expected.", pToken);
		}

		int returnCode = 0;

		int len = 1 + pToken->end - pToken->begin;

		if (!m_currentFile.compare(pToken->begin, len, "const"))
		{
			returnCode = declareVar(1, m_gameData.m_symbols);
		}
		else if (!m_currentFile.compare(pToken->begin, len, "var"))
		{
			returnCode = declareVar(0, m_gameData.m_symbols);
		}
		else if (!m_currentFile.compare(pToken->begin, len, "func"))
		{
			returnCode = declareFunc();
		}
		else if (!m_currentFile.compare(pToken->begin, len, "class"))
		{
			returnCode = declareClass();
		}
		else if (!m_currentFile.compare(pToken->begin, len, "instance"))
		{
			returnCode = declareInstance();
		}
		else if (!m_currentFile.compare(pToken->begin, len, "prototype"))
		{
			//		DeclarePrototype();
		}
		//not recognized?
		else
		{
			//gothic.exe checks against some kind of zero aswell
			//maybe for empty files
			//LOG(ERROR) << m_currentFile.substr(begin, m_pp - begin) << " is not a recognized symbol.";
			PARSINGERROR(m_currentFile.substr(pToken->begin, len) + " is not a recognized symbol type.", pToken);
		}
		if (returnCode == -1) return -1;
	}

	return 0;
}



// ***************************************************** //

int Parser::declareVar(bool _const, game::SymbolTable< game::Symbol >& _table)
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

	string word(m_lexer.getWord(*typeToken));

	//validate type
	int index = m_gameData.m_types.find(word);

	Token* nameToken;

	int arraySize = 1;

	if (index == -1) PARSINGERROR("Unknown type: " + word, typeToken);

	do
	{
		//get name
		nameToken = m_lexer.nextToken();
		if (!nameToken || nameToken->type != Symbol)
		{
			PARSINGERROR("Word expected.", nameToken);
		}

		//now filled with the name
		word = m_lexer.getWord(*nameToken);

		arraySize = 1;

		//array declaration
		//or end expression: ';'
		token = m_lexer.nextToken();
		NULLCHECK(token);

		//array declaration
		if (*token == SquareBracketLeft)
		{
			token = m_lexer.nextToken();
			NULLCHECK(token);

			if (*token != TokenType::ConstInt)
			{
				int constIndex = m_gameData.m_constInts.find(m_lexer.getWord(*token));

				if (constIndex != -1) arraySize = m_gameData.m_constInts[constIndex].value[0];
				else PARSINGERROR("Const int expected.", token);
			}
			else
				arraySize = m_lexer.getInt(*token);

			token = m_lexer.nextToken();
			NULLCHECK(token);
			if (*token != TokenType::SquareBracketRight) PARSINGERROR("Right square bracket ']' expected.", token);

			//read next one
			token = m_lexer.nextToken();
			NULLCHECK(token);
		}

		//only non consts are added after knowing name and type
		if (!_const)
		{
			bool check = _table.emplace(word, index);
			if (!check) parserLog(Warning, "Symbol redefinition.", nameToken);
		}

	} while (*token == Comma);

	//finished
	if (!_const && token->type == End)
	{
		return 0;
	}
	//constants need to be initialized instantly
	else if (m_currentFile[token->begin] == '=')
	{
		//const arrays start with '{'
		if (arraySize > 1) TOKEN(CurlyBracketLeft);

		//todo: remove the need for a garbage stack
		std::vector< game::StackInstruction > stack;

		//type decides how to parse
		if (index == 2)
		{
			game::ConstSymbol<int, 2> constSymbol(word);
			constSymbol.value.resize(arraySize);
			
			for (int i = 0; i < arraySize; ++i)
			{
				game::DummyInt result(0);
				if (Term(&result)) return -1;
				constSymbol.value[i] = result.value;

				if (i < arraySize - 1) TOKEN(Comma);
			}
			m_gameData.m_constInts.add(std::move(constSymbol));
		}
		else if (index == 1)
		{
			game::ConstSymbol<float, 1> constSymbol(word);
			constSymbol.value.resize(arraySize);

			for (int i = 0; i < arraySize; ++i)
			{
				game::DummyFloat result(0);
				if (Term(&result)) return -1;
				constSymbol.value[i] = result.value;

				if (i < arraySize - 1) TOKEN(Comma);
			}
			m_gameData.m_constFloats.add(std::move(constSymbol));
		}
		else if (index == 3)
		{
			game::ConstSymbol_String constSymbol(word);
			constSymbol.value.resize(arraySize);

			for (int i = 0; i < arraySize; ++i)
			{
				TOKENEXT(TokenType::ConstStr, strToken);
				constSymbol.value[i] = std::move(m_lexer.getWord(*strToken));
				if (i < arraySize - 1) TOKEN(Comma);
			}
			m_gameData.m_constStrings.add(std::move(constSymbol));
		}
		else PARSINGERROR("Only int, float and string can be const.", typeToken);

		if (arraySize > 1) TOKEN(CurlyBracketRight);

		TOKEN(End);

		return 0;
	}

	PARSINGERROR("Unexpected token.", token);
}

// ***************************************************** //

int Parser::declareFunc()
{
	TOKENEXT(Symbol, typeToken);
	TOKENEXT(Symbol, nameToken);

	game::Symbol_Function functionSymbol(m_lexer.getWord(*nameToken), getType(*typeToken));

	TOKEN(ParenthesisLeft);

	//required to make TOKENOPT work :(
	Token* tokenOpt;

	//optional param list
	if (!TOKENOPT(ParenthesisRight))
	{
		m_lexer.prev();
		do
		{
			//actually only 'var' is valid
			TOKEN(Symbol);
			//relevant information
			//parameter type
			TOKENEXT(Symbol, paramTypeToken);
			//parameter name
			TOKENEXT(Symbol, paramNameToken);

			functionSymbol.params.emplace(m_lexer.getWord(*paramNameToken), getType(*paramTypeToken));

		} while (TOKENOPT(Comma));

		//failed optional token
		m_lexer.prev();

		TOKEN(ParenthesisRight);
	}

	//externals have no body
	//instead there adress in this format:
	// func void foo() 0xAB7541F0;
	if (TOKENOPT(ConstInt))
	{
		TOKEN(End);
		functionSymbol.addFlag(game::Flag::External);
		m_gameData.m_functions.add(functionSymbol);
		return 0;
	}

	m_lexer.prev();

	assignFunctionParam(functionSymbol);

	//code block
	parseCodeBlock(functionSymbol);
	
	if (!TOKENOPT(End))
	{
		if (m_alwaysSemikolon)
		{
			PARSINGERROR("End';' expected.", tokenOpt);
		}
		else //read token is part of the next declaration
			m_lexer.prev();
	}

	//always end with a return
	//for some reason gothic even ends with two rets 
	//same happens here when a "return" is found
	//update: the compiler adds one return to every function including those without a body
	//functionSymbol.byteCode.emplace_back(game::Instruction::Ret);

	m_gameData.m_functions.add(std::move(functionSymbol));

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

		names.push_back(m_lexer.getWord(*nameToken));

	} while (TOKENOPT(Comma));

	m_lexer.prev();

	TOKEN(ParenthesisLeft);

	//type
	TOKENEXT(Symbol, typeToken);

	int i = getType(*typeToken);

	//type does not matter when it is derivated from a prototype or not existent
	//thus the check may happen afterwards
	game::Symbol_Instance instance(names[0], i);

	if (i == -1)
	{
		i = m_gameData.m_prototypes.find(m_lexer.getWord(*typeToken));

		if(i == -1) PARSINGERROR("Unknown type.", typeToken);

		//init function starts with a call to its prototype
		instance.byteCode.emplace_back(game::Instruction::call, m_gameData.m_prototypes[i].id);
	}

	TOKEN(ParenthesisRight);

	if(!TOKENOPT(End))
	{
		m_lexer.prev();

		parseCodeBlock(instance);

		if (!TOKENOPT(End))
		{
			if (m_alwaysSemikolon)
			{
				PARSINGERROR("End';' expected.", tokenOpt);
			}
			else //read token is part of the next declaration
				m_lexer.prev();
		}
	}

	if (names.size() > 1)
		for (int i = 1; i < names.size(); ++i)
		{
			m_gameData.m_instances.emplace(names[i], instance);
		}

	//try moving after the references are not needed anymore
	m_gameData.m_instances.add(std::move(instance));

	return 0;
}

// ***************************************************** //

int Parser::declarePrototype()
{
	Token* tokenOpt;

	string name;

	TOKENEXT(Symbol, nameToken);

	name = m_lexer.getWord(*nameToken);

	TOKEN(ParenthesisLeft);

	//type
	TOKENEXT(Symbol, typeToken);

	int i = getType(*typeToken);

	if (i == -1) PARSINGERROR("Unknown type.", typeToken);

	TOKEN(ParenthesisRight);

	m_gameData.m_prototypes.emplace(name, i);
	auto& prototype = m_gameData.m_prototypes.back();

	parseCodeBlock(prototype);

	if (!TOKENOPT(End))
	{
		if (m_alwaysSemikolon)
		{
			PARSINGERROR("End';' expected.", tokenOpt);
		}
		else //read token is part of the next declaration
			m_lexer.prev();
	}

	return 0;
}

// ***************************************************** //


int Parser::declareClass()
{
	//get name
	Token* token = m_lexer.nextToken();
	if (!token || token->type != Symbol)
	{
		PARSINGERROR("Word expected.", token);
	}

	string word = m_lexer.getWord(*token);

	token = m_lexer.nextToken();

	if (!token || token->type != CurlyBracketLeft)
	{
		PARSINGERROR("Curly Bracket '{' expected.", token);
	}

	game::Symbol_Type type(word);

	int ret = 0;
	//parse all members
	while ((token = m_lexer.nextToken()) && ret != -1 && !m_currentFile.compare(token->begin, 1 + token->end - token->begin, "var"))
		ret = declareVar(0, type.elem);

	m_gameData.m_types.add(std::move(type));

	if (ret == -1) return -1;

	//the next token is already retrieved by the loop

	NULLCHECK(token);

	if (token->type != CurlyBracketRight)
	{
		PARSINGERROR("Curly Bracket '}' expected.", token);
	}

	//gothic declarations always close with ';'
	
	token = m_lexer.nextToken();

	NULLCHECK(token);

	if (token->type != End)
	{
		if (m_alwaysSemikolon)
		{
			PARSINGERROR("End';' expected.", token);
		}
		else //read token is part of the next declaration
			m_lexer.prev();
	}

	return 0;

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
			if (m_currentFile[i] == '\n') ++lineCount;

		msg = "Found: \"" + m_lexer.getWord(*_token) + "\" - ";
	}

	msg += _msg;

	//since ERROR and WARNING are forwarded by a macro runtime evaluation needs a switch
	if (_lvl == Error)
		LOG(ERROR) << msg << " [l." << lineCount << "] " << "[" << m_currentFileName << "]";
	else if (_lvl == Warning)
		LOG(WARNING) << msg << " [l." << lineCount << "] " << "[" << m_currentFileName << "]";
}

int Parser::getType(Token& _token)
{
	return m_gameData.m_types.find(m_lexer.getWord(_token));
}

}