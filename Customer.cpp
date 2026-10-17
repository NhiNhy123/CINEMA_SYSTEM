#include "Customer.h"
#include <sstream>
#include <cstdlib>
// =====================================================
// GIAO DICH DIEM
// =====================================================

// Constructor mac dinh
GiaoDichDiem::GiaoDichDiem()
{
    maGiaoDich = "";
    tenKhachHang = "";
    soDienThoai = "";
    loaiGiaoDich = "";
    soDiem = 0;
    noiDung = "";
    thoiGian = "";
}


// Constructor
GiaoDichDiem::GiaoDichDiem(
    string maGiaoDich,
    string tenKhachHang,
    string soDienThoai,
    string loaiGiaoDich,
    int soDiem,
    string noiDung,
    string thoiGian)
{
    this->maGiaoDich = maGiaoDich;
    this->tenKhachHang = tenKhachHang;
    this->soDienThoai = soDienThoai;
    this->loaiGiaoDich = loaiGiaoDich;
    this->soDiem = soDiem;
    this->noiDung = noiDung;
    this->thoiGian = thoiGian;
}


// =====================================================
// GETTER GIAO DICH
// =====================================================

string GiaoDichDiem::getMaGiaoDich() const
{
    return maGiaoDich;
}

string GiaoDichDiem::getTenKhachHang() const
{
    return tenKhachHang;
}

string GiaoDichDiem::getSoDienThoai() const
{
    return soDienThoai;
}

string GiaoDichDiem::getLoaiGiaoDich() const
{
    return loaiGiaoDich;
}

int GiaoDichDiem::getSoDiem() const
{
    return soDiem;
}

string GiaoDichDiem::getNoiDung() const
{
    return noiDung;
}

string GiaoDichDiem::getThoiGian() const
{
    return thoiGian;
}

// Chuy?n giao d?ch thành chu?i CSV d? ghi vào b?ng [CUSTOMER_POINTS_HISTORY]
string GiaoDichDiem::toCSV() const
{
    stringstream ss;
    ss << maGiaoDich << "," 
       << soDienThoai << "," // Ho?c dùng maKhachHang tùy theo thi?t k? b?ng c?a b?n
       << loaiGiaoDich << "," 
       << soDiem << "," 
       << noiDung << "," 
       << thoiGian;
    return ss.str();
}

// Ð?c m?t dòng CSV d? tái t?o d?i tu?ng GiaoDichDiem
GiaoDichDiem GiaoDichDiem::fromCSV(const string& line)
{
    stringstream ss(line);
    string maGD, sdt, loaiGD, strSoDiem, nd, tg;

    getline(ss, maGD, ',');
    getline(ss, sdt, ',');
    getline(ss, loaiGD, ',');
    getline(ss, strSoDiem, ',');
    getline(ss, nd, ',');
    getline(ss, tg, ',');

    int soDiem = 0;
    if (!strSoDiem.empty()) {
        soDiem = atoi(strSoDiem.c_str());
    }

    return GiaoDichDiem(maGD, "", sdt, loaiGD, soDiem, nd, tg);
}


// =====================================================
// HIEN THI GIAO DICH
// =====================================================

void GiaoDichDiem::hienThi() const
{
    cout << "Ma giao dich: " << maGiaoDich << endl;
    cout << "Ten khach hang: " << tenKhachHang << endl;
    cout << "So dien thoai: " << soDienThoai << endl;
    cout << "Loai giao dich: " << loaiGiaoDich << endl;
    cout << "So diem: " << soDiem << endl;
    cout << "Noi dung: " << noiDung << endl;
    cout << "Thoi gian: " << thoiGian << endl;
}


// =====================================================
// CUSTOMER
// =====================================================

// Constructor mac dinh
Customer::Customer()
{
    maKH = "";
    hoTen = "";
    soDienThoai = "";
    email = "";

    diemTichLuy = 0;
    hangThanhVien = "Thuong";
    tongChiTieu = 0;
}


// =====================================================
// CUSTOMER CONSTRUCTOR KHACH HANG MOI
// =====================================================

Customer::Customer(
    string maKH,
    string hoTen,
    string soDienThoai,
    string email)
{
    this->maKH = maKH;
    this->hoTen = hoTen;
    this->soDienThoai = soDienThoai;
    this->email = email;

    diemTichLuy = 0;
    hangThanhVien = "Thuong";
    tongChiTieu = 0;
}


// =====================================================
// CUSTOMER CONSTRUCTOR DAY DU
// DUNG KHI DOC DU LIEU TU CSV
// =====================================================

Customer::Customer(
    string maKH,
    string hoTen,
    string soDienThoai,
    string email,
    int diemTichLuy,
    string hangThanhVien,
    long long tongChiTieu)
{
    this->maKH = maKH;
    this->hoTen = hoTen;
    this->soDienThoai = soDienThoai;
    this->email = email;

    this->diemTichLuy = diemTichLuy;
    this->hangThanhVien = hangThanhVien;
    this->tongChiTieu = tongChiTieu;
}


// =====================================================
// GETTER CUSTOMER
// =====================================================

string Customer::getMaKH() const
{
    return maKH;
}

string Customer::getHoTen() const
{
    return hoTen;
}

string Customer::getSoDienThoai() const
{
    return soDienThoai;
}

string Customer::getEmail() const
{
    return email;
}

int Customer::getDiemTichLuy() const
{
    return diemTichLuy;
}

string Customer::getHangThanhVien() const
{
    return hangThanhVien;
}

long long Customer::getTongChiTieu() const
{
    return tongChiTieu;
}


// =====================================================
// SETTER
// =====================================================

