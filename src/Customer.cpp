#include "../include/Customer.h"
#include "../include/DateFormat.h"
#include <sstream>
#include <cstdlib>

GiaoDichDiem::GiaoDichDiem()
{
    maGiaoDich = "";
    maKhachHang = "";
    loaiGiaoDich = "";
    soDiem = 0;
    noiDung = "";
    thoiGian = "";
}

GiaoDichDiem::GiaoDichDiem(
    string maGiaoDich,
    string maKhachHang,
    string loaiGiaoDich,
    int soDiem,
    string noiDung,
    string thoiGian)
{
    this->maGiaoDich = maGiaoDich;
    this->maKhachHang = maKhachHang;
    this->loaiGiaoDich = loaiGiaoDich;
    this->soDiem = soDiem;
    this->noiDung = noiDung;
    this->thoiGian = DateFormat::normalizeDateTime(thoiGian);
}

string GiaoDichDiem::getMaGiaoDich() const
{
    return maGiaoDich;
}

string GiaoDichDiem::getMaKhachHang() const
{
    return maKhachHang;
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

string GiaoDichDiem::toCSV() const
{
    stringstream ss;
    ss << maGiaoDich << ","
       << maKhachHang << ","
       << loaiGiaoDich << ","
       << soDiem << ","
       << noiDung << ","
       << thoiGian;
    return ss.str();
}

GiaoDichDiem GiaoDichDiem::fromCSV(const string& line)
{
    stringstream ss(line);
    string maGD, maKH, loaiGD, strSoDiem, nd, tg;

    getline(ss, maGD, ',');
    getline(ss, maKH, ',');
    getline(ss, loaiGD, ',');
    getline(ss, strSoDiem, ',');
    getline(ss, nd, ',');
    getline(ss, tg, ',');

    int soDiem = 0;
    if (!strSoDiem.empty()) {
        soDiem = atoi(strSoDiem.c_str());
    }

    return GiaoDichDiem(maGD, maKH, loaiGD, soDiem, nd, tg);
}

void GiaoDichDiem::hienThi() const
{
    cout << "Ma giao dich: " << maGiaoDich << endl;
    cout << "Ma khach hang: " << maKhachHang << endl;
    cout << "Loai giao dich: " << loaiGiaoDich << endl;
    cout << "So diem: " << soDiem << endl;
    cout << "Noi dung: " << noiDung << endl;
    cout << "Thoi gian: " << thoiGian << endl;
}

Customer::Customer()
{
    maKH = "";
    hoTen = "";
    soDienThoai = "";
    email = "";
    ngaySinh = "";
    gioiTinh = "";
    cccd = "";

    diemTichLuy = 0;
    hangThanhVien = "Thuong";
    tongChiTieu = 0;
}

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
    ngaySinh = "";
    gioiTinh = "";
    cccd = "";

    diemTichLuy = 0;
    hangThanhVien = "Thuong";
    tongChiTieu = 0;
}

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
    ngaySinh = "";
    gioiTinh = "";
    cccd = "";

    this->diemTichLuy = diemTichLuy;
    this->hangThanhVien = hangThanhVien;
    this->tongChiTieu = tongChiTieu;
}

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

string Customer::getNgaySinh() const
{
    return ngaySinh;
}

string Customer::getGioiTinh() const
{
    return gioiTinh;
}

string Customer::getCCCD() const
{
    return cccd;
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

void Customer::setNgaySinh(string ngaySinh)
{
    this->ngaySinh = DateFormat::normalizeDate(ngaySinh);
}

void Customer::setGioiTinh(string gioiTinh)
{
    this->gioiTinh = gioiTinh;
}

void Customer::setCCCD(string cccd)
{
    this->cccd = cccd;
}

string Customer::toCSV() const
{
    stringstream ss;
    ss << maKH << ","
       << diemTichLuy << ","
       << hangThanhVien << ","
       << tongChiTieu;
    return ss.str();
}

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
        maKH,
        "TichDiem",
        soDiem,
        noiDung,
        thoiGian);

    lichSuDiem.push_back(giaoDich);

    capNhatHangThanhVien();
}

bool Customer::doiDiem(
    int soDiem,
    string noiDung,
    string thoiGian)
{
    if (soDiem <= 0)
    {
        return false;
    }

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
        maKH,
        "DoiDiem",
        soDiem,
        noiDung,
        thoiGian);

    lichSuDiem.push_back(giaoDich);

    capNhatHangThanhVien();

    return true;
}

void Customer::themChiTieu(long long soTien)
{
    if (soTien <= 0)
    {
        return;
    }

    tongChiTieu += soTien;

    capNhatHangThanhVien();
}

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

void Customer::themGiaoDich(GiaoDichDiem giaoDich)
{
    lichSuDiem.push_back(giaoDich);
}

void Customer::hienThiLichSuDiem() const
{
    cout << endl;
    cout << "========== LICH SU TICH / DOI DIEM ==========" << endl;

    if (lichSuDiem.empty())
    {
        cout << "Khach hang chua co giao dich diem." << endl;
        return;
    }

    for (vector<GiaoDichDiem>::const_iterator it = lichSuDiem.begin();
     it != lichSuDiem.end();
     ++it)
    {
        const GiaoDichDiem& giaoDich = *it;
        cout << endl;
        giaoDich.hienThi();
        cout << "---------------------------------------------" << endl;
    }
}

void Customer::hienThiThongTin() const
{
    cout << endl;
    cout << "========== THONG TIN KHACH HANG ==========" << endl;
    cout << "Ma KH: " << maKH << endl;
    cout << "Ho ten: " << hoTen << endl;
    cout << "So dien thoai: " << soDienThoai << endl;
    cout << "Email: " << email << endl;
    cout << "Ngay sinh: " << ngaySinh << endl;
    cout << "Gioi tinh: " << gioiTinh << endl;
    cout << "Diem tich luy: " << diemTichLuy << endl;
    cout << "Hang thanh vien: " << hangThanhVien << endl;
    cout << "Tong chi tieu: " << tongChiTieu << " VND" << endl;
}
