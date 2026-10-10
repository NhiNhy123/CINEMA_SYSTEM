#ifndef PROMOTION_H
#define PROMOTION_H

#include <string>
#include <vector>

using namespace std;

// Forward declaration d? tích h?p v?i Database chung
class Database;

class Promotion {
private:
    string maKhuyenMai;       // Mã d?nh danh khuy?n mãi (PK)
    string tenKhuyenMai;      // Tên chuong trình uu dãi
    string maCode;            // Mã code dùng d? nh?p khi thanh toán (Unique)
    double phanTramGiam;      // Ph?n tram gi?m giá (%)
    double mucGiamToiDa;      // M?c gi?m ti?n t?i da (VNÐ)
    string ngayBatDau;        // Ngày hi?u l?c b?t d?u (YYYY-MM-DD ho?c DD/MM/YYYY)
    string ngayKetThuc;       // Ngày h?t hi?u l?c
    string phamViApDung;      // "All" ho?c mã phim c? th? (VD: P01)

public:
    Promotion();
    Promotion(string maKM, string tenKM, string code, double ptGiam, double maxGiam, string start, string end, string phamVi);

    // Getters
    string getMaKhuyenMai() const;
    string getTenKhuyenMai() const;
    string getMaCode() const;
    double getPhanTramGiam() const;
    double getMucGiamToiDa() const;
    string getNgayBatDau() const;
    string getNgayKetThuc() const;
    string getPhamViApDung() const;

    // Setters
    void setTenKhuyenMai(const string& ten);
    void setMaCode(const string& code);
    void setPhanTramGiam(double pt);
    void setMucGiamToiDa(double maxGia);
    void setNgayBatDau(const string& start);
    void setNgayKetThuc(const string& end);
    void setPhamViApDung(const string& pv);
    string toCSV() const;
    Promotion fromCSV(const string& line);

    // Hi?n th? thông tin khuy?n mãi ng?n g?n
    void hienThi() const;
};

class PromotionManager {
private:
    vector<Promotion> danhSachKhuyenMai;

public:
    PromotionManager();

    // T?i d? li?u t? file thông qua Database
    void taiDuLieu(Database& db);

    // --- PHÂN H? 1: QU?N TR? VIÊN (ADMIN) ---
    void themKhuyenMai(Database& db);
    void suaKhuyenMai(Database& db);
    void xoaKhuyenMai(Database& db);
    void hienThiDanhSach() const;

    // --- PHÂN H? 2: NHÂN VIÊN (STAFF) ---
    // Ki?m tra & áp d?ng mã voucher cho hóa don thanh toán c?a phim c? th?
    bool apDungVoucher(const string& maCodeNhap, const string& maPhimChieu, double tongTienDonHang, double& soTienDuocGiam) const;
};

#endif
