#pragma once
#include "gamedata.h"


namespace par{

	/* Compiler *****************************
	 * compiles gameData into bytecode
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
		//writes a single given symbol to the stream
		int compileSymbol( game::Symbol& _sym);

		int compileClass( game::Symbol_Type& _sym);

		//countes the size on the stack and adds the code offsets to the function symbol
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

