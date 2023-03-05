// TraderInfo.cpp : Сервер работает по протоколу WebSocket в многопоточном режиме
// Принимает данные от администраторов и рассылает их по пользователям
// Данные передаются в формате JSON
// Для хранения данных используется БД Redis
//

/*
* Схема сообщений для администратора
* { "command": "authorization", "username": "admin", "password": "12345678" }
* { "command": "add", "tickerSymbol" : "USD", "limits" : "Full amount" }
* { "command": "delete", "tickerSymbol" : "USD" }
*/

#include <iostream>
#include <string>
#include <string_view>
#include <vector>
#include <map>
#include <thread>
#include <algorithm>
#include <sstream>

#include <uwebsockets/App.h>
#include <nlohmann/json.hpp>

#include "Logger.h"
#include "TypeLog.h"
#include "PerSocketData.h"
#include "Events.h"
#include "Constants.h"


// Инициализация логгера
static Logger s_log("Main", LoggerSettings::TYPE_LOG);


// Получает настройки для uWS
static SettingsUWS getSettingsUWS()
{
	// Получаем контекст
	std::string context{ s_log.getContext() };
	s_log.info("Getting the settings for the application.", context, "getSettingsUWS " + std::to_string(__LINE__));

	SettingsUWS settings;
	// Порт сервера
	settings.port = ServerSettings::PORT;
	// Количество используемых потоков
	settings.threads = std::thread::hardware_concurrency();

	return settings;
}


int main()
{
	// Получаем контекст для логгера
	std::string context{ s_log.getContext() };
	s_log.info("Start websocket server!", context, std::to_string(__LINE__));


	// Получаем настройки для работы сервера
	SettingsUWS uWsSettings = getSettingsUWS();
	// Порт websocket'a
	s_log.info("The port: " + std::to_string(uWsSettings.port), context, std::to_string(__LINE__));


	// Задаём количество потоков для работы
	std::vector<std::thread*> threads(uWsSettings.threads);
	s_log.info("Threads num: " + std::to_string(uWsSettings.threads), context, std::to_string(__LINE__));

	// Инициализация потоков
	std::transform(threads.begin(), threads.end(), threads.begin(), [&uWsSettings](std::thread*/*t*/)
		{
			return new std::thread([&uWsSettings]()
				{
					// Контекст для данного потока
					std::string thContext{ s_log.getContext() + " "};

					// Запуск WebSocket сервера
					uWS::App().ws<PerSocketData>("/*",
						{
							// Настройки сервера
							.compression = uWS::CompressOptions(uWS::DEDICATED_COMPRESSOR_4KB | uWS::DEDICATED_DECOMPRESSOR),
							.maxPayloadLength = 100 * 1024 * 1024,
							.idleTimeout = 16,
							.maxBackpressure = 100 * 1024 * 1024,
							.closeOnBackpressureLimit = false,
							.resetIdleTimeoutOnSend = false,
							.sendPingsAutomatically = true,
							// Хендлеры сервера
							.upgrade = nullptr,
							.open = [&thContext](auto* ws)
							{
								// Создание соединения
								s_log.info("Processing a new connection.", thContext, ".open " + std::to_string(__LINE__));
								
								// Данные пользователя
								PerSocketData* data = ws->getUserData();
								
								// Назначаем ИН пользователю
								Events event;
								data->userId = event.uuid();
								data->login = ConstValue::NONE;

								// Подписываем пользователя на персональный канал
								ws->subscribe(ServerSettings::PREFIX_CHANNEL + data->userId);

								s_log.info("New user connected.", thContext + data->userId, ".open " + std::to_string(__LINE__));
						    },
						    .message = [&thContext](auto* ws, std::string_view message, uWS::OpCode opCode)
						    {
								// Обработка события
								PerSocketData* data = ws->getUserData();
								s_log.info("The event from the user.", thContext + data->userId, ".message " + std::to_string(__LINE__));

								Events event;
								try
								{
									if (data->auth)
									{
										// Пользователь уже авторизован
										if (data->isAdmin)
										{
											// Команда по изменению списка активных сигналов
											s_log.info("Changing signals.", thContext + data->userId, ".message " + std::to_string(__LINE__));
											event.signalize(ws, message, data->userId);
										}
										else
										{
											s_log.info("The user does not have the right to publish signals.", 
												thContext + data->userId, ".message " + std::to_string(__LINE__));
										}
									}
									else
									{
										// Авторизация пользователя
										s_log.info("User authorization.", thContext + data->userId, ".message " + std::to_string(__LINE__));
										event.authorization(ws, message, data->userId);
									}
								}
								catch (const std::exception& exp)
								{
									s_log.error("Standard exception: " + std::string(exp.what()), 
										thContext + data->userId, ".message " + std::to_string(__LINE__));
								}
						    },
						    .ping = [](auto*/*ws*/, std::string_view)
						    {
							    // PING
						    },
						    .pong = [](auto*/*ws*/, std::string_view)
						    {
							    // PONG
						    },
						    .close = [](auto*/*ws*/, int /*code*/, std::string_view /*message*/)
						    {
							    // Закрытие соединение
						    }
						}
					).listen(uWsSettings.port, [&uWsSettings, &thContext](auto* listen_socket)
						{
							if (listen_socket)
							{
								s_log.info("Listening on port " + std::to_string(uWsSettings.port), 
									thContext, ".listen " + std::to_string(__LINE__));
							}
							else
							{
								s_log.crit("Failed to listen on port " + std::to_string(uWsSettings.port), 
									thContext, ".listen " + std::to_string(__LINE__));
							}
						}
					).run();

				}
			);
		}
	);

	// Ожидание закрытия потоков
	std::for_each(threads.begin(), threads.end(), [](std::thread* t) 
		{
			t->join();
		}
	);
	s_log.info("Threads closed.", context, std::to_string(__LINE__));

	return 0;
}
