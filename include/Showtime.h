#ifndef SHOWTIME_H
#define SHOWTIME_H

#include <string>
#include <vector>
#include <sstream>
#include "Database.h" // Ð? tuong tác tr?c ti?p v?i Database d?c/ghi file

using namespace std;
class Database;

class Showtime {
private:
    string maSuatChieu;
    string maPhim;
    string maPhong;
    string ngayChieu;   // Ð?nh d?ng: YYYY-MM-DD
    string gioBatDau;   // Ð?nh d?ng: HH:MM
    string gioKetThuc;  // Ð?nh d?ng: HH:MM
    double giaVeCoBan;

public:
    // Constructor
    Showtime();
    Showtime(string maSC, string maPhim, string maPhong, string ngay, string gioBD, string gioKT, double giaVe);

    // Getters
    string getMaSuatChieu() const;
    string getMaPhim() const;
    string getMaPhong() const;
    string getNgayChieu() const;
    string getGioBatDau() const;
    string getGioKetThuc() const;
    double getGiaVeCoBan() const;

    // Chuy?n d?i Database CSV
    string toCSV() const;
    static Showtime fromCSV(const string& line);
    void hienThi() const;

    // --- CÁC TÍNH NANG NGHI?P V? (THAY TH? CHO ShowtimeManager) ---
    
    // Thêm su?t chi?u m?i có ki?m tra xung d?t phòng và th?i gian
    static void themSuatChieu(Database& db);

    // Hi?n th? danh sách su?t chi?u c?a m?t phim c? th? (Ph?c v? phân h? Bán vé c?a Nhân viên)
    static void hienThiSuatChieuTheoPhim(const string& maPhimCanXem, Database& db);
    
    // Hi?n th? toàn b? l?ch chi?u
    static void hienThiTatCa(Database& db);
};

#endif
