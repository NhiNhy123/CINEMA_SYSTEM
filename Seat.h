#ifndef SEAT_H
#define SEAT_H

#include <string>
#include <vector>

using namespace std;

// Forward declaration d? SeatManager nh?n di?n Database
class Database;

class Seat {
private:
    string maGhe;      // VD: A1, B2
    string maPhong;    // Thu?c phòng nào (VD: R01)
    string loaiGhe;    // Standard, VIP, Couple, IMAX...
    double donGiaGhe;  // Ðon giá co b?n (> 0)
    // Ðã lu?c b? tru?ng `trangThai` vì tr?ng thái gh? du?c qu?n lý riêng theo su?t chi?u trong [SEAT_STATUS]

public:
    Seat();
    Seat(string maGhe, string maPhong, string loaiGhe, double donGiaGhe);

    // Getters
    string getMaGhe() const;
    string getMaPhong() const;
    string getLoaiGhe() const;
    double getDonGiaGhe() const;

    // Setters
    void setLoaiGhe(const string& loai);
    void setDonGiaGhe(double gia);
    
    string toCSV() const;
    static Seat fromCSV(const string& line);

    // Hi?n th? thông tin gh? ng?n g?n
    void hienThi() const;
};

class SeatManager {
private:
    vector<Seat> danhSachGhe;

public:
    SeatManager();

    // T?i d? li?u gh? v?t lý thông qua Database chung
    void taiDuLieu(Database& db);

    // Phân h? Admin: T?o gh? hàng lo?t & Sao chép b? c?c
    void taoGheTuDongChoPhong(const string& maPhong, Database& db);
    void saoChepBucLuc(const string& phongDich, const string& phongNguon, Database& db);
    
    // ==========================================
    // N?P CH?NG HÀM HI?N TH? SO Ð? PHÒNG
    // ==========================================

    // 1. Dành cho Admin: Xem so d? b? c?c v?t lý (Không c?n su?t chi?u)
    void hienThiSoDoPhong(const string& maPhong) const;

    // 2. Dành cho Staff/Khách hàng: Xem so d? theo su?t chi?u (Load tr?ng thái d?ng Tr?ng / Ðã d?t)
    void hienThiSoDoPhong(const string& maPhong, const string& maSuatChieu, Database& db) const;
};

#endif
