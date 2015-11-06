#include <string>

namespace par{
	// All configurable parser settings.
	// For detailed information about there usage and default values look in the readme.
	struct ParserConfig
	{
		//config
		std::string m_sourceDir;
		bool m_caseSensitive;
		bool m_alwaysSemikolon;
		bool m_saveInOrder;
		bool m_showCodeSegmentOnError;
	};
}