#ifndef MOVIE_H
#define MOVIE_H

#include <string>
#include <vector>

using namespace std;

class Database; // Khai báo tru?c l?p Database

class Movie {
private:
    string maPhim;
    string tenPhim;
    int thoiLuong;          // Tính b?ng phút
    int nhanDoTuoi;         // Ví d?: 13, 16, 18
    string ngayKhoiChieu;   // Ð?nh d?ng: YYYY-MM-DD
    string ngayKetThuc;
    string moTa;
    string poster;          // Link ho?c tên file poster/trailer

public:
    // Constructor
    Movie();
    Movie(string ma, string ten, int thoiLuong, int nhanTuoi, string ngayKC, string ngayKT, string moTa, string poster);

    // Getters
    string getMaPhim() const;
    string getTenPhim() const;
    int getThoiLuong() const;
    int getNhanDoTuoi() const;
    string getNgayKhoiChieu() const;
    string getNgayKetThuc() const;
    string getMoTa() const;
    string getPoster() const;

    // Setters
    void setTenPhim(const string& ten);
    void setThoiLuong(int tl);
    void setNhanDoTuoi(int tuoi);
    void setNgayKhoiChieu(const string& ngay);
    void setNgayKetThuc(const string& ngay);
    void setMoTa(const string& desc);
    void setPoster(const string& p);

    // Ð?c/Ghi d? li?u CSV tuong thích v?i Database
    string toCSV() const;
    static Movie fromCSV(const string& line);

    // Hi?n th? thông tin
    void hienThi() const;

    // --- CÁC TÍNH NANG QU?N LÝ (THAY TH? CHO MOVIEMANAGER) ---
    static void hienThiDanhSach(Database& db);
    static void themPhim(Database& db);
    static void suaPhim(Database& db);
    static void xoaPhim(Database& db);
    static bool kiemTraPhimDangChieu(const string& maPhim, Database& db);
};

#endif
