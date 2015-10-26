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
		/**
		 * Creates a symbol with minimal params.
		 * @param _isTemp When true the symbol does not get a unique id
		 */
		Symbol(bool _isTemp, const std::string& _name) : name(_name), flags(0), parent(0xFFFFFFFF),
			Symbol_Core(0)
		{
			if (!_isTemp)
			{
				id = idCount++;

				allSymbols().emplace_back(this);
			}
		};

		/**
		 * Creates a symbol with the specified params
		 * and an unique id.
		 */
		Symbol(const std::string& _name, unsigned int _type, unsigned int _flags = 0, size_t _size = 1, int _parent = 0xFFFFFFFF)
			: name(_name),
			Symbol_Core(_type),
			size(_size),
			flags(_flags),
			parent(_parent)
		{
			id = idCount++;

			allSymbols().push_back(this);
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
			setById(_other.id, *this);
		}

		Symbol& operator=(Symbol&& _other)
		{
			type = _other.type;
			name = _other.name;
			size = _other.size;
			flags = _other.flags;
			parent = _other.parent;
			id = _other.id;

			setById(id, *this);

			return *this;
		}

		std::string name;

		int id;
		int parent;

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

		static Symbol& getById(size_t _id) { return *allSymbols()[_id]; };
		static void setById(size_t _id, Symbol& sym) { allSymbols()[_id] = &sym; };

		static int idCount; //initialized in "symbol.cpp"

	private:
		static std::vector < Symbol* >& allSymbols()
		{
			static std::vector < Symbol* > allSym;
			return allSym;
		}
	};


	// a symboltable operating on symbols
	typedef utils::SymbolTable < Symbol > StandardSymbolTable;
}