void Customer::setHoTen(string hoTen)
{
    this->hoTen = hoTen;
}

void Customer::setSoDienThoai(string soDienThoai)
{
    this->soDienThoai = soDienThoai;
}

void Customer::setEmail(string email)
{
    this->email = email;
}


// =====================================================
// TICH DIEM
// =====================================================
//
// Quy uoc:
// 1.000 VND = 1 diem
//
// Vi du:
// Chi 100.000 VND -> 100 diem
// =====================================================

// Chuy?n thông tin khách hàng thành chu?i CSV cho b?ng [CUSTOMERS]
string Customer::toCSV() const
{
    stringstream ss;
    ss << maKH << "," 
       << diemTichLuy << "," 
       << hangThanhVien << "," 
       << tongChiTieu;
    return ss.str();
}

// Ð?c thông tin t? dòng CSV c?a b?ng [CUSTOMERS]
Customer Customer::fromCSV(const string& line)
{
    stringstream ss(line);
    string ma, hang, strDiem, strChiTieu;

    getline(ss, ma, ',');
    getline(ss, strDiem, ',');
    getline(ss, hang, ',');
    getline(ss, strChiTieu, ',');

    int diem = 0;
    long long chiTieu = 0;

    if (!strDiem.empty()) diem = atoi(strDiem.c_str());
   if (!strChiTieu.empty()) chiTieu = atoll(strChiTieu.c_str());

    // Tr? v? d?i tu?ng (H? tên, SÐT, Email s? du?c ghép n?i t? b?ng [USERS] khi Database x? lý)
    return Customer(ma, "", "", "", diem, hang, chiTieu);
}

void Customer::tichDiem(
    int soDiem,
    string noiDung,
    string thoiGian)
{
    if (soDiem <= 0)
    {
        return;
    }

    diemTichLuy += soDiem;

    stringstream ss;
ss << "GD" << (lichSuDiem.size() + 1);
string maGiaoDich = ss.str();

    GiaoDichDiem giaoDich(
        maGiaoDich,
        hoTen,
        soDienThoai,
        "TichDiem",
        soDiem,
        noiDung,
        thoiGian);

    lichSuDiem.push_back(giaoDich);

    capNhatHangThanhVien();
}


// =====================================================
// DOI DIEM
// =====================================================
//
// 1.000 diem = 1.000 VND
// =====================================================

bool Customer::doiDiem(
    int soDiem,
    string noiDung,
    string thoiGian)
{
    if (soDiem <= 0)
    {
        return false;
    }

    // Khong du diem
    if (soDiem > diemTichLuy)
    {
        return false;
    }

    diemTichLuy -= soDiem;

    stringstream ss;
ss << "GD" << (lichSuDiem.size() + 1);
string maGiaoDich = ss.str();


    GiaoDichDiem giaoDich(
        maGiaoDich,
        hoTen,
        soDienThoai,
        "DoiDiem",
        soDiem,
        noiDung,
        thoiGian);

    lichSuDiem.push_back(giaoDich);

    capNhatHangThanhVien();

    return true;
}


// =====================================================
// THEM CHI TIEU
// =====================================================

void Customer::themChiTieu(long long soTien)
{
    if (soTien <= 0)
    {
        return;
    }

    tongChiTieu += soTien;

    capNhatHangThanhVien();
}


// =====================================================
// CAP NHAT HANG THANH VIEN
// =====================================================
//
// Quy tac nay co the thay doi theo yeu cau PBL.
// Tam thoi:
//
// Thuong:
// < 1.000.000 VND
//
// ThanThiet:
// >= 1.000.000 VND
//
// VIP:
// >= 5.000.000 VND
// =====================================================

void Customer::capNhatHangThanhVien()
{
    if (tongChiTieu >= 5000000)
    {
        hangThanhVien = "VIP";
    }
    else if (tongChiTieu >= 1000000)
    {
        hangThanhVien = "ThanThiet";
    }
    else
    {
        hangThanhVien = "Thuong";
    }
}


// =====================================================
// THEM GIAO DICH
// =====================================================

void Customer::themGiaoDich(
    GiaoDichDiem giaoDich)
{
    lichSuDiem.push_back(giaoDich);
}


// =====================================================
// HIEN THI LICH SU DIEM
// =====================================================

void Customer::hienThiLichSuDiem() const
{
    cout << endl;
    cout << "========== LICH SU TICH / DOI DIEM =========="
         << endl;

    if (lichSuDiem.empty())
    {
        cout << "Khach hang chua co giao dich diem."
             << endl;

        return;
    }

    for (vector<GiaoDichDiem>::const_iterator it = lichSuDiem.begin();
     it != lichSuDiem.end();
     ++it)
{
    const GiaoDichDiem& giaoDich = *it;

    cout << endl;

    giaoDich.hienThi();

    cout << "---------------------------------------------"
         << endl;
}

}


// =====================================================
// HIEN THI THONG TIN CUSTOMER
// =====================================================

void Customer::hienThiThongTin() const
{
    cout << endl;
    cout << "========== THONG TIN KHACH HANG =========="
         << endl;

    cout << "Ma KH: "
         << maKH
         << endl;

    cout << "Ho ten: "
         << hoTen
         << endl;

    cout << "So dien thoai: "
         << soDienThoai
         << endl;

    cout << "Email: "
         << email
         << endl;

    cout << "Diem tich luy: "
         << diemTichLuy
         << endl;

    cout << "Hang thanh vien: "
         << hangThanhVien
         << endl;

    cout << "Tong chi tieu: "
         << tongChiTieu
         << " VND"
         << endl;
}
