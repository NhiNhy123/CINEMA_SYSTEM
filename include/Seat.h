#ifndef SEAT_H
#define SEAT_H

#include <string>
#include <vector>

using namespace std;

class Database;

class Seat {
private:
    string maGhe;
    string maPhong;
    string loaiGhe;
    double donGiaGhe;
    string trangThai;

public:
    Seat();
    Seat(string maGhe, string maPhong, string loaiGhe, double donGiaGhe, string trangThai);

    string getMaGhe() const;
    string getMaPhong() const;
    string getLoaiGhe() const;
    double getDonGiaGhe() const;
    string getTrangThai() const;

    void setLoaiGhe(const string& loai);
    void setDonGiaGhe(double gia);
    void setTrangThai(const string& tt);
    string toCSV() const;
    Seat fromCSV(const string& line);

    void hienThi() const;
};

class SeatManager {
private:
    vector<Seat> danhSachGhe;

public:
    SeatManager();

    void taiDuLieu(Database& db);

    void taoGheTuDongChoPhong(const string& maPhong, Database& db);
    void taoGheHangLoat(const string& maPhong, Database& db);
    void saoChepBucLuc(const string& phongDich, const string& phongNguon, Database& db);
    void hienThiSoDoPhong(const string& maPhong) const;
    void hienThiDanhSachLoaiGhe(Database& db) const;
    void themLoaiGhe(Database& db);
    void suaLoaiGhe(Database& db);
    void xoaLoaiGhe(Database& db);

    void chonGheTamThoi(const string& maGhe, const string& maPhong, Database& db);
};

#endif
