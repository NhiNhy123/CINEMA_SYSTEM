#ifndef SHOWTIME_H
#define SHOWTIME_H

#include <string>
#include <vector>
#include <sstream>
#include "Database.h"

using namespace std;
class Database;

class Showtime {
private:
    string maSuatChieu;
    string maPhim;
    string maPhong;
    string ngayChieu;
    string gioBatDau;
    string gioKetThuc;
    double giaVeCoBan;

public:

    Showtime();
    Showtime(string maSC, string maPhim, string maPhong, string ngay, string gioBD, string gioKT, double giaVe);

    string getMaSuatChieu() const;
    string getMaPhim() const;
    string getMaPhong() const;
    string getNgayChieu() const;
    string getGioBatDau() const;
    string getGioKetThuc() const;
    double getGiaVeCoBan() const;

    string toCSV() const;
    static Showtime fromCSV(const string& line);
    void hienThi(Database& db) const;

    static void themSuatChieu(Database& db);

    static void hienThiSuatChieuTheoPhim(const string& maPhimCanXem, Database& db);

    static void hienThiTatCa(Database& db);
};

#endif
