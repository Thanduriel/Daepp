#include "compiler.h"
#include "easylogging++.h"

namespace par{

	Compiler::Compiler(game::GameData& _gameData)
		:m_gameData(_gameData)
	{
	}

	int Compiler::compile(const std::string& _outputFile)
	{
		fileStream.open(_outputFile, std::ios::out | std::ios::binary);

		if (!fileStream) LOG(ERROR) << "Could not create file " << _outputFile << ".";

		LOG(INFO) << "Compiling code to " << _outputFile << ".";

		fileStream.setf(std::ios_base::hex, std::ios_base::basefield);

		//(byte) version number
		fileStream << (unsigned char)0x32;

		//(int)[] symbol identifiers(id)
		unsigned int symbolCount = game::Symbol::idCount;

		//(int) number of symbols
		fileStream.write((char*)&symbolCount, 4);
		//fileStream << (unsigned int)symbolCount;

		compileSortedTable();
		//for (uint32_t i = 0; i < symbolCount; ++i)
		//	fileStream.write((char*)&i, 4);

		parent = 0xFFFFFFFF;

		// symbols
		for (int i = 0; i < m_gameData.m_symbols.size(); ++i)
			compileSymbol( m_gameData.m_symbols[i]);
		for (int i = 0; i < m_gameData.m_constFloats.size(); ++i)
			compileSymbol( *(game::Symbol*)&m_gameData.m_constFloats[i]);
		for (int i = 0; i < m_gameData.m_constInts.size(); ++i)
			compileSymbol( *(game::Symbol*)&m_gameData.m_constInts[i]);
		for (int i = 0; i < m_gameData.m_constStrings.size(); ++i)
			compileSymbol( *(game::Symbol*)&m_gameData.m_constStrings[i]);
		
		stackSize = 0;
		
		for (int i = 0; i < m_gameData.m_functions.size(); ++i)
			compileFunction( m_gameData.m_functions[i]);

		for (int i = 11; i < m_gameData.m_types.size(); ++i)
			compileClass( m_gameData.m_types[i]);

		for (int i = 0; i < m_gameData.m_prototypes.size(); ++i)
			compileFunction( m_gameData.m_prototypes[i]);
		for (int i = 0; i < m_gameData.m_instances.size(); ++i)
			compileFunction( m_gameData.m_instances[i]);

		//default value for no parent
		parent = 0xFFFFFFFF;

		for (int i = 0; i < m_gameData.m_internStrings.size(); ++i)
			compileSymbol(*(game::Symbol*)&m_gameData.m_internStrings[i]);

		//(int)data stack length
		fileStream.write((char*)&stackSize, 4);

		//(byte)[] data
		compileStack();

		fileStream.close();

		return 0;
	}

	// ************************************************************* //

	int Compiler::compileSortedTable()
	{
		for (int i = 0; i < m_gameData.m_symbols.size(); ++i)
			fileStream.write((char*)&m_gameData.m_symbols[i].id, 4);
		
		for (int i = 0; i < m_gameData.m_constFloats.size(); ++i)
			fileStream.write((char*)&m_gameData.m_constFloats[i].id, 4);
		
		for (int i = 0; i < m_gameData.m_constInts.size(); ++i)
			fileStream.write((char*)&m_gameData.m_constInts[i].id, 4);
		
		for (int i = 0; i < m_gameData.m_constStrings.size(); ++i)
			fileStream.write((char*)&m_gameData.m_constStrings[i].id, 4);
		
		for (int i = 0; i < m_gameData.m_functions.size(); ++i)
		{
			fileStream.write((char*)&m_gameData.m_functions[i].id, 4);

			for (int j = 0; j < m_gameData.m_functions[i].params.size(); ++j)
				fileStream.write((char*)&m_gameData.m_functions[i].params[j].id, 4);
			for (int j = 0; j < m_gameData.m_functions[i].locals.size(); ++j)
				fileStream.write((char*)&m_gameData.m_functions[i].locals[j].id, 4);
		}
		
		for (int i = 11; i < m_gameData.m_types.size(); ++i)
		{
			fileStream.write((char*)&m_gameData.m_types[i].id, 4);

			for (int j = 0; j < m_gameData.m_types[i].elem.size(); ++j)
				fileStream.write((char*)&m_gameData.m_types[i].elem[j].id, 4);
		}

		for (int i = 0; i < m_gameData.m_prototypes.size(); ++i)
		{
			fileStream.write((char*)&m_gameData.m_prototypes[i].id, 4);
		}

		for (int i = 0; i < m_gameData.m_instances.size(); ++i)
		{
			fileStream.write((char*)&m_gameData.m_instances[i].id, 4);
		}

		for (int i = 0; i < m_gameData.m_internStrings.size(); ++i)
			fileStream.write((char*)&m_gameData.m_internStrings[i].id, 4);

		return true;
	}

