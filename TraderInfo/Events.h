#ifndef EVENTS_H
#define EVENTS_H

#include <string>
#include <string_view>
#include <map>
#include <memory>
#include <random>
#include <exception>

#include <uwebsockets/App.h>

#include "Logger.h"
#include "EventsConst.h"
#include "PerSocketData.h"


// ѕровер€ет авторизацию пользователей
class Events
{
private:
	static Logger	m_log;

	std::string		m_context;


	std::mt19937 getMT();
	std::unique_ptr<UserInfo> checkUser(const std::string& login, 
										const std::string& password, 
										const std::string& postfixContext);
	
	bool userAuth(						PerSocketData* data, 
										const std::string& login, 
										const std::string& password);
	
	int sendSignals(					uWS::WebSocket<false, true, PerSocketData>* ws, 
										const std::string& postfixContext);
	

public:
	Events() try
	{
		m_context = m_log.getContext() + " ";
	}
	catch (const std::exception& ex)
	{
		m_log.crit("Standard error: " + std::string(ex.what()), m_log.getContext(), "Auth_constructor h" + std::to_string(__LINE__));
	}

	std::string uuid();
	
	void authorization(	uWS::WebSocket<false, true, PerSocketData>* ws, 
						const std::string_view message,
						const std::string& postfixContext);
	
	void signalize(		uWS::WebSocket<false, true, PerSocketData>* ws, 
						const std::string_view message, 
						const std::string& postfixContext);

};

#endif // !EVENTS_H
