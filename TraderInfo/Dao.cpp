#include "Dao.h"

#include <iomanip>
#include <string>
#include <map>
#include <iterator>
#include <sstream>
#include <algorithm>

#include <sw/redis++/redis++.h>
#include <argon2.h>

#include "Logger.h"
#include "TypeLog.h"
#include "DaoSettings.h"


// Инициализация логгера
Logger Dao::m_log("Dao", LoggerSettings::TYPE_LOG);


// Проверяет есть ли позиция в Redis
bool Dao::hCheck(				const std::string& db, 
								const std::string& key,
								const std::string& postfixContext)
{
	m_log.info("Check the key \"" + key + "\" into db \"" + db + '"', 
		m_context + postfixContext, "Dao::hCheck " + std::to_string(__LINE__));

	return m_redis.hexists(db, key);
}

// Создат пару ключ-значение в указанной хеш-тиблице и возвращает значение о создании или изменении
bool Dao::hSet(					const std::string& db, 
								const std::string& key, 
								const std::string& value,
								const std::string& postfixContext)
{
	m_log.info("Set value for key \"" + key + "\" from DB \"" + db + '"', 
		m_context + postfixContext, "Dao::hSet " + std::to_string(__LINE__));

	return m_redis.hset(db, key, value);
}

// Удаляет из указанной хеш-талицы значение по ключу и сообщает об успешности процесса
bool Dao::hDel(					const std::string& db, 
								const std::string& key,
								const std::string& postfixContext)
{
	m_log.info("Delete value for key \"" + key + "\" from DB \"" + db + '"', 
		m_context + postfixContext, "Dao::hDel " + std::to_string(__LINE__));

	return m_redis.hdel(db, key);
}

// Возвращает значение из указанной БД по предоставленному ключу
std::string Dao::hGet(			const std::string& db, 
								const std::string& key,
								const std::string& postfixContext)
{
	m_log.info("Get value for key \"" + key + "\" from DB \"" + db + '"', 
		m_context + postfixContext, "Dao::hGet " + std::to_string(__LINE__));
	std::string value = *m_redis.hget(db, key);

	return (value == "") ? ConstValue::NONE : value;
}

// Возвращает все значения из хеш-таблицы в Redis через параметры вывода и количество пар через значение возврата
// std::map<std::string, std::string>& output - параметры вывода
int Dao::hGetAll(				const std::string& db, 
								std::map<std::string, 
								std::string>& output,
								const std::string& postfixContext)
{
	// Проверяем есть ли значения в хеш-таблице
	int count = m_redis.hlen(DaoSettings::SIGNALS_DB);
	
	if (count)
	{
		m_redis.hscan(DaoSettings::SIGNALS_DB, 0, std::inserter(output, output.begin()));
	}

	m_log.info("The database \"" + db + "\" contains " + std::to_string(count) + " object(s).", 
		m_context + postfixContext, "Dao::hGetAll " + std::to_string(__LINE__));

	return count;
}


// Проверяет множество на наличие элемента
bool Dao::sCheck(				const std::string& db, 
								const std::string& value,
								const std::string& postfixContext)
{
	m_log.info("Check the value \"" + value + "\" into the Set of the db \"" + db + '"', 
		m_context + postfixContext, "Dao::sCheck " + std::to_string(__LINE__));

	return m_redis.sismember(db, value);
}


