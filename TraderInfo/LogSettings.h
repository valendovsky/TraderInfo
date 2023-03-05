#ifndef LOGSETTINGS_H
#define LOGSETTINGS_H

#include <string>

#include "Logger.h"


// Настройки логирования
namespace LoggerSettings
{
	const std::string LOG_DIR { "../log" };
	const std::string LOG_FILE{ "log.log" };

	// Уровень логирования
	const log4cpp::Priority::PriorityLevel  PRIORITY{ log4cpp::Priority::DEBUG };

}

#endif // !LOGSETTINGS_H
