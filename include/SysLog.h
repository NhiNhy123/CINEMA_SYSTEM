#ifndef SYSLOG_H
#define SYSLOG_H
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <windows.h>

#include "User.h"

using namespace std;

class SysLog
{
private:

    string fileCSV;

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

    SysLog(string fileCSV);

    User* dangNhap();
    bool doiMatKhau(User& user, const string& matKhauHienTai, const string& matKhauMoi);
};

#endif
