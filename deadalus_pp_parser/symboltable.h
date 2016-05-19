#pragma once

#include <vector>
#include <deque>
#include <initializer_list>
#include <memory>

namespace utils{


	/* SymbolTable *******************************************
	 * A container managing symbols with unique names.
	 */
template < typename _T >
class SymbolTable
{
public:
	/* SymbolTable() **************************
	 * fast constructor
	 */
	SymbolTable() : slowMode(false){};
	/* takes an initializer_list to init with its values
	 */
	SymbolTable(std::initializer_list< _T* >& _init)
		:slowMode(false)
	{
		for (auto* ty : _init)
			m_elem.emplace_back(ty);
	};
	~SymbolTable(){};
	
	/* add() ************************
	 * Tries to add an element to the table
	 * returns false when one with the same name already exists
	 */
	template< typename _Name, typename... _ArgsR >
	bool emplace(_Name& _name, _ArgsR&&... _argsR)
	{
		for (auto& name : m_names)
			if (name == _name)
				return false;
		m_elem.emplace_back(new _T(_name, std::forward< _ArgsR >(_argsR)...));
		m_names.push_back(_name);
		return true;
	};

	bool add(_T* _sym)
	{
		for (auto& name : m_names)
			if (name == _sym->name)
				return false;

		m_elem.emplace_back(_sym);
		m_names.push_back(_sym->name);

		return true;
	};

	void reserve(size_t _size)
	{
		m_elem.reserve(_size);
		m_names.reserve(_size);
	}


	void erase(size_t _i)
	{
		m_elem.erase(m_elem.begin() + _i);
	}

	/* insert() ************************************************
	 * Moves another container's content into this one after _begin.
	 * _source is empty afterwards and this symbolTable will switch to slowMode.
	 */
	template < typename _Container >
	void insert(typename std::vector< std::unique_ptr< _T > >::iterator _begin, _Container& _source)
	{
		m_elem.insert(_begin + 1, std::make_move_iterator(_source.begin()), std::make_move_iterator(_source.end()));
		slowMode = true;
	}

	//insert a complete container
	template < typename _Container >
	void insert(_Container& _source)
	{
		m_elem.insert(m_elem.end(), std::make_move_iterator(_source.begin()), std::make_move_iterator(_source.end()));
		slowMode = true;
	}

	typename std::vector< std::unique_ptr< _T > >::iterator begin() { return m_elem.begin(); };
	typename std::vector< std::unique_ptr< _T > >::iterator end() { return m_elem.end(); };

	/* find() ***********************
	 * @param _name name of the element
	 * @return index of the element
	 *		-1 when not found
	 */
	int find(const std::string& _name)
	{
		if (!slowMode)
		{
			for (int i = 0; i < m_elem.size(); ++i)
				if (m_names[i] == _name) return i;
		}
		else
		{
			for (int i = 0; i < (int)m_elem.size(); ++i)
				if (m_elem[i]->name == _name) return i;
		}

		return -1;
	};

	/* nameNotInUse() ***************
	 * checks whether a symbol with this name already exists
	 * without adding it to the table
	 * Use this to verify vars in local namespaces
	 * @return true if not found
	 */
	bool nameNotInUse(const std::string& _name)
	{
		return -1 == find(_name);
	}

	//direct access
	_T& operator[](size_t _i)
	{
		return *m_elem[_i];
	};

	//count of elements
	inline size_t size()
	{
		return m_elem.size();
	}

	// specific element access
	_T& back()
	{
		return *m_elem.back();
	}
private:
	std::vector< std::unique_ptr< _T > > m_elem;
	std::vector < std::string > m_names; //todo test vs no extra string table

	bool slowMode; // lookup in m_names is invalid
};

}//end namespace