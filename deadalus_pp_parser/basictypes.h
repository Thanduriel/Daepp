#pragma once

#include "symboltable.h"

namespace lang{
	//this may not be changed
	//gothic runs with this indizies
	std::initializer_list < game::Symbol_Type > basicTypes = { 
		game::Symbol_Type(std::string("void"), true),			//0
		game::Symbol_Type(std::string("float"), true),			//1
		game::Symbol_Type(std::string("int"), true),			//2
		game::Symbol_Type(std::string("string"), true),			//3
		game::Symbol_Type(std::string("class"), true),			//4
		game::Symbol_Type(std::string("func"), true),			//5
		game::Symbol_Type(std::string("prototype"), true),		//6
		game::Symbol_Type(std::string("instance"), true),		//7
		//this are interns used by the parser only
		//symbols of this type are completly inlined
		game::Symbol_Type(std::string("constInt"), true),		//8
		game::Symbol_Type(std::string("constFloat"), true),		//9
		game::Symbol_Type(std::string("operator"), true)		//10
		//starting from 11 custom types have their place
	};


} //end namespace