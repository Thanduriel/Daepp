#pragma once

#include "parserconfig.h"
#include "lexer.h"
#include "compiler.h"
#include "parserintern.h"

namespace par {

	enum LogLvl
	{
		Error = 16,
		Warning = 32
	};

/* Parser *******************************
 * The main part of the pipline
 * analyzes the content
 * error codes:
 * -1 parsing error
 * 0 success
 */

class Parser
{
public:
	Parser(const std::string& _configFile);
	~Parser();

	/* parseSource() ************************************
	 * parses a .src file with any files named within
	 */
	void parseSource(const std::string& _fileName);

	/* parseFile() ************************************
	* Parses a token stream hold by the given lexer
	* and adds all declarations to the current GameData.
	* This is not multithreading compatible.
	*
	*/
	int parseFile(Lexer& _lexer);
	static void tokenizeFile(const std::string& _fileName, Lexer* _lexer);


	/* compile() *************************************
	 * Compiles the parsed data into a (hopefully) by gothic readable .dat
	 */
	void compile() { m_compiler.compile("test.dat", m_config.m_saveInOrder); };

private:
	/* preCompilerDirective() *****************************
	 * Parses a pre directive and sets the required switches to process it
	 */
	void preDirective(const std::string& _directive);
	/* Term() *********************************
	* reads a mathematical term from the tokenstream
	* @param _value the calculated value of the term
	* can hold any int or float
	* @return parsing error code
	*/
	int Term(game::Symbol_Core* _ret = nullptr, game::Symbol_Function* _function = nullptr);

	int pushInstr(game::Symbol_Core* _sym, game::ByteCodeStack& _instrStack);

	inline int pushParamInstr(game::Symbol_Core* _sym, game::ByteCodeStack& _instrStack);

	//pushes the code that assigns the values on the stack to the function params
	void assignFunctionParam(game::Symbol_Function& _func);

	/* verifyParam() *****************************
	* @return true when the types match
	* takes into account implicit typecast and consts
	*/
	bool verifyParam(game::Symbol& _expected, game::Symbol_Core& _found) { return verifyParam(_expected.type, _found.type); };
	bool verifyParam(int _expected, int _found);

	/* codeBlock() ***********************
	* reads code from the tokenstream and translates it to bytecode
	* @param _token first token of the instruction
	* @param _stack the stack where the bytecode is pushed to
	*/
	int codeBlock(par::Token& _token, game::Symbol_Function& _functionSymbol);
	int codeBlock(game::Symbol_Function& _functionSymbol) { return codeBlock(*m_lexer->nextToken(), _functionSymbol); };

	int parseCodeBlock(CodeToParse& _codeToParse);
	int parseCodeBlock(game::Symbol_Function& _functionSymbol); //does the actual work and may be used recursive

	/* conditionalBlock() ***************************
	 * parses a conditional structure started by an "if"
	 */
	int conditionalBlock(game::Symbol_Function& _functionSymbol);

	/* declareVar() *********************************
	 * reads the type and adds the symbol to the table
	 * @param _const declares a constant whichs value cannot be changed after init
	 * @param _table the SymbolTable the symbol should be added to
	 * @return parsing error code
	 */
	int declareVar(bool _const, utils::SymbolTable< game::Symbol >& _table);

	/* declareFunc() ********************************
	 */
	int declareFunc();

	/* declareClass() *********************************
	* reads the type and adds the symbol to the table
	*/
	int declareClass();

	/* declareInstance() ********************************
	*/
	int declareInstance();

	/* declarePrototype *****************************
	*/
	int declarePrototype();

	/* parserLog() **********************************
	 * A wrapper for LOG(lvl) that
	 * prints the filename aswell as the line number
	 */
	void parserLog(LogLvl _lvl, const std::string& _msg, Token* _token = nullptr);

	inline int getType(Token& _token);

	//current tokenizer with the data as token stream
	Lexer* m_lexer;

	//
	Compiler m_compiler;
	
	//config
	ParserConfig m_config;

	//config set by precompiler directives
	bool m_parseInOrder;

	//state vars
	std::string m_currentFileName; //< name of the parsed file
	//std::string m_currentFile; //< content of the currently parsed file
	bool m_isCodeParsing; //< currently in code parsing mode
	
	int m_lineCount; //< current line

	//linker stuff
	//and stack simulation
	game::Symbol_Type* m_currentNamespace;
	utils::SymbolTable < UndeclaredSymbol > m_undeclaredSymbols;
	std::vector< CodeToParse >* m_codeQue;
	int m_thisInst; //< instance id that occupies the this-pointer in the current code
	//resolved content
	game::GameData m_gameData;
};

}