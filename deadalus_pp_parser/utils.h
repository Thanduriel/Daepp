#pragma once

#include <string>

namespace utils
{
	/* find()
	* lookup in an container with random access
	* for elements that have a member "string name"
	* works just like SymbolTable::find()
	*/
	template < typename _T >
	int find(_T& _container, std::string& _name)
	{
		for (size_t i = 0; i < _container.size(); ++i)
			if (_container[i].name == _name) return (int)i;

		return -1;
	};

	std::string strInterval(const std::string& _str, size_t _begin, size_t _end, size_t _count, char _del0 = '\n', char _del1 = '\n');
}