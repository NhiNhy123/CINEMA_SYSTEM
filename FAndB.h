#ifndef FANDB_H
#define FANDB_H

#include <string>
#include <vector>

using namespace std;

// Forward declaration d? tích h?p v?i Database chung
class Database;

class FoodAndBeverage {
private:
    string maMatHang;         // Mã d?nh danh m?t hàng (PK)
    string tenMatHang;        // Tên s?n ph?m (Ví d?: B?p ng?t l?n, Coca-Cola)
    string loai;              // Nhóm s?n ph?m: Bap, Nuoc, Combo
    double giaTien;           // Giá bán niêm y?t (> 0)
    string trangThaiKinhDoanh;// DangBan ho?c NgungBan

public:
    FoodAndBeverage();
    FoodAndBeverage(string maMatHang, string tenMatHang, string loai, double giaTien, string trangThaiKinhDoanh);

    // Getters
    string getMaMatHang() const;
    string getTenMatHang() const;
    string getLoai() const;
    double getGiaTien() const;
    string getTrangThaiKinhDoanh() const;

    // Setters
    void setTenMatHang(const string& ten);
    void setLoai(const string& l);
    void setGiaTien(double gia);
    void setTrangThaiKinhDoanh(const string& trangThai);
    string toCSV() const;
    FoodAndBeverage fromCSV(const string& line);
    // Hi?n th? thông tin m?t hàng ng?n g?n
    void hienThi() const;
};

class FAndBManager {
private:
    vector<FoodAndBeverage> danhSachFAndB;

public:
    FAndBManager();

    // T?i d? li?u t? file thông qua Database
    void taiDuLieu(Database& db);

    // --- PHÂN H? 1: QU?N TR? VIÊN (ADMIN) ---
    void themMatHang(Database& db);
    void suaMatHang(Database& db);
    void xoaMatHang(Database& db);
    void capNhatTrangThaiKinhDoanh(Database& db);
    void hienThiDanhSach() const;

    // --- PHÂN H? 2: NHÂN VIÊN (STAFF) ---
    // Ch?n s?n ph?m F&B => nh?p s? lu?ng => tính t?ng ti?n t?m tính
    void chonMuaFAndB() const;
};

#endif
