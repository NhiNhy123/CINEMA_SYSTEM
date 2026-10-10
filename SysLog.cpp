#include "SysLog.h"
#include <fstream>


// =====================================================
// CONSTRUCTOR
// =====================================================

SysLog::SysLog()
{
    fileCSV = "quan_ly_nhan_su.csv"; // S?a l?i cho kh?p v?i tên file th?c t? c?a b?n
}

SysLog::SysLog(string fileCSV)
{
    this->fileCSV = fileCSV;
}


// =====================================================
// GOTOXY
// =====================================================

void SysLog::gotoxy(int x, int y)
{
    COORD coord;

    coord.X = x;
    coord.Y = y;

    SetConsoleCursorPosition(
        GetStdHandle(STD_OUTPUT_HANDLE),
        coord);
}


// =====================================================
// SET COLOR
// =====================================================

void SysLog::setColor(int color)
{
    SetConsoleTextAttribute(
        GetStdHandle(STD_OUTPUT_HANDLE),
        color);
}


// =====================================================
// CLEAR SCREEN
// =====================================================

void SysLog::clearScreen()
{
    system("cls");
}


// =====================================================
// GIAO DIEN DANG NHAP
// =====================================================

void SysLog::hienThiGiaoDien()
{
    clearScreen();

    setColor(11);

    gotoxy(35, 4);
    cout << "========================================";

    gotoxy(35, 5);
    cout << "        HE THONG QUAN LY RAP PHIM       ";

    gotoxy(35, 6);
    cout << "========================================";

    setColor(7);

    gotoxy(40, 9);
    cout << "Tai khoan:";

    gotoxy(40, 12);
    cout << "Mat khau:";

    gotoxy(35, 16);
    cout << "========================================";

    gotoxy(38, 17);
    cout << "Nhap thong tin de dang nhap";

    gotoxy(35, 18);
    cout << "========================================";
}


// =====================================================
// NHAP TAI KHOAN
// =====================================================

string SysLog::nhapTaiKhoan()
{
    string taiKhoan;

    gotoxy(53, 9);
    cin >> taiKhoan;

    return taiKhoan;
}


// =====================================================
// NHAP MAT KHAU
// =====================================================

string SysLog::nhapMatKhau()
{
    string matKhau;

    gotoxy(53, 12);
    cin >> matKhau;

    return matKhau;
}


// =====================================================
// TIM TAI KHOAN
//
// CSV:
// maNguoiDung,
// hoTen,
// soDienThoai,
// ngaySinh,
// gioiTinh,
// email,
// cccd,
// taiKhoan,
// matKhau,
// vaiTro,
// trangThaiLamViec,
// trangThaiTaiKhoan,
// coTaiKhoan
// =====================================================

User* SysLog::timTaiKhoan(
    string taiKhoan,
    string matKhau)
{
    ifstream file(fileCSV.c_str());

    if (!file.is_open())
    {
        gotoxy(35, 20);

        setColor(12);

        cout << "Khong the mo file CSV!";

        setColor(7);

        return NULL;
    }

    string line;

    while (getline(file, line))
    {
        // Bo qua dong rong
        if (line.empty())
        {
            continue;
        }

        stringstream ss(line);

        string maNguoiDung;
        string hoTen;
        string soDienThoai;
        string ngaySinh;
        string gioiTinh;
        string email;
        string cccd;

        string tk;
        string mk;
        string vaiTro;

        string trangThaiLamViec;
        string trangThaiTaiKhoan;
        string coTaiKhoan;


        // =========================
        // DOC CSV
        // =========================

        getline(ss, maNguoiDung, ',');
        getline(ss, hoTen, ',');
        getline(ss, soDienThoai, ',');
        getline(ss, ngaySinh, ',');
        getline(ss, gioiTinh, ',');
        getline(ss, email, ',');
        getline(ss, cccd, ',');

        getline(ss, tk, ',');
        getline(ss, mk, ',');
        getline(ss, vaiTro, ',');

        getline(ss, trangThaiLamViec, ',');
        getline(ss, trangThaiTaiKhoan, ',');
        getline(ss, coTaiKhoan, ',');


        // =========================
        // KIEM TRA TAI KHOAN
        // =========================

        if (tk != taiKhoan)
        {
            continue;
        }

        // Sai mat khau
        if (mk != matKhau)
        {
            continue;
        }


        // =================================================
        // ADMIN
        // =================================================

        if (vaiTro == "Admin")
        {
            file.close();

            Admin* admin = new Admin(
                maNguoiDung,
                hoTen,
                soDienThoai,
                ngaySinh,
                gioiTinh,
                email,
                cccd,
                tk,
                mk);

            return admin;
        }


        // =================================================
        // STAFF
        // =================================================

        if (vaiTro == "Staff")
        {
            // -------------------------
            // Chua co tai khoan
            // -------------------------

            if (coTaiKhoan != "1")
            {
                file.close();

                gotoxy(35, 20);

                setColor(12);

                cout << "Nhan vien chua duoc cap tai khoan!";

                setColor(7);

                return NULL;
            }


            // -------------------------
            // Tai khoan bi khoa
            // -------------------------

            if (trangThaiTaiKhoan == "BiKhoa")
            {
                file.close();

                gotoxy(35, 20);

                setColor(12);

                cout << "Tai khoan da bi khoa!";

                setColor(7);

                return NULL;
            }


            // -------------------------
            // Nhan vien da nghi viec
            // -------------------------

            if (trangThaiLamViec == "DaNghiViec")
            {
                file.close();

                gotoxy(35, 20);

                setColor(12);

                cout << "Nhan vien da nghi viec!";

                setColor(7);

                return NULL;
            }


            // -------------------------
            // Chuyen string -> bool
            // -------------------------

            bool coTK = false;

            if (coTaiKhoan == "1")
            {
                coTK = true;
            }


            // -------------------------
            // Tao Staff
            // -------------------------

            Staff* staff = new Staff(
                maNguoiDung,
                hoTen,
                soDienThoai,
                ngaySinh,
                gioiTinh,
                email,
                cccd,
                tk,
                mk,
                trangThaiLamViec,
                trangThaiTaiKhoan,
                coTK);

            file.close();

            return staff;
        }
    }

    file.close();

    return NULL;
}


// =====================================================
// DANG NHAP
// =====================================================

User* SysLog::dangNhap()
{
    hienThiGiaoDien();

    // Nhap tai khoan
    string taiKhoan = nhapTaiKhoan();

    // Nhap mat khau
    string matKhau = nhapMatKhau();

    // Tim tai khoan
    User* user = timTaiKhoan(
        taiKhoan,
        matKhau);


    // =================================================
    // DANG NHAP THAT BAI
    // =================================================

    if (user == NULL)
    {
        gotoxy(35, 22);

        setColor(12);

        cout << "Tai khoan hoac mat khau khong dung!";

        setColor(7);

        gotoxy(35, 24);

        system("pause");

        return NULL;
    }


    // =================================================
    // DANG NHAP THANH CONG
    // =================================================

    clearScreen();

    setColor(10);

    gotoxy(35, 8);

    cout << "========================================";

    gotoxy(35, 9);

    cout << "         DANG NHAP THANH CONG!          ";

    gotoxy(35, 10);

    cout << "========================================";

    gotoxy(35, 13);

    cout << "Xin chao: "
         << user->getHoTen();

    gotoxy(35, 15);

    cout << "Vai tro: "
         << user->getVaiTro();

    setColor(7);

    gotoxy(35, 18);

    system("pause");

    return user;
}
