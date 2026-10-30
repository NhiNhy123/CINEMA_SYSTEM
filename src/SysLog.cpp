#include "../include/SysLog.h"
#include <fstream>
#include <vector>


SysLog::SysLog()
{
    fileCSV = "DATA/quan_ly_nhan_su.csv";
}

SysLog::SysLog(string fileCSV)
{
    this->fileCSV = fileCSV;
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
        if (line.empty() || line.find("[USERS]") != string::npos || line.find("maNguoiDung") != string::npos)
        {
            continue;
        }

        stringstream ss(line);

        string maNguoiDung;
        string hoTen;
        string soDienThoai;
        string email;
        string ngaySinh;
        string gioiTinh;
        string cccd;

        string tk;
        string mk;
        string vaiTro;

        string trangThaiLamViec;
        string trangThaiTaiKhoan;

        getline(ss, maNguoiDung, ',');
        getline(ss, hoTen, ',');
        getline(ss, soDienThoai, ',');
        getline(ss, email, ',');
        getline(ss, ngaySinh, ',');
        getline(ss, gioiTinh, ',');
        getline(ss, cccd, ',');

        getline(ss, tk, ',');
        getline(ss, mk, ',');
        getline(ss, vaiTro, ',');

        getline(ss, trangThaiLamViec, ',');
        getline(ss, trangThaiTaiKhoan, ',');

        if (tk != taiKhoan)
        {
            continue;
        }

        if (mk != matKhau)
        {
            continue;
        }

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

        if (vaiTro == "Staff")
        {
            if (trangThaiTaiKhoan == "BiKhoa")
            {
                file.close();
                gotoxy(35, 20);
                setColor(12);
                cout << "Tai khoan da bi khoa!";
                setColor(7);
                return NULL;
            }

            if (trangThaiLamViec == "DaNghiViec")
            {
                file.close();
                gotoxy(35, 20);
                setColor(12);
                cout << "Nhan vien da nghi viec!";
                setColor(7);
                return NULL;
            }

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
                true);

            file.close();

            return staff;
        }
    }

    file.close();

    return NULL;
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

    ifstream input(fileCSV.c_str());
    if (!input.is_open())
    {
        cerr << "Khong the mo file tai khoan: " << fileCSV << "\n";
        return false;
    }

    vector<string> lines;
    string line;
    bool readingUsers = false;
    bool found = false;
    while (getline(input, line))
    {
        if (!line.empty() && line[0] == '[')
        {
            readingUsers = line == "[USERS]";
            lines.push_back(line);
            continue;
        }

        if (readingUsers && line.find("maNguoiDung") != 0)
        {
            stringstream row(line);
            string fields[10];
            int fieldCount = 0;
            while (fieldCount < 10 && getline(row, fields[fieldCount], ','))
            {
                ++fieldCount;
            }

            if (fieldCount >= 10 &&
                fields[0] == user.getMaNguoiDung() &&
                fields[7] == user.getTaiKhoan() &&
                fields[9] == user.getVaiTro())
            {
                if (found)
                {
                    cerr << "Tim thay tai khoan bi trung lap: " << user.getMaNguoiDung() << "\n";
                    return false;
                }

                size_t fieldStart = 0;
                for (int i = 0; i < 8; ++i)
                {
                    fieldStart = line.find(',', fieldStart);
                    if (fieldStart == string::npos)
                    {
                        cerr << "Dong tai khoan khong dung dinh dang CSV.\n";
                        return false;
                    }
                    ++fieldStart;
                }
                const size_t passwordEnd = line.find(',', fieldStart);
                line = line.substr(0, fieldStart) + matKhauMoi +
                       (passwordEnd == string::npos ? "" : line.substr(passwordEnd));
                found = true;
            }
        }
        lines.push_back(line);
    }
    if (input.bad())
    {
        cerr << "Loi khi doc file tai khoan: " << fileCSV << "\n";
        return false;
    }
    input.close();

    if (!found)
    {
        cerr << "Khong tim thay tai khoan trong muc [USERS]: "
             << user.getMaNguoiDung() << "\n";
        return false;
    }

    ofstream output(fileCSV.c_str(), ios::trunc);
    if (!output.is_open())
    {
        cerr << "Khong the ghi file tai khoan: " << fileCSV << "\n";
        return false;
    }
    for (size_t i = 0; i < lines.size(); ++i)
    {
        output << lines[i] << "\n";
    }
    output.close();
    if (!output)
    {
        cerr << "Loi khi luu file tai khoan: " << fileCSV << "\n";
        return false;
    }

    user.setMatKhau(matKhauMoi);
    return true;
}
