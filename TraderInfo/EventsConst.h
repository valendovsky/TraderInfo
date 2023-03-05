#ifndef EVENTSCONST_H
#define EVENTSCONST_H

#include <string>


struct UserInfo
{
	std::string login;
	// Значения по умолчанию
	bool		auth	= false;
	bool		isAdmin	= false;
};

namespace JsonValue
{
	const std::string COMMAND		{ "command" };
	const std::string AUTH			{ "authorization" };
	const std::string USERNAME		{ "username" };
	const std::string PASSWORD		{ "password" };
	const std::string AUTH_FALSE	{ "false" };
	const std::string ACTIVE_SIGNAL	{ "active" };
	const std::string TICKER		{ "tickerSymbol" };
	const std::string LIMITS		{ "limits" };
	const std::string ADD_SIGNAL	{ "add" };
	const std::string DEL_SIGNAL	{ "delete" };
	const std::string ACTION_SUCCESS{ "success" };
	const std::string ACTION_FAIL	{ "fail" };
	const std::string ACTION_UNKNOWN{ "unknown_command" };

}

namespace DaoSettings
{
	const std::string REDIS_SOCKET	{ "tcp://127.0.0.1:6379" };

}

namespace ServerSettings
{
	const std::string BROADCAST		{ "broadcast" };

}

#endif // !EVENTSCONST_H
