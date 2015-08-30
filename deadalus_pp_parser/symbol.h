#pragma once

#include <string>
#include <fstream>
#include "symboltable.h"

namespace game{

	// base structure for symbols
	//only used in term parsing
	struct Symbol_Core
	{
		Symbol_Core(unsigned int _type, bool _isOperator = false) : type(_type), isOperator(_isOperator) {};
		Symbol_Core() {};

		unsigned int type;

		bool isOperator; // a helper only usefull in termparsing
	};

	//symbol flags
	//encoded in bitfield with << 16
	enum Flag
	{
		Const = 1,
		Return = 2,
		Classvar = 4,
		External = 8,
		Merged = 16,
		AlwaysSet = 64 //seems like every symbol has this
	};

	//basic data/object structure of gothic
	//has no additional content
	//inherit for more complex structures
	struct Symbol: public Symbol_Core
	{
		Symbol(const std::string& _name, unsigned int _type, unsigned int _flags = 0, size_t _size = 1, int _parent = 0)
			: name(_name),
			Symbol_Core(_type),
			size(_size),
			flags(_flags)
		{
			id = idCount++;
		};

		std::string name;

		int id;

		//all part of one dword-bitfield
		
		//unsigned int type;
		unsigned int flags;

		
		void addFlag(Flag _flag)
		{
			flags |= _flag;
		}

		void setFlag(Flag _flag)
		{
			flags ^= flags | _flag;
		}

		bool testFlag(Flag _flag)
		{
			return (flags & _flag) != 0;
		}

		size_t size;

		static const int defaultContentInt = 0;
		static const int defaultContentStr = 0x0000000A;

		virtual void saveContent(std::ofstream& _stream) 
		{
			// string vars require a single termination char as content
			// while int and float want a dword

			//class members are not initialized
			if (!testFlag(Flag::Classvar))
				for (size_t i = 0; i < size; ++i) _stream.write((char*)&defaultContentStr, type == 3 ? 1 : 4);

		};

		static int idCount; //initialized in "symbol.cpp"
	};


	// a symboltable operating on symbols
	typedef SymbolTable < Symbol > StandardSymbolTable;

	// structures for bytecode
	enum Instruction
	{
		//basic operations
		//@param none
		Add,		// +
		Sub,		// -
		Mul,		// *
		Div,		// /
		Mod,		// %
		Or,			// |
		And,		// &
		Less,		// <
		Higher,		// >
		assign,		// =
		lor,
		land,
		shl,
		shr,
		leq,
		eq,
		neq,
		heg,
		assignAdd,
		assignSub,
		assignMul,
		assignDiv,
		uPlus,
		uMinus,
		Not = 32,	// !
		negate,		// ~
		Ret = 60,
		call = 61,
		callExtern,
		pushInt = 64,
		pushVar,
		pushStr,
		pushInst,
		pushIndex,
		popVar,
		assignStr,
		assignStrP, //???
		assignFunc,
		assignFloat,
		assignInst,
		jmp,
		jmpf,
		setInst,
		pushArray = 245,
		//helpers
		Push,
		Binop,
		Unop
	};
	struct StackInstruction
	{
		StackInstruction() {};
		StackInstruction(Instruction _instr) : instruction(_instr), hasParam(false) {};
		StackInstruction(Instruction _instr, int _param) : instruction(_instr), hasParam(true), param(_param){};

		inline void set(Instruction _instr) { instruction = _instr; hasParam = false; };
		inline void set(Instruction _instr, int _param) { instruction = _instr; hasParam = true; param = _param; };

		Instruction instruction;
		//actually dependend on the instruction type
		bool hasParam;
		int param;
	};

	//symbol for functions
	struct Symbol_Function : public Symbol
	{
		Symbol_Function(const std::string& _name, unsigned int _retType, unsigned int _flags = Flag::Const)
			: Symbol(_name, 5, _flags, 0),
			returnType(_retType)
		{
		}

		
		unsigned int returnType; // type index; is translated by the compiler

