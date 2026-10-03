#ifndef CUSTOMER_H
#define CUSTOMER_H

#include <iostream>
#include <string>
#include <vector>

using namespace std;

// =====================================================
// LICH SU GIAO DICH DIEM
// =====================================================

class GiaoDichDiem
{
private:
    string maGiaoDich;
    string tenKhachHang;
    string soDienThoai;

    // TichDiem / DoiDiem
    string loaiGiaoDich;

    int soDiem;

    string noiDung;
    string thoiGian;

public:
    // Constructor
    GiaoDichDiem();

    GiaoDichDiem(
        string maGiaoDich,
        string tenKhachHang,
        string soDienThoai,
        string loaiGiaoDich,
        int soDiem,
        string noiDung,
        string thoiGian);

    // Getter
    string getMaGiaoDich() const;
    string getTenKhachHang() const;
    string getSoDienThoai() const;
    string getLoaiGiaoDich() const;
    int getSoDiem() const;
    string getNoiDung() const;
    string getThoiGian() const;

    // Hien thi
    void hienThi() const;
};


// =====================================================
// LOP CUSTOMER
// KHACH HANG CRM
// =====================================================

class Customer
{
private:
    // Thong tin khach hang
    string maKH;
    string hoTen;
    string soDienThoai;
    string email;

    // CRM
    int diemTichLuy;

    // Thuong / ThanThiet / VIP
    string hangThanhVien;

    // Don vi: VND
    long long tongChiTieu;

    // Lich su tich/doi diem
    vector<GiaoDichDiem> lichSuDiem;

public:
    // =================================================
    // CONSTRUCTOR
    // =================================================

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

    // =================================================
    // GETTER
    // =================================================

    string getMaKH() const;
    string getHoTen() const;
    string getSoDienThoai() const;
    string getEmail() const;

    int getDiemTichLuy() const;
    string getHangThanhVien() const;
    long long getTongChiTieu() const;

    // =================================================
    // SETTER
    // =================================================

    void setHoTen(string hoTen);
    void setSoDienThoai(string soDienThoai);
    void setEmail(string email);

    // =================================================
    // QUAN LY DIEM
    // =================================================

    // Tich diem
    void tichDiem(
        int soDiem,
        string noiDung,
        string thoiGian);

    // Doi diem
    bool doiDiem(
        int soDiem,
        string noiDung,
        string thoiGian);

    // =================================================
    // CHI TIEU
    // =================================================

    void themChiTieu(long long soTien);

    // =================================================
    // HANG THANH VIEN
    // =================================================

    void capNhatHangThanhVien();

    // =================================================
    // LICH SU
    // =================================================

    void themGiaoDich(
        GiaoDichDiem giaoDich);

    void hienThiLichSuDiem() const;

    // =================================================
    // HIEN THI
    // =================================================

    void hienThiThongTin() const;
};

#endif
