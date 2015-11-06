#include "utils.h"
#include <vector>
#include <algorithm>

namespace utils {

	std::string strInterval(const std::string& _str, size_t _begin, size_t _end, size_t _count, char _del0, char _del1)
	{
		int lnCount = 0;
		int begin = _begin;
		for (int i = begin; i >= 0; --i)
		{
			if (_str[i] == _del0)
			{
				lnCount++;
				if (lnCount >= _count)
				{
					begin = i;
					break;
				}
			}
		}
		lnCount = 0;
		int end = _end;
		for (int i = end; i < _str.size(); ++i)
		{
			if (_str[i] == _del1)
			{
				lnCount++;
				if (lnCount >= _count)
				{
					end = i;
					break;
				}
			}
		}

		return std::string(_str.begin() + begin, _str.begin() + end + 1);
	}

	// **************************************** //
	void toLower(std::string& _str)
	{
		std::transform(_str.begin(), _str.end(), _str.begin(), ::tolower);
	}
}
