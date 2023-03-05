#ifndef CONSTANTS_H
#define CONSTANTS_H

#include <string>


struct SettingsUWS
{
	unsigned int port;
	unsigned int threads;
};

namespace ServerSettings
{
	const unsigned int	PORT(9001);
	const std::string	PREFIX_CHANNEL{ "user_" };

}

#endif // !CONSTANTS_H
