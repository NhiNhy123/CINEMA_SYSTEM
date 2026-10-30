#ifndef MOVIE_H
#define MOVIE_H

#include <string>
#include <vector>

using namespace std;

class Database;

class Movie {
private:
    string maPhim;
    string tenPhim;
    int thoiLuong;
    int nhanDoTuoi;
    string ngayKhoiChieu;
    string ngayKetThuc;
    string moTa;
    string poster;

public:

    Movie();
    Movie(string ma, string ten, int thoiLuong, int nhanTuoi, string ngayKC, string ngayKT, string moTa, string poster);

    string getMaPhim() const;
    string getTenPhim() const;
    int getThoiLuong() const;
    int getNhanDoTuoi() const;
    string getNgayKhoiChieu() const;
    string getNgayKetThuc() const;
    string getMoTa() const;
    string getPoster() const;

    void setTenPhim(const string& ten);
    void setThoiLuong(int tl);
    void setNhanDoTuoi(int tuoi);
    void setNgayKhoiChieu(const string& ngay);
    void setNgayKetThuc(const string& ngay);
    void setMoTa(const string& desc);
    void setPoster(const string& p);

    string toCSV() const;
    static Movie fromCSV(const string& line);

    void hienThi(Database& db) const;

    static void hienThiDanhSach(Database& db);
    static void themPhim(Database& db);
    static void suaPhim(Database& db);
    static void xoaPhim(Database& db);
    static bool kiemTraPhimDangChieu(const string& maPhim, Database& db);
};

#endif
