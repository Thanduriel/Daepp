#pragma once

#include <vector>
#include <deque>
#include <initializer_list>

namespace game{



template < typename _T >
class SymbolTable
{
public:
	/* constructor() **************************
	 * fast one
	 */
	SymbolTable(){};
	/* takes an initializer_list to init with its values
	 */
	SymbolTable(std::initializer_list< _T >& _init): m_elem(_init){};
	~SymbolTable(){};
	
	/* add() ************************
	 * Tries to add an element to the table
	 * returns false when one with the same name already exists
	 */
	template< typename _Name, typename... _ArgsR >
	bool emplace(_Name _name, _ArgsR&&... _argsR)
	{
		for (auto& _el : m_elem)
			if (_el.name == _name)
				return false;
		m_elem.emplace_back(_name, std::forward< _ArgsR >(_argsR)...);
		return true;
	};

	bool add(_T& _sym)
	{
		for (auto& el : m_elem)
			if (el.name == _sym.name)
				return false;

		//try using the move constructor
		m_elem.push_back(std::move(_sym));

		return true;
	};

	void erase(size_t _i)
	{
		m_elem.erase(m_elem.begin() + _i);
	}

	/* find() ***********************
	 * @param _name name of the element
	 * @return index of the element
	 *		-1 when not found
	 */
	int find(const std::string& _name)
	{
		for (int i = 0; i < m_elem.size(); ++i)
			if (m_elem[i].name == _name) return i;

		return -1;
	};

	_T& operator[](size_t _i)
	{
		return m_elem[_i];
	};

	inline size_t size()
	{
		return m_elem.size();
	}

	_T& back()
	{
		return m_elem.back();
	}
private:
	std::deque< _T > m_elem;
};

}//end namespace