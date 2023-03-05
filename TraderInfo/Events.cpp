#include "Events.h"

#include <string>
#include <string_view>
#include <map>
#include <mutex>
#include <random>
#include <memory>

#include <uwebsockets/App.h>
#include <nlohmann/json.hpp>
#include <uuid.h>

#include "Logger.h"
#include "TypeLog.h"
#include "EventsConst.h"
#include "PerSocketData.h"
#include "Dao.h"


// Мютекс для формирования uuid
std::mutex mtxUuid;

// Инициализация логгера
Logger Events::m_log("Events", LoggerSettings::TYPE_LOG);


// Инициализация Вихря Мерсенна
std::mt19937 Events::getMT()
{
    std::random_device rd;
    auto seed_data = std::array<int, std::mt19937::state_size> {};
    std::generate(std::begin(seed_data), std::end(seed_data), std::ref(rd));
    std::seed_seq seq(std::begin(seed_data), std::end(seed_data));
    std::mt19937 generator(seq);

    return generator;
}

// Возвращает uuid v4
std::string Events::uuid()
{
	std::unique_lock ul(mtxUuid);

    // Инициализация генератора
    static std::mt19937 mTwister(getMT());
    static uuids::uuid_random_generator gen{ mTwister };

    // Генерируем uuid
    uuids::uuid uuidValue = gen();

    return uuids::to_string(uuidValue);
}

// Проводит аутентификацию пользователя и возвращает информацию о нём
std::unique_ptr<UserInfo> Events::checkUser(const std::string& login, 
                                            const std::string& password, 
                                            const std::string& postfixContext)
{
    auto user = std::make_unique<UserInfo>();  // по умолчанию auth = false
    user->login = login;
    
    // Аутентифицируем пользователя
    Dao db(DaoSettings::REDIS_SOCKET);
    user->auth = db.checkPass(login, password, postfixContext);
    if (user->auth)
    {
        m_log.info("The user \"" + login + "\" is authenticated successfully.", 
            m_context + postfixContext, "Events::checkUser " + std::to_string(__LINE__));
        
        // Проверка статуса администратора
        user->isAdmin = db.checkAdminStatus(login, postfixContext);
    }
    else
    {
        m_log.info("Failed user authentication. The username is \"" + login + '"', 
            m_context + postfixContext, "Events::checkUser " + std::to_string(__LINE__));
    }

    return user;
}

// Авторизовывает пользователей на сервере
// PerSocketData* dataOut - параметр вывода
bool Events::userAuth(                      PerSocketData* dataOut, 
                                            const std::string& login, const std::string& password)
{
    // Авторизовываем пользователя
    std::unique_ptr<UserInfo> user = checkUser(login, password, dataOut->userId);
    
    // Пользователь не прошел аутентификацию
    if (!user->auth)
    {
        m_log.info("Failed authorization of the user.", m_context + dataOut->userId, "Events::userAuth " + std::to_string(__LINE__));

        return false;
    }
    
    dataOut->login = user->login;
    dataOut->auth = true;
    dataOut->isAdmin = user->isAdmin;

    m_log.info("The user with username \"" + dataOut->login + "\" is logged in.",
        m_context + dataOut->userId, "Events::userAuth " + std::to_string(__LINE__));
    
    return true;
}

// Отправляет список активных сигналов пользователю и возвращает их количество
int Events::sendSignals(                    uWS::WebSocket<false, true, PerSocketData>* ws, 
                                            const std::string& postfixContext)
{
    // Получаем список всех активных сигналов
    std::map<std::string, std::string> signals;

    Dao db(DaoSettings::REDIS_SOCKET);
    int count = db.getAllSignals(signals, postfixContext);

    // Отправляем все активные сигналы пользователю
    for (const auto& el : signals)
    {
        nlohmann::json response;
        response[JsonValue::COMMAND] = JsonValue::ACTIVE_SIGNAL;
        response[JsonValue::TICKER] = el.first;
        response[JsonValue::LIMITS] = el.second;

        ws->send(response.dump(), uWS::OpCode::TEXT);
    }

    m_log.info(std::to_string(count) + " signals were sent to the user", 
        m_context + postfixContext, "Events::sendSignals " + std::to_string(__LINE__));

    return count;
}

