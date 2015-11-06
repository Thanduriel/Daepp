#pragma once

#include <string>
#include <vector>

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

	//convert a string to lowercase
	void toLower(std::string& _str);

	/*wildcardMatch() ************************
	 * Searches the given string for wildcards
	 * and sets all strings in _container to "" that do not match the sequence.
	 *@param _container A container with access by key [] enabled.
	 * Valid wildcards are '?' for a single char and '*' substituting for a variable amount.
	 */
	template< class _Container >
	void wildcardMatch(const std::string& _str, _Container& _container, size_t _begin, size_t _end = 0)
	{
		if (_end == 0) _end = _container.size() - 1;
		//match the filename with every file in the range
		for (int i = (int)_begin; i < (int)_end; ++i)
		{
			bool fits = false;
			if (_str.size() <= _container[i].size())
			{
				fits = true;
				unsigned int j;
				int offset = 0;
				for (j = 0; j < _str.size(); ++j)
				{
					// '?' => ignore the next char
					if (_str[j] == '?') continue;
					// '*'
					else if (_str[j] == '*')
					{
						//the char afterwards is where the '*' part ends
						//'*' is never at the end since there are only two types of significant endings that should never overlap
						j++;
						//todo find all possible points to continiue so that this works: a*b -> abab
						bool fits = false;
						for (; j + offset < _container[i].size(); ++offset)
							//found where the '*' ends
							if ((_container[i][j + offset]) == _str[j])
							{
							fits = true;
							break;
							}
					}
					else
					{
						//check char by char
						//file does not match
						if (_str[j] != _container[i][j + offset])
						{
							fits = false;
						}
					}
				}
				//char by char comparisation iterates only throught _str; _container[x] could be even longer
				//after the '*' loop offset is already larger then the index
				if (j + offset != _container[i].size()) fits = false;
			}
			//mark string instead of removing him to prevent movement in the container
			if (!fits) _container[i] = "";
		}
	}
}