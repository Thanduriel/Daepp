#include <vector>

namespace utils{

	/* ReferenceContainer
	 * A container operating with pointers
	 * while providing reference like access
	 */

	template < typename _T >
	class ReferenceContainer
	{
	public:
		ReferenceContainer() {};

		ReferenceContainer(std::initializer_list< _T* >& _init)
		{
			for (auto* ty : _init)
				m_elem.push_back(ty);
		};

		void push_back(_T& _el)
		{
			m_elem.push_back(&_el);
		}

		//random access
		_T& operator[](size_t _i)
		{
			return *m_elem[_i];
		};

		//count of elements
		inline size_t size()
		{
			return m_elem.size();
		}

		_T& back()
		{
			return *m_elem.back();
		}
	private:
		std::vector < _T* > m_elem;
	};
}