// Авторизовывает пользователя
void Events::authorization(                 uWS::WebSocket<false, true, PerSocketData>* ws, 
                                            const std::string_view message,
                                            const std::string& postfixContext)
{
    PerSocketData* data = ws->getUserData();
    
    // Получаем логин и пароль
    nlohmann::json parsed = nlohmann::json::parse(message);
    if (parsed[JsonValue::COMMAND] != JsonValue::AUTH)
    {
        m_log.info("The user is not authorized", m_context + postfixContext, "Events::authorization " + std::to_string(__LINE__));

        return;
    }
    const std::string login = parsed[JsonValue::USERNAME];
    const std::string password = parsed[JsonValue::PASSWORD];

    
    if (userAuth(data, login, password))
    {
        // Подписываем пользователя на канал с сигналами
        ws->subscribe(ServerSettings::BROADCAST);
        m_log.info("The user \"" + data->login + "\" is subscribed to a channel with signals.", 
            m_context + postfixContext, "Events::authorization " + std::to_string(__LINE__));
    }
    else
    {
        // Неверный логин или пароль
        nlohmann::json response;
        response[JsonValue::AUTH] = JsonValue::AUTH_FALSE;

        ws->send(response.dump(), uWS::OpCode::TEXT);

        return;
    }

    // Отправляем все активные сигналы пользователю
    sendSignals(ws, data->userId);
}

// Добавляет и удаляет сигналы
void Events::signalize(                     uWS::WebSocket<false, true, PerSocketData>* ws, 
                                            const std::string_view message, 
                                            const std::string& postfixContext)
{
    // Получаем тип задачи и информацию по сигналу
    nlohmann::json parsed = nlohmann::json::parse(message);
    std::string command = parsed[JsonValue::COMMAND];
    std::string tickerSymbol = parsed[JsonValue::TICKER];
    std::string limits;

    // Определяем команду и выполняем её
    nlohmann::json response;
    if (command == JsonValue::ADD_SIGNAL)
    {
        // Добавляем сигнал
        limits = parsed[JsonValue::LIMITS];
        
        Dao db(DaoSettings::REDIS_SOCKET);
        response[JsonValue::COMMAND] = (db.setSignal(tickerSymbol, limits, postfixContext) ? JsonValue::ACTION_SUCCESS : JsonValue::ACTION_FAIL);

    }
    else if (command == JsonValue::DEL_SIGNAL)
    {
        // Удаляем сигнал
        Dao db(DaoSettings::REDIS_SOCKET);
        response[JsonValue::COMMAND] = (db.delSignal(tickerSymbol, postfixContext) ? JsonValue::ACTION_SUCCESS : JsonValue::ACTION_FAIL);
    }
    else
    {
        // Неизвестная команда
        m_log.warn("Unknown command from the user.", m_context + postfixContext, "Events::signalize " + std::to_string(__LINE__));
        response[JsonValue::COMMAND] = JsonValue::ACTION_UNKNOWN;
    }

    // Сообщение пользователю
    ws->send(response.dump(), uWS::OpCode::TEXT);

    // В случае успеха изменений отправляем сообщение в общий чат
    if (response[JsonValue::COMMAND] == JsonValue::ACTION_SUCCESS)
    {
        nlohmann::json resBroadcast;
        resBroadcast[JsonValue::COMMAND] = command;
        resBroadcast[JsonValue::TICKER] = tickerSymbol;
        if (limits != "")
        {
            resBroadcast[JsonValue::LIMITS] = limits;
        }

        ws->publish(ServerSettings::BROADCAST, resBroadcast.dump());

        m_log.info("A new signal is published.", m_context + postfixContext, "Events::signalize " + std::to_string(__LINE__));
    }
}
