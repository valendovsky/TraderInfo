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


// ������ ��� ������������ uuid
std::mutex mtxUuid;

// ������������� �������
Logger Events::m_log("Events", LoggerSettings::TYPE_LOG);


// ������������� ����� ��������
std::mt19937 Events::getMT()
{
    std::random_device rd;
    auto seed_data = std::array<int, std::mt19937::state_size> {};
    std::generate(std::begin(seed_data), std::end(seed_data), std::ref(rd));
    std::seed_seq seq(std::begin(seed_data), std::end(seed_data));
    std::mt19937 generator(seq);

    return generator;
}

// ���������� uuid v4
std::string Events::uuid()
{
	std::unique_lock ul(mtxUuid);

    // ������������� ����������
    static std::mt19937 mTwister(getMT());
    static uuids::uuid_random_generator gen{ mTwister };

    // ���������� uuid
    uuids::uuid uuidValue = gen();

    return uuids::to_string(uuidValue);
}

// �������� �������������� ������������ � ���������� ���������� � ��
std::unique_ptr<UserInfo> Events::checkUser(const std::string& login, 
                                            const std::string& password, 
                                            const std::string& postfixContext)
{
    auto user = std::make_unique<UserInfo>();  // �� ��������� auth = false
    user->login = login;
    
    // ��������������� ������������
    Dao db(DaoSettings::REDIS_SOCKET);
    user->auth = db.checkPass(login, password, postfixContext);
    if (user->auth)
    {
        m_log.info("The user \"" + login + "\" is authenticated successfully.", 
            m_context + postfixContext, "Events::checkUser " + std::to_string(__LINE__));
        
        // �������� ������� ��������������
        user->isAdmin = db.checkAdminStatus(login, postfixContext);
    }
    else
    {
        m_log.info("Failed user authentication. The username is \"" + login + '"', 
            m_context + postfixContext, "Events::checkUser " + std::to_string(__LINE__));
    }

    return user;
}

// �������������� ������������� �� �������
// PerSocketData* dataOut - �������� ������
bool Events::userAuth(                      PerSocketData* dataOut, 
                                            const std::string& login, const std::string& password)
{
    // �������������� ������������
    std::unique_ptr<UserInfo> user = checkUser(login, password, dataOut->userId);
    
    // ������������ �� ������ ��������������
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

// ���������� ������ �������� �������� ������������ � ���������� �� ����������
int Events::sendSignals(                    uWS::WebSocket<false, true, PerSocketData>* ws, 
                                            const std::string& postfixContext)
{
    // �������� ������ ���� �������� ��������
    std::map<std::string, std::string> signals;

    Dao db(DaoSettings::REDIS_SOCKET);
    int count = db.getAllSignals(signals, postfixContext);

    // ���������� ��� �������� ������� ������������
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

// �������������� ������������
void Events::authorization(                 uWS::WebSocket<false, true, PerSocketData>* ws, 
                                            const std::string_view message,
                                            const std::string& postfixContext)
{
    PerSocketData* data = ws->getUserData();
    
    // �������� ����� � ������
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
        // ����������� ������������ �� ����� � ���������
        ws->subscribe(ServerSettings::BROADCAST);
        m_log.info("The user \"" + data->login + "\" is subscribed to a channel with signals.", 
            m_context + postfixContext, "Events::authorization " + std::to_string(__LINE__));
    }
    else
    {
        // �������� ����� ��� ������
        nlohmann::json response;
        response[JsonValue::AUTH] = JsonValue::AUTH_FALSE;

        ws->send(response.dump(), uWS::OpCode::TEXT);

        return;
    }

    // ���������� ��� �������� ������� ������������
    sendSignals(ws, data->userId);
}

// ��������� � ������� �������
void Events::signalize(                     uWS::WebSocket<false, true, PerSocketData>* ws, 
                                            const std::string_view message, 
                                            const std::string& postfixContext)
{
    // �������� ��� ������ � ���������� �� �������
    nlohmann::json parsed = nlohmann::json::parse(message);
    std::string command = parsed[JsonValue::COMMAND];
    std::string tickerSymbol = parsed[JsonValue::TICKER];
    std::string limits;

    // ���������� ������� � ��������� �
    nlohmann::json response;
    if (command == JsonValue::ADD_SIGNAL)
    {
        // ��������� ������
        limits = parsed[JsonValue::LIMITS];
        
        Dao db(DaoSettings::REDIS_SOCKET);
        response[JsonValue::COMMAND] = (db.setSignal(tickerSymbol, limits, postfixContext) ? JsonValue::ACTION_SUCCESS : JsonValue::ACTION_FAIL);

    }
    else if (command == JsonValue::DEL_SIGNAL)
    {
        // ������� ������
        Dao db(DaoSettings::REDIS_SOCKET);
        response[JsonValue::COMMAND] = (db.delSignal(tickerSymbol, postfixContext) ? JsonValue::ACTION_SUCCESS : JsonValue::ACTION_FAIL);
    }
    else
    {
        // ����������� �������
        m_log.warn("Unknown command from the user.", m_context + postfixContext, "Events::signalize " + std::to_string(__LINE__));
        response[JsonValue::COMMAND] = JsonValue::ACTION_UNKNOWN;
    }

    // ��������� ������������
    ws->send(response.dump(), uWS::OpCode::TEXT);

    // � ������ ������ ��������� ���������� ��������� � ����� ���
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
