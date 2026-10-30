#ifndef USER_H
#define USER_H

#include <iostream>
#include <string>

using namespace std;

class Staff;

class User
{
protected:

    string maNguoiDung;
    string hoTen;
    string soDienThoai;
    string ngaySinh;
    string gioiTinh;
    string email;
    string cccd;

    string taiKhoan;
    string matKhau;
    string vaiTro;

public:

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

    string getMaNguoiDung() const;
    string getHoTen() const;
    string getSoDienThoai() const;
    string getNgaySinh() const;
    string getGioiTinh() const;
    string getEmail() const;
    string getCCCD() const;

    string getTaiKhoan() const;
    string getVaiTro() const;

    void setHoTen(string hoTen);
    void setSoDienThoai(string soDienThoai);
    void setNgaySinh(string ngaySinh);
    void setGioiTinh(string gioiTinh);
    void setEmail(string email);

    void setTaiKhoan(string taiKhoan);
    void setMatKhau(string matKhau);

    virtual bool kiemTraDangNhap(
        string taiKhoan,
        string matKhau) const;

    virtual void hienThiThongTin() const;

    virtual void hienThiChucNang() const = 0;
};

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

    void capTaiKhoanStaff(
        Staff& staff,
        string taiKhoan,
        string matKhau);

    void xoaTaiKhoanStaff(Staff& staff);

    void khoaTaiKhoanStaff(Staff& staff);

    void moKhoaTaiKhoanStaff(Staff& staff);

    void choNghiViecStaff(Staff& staff);

    void choLamLaiStaff(Staff& staff);

    void hienThiThongTin() const override;

    void hienThiChucNang() const override;
};

class Staff : public User
{
private:

    string trangThaiLamViec;

    string trangThaiTaiKhoan;

    bool coTaiKhoan;

    friend class Admin;

public:

    Staff();

    Staff(string maNguoiDung,
          string hoTen,
          string soDienThoai,
          string ngaySinh,
          string gioiTinh,
          string email,
          string cccd);

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

    string getTrangThaiLamViec() const;

    string getTrangThaiTaiKhoan() const;

    bool getCoTaiKhoan() const;

    bool kiemTraDangNhap(
        string taiKhoan,
        string matKhau) const override;

    void hienThiThongTin() const override;

    void hienThiChucNang() const override;
};

#endif
