#ifndef PERSOCKETDATA_H
#define PERSOCKETDATA_H

#include <string>


// Данные пользователей
struct PerSocketData
{
	std::string userId;				// ИН пользователя вебсервера
	std::string login;				// логин пользователя
	
	// По умолчанию false
	bool        auth	= false;	// пометка о пройденой авторизации
	bool        isAdmin = false;	// пометка о статусе администратора
};

#endif // !PERSOCKETDATA_H
