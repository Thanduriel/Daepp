#include "compiler.h"
#include "easylogging++.h"

#define APPEND(a) for (int i = 0; i < a .size(); ++i) allSymbols[c + i] = &(a)[i]; c += m_gameData.(a).size();

namespace par{

	Compiler::Compiler(game::GameData& _gameData)
		:m_gameData(_gameData)
	{
	}

	// ********************************************* //

	bool symbolCmp(game::Symbol* _slf, game::Symbol* _oth) { return _slf->id < _oth->id; }

	int Compiler::compile(const std::string& _outputFile, bool _saveInOrder)
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

		stackSize = 0;
		parent = 0xFFFFFFFF;

		if (_saveInOrder)
		{
			for (uint32_t i = 0; i < symbolCount; ++i)
				fileStream.write((char*)&i, 4);

			size_t c = 0;
			
			//compile in order
			for (size_t i = 0; i < game::Symbol::idCount; ++i)
			{
				game::Symbol* symbol = &game::Symbol::getById(i);

				if (dynamic_cast<game::Symbol_Type*> (symbol))
				{
					compileClass(*(game::Symbol_Type*)symbol);
				}
				else if (dynamic_cast<game::Symbol_Function*> (symbol))
				{
					compileFunction(*(game::Symbol_Function*)symbol);
					m_functions.push_back((game::Symbol_Function*)symbol);
				}
				else
					compileSymbol(*symbol);
			}
		}

		//(int)data stack length
		fileStream.write((char*)&stackSize, 4);

		//(byte)[] data
		compileStack();

		fileStream.close();

		return 0;
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
		fileStream.write((char*)&offset, 4);
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
	//	tmp = _sym.type > 7 ? m_gameData.m_types[_sym.type].id : parent;
		fileStream.write((char*)&_sym.parent, 4);
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

		//	compileSymbol(_sym.elem[i]);
		}

		return 0;
	}

	// ********************************************* //

	int Compiler::compileFunction(game::Symbol_Function& _sym)
	{
		//compile information to write into a file
		_sym.size = _sym.params.size();
		_sym.stackBegin = stackSize;
		offset = _sym.returnType;

		//calculate size the code will take
		stackSize += (int)_sym.byteCode.getStackSize();

		//for the return instruction
		stackSize++;

		compileSymbol(_sym);

		//reset offset value in case a symbol with default value follows
		offset = 0;

		//after the function params and locals follow

		for (int i = 0; i < _sym.params.size(); ++i)
		{
			_sym.params[i].name = _sym.name + '.' + _sym.params[i].name;
		}

		for (int i = 0; i < _sym.locals.size(); ++i)
		{
			_sym.locals[i].name = _sym.name + '.' + _sym.locals[i].name;
		}

		return true;
	}

	// ********************************************* //

	int Compiler::compileStack()
	{
		for (int i = 0; i < m_functions.size(); ++i)
			compileByteCode(m_functions[i]->byteCode, m_functions[i]->stackBegin);

		return 0;
	}

	// ********************************************* //

	void Compiler::compileByteCode(game::ByteCodeStack& _byteCode, unsigned int _offset)
	{
		static game::Instruction retInstr = game::Instruction::Ret;

		for (auto& inst : _byteCode)
		{
			int opCode = inst.instruction;
			fileStream.write((char*)&opCode, 1);

			//calls take a direct stack adr as param
			// array indecies can be mistaken for call instructions but have hasParam==false
			if (opCode == game::Instruction::call && inst.hasParam)
			{
				inst.param = ((game::Symbol_Function*)&game::Symbol::getById(inst.param))->stackBegin;
			}
			//jumps are direct stack addresses
			else if (opCode == game::Instruction::jmp || opCode == game::Instruction::jmpf)
			{
				inst.param += _offset;
			}
			//optional param
			if (inst.hasParam) fileStream.write((char*)&inst.param, 4);
		}

		//every function ends with a return
		fileStream.write((char*)&retInstr, 1);
	}
}