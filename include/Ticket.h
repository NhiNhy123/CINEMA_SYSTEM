#ifndef TICKET_H
#define TICKET_H

#include <string>

using namespace std;

class Database;

class Ticket {
private:
    string maVe;
    string maHoaDon;
    string maSuatChieu;
    string maGhe;
    double giaVeThucTe;
    string maQRCode;
    string trangThaiSuDung;

public:
    Ticket();
    Ticket(string maVe, string maHD, string maSC, string maGhe, double gia, string qr, string trangThai);

    string getMaVe() const;
    string getMaHoaDon() const;
    string getMaSuatChieu() const;
    string getMaGhe() const;
    double getGiaVeThucTe() const;
    string getMaQRCode() const;
    string getTrangThaiSuDung() const;

    void setTrangThaiSuDung(const string& trangThai);
    string toCSV() const;
    static Ticket fromCSV(const string& line);
    void hienThi() const;
};

class TicketManager {
public:
    void quyTrinhBanVe(Database& db, const string& maNV);
    void hienThiDanhSachVe(Database& db);
    void checkInVe(Database& db, const string& maNhanVien);
};

#endif
