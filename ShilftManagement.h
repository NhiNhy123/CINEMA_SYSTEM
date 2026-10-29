#ifndef SHIFT_MANAGEMENT_H
#define SHIFT_MANAGEMENT_H

#include <string>
#include <vector>
#include "Database.h"

using namespace std;

class ShiftReport {
private:
    string maCa;
    string maNhanVien;
    string tenNhanVien;
    string ngayLam;
    string gioBatDau;
    string gioKetThuc;
    long long doanhThuHeThong;
    long long tienThucTe;
    long long chenhLech;
    string trangThai;
    bool daKhoaBaoCao;

    void tinhChenhLech();

public:
    ShiftReport();
    ShiftReport(string maCa, string maNhanVien, string tenNhanVien, string ngayLam, 
                string gioBatDau, string gioKetThuc, long long doanhThuHeThong, 
                long long tienThucTe, string trangThai, bool daKhoaBaoCao);

    // Getters
    string getMaCa() const;
    string getMaNhanVien() const;
    string getTenNhanVien() const;
    string getNgayLam() const;
    string getGioBatDau() const;
    string getGioKetThuc() const;
    long long getDoanhThuHeThong() const;
    long long getTienThucTe() const;
    long long getChenhLech() const;
    string getTrangThai() const;
    bool getDaKhoaBaoCao() const;

    // Setters & Actions
    void setTienThucTe(long long tienThucTe);
    void duyetCa();
    void khoaBaoCao();
    void hienThi() const;
};

class ShiftManagement {
private:
    vector<ShiftReport> danhSachCa;

public:
    ShiftManagement();

    void taiDuLieu(Database& db);
    void luuDuLieu(Database& db) const;

    void themBaoCaoCa(ShiftReport ca);
    
    // �� kh?p v?i h�m chotCa t? d?ng l?y th?i gian h? th?ng trong file .cpp
    void chotCa(string maCa, string maNhanVien, string gioBatDau, long long doanhThuHeThong, long long tienThucTe, Database& db);

    ShiftReport* timCa(string maCa);
    bool duyetBaoCao(string maCa, Database& db);
    bool khoaBaoCao(string maCa, Database& db);

    // C�c h�m l?c v� th?ng k�
    vector<ShiftReport> locTheoThoiGian(string tuNgay, string denNgay) const;
    vector<ShiftReport> locTheoNhanVien(string maNhanVien) const;
    vector<ShiftReport> locBaoCao(string tuNgay, string denNgay, string maNhanVien) const;

    void hienThiBaoCaoNhanVien(string maNhanVien, string ngayLam) const;
    void hienThiCaChoDuyet() const;
    void hienThiCaDaXuLy() const;

    long long tinhTongDoanhThu() const;
    long long tinhTongChenhLech() const;
    void hienThiTongHop() const;
    void hienThiTatCa() const;
};

#endif
