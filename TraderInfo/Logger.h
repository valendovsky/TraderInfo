#ifndef LOGGER_H
#define LOGGER_H

#include <iostream>
#include <string>

#include <log4cpp/Appender.hh>
#include <log4cpp/FileAppender.hh>
#include <log4cpp/OstreamAppender.hh>
#include <log4cpp/Layout.hh>
#include <log4cpp/BasicLayout.hh>
#include <log4cpp/Category.hh>
#include <log4cpp/Priority.hh>

#include "LogSettings.h"


// Класс логирования работы приложения
class Logger
{
public:
	enum TypeLog
	{
		FILE	= 0,
		STDERR	= 1,
		STDOUT	= 2,
		STDLOG	= 3
	};


private:
	log4cpp::Appender*	m_appender;
	log4cpp::Layout*	m_logLayout;
	log4cpp::Category*	m_category;

	void appenderInit(	std::ostream* outStream);
	
	void appenderInit(	const std::string& dirName, 
						const std::string& fileName);
	
	void publish(		void (log4cpp::Category::*ptr)(const std::string&), 
						const std::string& message, 
						const std::string& context1 = "", 
						const std::string& context2 = "");


public:
	Logger(				const std::string& category, 
						TypeLog type = TypeLog::FILE, 
						const std::string& fileName = LoggerSettings::LOG_FILE)
	{
		// Способ сохранения логов
		switch (type)
		{
		case TypeLog::FILE:
			appenderInit(LoggerSettings::LOG_DIR, fileName);
			break;
		case TypeLog::STDERR:
			appenderInit(&std::cerr);
			break;
		case TypeLog::STDOUT:
			appenderInit(&std::cout);
			break;
		case TypeLog::STDLOG:
			appenderInit(&std::clog);
			break;
		default:
			// Если что-то пошло не так
			appenderInit("../log", "log.log");
			break;
		}

		// Определяем раскладку
		m_logLayout = new log4cpp::BasicLayout();
		m_appender->setLayout(m_logLayout);

		// Создаём категорию
		m_category = &log4cpp::Category::getInstance(category);
		m_category->addAppender(m_appender);
		m_category->setPriority(LoggerSettings::PRIORITY);
	}

	~Logger()
	{
		// Очистка всех Appender'ов
		log4cpp::Category::shutdown();
	}

	// Генератор контекста
	std::string getContext();

	// Методы логирования
	void alert(	const std::string& message, 
				const std::string& context1 = "", 
				const std::string& context2 = "");
	
	void crit(	const std::string& message, 
				const std::string& context1 = "", 
				const std::string& context2 = "");
	
	void error(	const std::string& message, 
				const std::string& context1 = "", 
				const std::string& context2 = "");
	
	void warn(	const std::string& message, 
				const std::string& context1 = "", 
				const std::string& context2 = "");
	
	void notice(const std::string& message, 
				const std::string& context1 = "", 
				const std::string& context2 = "");
	
	void info(	const std::string& message, 
				const std::string& context1 = "", 
				const std::string& context2 = "");
	
	void debug(	const std::string& message, 
				const std::string& context1 = "", 
				const std::string& context2 = "");

};

#endif // !LOGGER_H
