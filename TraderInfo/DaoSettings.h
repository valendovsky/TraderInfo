#ifndef DAOSETTINGS_H
#define DAOSETTINGS_H

#include <string>


namespace DaoSettings 
{
	const std::string	USERS_DB	{ "users" };
	const std::string	ADMINS_DB	{ "admins" };
	const std::string	SIGNALS_DB	{ "signals" };

	const size_t		HASH_LEN	(32U);
	const size_t		MIN_SALT_LEN(8U);

}

#endif // !DAOSETTINGS_H
