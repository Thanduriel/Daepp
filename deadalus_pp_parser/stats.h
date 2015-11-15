#include <vector>
#include <utility>

namespace utils{

	//type of the data to analyse
	template < typename _T>
	class StatCalculator
	{
	public:
		//_epsilon the size of an interval in which data is considered equal.
		StatCalculator(_T _epsilon) : m_epsilon(_epsilon){ expand(10); };

		~StatCalculator()
		{
			//write stats into a file
			std::ofstream fileStream("stats.txt", std::ios::out);

			for (auto& pair : m_data)
				fileStream << pair.first << ';' << pair.second << std::endl;
		};

		void inc(_T _el)
		{
			for (int i = 0;; ++i)
			{
				if (i == (int)m_data.size()) expand(max(16, i - (int)m_data.size()));

				if (m_data[i].first > _el)
				{
					m_data[i - 1].second++;
					return;
				}
			}
		};

	private:
		//expand the array by _size elements
		void expand(size_t _size)
		{
			int i = (int)m_data.size();
			m_data.resize(i + _size);

			for (; i < m_data.size(); ++i)
			{
				m_data[i].first = i * m_epsilon;
				m_data[i].second = 0;
			}
		};

		_T m_epsilon;
		std::vector < std::pair < _T, unsigned int > > m_data;
	};
}