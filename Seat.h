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
    string maPhong;    // Thu?c phòng nào (VD: P01)
    string loaiGhe;    // Standard, VIP, Couple, 4DX, IMAX...
    double donGiaGhe;   // Ðon giá co b?n (> 0)
    string trangThai;  // Trong, DangChon, DaDat, KhongSuDung

public:
    Seat();
    Seat(string maGhe, string maPhong, string loaiGhe, double donGiaGhe, string trangThai);

    // Getters
    string getMaGhe() const;
    string getMaPhong() const;
    string getLoaiGhe() const;
    double getDonGiaGhe() const;
    string getTrangThai() const;

    // Setters
    void setLoaiGhe(const string& loai);
    void setDonGiaGhe(double gia);
    void setTrangThai(const string& tt);
    string toCSV() const;
    Seat fromCSV(const string& line);

    // Hi?n th? thông tin gh? ng?n g?n
    void hienThi() const;
};

class SeatManager {
private:
    vector<Seat> danhSachGhe;

public:
    SeatManager();

    // T?i d? li?u gh? thông qua Database chung
    void taiDuLieu(Database& db);

    // Phân h? Admin: T?o gh? hàng lo?t & Sao chép b? c?c
    void taoGheHangLoat(const string& maPhong, Database& db);
    void saoChepBucLuc(const string& phongDich, const string& phongNguon, Database& db);
    void hienThiSoDoPhong(const string& maPhong) const;

    // Phân h? Staff: Ch?n gh? & Khóa t?m th?i (15 phút)
    void chonGheTamThoi(const string& maGhe, const string& maPhong, Database& db);
};

#endif
