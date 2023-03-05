#ifndef DAO_H
#define DAO_H

#include <string>
#include <map>

#include <sw/redis++/redis++.h>

#include "Logger.h"


// Data Access Object - Объект доступа к данным в БД Redis
class Dao
{
private:
	static Logger		m_log;

	sw::redis::Redis	m_redis;
	std::string			m_context;


	bool hCheck(			const std::string& db, 
							const std::string& key,
							const std::string& postfixContext);
	
	bool hSet(				const std::string& db, 
							const std::string& key, 
							const std::string& value,
							const std::string& postfixContext);
	
	bool hDel(				const std::string& db, 
							const std::string& key,
							const std::string& postfixContext);
	
	std::string hGet(		const std::string& db, 
							const std::string& key,
							const std::string& postfixContext);
	
	int hGetAll(			const std::string& db, 
							std::map<std::string, 
							std::string>& output,
							const std::string& postfixContext);
	
	bool sCheck(			const std::string& db, 
							const std::string& value,
							const std::string& postfixContext);
	
	std::string encodeArgon2(const std::string& password, 
							const std::string& saltStr,
							const std::string& postfixContext);


public:
	Dao(const std::string& redisSocket) try : m_redis(sw::redis::Redis(redisSocket))
	{
		m_context = m_log.getContext() + " ";
	}
	catch (const sw::redis::Error& err)
	{
		m_log.crit("Standard Redis error: " + std::string(err.what()), m_log.getContext(), "Dao_constructor h" + std::to_string(__LINE__));
	}


	bool checkPass(			const std::string& login, 
							const std::string& password,
							const std::string& postfixContext);
	
	bool checkAdminStatus(	const std::string& login,
							const std::string& postfixContext);
	
	bool setSignal(			const std::string& tickerSymbol, 
							const std::string& limits,
							const std::string& postfixContext);
	
	bool delSignal(			const std::string& tickerSymbol,
							const std::string& postfixContext);
	
	std::string getSignal(	const std::string& tickerSymbol,
							const std::string& postfixContext);
	
	int getAllSignals(		std::map<std::string, std::string>& signals,
							const std::string& postfixContext);

};

#endif // !DAO_H
