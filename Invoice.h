#ifndef INVOICE_H
#define INVOICE_H

#include <string>
#include <vector>

using namespace std;

// Forward declaration d? tích h?p v?i Database chung
class Database;

// C?u trúc chi ti?t F&B trong hóa don (B?ng trung gian Invoice_FB)
struct InvoiceFBItem {
    string maHoaDon;
    string maMatHang;
    int soLuong;
    double thanhTien;
};

class Invoice {
private:
    string maHoaDon;          // Mã d?nh danh hóa don (PK)
    string maKhachHang;       // Khách hàng th?c hi?n giao d?ch (Nullable)
    string maNhanVien;        // Nhân viên thu ngân t?o don (FK t?i Users)
    string thoiGianLap;       // Th?i di?m kh?i t?o và thanh toán
    double tongTien;          // T?ng giá tr? thanh toán cu?i cùng
    string hinhThucThanhToan; // TienMat, TheNganHang, QueQR
    string maGiaoDich;        // Mã giao d?ch ngân hàng/ví di?n t? (Nullable)
    string trangThaiThanhToan;// ChuaTra, DaTra

    // Danh sách chi ti?t F&B di kèm trong hóa don này
    vector<InvoiceFBItem> chiTietFB;

public:
    Invoice();
    Invoice(string maHD, string maKH, string maNV, string thoiGian, double tongT, string hinhThuc, string maGD, string trangThai);

    // Getters
    string getMaHoaDon() const;
    string getMaKhachHang() const;
    string getMaNhanVien() const;
    string getThoiGianLap() const;
    double getTongTien() const;
    string getHinhThucThanhToan() const;
    string getMaGiaoDich() const;
    string getTrangThaiThanhToan() const;
    const vector<InvoiceFBItem>& getChiTietFB() const;

    // Setters & Methods
    void themChiTietFB(const InvoiceFBItem& item);
    void setTrangThaiThanhToan(const string& trangThai);
    string toCSV() const;
    Invoice fromCSV(const string& line);
    // Hi?n th? chi ti?t hóa don
    void hienThi() const;
};

class InvoiceManager {
private:
    vector<Invoice> danhSachHoaDon;

public:
    InvoiceManager();

    // T?i d? li?u hóa don và chi ti?t F&B t? file thông qua Database
    void taiDuLieu(Database& db);

    // --- PHÂN H? 2: NHÂN VIÊN (STAFF) - THANH TOÁN & XU?T HÓA ÐON ---
    void taoHoaDonMoi(Database& db, const string& maNV, const string& maKH, double tongTienHang, 
                      const vector<InvoiceFBItem>& dsFB, const string& phimChieuCode);

    // --- PHÂN H? 1: QU?N TR? VIÊN (ADMIN) - QU?N LÝ DOANH THU & TH?NG KÊ ---
    void hienThiLichSuGiaoDich() const;
    void locDoanhThuTheoThoiGian(const string& tuNgay, const string& denNgay) const;
    void thongKeTongQuanDoanhThu() const;
};

#endif