// Возвращает хеш Argon2
std::string Dao::encodeArgon2(	const std::string& password, 
								const std::string& saltStr,
								const std::string& postfixContext)
{
	// Хеш пароля
	uint8_t hash[DaoSettings::HASH_LEN];

	// Соль для пароля
	size_t saltLen = saltStr.length();
	if (saltLen < DaoSettings::MIN_SALT_LEN)
	{
		// Соль короче допустимого значения
		m_log.warn("The salt: \"" + saltStr + "\" is less than the permissible value.", 
			m_context + postfixContext, "Dao::encodeArgon2 " + std::to_string(__LINE__));

		return ConstValue::NONE;
	}
	uint8_t* saltBin = new uint8_t[saltLen];
	// Конвертируем строку в бинарные данные
	std::copy(saltStr.begin(), saltStr.end(), saltBin);

#pragma warning(suppress : 4996)  // для VS
	uint8_t* pwd = (uint8_t*)strdup(password.c_str());
	uint32_t pwdlen = strlen((char*)password.c_str());

	uint32_t t_cost = 2;            // 2-pass computation
	uint32_t m_cost = (1 << 10);    // 64 mebibytes memory usage
	uint32_t parallelism = 1;       // number of threads and lanes

	// high-level API
	argon2i_hash_raw(t_cost, m_cost, parallelism, pwd, pwdlen, saltBin, saltLen, hash, DaoSettings::HASH_LEN);

	free(pwd);
	delete[] saltBin;

	// Конвертируем хеш в строку
	std::ostringstream oss;
	oss << std::hex;
	for (int index = 0; index < DaoSettings::HASH_LEN; ++index)
	{
		oss << std::setfill('0') << std::setw(sizeof(uint8_t) * 2) << static_cast<int>(hash[index]);
	}

	m_log.info("The password encoded.", m_context + postfixContext, "Dao::encodeArgon2 " + std::to_string(__LINE__));

	return oss.str();
}


// Проверяет правильность пары логин пароль
bool Dao::checkPass(			const std::string& login, 
								const std::string& password,
								const std::string& postfixContext)
{
	try
	{
		// Проверяем наличия логина в БД пользователей
		if (hCheck(DaoSettings::USERS_DB, login, postfixContext))
		{
			m_log.info("Checking the username \"" + login + "\" is successful.", 
				m_context + postfixContext, "Dao::checkPass " + std::to_string(__LINE__));

			// Получаем хеш пароля и соль к нему ("hash:salt")
			std::string passSalt = hGet(DaoSettings::USERS_DB, login, postfixContext);
			if (passSalt == ConstValue::NONE)
			{
				// Не удалось получить занчени
				m_log.warn("Failed get a password from DB.", m_context + postfixContext, "Dao::checkPass " + std::to_string(__LINE__));

				return false;
			}

			// Выделяем хеш и соль
			unsigned int delimiter = passSalt.find(':');
			if (delimiter == std::string::npos)
			{
				// Делитель не найден, неверное значение passSalt
				m_log.warn("Invalid password hash and salt in DB for username \"" + login + '\"', 
					m_context + postfixContext, "Dao::checkPass " + std::to_string(__LINE__));

				return false;
			}
			std::string passHash(passSalt.begin(), passSalt.begin() + delimiter);
			std::string salt(passSalt.begin() + delimiter + 1, passSalt.end());

			// Получаем хеш тестируемого пароля
			std::string testHash = encodeArgon2(password, salt, postfixContext);
			if (testHash == ConstValue::NONE)
			{
				// Не удалось получить хеш
				m_log.warn("Failed encoded the password hash for username \"" + login + '\"', 
					m_context + postfixContext, "Dao::checkPass " + std::to_string(__LINE__));

				return false;
			}

			return passHash == testHash;
		}
		m_log.info("Invalid username \"" + login + '"', m_context + postfixContext, "Dao::checkPass " + std::to_string(__LINE__));
	}
	catch (const sw::redis::Error& err)
	{
		m_log.error("Standard Redis error: " + std::string(err.what()), 
			m_context + postfixContext, "Dao::checkPass " + std::to_string(__LINE__));
	}

	return false;
}

// Проверка статуса админа для пользователя
bool Dao::checkAdminStatus(		const std::string& login,
								const std::string& postfixContext)
{
	bool status = sCheck(DaoSettings::ADMINS_DB, login, postfixContext);
	m_log.info("The admin status of the username \"" + login + "\" is " + (status ? "true" : "false"), 
		m_context + postfixContext, "Dao::checkAdminStatus " + std::to_string(__LINE__));

	return status;
}

