#ifndef DATABASE_H
#define DATABASE_H

#include <vector>
#include <string>
#include <utility>
#include "Customer.h"

// Khai báo forward các l?p mô hình tuong ?ng
class Seat;
class FoodAndBeverage;
class Promotion;
class Invoice;
class Room;
class Movie;
class Showtime;
class Ticket;
class ShiftReport;
class User;

using namespace std;

class Database {
public:
    Database();

    // --- NHÓM 1: QU?N LÝ NHÂN S? & KHÁCH HÀNG (quan_ly_nhan_su.txt) ---
    // (Bao g?m Users, Customers, CustomerPointsHistory n?u c?n)
    // Thêm các dòng này vào bên trong khai báo class Database (trong Database.h)
    // --- Qu?n lý Khách hàng & Tích di?m (CRM) ---
	void docDanhSachUsers(vector<Customer>& danhSachKH); // Ð?c t? b?ng [USERS] và [CUSTOMERS]
	string layTenNhanVienQuaMa(string maNhanVien);
	void ghiDanhSachUsers(const vector<Customer>& danhSachKH); // Ghi d?ng b?
	void docLichSuGiaoDichDiem(vector<GiaoDichDiem>& danhSachGD);
	void ghiLichSuGiaoDichDiem(const vector<GiaoDichDiem>& danhSachGD);
// --- Qu?n lý Th? lo?i & Phim ---
	// --- Qu?n lý Th? lo?i & Liên k?t Phim - Th? lo?i ---
	void docDanhSachTheLoai(vector<pair<string, string> >& danhSachTheLoai);
	void docMovieGenres(vector<pair<string, string> >& movieGenres);
	string layTheLoaiCuaPhim(string maPhimToFind);
	void themTheLoaiChoPhim(string maPhim, string tenTheLoaiMoi);
	void capNhatTheLoaiChoPhim(string maPhim, string tenTheLoaiMoi);
    // --- NHÓM 2: KHO PHIM & H? T?NG R?P (quan_ly_phim_co_so.txt) ---
    void docDanhSachPhong(vector<Room>& danhSachPhong);
    void ghiDanhSachPhong(const vector<Room>& danhSachPhong);

    void docDanhSachGhe(vector<Seat>& danhSachGhe);
    void ghiDanhSachGhe(const vector<Seat>& danhSachGhe);
    void docTrangThaiGheSuatChieu(const string& maSuatChieuCanTim, vector<pair<string, string> >& trangThaiGheList);
    // Hàm t? d?ng sinh gh? d?a theo s?c ch?a phòng (dã bàn ? bu?c trên)
    void taoGheTuDongChoPhong(const string& maPhongCanTao, int sucChuaPhong);
    
    void docDanhSachPhim(vector<Movie>& danhSachPhim);
    void ghiDanhSachPhim(const vector<Movie>& danhSachPhim);

    // --- NHÓM 3: L?CH CHI?U & VÉ (lich_va_ve.txt) ---
    void docDanhSachSuatChieu(vector<Showtime>& danhSachSuatChieu);
    void ghiDanhSachSuatChieu(const vector<Showtime>& danhSachSuatChieu);
    // C?p nh?t ho?c thêm m?i tr?ng thái gh? theo su?t chi?u vào lich_va_ve.csv
	void capNhatTrangThaiGheSuatChieu(const string& maSuatChieu, const string& maGhe, const string& trangThaiMoi);

    void docDanhSachVe(vector<Ticket>& danhSachVe);
    void ghiDanhSachVe(const vector<Ticket>& danhSachVe);

    // --- NHÓM 4: DOANH THU, F&B & KHUY?N MÃI (giao_dich_fnb.txt) ---
    void docDanhSachFAndB(vector<FoodAndBeverage>& danhSachFAndB);
    void ghiDanhSachFAndB(const vector<FoodAndBeverage>& danhSachFAndB);

    void docDanhSachKhuyenMai(vector<Promotion>& danhSachKhuyenMai);
    void ghiDanhSachKhuyenMai(const vector<Promotion>& danhSachKhuyenMai);

    void docDanhSachHoaDon(vector<Invoice>& danhSachHoaDon);
    void ghiDanhSachHoaDon(const vector<Invoice>& danhSachHoaDon);

    // --- NHÓM 5: V?N HÀNH H? TH?NG (van_hanh_he_thong.txt) ---
    void docDanhSachCa(vector<ShiftReport>& danhSachCa);
	void ghiDanhSachCa(const vector<ShiftReport>& danhSachCa);
};

#endif
