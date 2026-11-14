#ifndef SYSLOG_H
#define SYSLOG_H
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#define byte win_byte_override
#include <windows.h>
#undef byte

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>

#include "User.h"

using namespace std;

class SysLog
{
private:

    string databaseFile;

    void gotoxy(int x, int y);

    void setColor(int color);

    void clearScreen();

    void hienThiGiaoDien();

    string nhapTaiKhoan();

    string nhapMatKhau();

    User* timTaiKhoan(
        string taiKhoan,
        string matKhau);

public:
    SysLog();

    SysLog(string databaseFile);

    User* dangNhap();
    bool doiMatKhau(User& user, const string& matKhauHienTai, const string& matKhauMoi);
};

#endif
