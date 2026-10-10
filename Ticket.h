#ifndef TICKET_H
#define TICKET_H

#include <string>
#include <vector>

using namespace std;

class Database; // Khai báo tru?c l?p Database

class Ticket {
private:
    string maVe;
    string maHoaDon;
    string maSuatChieu;
    string maGhe;
    double giaVeThucTe;     // Ðã c?ng ph? thu gh? (VIP, v.v.)
    string maQRCode;        // Chu?i d?nh danh mã v?ch/QR vé
    string trangThaiSuDung; // "ChuaCheckIn" ho?c "DaCheckIn"

public:
    // Constructor
    Ticket();
    Ticket(string maVe, string maHD, string maSC, string maGhe, double gia, string qr, string trangThai = "ChuaCheckIn");

    // Getters
    string getMaVe() const;
    string getMaHoaDon() const;
    string getMaSuatChieu() const;
    string getMaGhe() const;
    double getGiaVeThucTe() const;
    string getMaQRCode() const;
    string getTrangThaiSuDung() const;

    // Setters
    void setTrangThaiSuDung(const string& trangThai);

    // Chuy?n d?i d? li?u File / CSV
    string toCSV() const;
    static Ticket fromCSV(const string& line);

    // Hi?n th? thông tin vé
    void hienThi() const;
};

// L?p qu?n lý quy trình nghi?p v? bán vé t?i qu?y (Nhân viên)
class TicketManager {
public:
    // Quy trình 6 bu?c bán vé t?i qu?y (Ð?c/ghi d? li?u qua Database)
    static void quyTrinhBanVe(Database& db);
    
    // Qu?n lý vé & Check-in
    static void hienThiDanhSachVe(Database& db);
    static void checkInVe(Database& db);
};

#endif
