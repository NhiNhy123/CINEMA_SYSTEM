#ifndef CUSTOMER_H
#define CUSTOMER_H

#include <iostream>
#include <string>
#include <vector>

using namespace std;

class GiaoDichDiem
{
private:
    string maGiaoDich;
    string maKhachHang;
    string loaiGiaoDich;
    int soDiem;
    string noiDung;
    string thoiGian;

public:

    GiaoDichDiem();

    GiaoDichDiem(
        string maGiaoDich,
        string maKhachHang,
        string loaiGiaoDich,
        int soDiem,
        string noiDung,
        string thoiGian);

    string getMaGiaoDich() const;
    string getMaKhachHang() const;
    string getLoaiGiaoDich() const;
    int getSoDiem() const;
    string getNoiDung() const;
    string getThoiGian() const;

    string toCSV() const;
    static GiaoDichDiem fromCSV(const string& line);

    void hienThi() const;
};

class Customer
{
private:

    string maKH;
    string hoTen;
    string soDienThoai;
    string email;
    string ngaySinh;
    string gioiTinh;
    string cccd;

    int diemTichLuy;
    string hangThanhVien;
    long long tongChiTieu;

    vector<GiaoDichDiem> lichSuDiem;

public:

    Customer();

    Customer(
        string maKH,
        string hoTen,
        string soDienThoai,
        string email);

    Customer(
        string maKH,
        string hoTen,
        string soDienThoai,
        string email,
        int diemTichLuy,
        string hangThanhVien,
        long long tongChiTieu);

    string getMaKH() const;
    string getHoTen() const;
    string getSoDienThoai() const;
    string getEmail() const;
    string getNgaySinh() const;
    string getGioiTinh() const;
    string getCCCD() const;

    int getDiemTichLuy() const;
    string getHangThanhVien() const;
    long long getTongChiTieu() const;

    void setHoTen(string hoTen);
    void setSoDienThoai(string soDienThoai);
    void setEmail(string email);
    void setNgaySinh(string ngaySinh);
    void setGioiTinh(string gioiTinh);
    void setCCCD(string cccd);

    string toCSV() const;
    static Customer fromCSV(const string& line);

    void tichDiem(
        int soDiem,
        string noiDung,
        string thoiGian);

    bool doiDiem(
        int soDiem,
        string noiDung,
        string thoiGian);

    void themChiTieu(long long soTien);
    void capNhatHangThanhVien();

    void themGiaoDich(GiaoDichDiem giaoDich);
    void hienThiLichSuDiem() const;
    void hienThiThongTin() const;
};

#endif
