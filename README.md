# TraderInfo

<p align="center">
   <img src="https://img.shields.io/badge/%D0%A1%2B%2B-20-blue" alt="C++ Version">
   <img src="https://img.shields.io/badge/uWebSockets-20.36.0-success" alt="uWebSockets Version">
   <img src="https://img.shields.io/badge/redis%2B%2B-1.3.7-orange" alt="redis++ Version">
   <img src="https://img.shields.io/badge/nlohmann%2Fjson-3.11.2-blueviolet" alt="nlohmann/json Version">
   <img src="https://img.shields.io/badge/stduuid-1.2.3-green" alt="stduuid Version">
</p>

<p align="center">
   <img src="https://img.shields.io/badge/log4cpp-1.1.4rc2-lightgrey" alt="log4cpp Version">
   <img src="https://img.shields.io/badge/version-1.0-yellow" alt="Application Version">
   <img src="https://img.shields.io/badge/license-MIT-red" alt="License">
</p>

## The application for the Copy Trading.

### About
The server application is designed for copy trading.
The administrator can add and remove the signal.
Users only receive signals.

### Documentation
Documentation Here
The application runs on the WebSocket Protocol.
The database is implemented in Redis.
Users and applications communicate using JSON messages.
Argon2 hashes passwords.

The message schema.

Authorization: { "command": "authorization", "username": "login", "password": "pass" }

Add a signal: { "command": "add", "tickerSymbol": "xxx", "limits": "amount" }

Delete a signal: { "command": "delete", "tickerSymbol": "xxx" }

### Developers

- [Valendovsky](https://github.com/valendovsky)

### License

Project TraderInfo is distributed under the MIT license.

---

## Серверное приложение для копитрейдинга.

### О проекте
Серверное приложение на websocket'ах.
Программа предназначена для копирующей торговли.
Администраторы добавляют и удаляют сигналы.
Пользователи могут только получать сигналы.

### Документация
Сервер работает на протоколе websocket.
Приложение использует в виде базы данных сервер Redis.
Общение пользователей с сервером происходит при помощи JSON сообщений.
Для хеширования паролей используется Argon2.

Схема сообщений.

Авторизация: { "command": "authorization", "username": "login", "password": "pass" }

Добавить сигнал: { "command": "add", "tickerSymbol": "xxx", "limits": "amount" }

Удалить сигнал: { "command": "delete", "tickerSymbol": "xxx" }

### Разработчики
- [Valendovsky](https://github.com/valendovsky)

### Лицензия
Проект TraderInfo распространяется под лицензией MIT.