		unsigned int stackBegin;

		StandardSymbolTable params; // parameters; found at the ids after the Function
		StandardSymbolTable locals; // local vars; found after the params

		std::vector< StackInstruction > byteCode; // bytecode; found on the stack

		void saveContent(std::ofstream& _stream) override
		{
			//content[0] where the byte code is found
			_stream.write((char*)&stackBegin, 4);
		};
	};

	struct Symbol_Instance : public Symbol_Function
	{
		Symbol_Instance(const std::string& _name, unsigned int _type, Symbol_Instance* _prototype = nullptr) :
			Symbol_Function(_name, 0, 0) //no additional flags set
		{
			//type is imlicitly initialized by Symbol_Function
			//so it has to be overwritten
			type = _type;
		}
		//copy like constructor that takes all attributes of the _parent symbol
		//while giving it a unique name and id
		Symbol_Instance(const std::string& _name, const Symbol_Instance& _parent) :
			Symbol_Function(_name, 0)
		{
			type = _parent.type;
			flags = _parent.flags;
			size = _parent.size;
			stackBegin = _parent.stackBegin;
			id = _parent.id;

			byteCode = _parent.byteCode;
		}
	};

	/* Symbol_Type *******************************o
	 * a symbol for Classes
	 */
	struct Symbol_Type: public Symbol
	{
		//atom or default types do not need a unique id
		Symbol_Type(std::string& _name, bool _isAtom = false) : Symbol(_name, 4){ if (_isAtom) idCount--; };

		//complex types consist of other types
		//atom types have elem.size() = 0
		//since they need an identifier aswell
		StandardSymbolTable elem;

		virtual void saveContent(std::ofstream& _stream) override
		{
			//content[0] where the byte code is found
			_stream.write((char*)&defaultContentInt, 4);
		};
	};

	/* ********************************************* 
	 * a symbol that can store values 
	 * saving only works for types without a dynamic buffer
	 * @param _T the type of the values
	 * @param _type 
	 */
	template< typename _T, unsigned int _type>
	struct ConstSymbol : public Symbol
	{
		ConstSymbol(const std::string& _name, size_t _size = 1)
			: Symbol(_name, _type, Const, _size)//(1 << 16)
		{
		};

		ConstSymbol(const std::string& _name, std::initializer_list< _T >  _val)
			: Symbol(_name, _type, Const),//(1 << 16)
			value(_val)
		{
		};

		std::vector< _T > value;

		void saveContent(std::ofstream& _stream) override
		{
			_stream.write((char*)&value[0], sizeof(_T) * value.size());
		//	for (auto& val : value)
		//		_stream << val;
		};
	};

	//symbol for const strings
	struct ConstSymbol_String : public ConstSymbol < std::string, 3 >
	{
		ConstSymbol_String(const std::string& _name, size_t _size = 1)
			: ConstSymbol(_name, _size)
		{}

		//constructor for const strings (like "fooo!") in code segments
		//because the script interpreter can not handle string data, only references
		ConstSymbol_String()
			: ConstSymbol("")
		{
			//the name is generated using the id and adding a 0xFF to the front
			//this seems to be the way the original parser is doing it
			name = char(0xFF) + std::to_string(id);
		}


		void saveContent(std::ofstream& _stream) override
		{
			for (std::string& str : value)
			{
				//add string termination
				str += char(0x0A);
				_stream.write((char*)&str[0], str.size());
			}
		};
	};


	/*structure similar to ConstSymbol
	 * without the overhead
	 * used for inlined symbols
	 */ 
	template < typename _T, unsigned int _TypeIndex >
	struct Symbol_Dummy: public Symbol_Core
	{
		Symbol_Dummy() {};
		Symbol_Dummy(_T _val) : Symbol_Core(_TypeIndex), value(_val) {};

		_T value;
	};

	typedef Symbol_Dummy < int, 8 > DummyInt;
	typedef Symbol_Dummy < float, 9 > DummyFloat;
	typedef Symbol_Dummy < int, 10 > DummyOperator;
}