#ifndef TYPELOG_H
#define TYPELOG_H

#include <string>

#include "Logger.h"


namespace LoggerSettings
{
	// Способ вывода логов
	const Logger::TypeLog TYPE_LOG{ Logger::TypeLog::STDOUT };

}

namespace ConstValue
{
	const std::string NONE{ "none" };

}

#endif // !TYPELOG_H

