#ifndef PROMOTION_H
#define PROMOTION_H

#include <string>
#include <vector>

using namespace std;

class Database;

class Promotion {
private:
    string maKhuyenMai;
    string tenKhuyenMai;
    string maCode;
    double phanTramGiam;
    double mucGiamToiDa;
    string ngayBatDau;
    string ngayKetThuc;
    string phamViApDung;

public:
    Promotion();
    Promotion(string maKM, string tenKM, string code, double ptGiam, double maxGiam, string start, string end, string phamVi);

    string getMaKhuyenMai() const;
    string getTenKhuyenMai() const;
    string getMaCode() const;
    double getPhanTramGiam() const;
    double getMucGiamToiDa() const;
    string getNgayBatDau() const;
    string getNgayKetThuc() const;
    string getPhamViApDung() const;

    void setTenKhuyenMai(const string& ten);
    void setMaCode(const string& code);
    void setPhanTramGiam(double pt);
    void setMucGiamToiDa(double maxGia);
    void setNgayBatDau(const string& start);
    void setNgayKetThuc(const string& end);
    void setPhamViApDung(const string& pv);
    string toCSV() const;
    Promotion fromCSV(const string& line);

    void hienThi() const;
};

class PromotionManager {
private:
    vector<Promotion> danhSachKhuyenMai;

public:
    PromotionManager();

    void taiDuLieu(Database& db);

    void themKhuyenMai(Database& db);
    void suaKhuyenMai(Database& db);
    void xoaKhuyenMai(Database& db);
    void hienThiDanhSach() const;

    bool apDungVoucher(const string& maCodeNhap, const string& maPhimChieu, double tongTienDonHang, double& soTienDuocGiam) const;
};

#endif
