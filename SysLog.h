#ifndef SYSLOG_H
#define SYSLOG_H

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
    // Duong dan file CSV
    string fileCSV;

    // =========================
    // GIAO DIEN
    // =========================

    void gotoxy(int x, int y);

    void setColor(int color);

    void clearScreen();

    void hienThiGiaoDien();

    // =========================
    // NHAP DU LIEU
    // =========================

    string nhapTaiKhoan();

    string nhapMatKhau();

    // =========================
    // KIEM TRA TAI KHOAN
    // =========================

    User* timTaiKhoan(
        string taiKhoan,
        string matKhau);

public:
    SysLog();

    SysLog(string fileCSV);

    // Ham dang nhap chinh
    User* dangNhap();
};

#endif
