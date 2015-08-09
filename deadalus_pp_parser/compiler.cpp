#include "compiler.h"
#include "easylogging++.h"

namespace par{

	Compiler::Compiler(game::GameData& _gameData)
		:m_gameData(_gameData)
	{
	}

	int Compiler::compile(const std::string& _outputFile)
	{
		std::ofstream stream(_outputFile, std::ios::out | std::ios::binary);

		if (!stream) LOG(ERROR) << "Could not create file " << _outputFile << ".";

		LOG(INFO) << "Compiling code to " << _outputFile << ".";

		stream.setf(std::ios_base::hex, std::ios_base::basefield);

		//(byte) version number
		stream << (unsigned char)0x32;

		//(int)[] symbol identifiers(id)
		unsigned int symbolCount = m_gameData.m_symbols.size()
			+ m_gameData.m_constFloats.size()
			+ m_gameData.m_constInts.size()
			+ m_gameData.m_constStrings.size()
			+ m_gameData.m_internStrings.size();

		//(int) number of symbols
		stream.write((char*)&symbolCount, 4);
		//stream << (unsigned int)symbolCount;

		compileSortedTable(stream);
		//for (uint32_t i = 0; i < symbolCount; ++i)
		//	stream.write((char*)&i, 4);

		parent = 0xFFFFFFFF;

		// symbols
		for (int i = 0; i < m_gameData.m_symbols.size(); ++i)
			compileSymbol(stream, m_gameData.m_symbols[i]);
		for (int i = 0; i < m_gameData.m_constFloats.size(); ++i)
			compileSymbol(stream, *(game::Symbol*)&m_gameData.m_constFloats[i]);
		for (int i = 0; i < m_gameData.m_constInts.size(); ++i)
			compileSymbol(stream, *(game::Symbol*)&m_gameData.m_constInts[i]);
		for (int i = 0; i < m_gameData.m_constStrings.size(); ++i)
			compileSymbol(stream, *(game::Symbol*)&m_gameData.m_constStrings[i]);
		
		stackSize = 0;
		
		for (int i = 0; i < m_gameData.m_functions.size(); ++i)
			compileFunction(stream, m_gameData.m_functions[i]);

		for (int i = 11; i < m_gameData.m_types.size(); ++i)
			compileClass(stream, m_gameData.m_types[i]);

		//default value for no parent
		parent = 0xFFFFFFFF;

		for (int i = 0; i < m_gameData.m_internStrings.size(); ++i)
			compileSymbol(stream, *(game::Symbol*)&m_gameData.m_internStrings[i]);

		//(int)data stack length
		stream.write((char*)&stackSize, 4);

		//(byte)[] data
		compileStack(stream);

		return 0;
	}

	// ************************************************************* //

	int Compiler::compileSortedTable(std::ofstream& _stream)
	{
		for (int i = 0; i < m_gameData.m_symbols.size(); ++i)
			_stream.write((char*)&m_gameData.m_symbols[i].id, 4);
		
		for (int i = 0; i < m_gameData.m_constFloats.size(); ++i)
			_stream.write((char*)&m_gameData.m_constFloats[i].id, 4);
		
		for (int i = 0; i < m_gameData.m_constInts.size(); ++i)
			_stream.write((char*)&m_gameData.m_constInts[i].id, 4);
		
		for (int i = 0; i < m_gameData.m_constStrings.size(); ++i)
			_stream.write((char*)&m_gameData.m_constStrings[i].id, 4);
		
		for (int i = 0; i < m_gameData.m_functions.size(); ++i)
		{
			_stream.write((char*)&m_gameData.m_functions[i].id, 4);

			for (int j = 0; j < m_gameData.m_functions[i].params.size(); ++j)
				_stream.write((char*)&m_gameData.m_functions[i].params[j].id, 4);
			for (int j = 0; j < m_gameData.m_functions[i].locals.size(); ++j)
				_stream.write((char*)&m_gameData.m_functions[i].locals[j].id, 4);
		}
		
		for (int i = 11; i < m_gameData.m_types.size(); ++i)
		{
			_stream.write((char*)&m_gameData.m_types[i].id, 4);

			for (int j = 0; j < m_gameData.m_types[i].elem.size(); ++j)
				_stream.write((char*)&m_gameData.m_types[i].elem[j].id, 4);
		}

		for (int i = 0; i < m_gameData.m_internStrings.size(); ++i)
			_stream.write((char*)&m_gameData.m_internStrings[i].id, 4);

		return true;
	}

