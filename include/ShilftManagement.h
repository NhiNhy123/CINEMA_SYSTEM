#ifndef SHIFTMANAGEMENT_H
#define SHIFTMANAGEMENT_H

#include <iostream>
#include <string>
#include <vector>

using namespace std;

class Database;

class ShiftReport
{
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

public:

    ShiftReport();

    ShiftReport(
        string maCa,
        string maNhanVien,
        string tenNhanVien,
        string ngayLam,
        string gioBatDau,
        string gioKetThuc,
        long long doanhThuHeThong,
        long long tienThucTe,
        string trangThai = "ChoDuyet",
        bool daKhoaBaoCao = false
    );

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

    void setTienThucTe(long long tienThucTe);

    void tinhChenhLech();

    void duyetCa();

    void khoaBaoCao();

    void hienThi() const;
};

class ShiftManagement
{
private:
    vector<ShiftReport> danhSachCa;

public:
    ShiftManagement();

    void taiDuLieu(Database& db);
    void luuDuLieu(Database& db) const;

    void themBaoCaoCa(ShiftReport ca);

    void chotCa(
        string maCa,
        string maNhanVien,
        string gioBatDau,
        long long doanhThuHeThong,
        long long tienThucTe,
        Database& db
    );

    ShiftReport* timCa(string maCa);

    bool duyetBaoCao(string maCa, Database& db);

    bool khoaBaoCao(string maCa, Database& db);

    vector<ShiftReport> locTheoThoiGian(
        string tuNgay,
        string denNgay
    ) const;

    vector<ShiftReport> locTheoNhanVien(
        string maNhanVien
    ) const;

    vector<ShiftReport> locBaoCao(
        string tuNgay,
        string denNgay,
        string maNhanVien
    ) const;

    void hienThiBaoCaoNhanVien(
        string maNhanVien,
        string ngayLam
    ) const;

    void hienThiCaChoDuyet() const;

    void hienThiCaDaXuLy() const;

    long long tinhTongDoanhThu() const;

    long long tinhTongChenhLech() const;

    void hienThiTongHop() const;

    void hienThiTatCa() const;
};

#endif
