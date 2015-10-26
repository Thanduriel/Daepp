#pragma once

#include "symboltable.h"

namespace lang{
	//this may not be changed
	//gothic runs with this indizies
	std::initializer_list < game::Symbol_Type* > basicTypes = { 
		new game::Symbol_Type(std::string("void"), true),			//0
		new game::Symbol_Type(std::string("float"), true),			//1
		new game::Symbol_Type(std::string("int"), true),			//2
		new game::Symbol_Type(std::string("string"), true),			//3
		new game::Symbol_Type(std::string("class"), true),			//4
		new game::Symbol_Type(std::string("func"), true),			//5
		new game::Symbol_Type(std::string("prototype"), true),		//6
		new game::Symbol_Type(std::string("instance"), true),		//7
		//this are interns used by the parser only
		//symbols of this type are completly inlined
		new game::Symbol_Type(std::string("constInt"), true),		//8
		new game::Symbol_Type(std::string("constFloat"), true),		//9
		new game::Symbol_Type(std::string("operator"), true),		//10
		new game::Symbol_Type(std::string("thisInst"), true),		//11
		new game::Symbol_Type(std::string("beginCustomTypes"), true)
		//starting from 11 custom types have their place
	};


} //end namespace