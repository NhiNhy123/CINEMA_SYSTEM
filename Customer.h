#ifndef CUSTOMER_H
#define CUSTOMER_H

#include <iostream>
#include <string>
#include <vector>

using namespace std;

// =====================================================
// LOP GIAO DICH DIEM (B?ng [CUSTOMER_POINTS_HISTORY])
// =====================================================

class GiaoDichDiem
{
private:
    string maGiaoDich;
    string maKhachHang; // Liên k?t khóa ngo?i t?i b?ng [CUSTOMERS] và [USERS]
    string loaiGiaoDich; // TichDiem / DoiDiem
    int soDiem;
    string noiDung;
    string thoiGian;

public:
    // Constructor m?c d?nh
    GiaoDichDiem();

    // Constructor chu?n kh?p 6 c?t c?a [CUSTOMER_POINTS_HISTORY]
    GiaoDichDiem(
        string maGiaoDich,
        string maKhachHang,
        string loaiGiaoDich,
        int soDiem,
        string noiDung,
        string thoiGian);

    // Getter
    string getMaGiaoDich() const;
    string getMaKhachHang() const;
    string getLoaiGiaoDich() const;
    int getSoDiem() const;
    string getNoiDung() const;
    string getThoiGian() const;
    
    string toCSV() const;
    static GiaoDichDiem fromCSV(const string& line);

    // Hien thi
    void hienThi() const;
};


// =====================================================
// LOP CUSTOMER (B?ng [CUSTOMERS] & thông tin liên k?t [USERS])
// =====================================================

class Customer
{
private:
    // Thông tin d?nh danh (Th?c ch?t du?c ánh x? t? b?ng [USERS] khi d?c Database)
    string maKH;
    string hoTen;
    string soDienThoai;
    string email;

    // D? li?u CRM chính th?c luu trong b?ng [CUSTOMERS]
    int diemTichLuy;
    string hangThanhVien; // Thuong / ThanThiet / VIP
    long long tongChiTieu; // Ðon v?: VND

    // Lich su tich/doi diem
    vector<GiaoDichDiem> lichSuDiem;

public:
    // =================================================
    // CONSTRUCTOR
    // =================================================

    Customer();

    // Dùng khi t?o khách hàng m?i
    Customer(
        string maKH,
        string hoTen,
        string soDienThoai,
        string email);

    // Dùng khi d?c d?y d? t? Database (ghép t? [USERS] và [CUSTOMERS])
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
    // QUAN LY DIEM & CSV
    // =================================================
    
    // Xu?t ra CSV ch? ghi dúng 4 tru?ng c?a b?ng [CUSTOMERS]: maKhachHang,diemTichLuy,hangThanhVien,tongChiTieu
    string toCSV() const;
    static Customer fromCSV(const string& line);

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
    // CHI TIEU & H?NG THÀNH VIÊN
    // =================================================

    void themChiTieu(long long soTien);
    void capNhatHangThanhVien();

    // =================================================
    // LICH SU & HI?N TH?
    // =================================================

    void themGiaoDich(GiaoDichDiem giaoDich);
    void hienThiLichSuDiem() const;
    void hienThiThongTin() const;
};

#endif
