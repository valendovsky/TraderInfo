#ifndef LOGSETTINGS_H
#define LOGSETTINGS_H

#include <string>

#include "Logger.h"


// ��������� �����������
namespace LoggerSettings
{
	const std::string LOG_DIR { "../log" };
	const std::string LOG_FILE{ "log.log" };

	// ������� �����������
	const log4cpp::Priority::PriorityLevel  PRIORITY{ log4cpp::Priority::DEBUG };

}

#endif // !LOGSETTINGS_H