	// ************************************************************* //

	int Compiler::compileSymbol(game::Symbol& _sym)
	{
		uint32_t tmp = 1;
		//(int) b_hasName
		fileStream.write((char*)&tmp, 4);
		//fileStream << int(1);

		//(string) name
		fileStream << _sym.name;

		//name is terminated by '\n'
		fileStream << '\n';

		//(int) offset
		tmp = 0;
		fileStream.write((char*)&tmp, 4);
		//fileStream << (int)0;

		//(int) bitfield
		//types higher then 7 are saved as parent with type 7
		//add flag that is always set for uknown reasons
		unsigned int bitfield = ((_sym.type > 7 ? 7 :_sym.type) << 12) | ((_sym.flags | game::Flag::AlwaysSet) << 16) | _sym.size;
		fileStream.write((char*)&bitfield, 4);

		//(int) filenr
		fileStream.write((char*)&tmp, 4);
		//fileStream << (int)0;

		//(int) line
		//the line of the code in the source file
		//probably relevant for some dbg msgs
		fileStream.write((char*)&tmp, 4);
		//fileStream << (int)0;

		//(int) line_anz
		//number of lines the code takes in the source file
		tmp++;
		fileStream.write((char*)&tmp, 4);
		//fileStream << (int)1;

		//(int) pos_beg
		tmp = 0;
		fileStream.write((char*)&tmp, 4);
		//fileStream << (int)0;

		//(int) pos_anz
		fileStream.write((char*)&tmp, 4);
	//	fileStream << (int)0;

		//for consts and functions the content
	//	if (_sym.testFlag(game::Flag::Const))
	//	{
		_sym.saveContent(fileStream);
	//	}

		//parent
		tmp = _sym.type > 7 ? m_gameData.m_types[_sym.type].id : parent;
		fileStream.write((char*)&tmp, 4);
	//	fileStream << (int)0xFFFFFFFF;

		return 0;
	}

	// ********************************************* //

	int Compiler::compileClass(game::Symbol_Type& _sym)
	{
		//update size information
		_sym.size = _sym.elem.size();

		compileSymbol(_sym);

		parent = _sym.id;

		for (int i = 0; i < _sym.elem.size(); ++i)
		{
			//add name to it
			_sym.elem[i].name = _sym.name + '.' + _sym.elem[i].name;

			compileSymbol(_sym.elem[i]);
		}

		return 0;
	}

	// ********************************************* //

	int Compiler::compileFunction(game::Symbol_Function& _sym)
	{
		_sym.stackBegin = stackSize;

		//calculate size the code will take
		for (int i = 0; i < _sym.byteCode.size(); ++i)
		{
			stackSize += _sym.byteCode[i].hasParam ? 5 : 1;
		}
		//for the return instruction
		stackSize++;

		compileSymbol(_sym);

		//after the function params and locals follow

		for (int i = 0; i < _sym.params.size(); ++i)
		{
			_sym.params[i].name = _sym.name + '.' + _sym.params[i].name;

			compileSymbol(_sym.params[i]);
		}

		for (int i = 0; i < _sym.locals.size(); ++i)
		{
			_sym.locals[i].name = _sym.name + '.' + _sym.locals[i].name;

			compileSymbol(_sym.locals[i]);
		}

		return true;
	}

	// ********************************************* //

	int Compiler::compileStack()
	{

		for (int i = 0; i < m_gameData.m_functions.size(); ++i)
			compileByteCode(m_gameData.m_functions[i].byteCode);
		for (int i = 0; i < m_gameData.m_prototypes.size(); ++i)
			compileByteCode(m_gameData.m_prototypes[i].byteCode);
		for (int i = 0; i < m_gameData.m_instances.size(); ++i)
			compileByteCode(m_gameData.m_instances[i].byteCode);

		return 0;
	}

	void Compiler::compileByteCode(std::vector< game::StackInstruction >& _byteCode)
	{
		static game::Instruction retInstr = game::Instruction::Ret;

		for (auto& inst : _byteCode)
		{
			int opCode = inst.instruction;
			fileStream.write((char*)&opCode, 1);
			//optional param
			if (inst.hasParam) fileStream.write((char*)&inst.param, 4);
		}

		//every function ends with a return
		fileStream.write((char*)&retInstr, 1);
	}
}