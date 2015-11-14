#pragma once
#include "gamedata.h"


namespace par{

	/* Compiler *****************************
	 * Takes a gamaData structure produced by a Parser
	 * It first moves all vital data into m_symbols
	 * making many fast access routes invalid
	 * and fixes specific syntactic rules of the gothic compiled stuff.
	 * Then all symbols are written into a file,
	 * while the stack is build in the right order and including every
	 * intern function(functions, prototypes, instances).
	 */

	class Compiler
	{
	public:
		Compiler(game::GameData& _gameData);

		/* compile() ************************
		 * Compiles the given gameData into bytecode written in a file
		 * @param _outputFile the file the code is written to
		 *
		 */
		int compile(const std::string& _outputFile, bool _saveInOrder);

	private:
		/* updateVirtualIds() **************************
		 * Exchanges all uses of symbol pointers with symbol ids.
		 * Ids are finalized here and substituted in symbol.parent and in instructions.
		 */
		void updateVirtualIds();

		//add const strings to the symboltable
		void addConstStrings();

		//move params and locals
		void exportFunctionMembers();

		//move members
		void exportClassMembers();
		
		//writes a single given symbol to the stream
		int compileSymbol( game::Symbol& _sym);

		//updates class specific attributes and writes the symbol
		int compileClass( game::Symbol_Type& _sym);

		//counts the size on the stack and adds the code offsets to the function symbol
		//before writing it to the file
		int compileFunction( game::Symbol_Function& _sym);
		
		int compileStack();

		void compileByteCode(game::ByteCodeStack& _byteCode, unsigned int _offset);

		std::ofstream fileStream;

		game::GameData& m_gameData; // reference to the gameData to compile
		std::vector < game::Symbol_Function* > m_functions;

		int stackSize; //holds the current size of the stack in byte

		unsigned int parent; // parent of the currently written symbol
		unsigned int offset; // offset of the currently written symbol
	};

}

