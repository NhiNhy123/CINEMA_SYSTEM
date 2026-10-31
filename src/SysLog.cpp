#include "../include/SysLog.h"
#include <sqlite3.h>


SysLog::SysLog()
{
    databaseFile = "DATA/cinema.db";
}

SysLog::SysLog(string databaseFile)
{
    this->databaseFile = databaseFile;
}


void SysLog::gotoxy(int x, int y)
{
    COORD coord;

    coord.X = x;
    coord.Y = y;

    SetConsoleCursorPosition(
        GetStdHandle(STD_OUTPUT_HANDLE),
        coord);
}


void SysLog::setColor(int color)
{
    SetConsoleTextAttribute(
        GetStdHandle(STD_OUTPUT_HANDLE),
        color);
}


void SysLog::clearScreen()
{
    system("cls");
}


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


string SysLog::nhapTaiKhoan()
{
    string taiKhoan;

    gotoxy(53, 9);
    cin >> taiKhoan;

    return taiKhoan;
}


string SysLog::nhapMatKhau()
{
    string matKhau;

    gotoxy(53, 12);
    cin >> matKhau;

    return matKhau;
}


User* SysLog::timTaiKhoan(
    string taiKhoan,
    string matKhau)
{
    sqlite3* database = NULL;
    if (sqlite3_open(databaseFile.c_str(), &database) != SQLITE_OK) {
        gotoxy(35, 20);
        setColor(12);
        cout << "Khong the mo co so du lieu SQLite!";
        setColor(7);
        if (database != NULL) {
            cerr << "SQLite: " << sqlite3_errmsg(database) << "\n";
            sqlite3_close(database);
        }
        return NULL;
    }

    sqlite3_stmt* statement = NULL;
    const char* sql = "SELECT maNguoiDung,hoTen,soDienThoai,email,ngaySinh,gioiTinh,cccd,"
                      "taiKhoan,matKhau,vaiTro,trangThaiLamViec,trangThaiTaiKhoan "
                      "FROM users WHERE taiKhoan=? AND matKhau=? ORDER BY rowid LIMIT 1";
    if (sqlite3_prepare_v2(database, sql, -1, &statement, NULL) != SQLITE_OK) {
        gotoxy(35, 20);
        setColor(12);
        cout << "Khong the truy van tai khoan!";
        setColor(7);
        cerr << "SQLite: " << sqlite3_errmsg(database) << "\n";
        sqlite3_close(database);
        return NULL;
    }
    sqlite3_bind_text(statement, 1, taiKhoan.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(statement, 2, matKhau.c_str(), -1, SQLITE_TRANSIENT);

    User* result = NULL;
    const int step = sqlite3_step(statement);
    if (step == SQLITE_ROW) {
        const char* fields[12];
        for (int i = 0; i < 12; ++i) {
            const unsigned char* text = sqlite3_column_text(statement, i);
            fields[i] = text != NULL ? reinterpret_cast<const char*>(text) : "";
        }
        const string maNguoiDung(fields[0]);
        const string hoTen(fields[1]);
        const string soDienThoai(fields[2]);
        const string email(fields[3]);
        const string ngaySinh(fields[4]);
        const string gioiTinh(fields[5]);
        const string cccd(fields[6]);
        const string tk(fields[7]);
        const string mk(fields[8]);
        const string vaiTro(fields[9]);
        const string trangThaiLamViec(fields[10]);
        const string trangThaiTaiKhoan(fields[11]);

        if (vaiTro == "Admin") {
            result = new Admin(maNguoiDung, hoTen, soDienThoai, ngaySinh,
                               gioiTinh, email, cccd, tk, mk);
        } else if (vaiTro == "Staff") {
            if (trangThaiTaiKhoan == "BiKhoa") {
                gotoxy(35, 20);
                setColor(12);
                cout << "Tai khoan da bi khoa!";
                setColor(7);
            } else if (trangThaiLamViec == "DaNghiViec") {
                gotoxy(35, 20);
                setColor(12);
                cout << "Nhan vien da nghi viec!";
                setColor(7);
            } else {
                result = new Staff(maNguoiDung, hoTen, soDienThoai, ngaySinh,
                                   gioiTinh, email, cccd, tk, mk,
                                   trangThaiLamViec, trangThaiTaiKhoan, true);
            }
        }
    } else if (step != SQLITE_DONE) {
        cerr << "Loi khi truy van tai khoan: " << sqlite3_errmsg(database) << "\n";
    }
    sqlite3_finalize(statement);
    sqlite3_close(database);
    return result;
}


User* SysLog::dangNhap()
{
    hienThiGiaoDien();

    string taiKhoan = nhapTaiKhoan();

    string matKhau = nhapMatKhau();

    User* user = timTaiKhoan(
        taiKhoan,
        matKhau);


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


    clearScreen();

    setColor(10);

    gotoxy(35, 8);

    cout << "========================================";

    gotoxy(35, 9);

    cout << "         DANG NHAP THANH CONG!          ";

    gotoxy(35, 10);

    cout << "========================================";

    gotoxy(35, 13);

    cout << "Vai tro: "
         << user->getVaiTro();

    setColor(7);

    gotoxy(35, 18);

    system("pause");

    return user;
}

bool SysLog::doiMatKhau(User& user, const string& matKhauHienTai, const string& matKhauMoi)
{
    if (!user.kiemTraDangNhap(user.getTaiKhoan(), matKhauHienTai))
    {
        cout << "Mat khau hien tai khong chinh xac.\n";
        return false;
    }

    if (matKhauMoi.empty() ||
        matKhauMoi.find_first_of(",\r\n") != string::npos ||
        matKhauMoi == matKhauHienTai)
    {
        cout << "Mat khau moi khong hop le hoac trung voi mat khau hien tai.\n";
        return false;
    }

    sqlite3* database = NULL;
    if (sqlite3_open(databaseFile.c_str(), &database) != SQLITE_OK) {
        cerr << "Khong the mo co so du lieu tai khoan: "
             << (database != NULL ? sqlite3_errmsg(database) : "khong du bo nho") << "\n";
        if (database != NULL) sqlite3_close(database);
        return false;
    }

    sqlite3_stmt* lookup = NULL;
    const char* lookupSql = "SELECT rowid FROM users WHERE maNguoiDung=? AND taiKhoan=? "
                            "AND vaiTro=? AND matKhau=? ORDER BY rowid";
    if (sqlite3_prepare_v2(database, lookupSql, -1, &lookup, NULL) != SQLITE_OK) {
        cerr << "Khong the truy van tai khoan: " << sqlite3_errmsg(database) << "\n";
        sqlite3_close(database);
        return false;
    }
    sqlite3_bind_text(lookup, 1, user.getMaNguoiDung().c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(lookup, 2, user.getTaiKhoan().c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(lookup, 3, user.getVaiTro().c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(lookup, 4, matKhauHienTai.c_str(), -1, SQLITE_TRANSIENT);

    sqlite3_int64 rowId = 0;
    int step = sqlite3_step(lookup);
    if (step == SQLITE_ROW) {
        rowId = sqlite3_column_int64(lookup, 0);
        step = sqlite3_step(lookup);
        if (step == SQLITE_ROW) {
            cerr << "Tim thay tai khoan bi trung lap: " << user.getMaNguoiDung() << "\n";
            sqlite3_finalize(lookup);
            sqlite3_close(database);
            return false;
        }
    }
    sqlite3_finalize(lookup);
    if (rowId == 0) {
        if (step != SQLITE_DONE) {
            cerr << "Loi khi tim tai khoan: " << sqlite3_errmsg(database) << "\n";
        } else {
            cerr << "Khong tim thay tai khoan trong SQLite: " << user.getMaNguoiDung() << "\n";
        }
        sqlite3_close(database);
        return false;
    }

    sqlite3_stmt* update = NULL;
    if (sqlite3_prepare_v2(database, "UPDATE users SET matKhau=? WHERE rowid=?",
                           -1, &update, NULL) != SQLITE_OK) {
        cerr << "Khong the chuan bi cap nhat mat khau: " << sqlite3_errmsg(database) << "\n";
        sqlite3_close(database);
        return false;
    }
    sqlite3_bind_text(update, 1, matKhauMoi.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int64(update, 2, rowId);
    const int updateResult = sqlite3_step(update);
    const bool updated = updateResult == SQLITE_DONE && sqlite3_changes(database) == 1;
    if (!updated) {
        cerr << "Loi khi cap nhat mat khau: " << sqlite3_errmsg(database) << "\n";
    }
    sqlite3_finalize(update);
    sqlite3_close(database);

    if (!updated) return false;
    user.setMatKhau(matKhauMoi);
    return true;
}
