#ifndef USER_H
#define USER_H

#include <iostream>
#include <string>

using namespace std;

class Staff;

// =====================================================
// LOP CHA USER
// =====================================================

class User
{
protected:
    // Thong tin chung
    string maNguoiDung;
    string hoTen;
    string soDienThoai;
    string ngaySinh;
    string gioiTinh;
    string email;
    string cccd;

    // Thong tin tai khoan
    // Ca Admin va Staff deu co
    string taiKhoan;
    string matKhau;
    string vaiTro;

public:
    // Constructor
    User();

    User(string maNguoiDung,
         string hoTen,
         string soDienThoai,
         string ngaySinh,
         string gioiTinh,
         string email,
         string cccd,
         string taiKhoan,
         string matKhau,
         string vaiTro);

    virtual ~User();

    // =========================
    // GETTER
    // =========================

    string getMaNguoiDung() const;
    string getHoTen() const;
    string getSoDienThoai() const;
    string getNgaySinh() const;
    string getGioiTinh() const;
    string getEmail() const;
    string getCCCD() const;

    string getTaiKhoan() const;
    string getVaiTro() const;

    // =========================
    // SETTER
    // =========================

    void setHoTen(string hoTen);
    void setSoDienThoai(string soDienThoai);
    void setNgaySinh(string ngaySinh);
    void setGioiTinh(string gioiTinh);
    void setEmail(string email);

    // Dung khi cap tai khoan
    void setTaiKhoan(string taiKhoan);
    void setMatKhau(string matKhau);

    // =========================
    // KIEM TRA DANG NHAP
    // =========================

    virtual bool kiemTraDangNhap(
        string taiKhoan,
        string matKhau) const;

    // =========================
    // HIEN THI
    // =========================

    virtual void hienThiThongTin() const;

    virtual void hienThiChucNang() const = 0;
};


// =====================================================
// LOP ADMIN
// =====================================================

class Admin : public User
{
public:
    Admin();

    Admin(string maNguoiDung,
          string hoTen,
          string soDienThoai,
          string ngaySinh,
          string gioiTinh,
          string email,
          string cccd,
          string taiKhoan,
          string matKhau);

    // =========================
    // QUAN LY TAI KHOAN STAFF
    // =========================

    void capTaiKhoanStaff(
        Staff& staff,
        string taiKhoan,
        string matKhau);

    void xoaTaiKhoanStaff(Staff& staff);

    void khoaTaiKhoanStaff(Staff& staff);

    void moKhoaTaiKhoanStaff(Staff& staff);

    // =========================
    // QUAN LY TRANG THAI LAM VIEC
    // =========================

    void choNghiViecStaff(Staff& staff);

    void choLamLaiStaff(Staff& staff);

    // =========================
    // HIEN THI
    // =========================

    void hienThiThongTin() const override;

    void hienThiChucNang() const override;
};


// =====================================================
// LOP STAFF
// =====================================================

class Staff : public User
{
private:
    // DangLam / DaNghiViec
    string trangThaiLamViec;

    // HoatDong / BiKhoa / ChuaCoTaiKhoan
    string trangThaiTaiKhoan;

    // true = da co tai khoan
    // false = chua co tai khoan
    bool coTaiKhoan;

    // Admin duoc phep thay doi
    friend class Admin;

public:
    // Constructor mac dinh
    Staff();

    // Constructor khi them nhan vien moi
    Staff(string maNguoiDung,
          string hoTen,
          string soDienThoai,
          string ngaySinh,
          string gioiTinh,
          string email,
          string cccd);

    // Constructor day du
    // Dung khi doc Staff tu CSV
    Staff(string maNguoiDung,
          string hoTen,
          string soDienThoai,
          string ngaySinh,
          string gioiTinh,
          string email,
          string cccd,
          string taiKhoan,
          string matKhau,
          string trangThaiLamViec,
          string trangThaiTaiKhoan,
          bool coTaiKhoan);

    // =========================
    // GETTER
    // =========================

    string getTrangThaiLamViec() const;

    string getTrangThaiTaiKhoan() const;

    bool getCoTaiKhoan() const;

    // =========================
    // KIEM TRA DANG NHAP
    // =========================

    bool kiemTraDangNhap(
        string taiKhoan,
        string matKhau) const override;

    // =========================
    // HIEN THI
    // =========================

    void hienThiThongTin() const override;

    void hienThiChucNang() const override;
};

#endif
