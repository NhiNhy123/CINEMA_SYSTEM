#ifndef SHIFTMANAGEMENT_H
#define SHIFTMANAGEMENT_H

#include <iostream>
#include <string>
#include <vector>

using namespace std;


// =====================================================
// LOP BAO CAO CA LAM VIEC
// =====================================================

class ShiftReport
{
private:
    string maCa;
    string maNhanVien;
    string tenNhanVien;

    // Dinh dang ngay: dd/mm/yyyy
    string ngayLam;

    // Dinh dang gio: HH:mm
    string gioBatDau;
    string gioKetThuc;

    long long doanhThuHeThong;
    long long tienThucTe;
    long long chenhLech;

    // ChoDuyet / DaDuyet
    string trangThai;

    // false: chua khoa
    // true : da khoa
    bool daKhoaBaoCao;

public:
    // Ham dung mac dinh
    ShiftReport();

    // Ham dung day du
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

    // Getter
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

    // Cap nhat tien thuc te
    void setTienThucTe(long long tienThucTe);

    // Tinh chenh lech
    void tinhChenhLech();

    // Duyet ca
    void duyetCa();

    // Khoa bao cao
    void khoaBaoCao();

    // Hien thi thong tin
    void hienThi() const;
};


// =====================================================
// LOP QUAN LY CA LAM VIEC
// =====================================================

class ShiftManagement
{
private:
    vector<ShiftReport> danhSachCa;

public:
    ShiftManagement();

    // Them bao cao ca
    void themBaoCaoCa(ShiftReport ca);

    // Nhan vien chot ca
    void chotCa(
        string maCa,
        string maNhanVien,
        string tenNhanVien,
        string ngayLam,
        string gioBatDau,
        string gioKetThuc,
        long long doanhThuHeThong,
        long long tienThucTe
    );

    // Tim ca theo ma ca
    ShiftReport* timCa(string maCa);

    // Duyet bao cao
    bool duyetBaoCao(string maCa);

    // Khoa bao cao
    bool khoaBaoCao(string maCa);

    // Loc theo khoang thoi gian
    vector<ShiftReport> locTheoThoiGian(
        string tuNgay,
        string denNgay
    ) const;

    // Loc theo nhan vien
    vector<ShiftReport> locTheoNhanVien(
        string maNhanVien
    ) const;

    // Loc theo thoi gian va nhan vien
    vector<ShiftReport> locBaoCao(
        string tuNgay,
        string denNgay,
        string maNhanVien
    ) const;

    // Hien thi bao cao cua nhan vien trong ngay
    void hienThiBaoCaoNhanVien(
        string maNhanVien,
        string ngayLam
    ) const;

    // Hien thi cac ca cho duyet
    void hienThiCaChoDuyet() const;

    // Hien thi cac ca da xu ly
    void hienThiCaDaXuLy() const;

    // Tinh tong doanh thu
    long long tinhTongDoanhThu() const;

    // Tinh tong chenh lech
    long long tinhTongChenhLech() const;

    // Hien thi tong hop
    void hienThiTongHop() const;

    // Hien thi tat ca bao cao
    void hienThiTatCa() const;
};

#endif
