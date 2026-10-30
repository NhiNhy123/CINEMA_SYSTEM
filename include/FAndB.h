#ifndef FANDB_H
#define FANDB_H

#include <string>
#include <vector>

using namespace std;

class Database;

class FoodAndBeverage {
private:
    string maMatHang;
    string tenMatHang;
    string loai;
    double giaTien;
    string trangThaiKinhDoanh;

public:
    FoodAndBeverage();
    FoodAndBeverage(string maMatHang, string tenMatHang, string loai, double giaTien, string trangThaiKinhDoanh);

    string getMaMatHang() const;
    string getTenMatHang() const;
    string getLoai() const;
    double getGiaTien() const;
    string getTrangThaiKinhDoanh() const;

    void setTenMatHang(const string& ten);
    void setLoai(const string& l);
    void setGiaTien(double gia);
    void setTrangThaiKinhDoanh(const string& trangThai);
    string toCSV() const;
    FoodAndBeverage fromCSV(const string& line);

    void hienThi() const;
};

class FAndBManager {
private:
    vector<FoodAndBeverage> danhSachFAndB;

public:
    FAndBManager();

    void taiDuLieu(Database& db);

    void themMatHang(Database& db);
    void suaMatHang(Database& db);
    void xoaMatHang(Database& db);
    void capNhatTrangThaiKinhDoanh(Database& db);
    void hienThiDanhSach() const;

    void chonMuaFAndB() const;
};

#endif