// Создаёт сигнал в БД
bool Dao::setSignal(			const std::string& tickerSybmol, 
								const std::string& limits,
								const std::string& postfixContext)
{
	try
	{
		if (hSet(DaoSettings::SIGNALS_DB, tickerSybmol, limits, postfixContext))
		{
			m_log.info("Created the new signal for the ticker \"" + tickerSybmol + '"', 
				m_context + postfixContext, "Dao::setSignal " + std::to_string(__LINE__));
		}
		else
		{
			m_log.info("Upgraded the signal for the ticker \"" + tickerSybmol + '"', 
				m_context + postfixContext, "Dao::setSignal " + std::to_string(__LINE__));
		}

		return true;
	}
	catch (const sw::redis::Error& err)
	{
		m_log.error("Standard Redis error: " + std::string(err.what()), 
			m_context + postfixContext, "Dao::setSignal " + std::to_string(__LINE__));
	}

	return false;
}

// Удаляет сигнал из БД
bool Dao::delSignal(			const std::string& tickerSymbol,
								const std::string& postfixContext)
{
	try
	{
		if (hDel(DaoSettings::SIGNALS_DB, tickerSymbol, postfixContext))
		{
			m_log.info("The signal with the ticker \"" + tickerSymbol + "\" is removed.", 
				m_context + postfixContext, "Dao::setSignal " + std::to_string(__LINE__));

			return true;
		}
		else
		{
			m_log.info("The signal with the ticker \"" + tickerSymbol + "\" is not removed.", 
				m_context + postfixContext, "Dao::setSignal " + std::to_string(__LINE__));
		}
	}
	catch (const sw::redis::Error& err)
	{
		m_log.error("Standard Redis error: " + std::string(err.what()), 
			m_context + postfixContext, "Dao::setSignal " + std::to_string(__LINE__));
	}

	return false;
}

// Возвращает информацию о сигнале
std::string Dao::getSignal(		const std::string& tickerSymbol,
								const std::string& postfixContext)
{
	std::string result{ ConstValue::NONE };
	try
	{
		// Проверяем наличия сигнала в БД
		if (hCheck(DaoSettings::SIGNALS_DB, tickerSymbol, postfixContext))
		{
			m_log.info("Checking the ticker symbol \"" + tickerSymbol + "\" is successful.", 
				m_context + postfixContext, "Dao::getSignal " + std::to_string(__LINE__));

			// Получаем значение сигнала
			result = hGet(DaoSettings::SIGNALS_DB, tickerSymbol, postfixContext);
		}
		else
		{
			// Тикер в БД не найден
			m_log.info("Invalid ticker symbol \"" + tickerSymbol + '"', 
				m_context + postfixContext, "Dao::getSignal " + std::to_string(__LINE__));
		}
	}
	catch (const sw::redis::Error& err)
	{
		m_log.error("Standard Redis error: " + std::string(err.what()), 
			m_context + postfixContext, "Dao::getSignal " + std::to_string(__LINE__));
	}

	return result;
}

// Возвращает список всех сигналов через параметр вывода и количество сигналов через возвращаемое значение
// std::map<std::string, std::string>& signals - параметры вывода
int Dao::getAllSignals(			std::map<std::string, std::string>& signals,
								const std::string& postfixContext)
{
	m_log.info("Get all keys and values of signals",  m_context + postfixContext, "Dao::getAllSignals " + std::to_string(__LINE__));

	try
	{
		return hGetAll(DaoSettings::SIGNALS_DB, signals, postfixContext);
	}
	catch (const sw::redis::Error& err)
	{
		m_log.error("Standard Redis error: " + std::string(err.what()), 
			m_context + postfixContext, "Dao::getAllSignals " + std::to_string(__LINE__));
	}

	return 0;
}
