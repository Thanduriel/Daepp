#pragma once

#include <string>
#include <fstream>
#include "symboltable.h"

extern size_t g_atomTypeCount;

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
	struct Symbol : public Symbol_Core
	{
		//a structure that can hold a symbol identifier or a instruction param.
		//In parsing state references are hold as unique identifier
		//while in compiling state int id are used(like in gothic).
		struct VirtualId
		{
			VirtualId() = default;
			VirtualId(int _i) { id = _i; };
			VirtualId(Symbol* _ptr){ ptr = _ptr; };
			union
			{
				//-- instruction params
				int constNum; //const numerical
				int sadr; //stack adr
				//-- references
				int id; //id of a symbol
				Symbol* ptr; //pointer to a symbol
			};
		};
		/**
		 * Creates a symbol with minimal params.
		 * @param _isTemp When true the symbol does not get a unique id
		 */
		Symbol(bool _isTemp, const std::string& _name) : name(_name), flags(0), parent((int)0xFFFFFFFF),
			Symbol_Core(0)
		{
		};

		/**
		 * Creates a symbol with the specified params
		 * and an unique id.
		 */
		Symbol(const std::string& _name, unsigned int _type, unsigned int _flags = 0, size_t _size = 1, VirtualId _parent = 0xFFFFFFFF)
			: name(_name),
			Symbol_Core(_type),
			size(_size),
			flags(_flags),
			parent(_parent)
		{
		};

		//move constructor that handles id managment
		Symbol(Symbol&& _other)
			: Symbol_Core(_other.type),
			name(_other.name),
			size(_other.size),
			flags(_other.flags),
			parent(_other.parent),
			id(_other.id)
		{
		}

		//move assignment
		Symbol& operator=(Symbol&& _other)
		{
			type = _other.type;
			name = _other.name;
			size = _other.size;
			flags = _other.flags;
			parent = _other.parent;
			id = _other.id;

			return *this;
		}

		// sets the id of this symbol and returns how many ids where used for itself and its members
		virtual int setId(int _id) { id = _id; return 1; }

		std::string name;

		int id;
		VirtualId parent;

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

			//class members have no content
			if (!testFlag(Flag::Classvar))
				for (size_t i = 0; i < size; ++i) _stream.write((char*)&defaultContentStr, type == 3 ? 1 : 4);

		};
	private:
	};


	// a symboltable operating on symbols
	typedef utils::SymbolTable < Symbol > StandardSymbolTable;
}