	// ************************************************************* //

	int Compiler::compileSymbol(std::ofstream& _stream, game::Symbol& _sym)
	{
		uint32_t tmp = 1;
		//(int) b_hasName
		_stream.write((char*)&tmp, 4);
		//_stream << int(1);

		//(string) name
		_stream << _sym.name;

		//name is terminated by '\n'
		_stream << '\n';

		//(int) offset
		tmp = 0;
		_stream.write((char*)&tmp, 4);
		//_stream << (int)0;

		//(int) bitfield
		//types higher then 7 are saved as parent with type 7
		//add flag that is always set for uknown reasons
		unsigned int bitfield = ((_sym.type > 7 ? 7 :_sym.type) << 12) | ((_sym.flags | game::Flag::AlwaysSet) << 16) | _sym.size;
		_stream.write((char*)&bitfield, 4);
		//_stream << bitfield;

		//(int) filenr
		_stream.write((char*)&tmp, 4);
		//_stream << (int)0;

		//(int) line
		//the line of the code in the source file
		//probably relevant for some dbg msgs
		_stream.write((char*)&tmp, 4);
		//_stream << (int)0;

		//(int) line_anz
		//number of lines the code takes in the source file
		tmp++;
		_stream.write((char*)&tmp, 4);
		//_stream << (int)1;

		//(int) pos_beg
		tmp = 0;
		_stream.write((char*)&tmp, 4);
		//_stream << (int)0;

		//(int) pos_anz
		_stream.write((char*)&tmp, 4);
	//	_stream << (int)0;

		//for consts and functions the content
		if (_sym.testFlag(game::Flag::Const))
		{
			_sym.saveContent(_stream);
		}

		//parent
		tmp = _sym.type > 7 ? m_gameData.m_types[_sym.type].id : parent;
		_stream.write((char*)&tmp, 4);
	//	_stream << (int)0xFFFFFFFF;

		return 0;
	}

	// ********************************************* //

	int Compiler::compileClass(std::ofstream& _stream, game::Symbol_Type& _sym)
	{
		compileSymbol(_stream, _sym);

		parent = _sym.id;

		for (int i = 0; i < _sym.elem.size(); ++i)
		{
			//add name to it
			_sym.elem[i].name = _sym.name + '.' + _sym.elem[i].name;

			compileSymbol(_stream, _sym.elem[i]);
		}

		return 0;
	}

	// ********************************************* //

	int Compiler::compileFunction(std::ofstream& _stream, game::Symbol_Function& _sym)
	{
		_sym.stackBegin = stackSize;

		for (int i = 0; i < _sym.byteCode.size(); ++i)
			stackSize += _sym.byteCode[i].hasParam ? 5 : 1;

		compileSymbol(_stream, _sym);

		//after the function params and locals follow

		for (int i = 0; i < _sym.params.size(); ++i)
		{
			_sym.params[i].name = _sym.name + '.' + _sym.params[i].name;

			compileSymbol(_stream, _sym.params[i]);
		}

		for (int i = 0; i < _sym.locals.size(); ++i)
		{
			_sym.locals[i].name = _sym.name + '.' + _sym.locals[i].name;

			compileSymbol(_stream, _sym.locals[i]);
		}

		return true;
	}

	// ********************************************* //

	int Compiler::compileStack(std::ofstream& _stream)
	{
		for (int i = 0; i < m_gameData.m_functions.size(); ++i)
		{
			for (auto& inst : m_gameData.m_functions[i].byteCode)
			{
				int opCode = inst.instruction;
				_stream.write((char*)&opCode, 1);
				//optional param
				if (inst.hasParam) _stream.write((char*)&inst.param, 4);
			}
		}

		return 0;
	}
}