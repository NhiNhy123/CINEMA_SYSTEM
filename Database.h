#ifndef DATABASE_H
#define DATABASE_H

#include <vector>
#include <string>

// Khai báo forward các l?p mô hình tuong ?ng
class Seat;
class FoodAndBeverage;
class Promotion;
class Invoice;
class Room;
class Movie;
class Showtime;
class Ticket;

using namespace std;

class Database {
public:
    Database();

    // --- NHÓM 1: QU?N LÝ NHÂN S? & KHÁCH HÀNG (quan_ly_nhan_su.txt) ---
    // (Bao g?m Users, Customers, CustomerPointsHistory n?u c?n)

    // --- NHÓM 2: KHO PHIM & H? T?NG R?P (quan_ly_phim_co_so.txt) ---
    void docDanhSachPhong(vector<Room>& danhSachPhong);
    void ghiDanhSachPhong(const vector<Room>& danhSachPhong);

    void docDanhSachGhe(vector<Seat>& danhSachGhe);
    void ghiDanhSachGhe(const vector<Seat>& danhSachGhe);

    void docDanhSachPhim(vector<Movie>& danhSachPhim);
    void ghiDanhSachPhim(const vector<Movie>& danhSachPhim);

    // --- NHÓM 3: L?CH CHI?U & VÉ (lich_va_ve.txt) ---
    void docDanhSachSuatChieu(vector<Showtime>& danhSachSuatChieu);
    void ghiDanhSachSuatChieu(const vector<Showtime>& danhSachSuatChieu);

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
};

#endif
