#ifndef INVOICE_H
#define INVOICE_H

#include <string>
#include <vector>

using namespace std;

class Database;
class ShiftReport;

struct InvoiceFBItem {
    string maHoaDon;
    string maMatHang;
    int soLuong;
    double thanhTien;
};

class Invoice {
private:
    string maHoaDon;
    string maKhachHang;
    string maNhanVien;
    string thoiGianLap;
    double tongTien;
    string hinhThucThanhToan;
    string maGiaoDich;
    string trangThaiThanhToan;

    vector<InvoiceFBItem> chiTietFB;

public:
    Invoice();
    Invoice(string maHD, string maKH, string maNV, string thoiGian, double tongT, string hinhThuc, string maGD, string trangThai);

    string getMaHoaDon() const;
    string getMaKhachHang() const;
    string getMaNhanVien() const;
    string getThoiGianLap() const;
    double getTongTien() const;
    string getHinhThucThanhToan() const;
    string getMaGiaoDich() const;
    string getTrangThaiThanhToan() const;
    const vector<InvoiceFBItem>& getChiTietFB() const;

    void themChiTietFB(const InvoiceFBItem& item);
    void setTrangThaiThanhToan(const string& trangThai);
    string toCSV() const;
    Invoice fromCSV(const string& line);

    void hienThi() const;
};

class InvoiceManager {
private:
    vector<Invoice> danhSachHoaDon;

public:
    InvoiceManager();

    void taiDuLieu(Database& db);

    bool taoHoaDonMoi(Database& db, const string& maNV, const string& maKH, double tongTienHang,
                      const vector<InvoiceFBItem>& dsFB, const string& phimChieuCode,
                      double soTienDoiDiem, const string& maSuatChieu,
                      const vector<string>& dsGheGiu, const string& maPhienGiu);

    void hienThiLichSuGiaoDich() const;
    void locDoanhThuTheoThoiGian(const string& tuNgay, const string& denNgay) const;
    void thongKeTongQuanDoanhThu() const;
    void thongKeDoanhThuTheoCa(const ShiftReport& ca) const;
    void thongKeDoanhThuTheoThuNgan(const string& maThuNgan) const;
    void thongKeDoanhThuTheoPhuongThuc(const string& phuongThuc) const;
};

#endif
