#ifndef PERSOCKETDATA_H
#define PERSOCKETDATA_H

#include <string>


// ������ �������������
struct PerSocketData
{
	std::string userId;				// �� ������������ ����������
	std::string login;				// ����� ������������
	
	// �� ��������� false
	bool        auth	= false;	// ������� � ��������� �����������
	bool        isAdmin = false;	// ������� � ������� ��������������
};

#endif // !PERSOCKETDATA_H
