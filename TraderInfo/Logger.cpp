#include "Logger.h"

#include <iostream>
#include <string>
#include <filesystem>
#include <sstream>
#include <thread>

#include <log4cpp/Appender.hh>
#include <log4cpp/FileAppender.hh>
#include <log4cpp/OstreamAppender.hh>
#include <log4cpp/Category.hh>
#include <log4cpp/NDC.hh>


// Инициализация логгера для вывода в консоль
void Logger::appenderInit(	std::ostream* outStream)
{
	m_appender = new log4cpp::OstreamAppender("console", outStream);
}

// Инициализация логгера для вывода в файл
void Logger::appenderInit(	const std::string& dirName, 
							const std::string& fileName)
{
	// Проверка существования директории для логирования
	if (!std::filesystem::exists(dirName))
	{
		std::filesystem::create_directories(dirName);
	}

	m_appender = new log4cpp::FileAppender("FileAppender", dirName + "/" + fileName);
}


// Генератор контекста на базе номеров исполняемых потоков
std::string Logger::getContext()
{
	// Формируем сообщение контекста
	std::ostringstream oss;
	oss << std::this_thread::get_id();
	
	return "Thread_" + oss.str();
}


// Методы логирования по уровням
void Logger::publish(void (log4cpp::Category::*ptr)(const std::string&), 
					const std::string& message, 
					const std::string& context1, 
					const std::string& context2)
{
	if (context1 != "")
	{
		// Задаём первый контекст
		log4cpp::NDC::push(context1);

		if (context2 != "")
		{
			// Задаём второй контекст
			log4cpp::NDC::push(context2);

			(this->m_category->*ptr)(message);

			// Удаляем второй контекст
			log4cpp::NDC::pop();
		}
		else
		{
			(this->m_category->*ptr)(message);
		}

		// Удаляем первый контекст
		log4cpp::NDC::pop();
	}
	else
	{
		(this->m_category->*ptr)(message);
	}
}

void Logger::alert(	const std::string& message, 
					const std::string& context1, 
					const std::string& context2)
{
	publish(&log4cpp::Category::alert, message, context1, context2);
}

void Logger::crit(	const std::string& message, 
					const std::string& context1, 
					const std::string& context2)
{
	publish(&log4cpp::Category::crit, message, context1, context2);
}

void Logger::error(	const std::string& message, 
					const std::string& context1, 
					const std::string& context2)
{
	publish(&log4cpp::Category::error, message, context1, context2);
}

void Logger::warn(	const std::string& message, 
					const std::string& context1, 
					const std::string& context2)
{
	publish(&log4cpp::Category::warn, message, context1, context2);
}

void Logger::notice(const std::string& message, 
					const std::string& context1, 
					const std::string& context2)
{
	publish(&log4cpp::Category::notice, message, context1, context2);
}

void Logger::info(	const std::string& message, 
					const std::string& context1, 
					const std::string& context2)
{
	publish(&log4cpp::Category::info, message, context1, context2);
}

void Logger::debug(	const std::string& message, 
					const std::string& context1, 
					const std::string& context2)
{
	publish(&log4cpp::Category::debug, message, context1, context2);
}
