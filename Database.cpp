#include "Database.h"
#include "Seat.h"
#include "FAndB.h"
#include "Promotion.h"
#include "Invoice.h"
#include "Room.h"
#include "Movie.h"
#include "Showtime.h"
#include "Ticket.h"

#include <fstream>
#include <sstream>
#include <iostream>
#include <cstdlib>

using namespace std;

Database::Database() {}

// Hàm ti?n ích n?i b? d? tách chu?i CSV phân cách b?ng d?u ph?y ','
static vector<string> tachChuoiCSV(const string& line) {
    vector<string> tokens;
    stringstream ss(line);
    string item;
    while (getline(ss, item, ',')) {
        // Xóa ký t? '\r' ? cu?i chu?i n?u ch?y trên Windows (d?nh d?ng CRLF)
        if (!item.empty() && item[item.length() - 1] == '\r') {
            item.erase(item.length() - 1);
        }
        tokens.push_back(item);
    }
    return tokens;
}


// =========================================================================
// 2. KHO PHIM & H? T?NG R?P (DATA/quan_ly_phim.csv)
// =========================================================================

// --- Qu?n lý Phòng chi?u (Rooms) ---
void Database::docDanhSachPhong(vector<Room>& danhSachPhong) {
    danhSachPhong.clear();
    ifstream inFile("DATA/quan_ly_phim.csv");
    if (!inFile.is_open()) return;

    string line;
    bool reading = false;
    while (getline(inFile, line)) {
        if (line.empty()) continue;
        if (line.find("[ROOMS]") != string::npos) {
            reading = true;
            continue;
        }
        if (reading && line[0] == '[') {
            reading = false;
            break;
        }

        if (reading) {
            if (line.find("maPhong") != string::npos) continue; // B? qua dòng tiêu d? c?t
            vector<string> parts = tachChuoiCSV(line);
            if (parts.size() >= 5) {
                Room r(parts[0], parts[1], parts[2], atoi(parts[3].c_str()), parts[4]);
                danhSachPhong.push_back(r);
            }
        }
    }
    inFile.close();
}

void Database::ghiDanhSachPhong(const vector<Room>& danhSachPhong) {
    vector<Seat> danhSachGhe;
    vector<Movie> danhSachPhim;
    docDanhSachGhe(danhSachGhe);
    docDanhSachPhim(danhSachPhim);

    ofstream outFile("DATA/quan_ly_phim.csv", ios::trunc);
    if (!outFile.is_open()) return;

    outFile << "[ROOMS]\n";
    outFile << "maPhong,tenPhong,loaiPhong,sucChua,trangThaiPhong\n";
    for (size_t i = 0; i < danhSachPhong.size(); ++i) {
        outFile << danhSachPhong[i].getMaPhong() << "," << danhSachPhong[i].getTenPhong() << ","  
                << danhSachPhong[i].getLoaiPhong() << "," << danhSachPhong[i].getSucChua() << "," << danhSachPhong[i].getTrangThaiPhong() << "\n";
    }

    outFile << "\n[SEATS]\n";
    outFile << "maGhe,maPhong,loaiGhe,donGiaGhe,trangThai\n";
    for (size_t i = 0; i < danhSachGhe.size(); ++i) {
        outFile << danhSachGhe[i].getMaGhe() << "," << danhSachGhe[i].getMaPhong() << ","  
                << danhSachGhe[i].getLoaiGhe() << "," << danhSachGhe[i].getDonGiaGhe() << "," << danhSachGhe[i].getTrangThai() << "\n";
    }

    outFile << "\n[MOVIES]\n";
    outFile << "maPhim,tenPhim,thoiLuong,nhanDoTuoi,ngayKhoiChieu,ngayKetThuc,moTa,poster\n";
    for (size_t i = 0; i < danhSachPhim.size(); ++i) {
        outFile << danhSachPhim[i].getMaPhim() << "," << danhSachPhim[i].getTenPhim() << "," << danhSachPhim[i].getThoiLuong() << ","  
                << danhSachPhim[i].getNhanDoTuoi() << "," << danhSachPhim[i].getNgayKhoiChieu() << "," << danhSachPhim[i].getNgayKetThuc() << ","  
                << danhSachPhim[i].getMoTa() << "," << danhSachPhim[i].getPoster() << "\n";
    }
    outFile.close();
}

// --- Qu?n lý Gh? (Seats) ---
void Database::docDanhSachGhe(vector<Seat>& danhSachGhe) {
    danhSachGhe.clear();
    ifstream inFile("DATA/quan_ly_phim.csv");
    if (!inFile.is_open()) return;

    string line;
    bool reading = false;
    while (getline(inFile, line)) {
        if (line.empty()) continue;
        if (line.find("[SEATS]") != string::npos) {
            reading = true;
            continue;
        }
        if (reading && line[0] == '[') {
            reading = false;
            break;
        }

        if (reading) {
            if (line.find("maGhe") != string::npos) continue; // B? qua dòng tiêu d? c?t
            vector<string> parts = tachChuoiCSV(line);
            if (parts.size() >= 5) {
                Seat s(parts[0], parts[1], parts[2], atof(parts[3].c_str()), parts[4]);
                danhSachGhe.push_back(s);
            }
        }
    }
    inFile.close();
}

void Database::ghiDanhSachGhe(const vector<Seat>& danhSachGhe) {
    vector<Room> danhSachPhong;
    vector<Movie> danhSachPhim;
    docDanhSachPhong(danhSachPhong);
    docDanhSachPhim(danhSachPhim);

    ofstream outFile("DATA/quan_ly_phim.csv", ios::trunc);
    if (!outFile.is_open()) return;

    outFile << "[ROOMS]\n";
    outFile << "maPhong,tenPhong,loaiPhong,sucChua,trangThaiPhong\n";
    for (size_t i = 0; i < danhSachPhong.size(); ++i) {
        outFile << danhSachPhong[i].getMaPhong() << "," << danhSachPhong[i].getTenPhong() << ","  
                << danhSachPhong[i].getLoaiPhong() << "," << danhSachPhong[i].getSucChua() << "," << danhSachPhong[i].getTrangThaiPhong() << "\n";
    }

    outFile << "\n[SEATS]\n";
    outFile << "maGhe,maPhong,loaiGhe,donGiaGhe,trangThai\n";
    for (size_t i = 0; i < danhSachGhe.size(); ++i) {
        outFile << danhSachGhe[i].getMaGhe() << "," << danhSachGhe[i].getMaPhong() << ","  
                << danhSachGhe[i].getLoaiGhe() << "," << danhSachGhe[i].getDonGiaGhe() << "," << danhSachGhe[i].getTrangThai() << "\n";
    }

    outFile << "\n[MOVIES]\n";
    outFile << "maPhim,tenPhim,thoiLuong,nhanDoTuoi,ngayKhoiChieu,ngayKetThuc,moTa,poster\n";
    for (size_t i = 0; i < danhSachPhim.size(); ++i) {
        outFile << danhSachPhim[i].getMaPhim() << "," << danhSachPhim[i].getTenPhim() << "," << danhSachPhim[i].getThoiLuong() << ","  
                << danhSachPhim[i].getNhanDoTuoi() << "," << danhSachPhim[i].getNgayKhoiChieu() << "," << danhSachPhim[i].getNgayKetThuc() << ","  
                << danhSachPhim[i].getMoTa() << "," << danhSachPhim[i].getPoster() << "\n";
    }
    outFile.close();
}

// --- Qu?n lý Phim (Movies) ---
void Database::docDanhSachPhim(vector<Movie>& danhSachPhim) {
    danhSachPhim.clear();
    ifstream inFile("DATA/quan_ly_phim.csv");
    if (!inFile.is_open()) return;

    string line;
    bool reading = false;
    while (getline(inFile, line)) {
        if (line.empty()) continue;
        if (line.find("[MOVIES]") != string::npos) {
            reading = true;
            continue;
        }
        if (reading && line[0] == '[') {
            reading = false;
            break;
        }

        if (reading) {
            if (line.find("maPhim") != string::npos) continue; // B? qua dòng tiêu d? c?t
            vector<string> parts = tachChuoiCSV(line);
            if (parts.size() >= 8) {
                Movie m(parts[0], parts[1], atoi(parts[2].c_str()), atoi(parts[3].c_str()), parts[4], parts[5], parts[6], parts[7]);
                danhSachPhim.push_back(m);
            }
        }
    }
    inFile.close();
}

void Database::ghiDanhSachPhim(const vector<Movie>& danhSachPhim) {
    vector<Room> danhSachPhong;
    vector<Seat> danhSachGhe;
    docDanhSachPhong(danhSachPhong);
    docDanhSachGhe(danhSachGhe);

    ofstream outFile("DATA/quan_ly_phim.csv", ios::trunc);
    if (!outFile.is_open()) return;

    outFile << "[ROOMS]\n";
    outFile << "maPhong,tenPhong,loaiPhong,sucChua,trangThaiPhong\n";
    for (size_t i = 0; i < danhSachPhong.size(); ++i) {
        outFile << danhSachPhong[i].getMaPhong() << "," << danhSachPhong[i].getTenPhong() << ","  
                << danhSachPhong[i].getLoaiPhong() << "," << danhSachPhong[i].getSucChua() << "," << danhSachPhong[i].getTrangThaiPhong() << "\n";
    }

    outFile << "\n[SEATS]\n";
    outFile << "maGhe,maPhong,loaiGhe,donGiaGhe,trangThai\n";
    for (size_t i = 0; i < danhSachGhe.size(); ++i) {
        outFile << danhSachGhe[i].getMaGhe() << "," << danhSachGhe[i].getMaPhong() << ","  
                << danhSachGhe[i].getLoaiGhe() << "," << danhSachGhe[i].getDonGiaGhe() << "," << danhSachGhe[i].getTrangThai() << "\n";
    }

    outFile << "\n[MOVIES]\n";
    outFile << "maPhim,tenPhim,thoiLuong,nhanDoTuoi,ngayKhoiChieu,ngayKetThuc,moTa,poster\n";
    for (size_t i = 0; i < danhSachPhim.size(); ++i) {
        outFile << danhSachPhim[i].getMaPhim() << "," << danhSachPhim[i].getTenPhim() << "," << danhSachPhim[i].getThoiLuong() << ","  
                << danhSachPhim[i].getNhanDoTuoi() << "," << danhSachPhim[i].getNgayKhoiChieu() << "," << danhSachPhim[i].getNgayKetThuc() << ","  
                << danhSachPhim[i].getMoTa() << "," << danhSachPhim[i].getPoster() << "\n";
    }
    outFile.close();
}


// =========================================================================
// 3. L?CH CHI?U & VÉ (DATA/lich_va_ve.csv)
// =========================================================================

// --- Qu?n lý Su?t chi?u (Showtimes) ---
void Database::docDanhSachSuatChieu(vector<Showtime>& danhSachSuatChieu) {
    danhSachSuatChieu.clear();
    ifstream inFile("DATA/lich_va_ve.csv");
    if (!inFile.is_open()) return;

    string line;
    bool reading = false;

    while (getline(inFile, line)) {
        if (line.empty()) continue;

        if (line.find("[SHOWTIMES]") != string::npos) {
            reading = true;
            continue;
        }

        if (reading && line[0] == '[') {
            reading = false;
            break;
        }

        if (reading) {
            if (line.find("maSuatChieu") != string::npos) continue; // B? qua dòng tiêu d? c?t
            vector<string> parts = tachChuoiCSV(line);
            if (parts.size() >= 7) {
                Showtime st(parts[0], parts[1], parts[2], parts[3], parts[4], parts[5], atof(parts[6].c_str()));
                danhSachSuatChieu.push_back(st);
            }
        }
    }
    inFile.close();
}

void Database::ghiDanhSachSuatChieu(const vector<Showtime>& danhSachSuatChieu) {
    vector<Ticket> danhSachVe;
    docDanhSachVe(danhSachVe);

    ofstream outFile("DATA/lich_va_ve.csv", ios::trunc);
    if (!outFile.is_open()) return;

    outFile << "[SHOWTIMES]\n";
    outFile << "maSuatChieu,maPhim,maPhong,ngayChieu,gioBatDau,gioKetThuc,giaVeCoBan\n";
    for (size_t i = 0; i < danhSachSuatChieu.size(); ++i) {
        outFile << danhSachSuatChieu[i].getMaSuatChieu() << "," << danhSachSuatChieu[i].getMaPhim() << "," << danhSachSuatChieu[i].getMaPhong() << ","  
                << danhSachSuatChieu[i].getNgayChieu() << "," << danhSachSuatChieu[i].getGioBatDau() << "," << danhSachSuatChieu[i].getGioKetThuc() << ","  
                << danhSachSuatChieu[i].getGiaVeCoBan() << "\n";
    }

    outFile << "\n[TICKETS]\n";
    outFile << "maVe,maHoaDon,maSuatChieu,maGhe,giaVeThucTe,maQRCode,trangThaiSuDung\n";
    for (size_t i = 0; i < danhSachVe.size(); ++i) {
        outFile << danhSachVe[i].getMaVe() << "," << danhSachVe[i].getMaHoaDon() << "," << danhSachVe[i].getMaSuatChieu() << ","  
                << danhSachVe[i].getMaGhe() << "," << danhSachVe[i].getGiaVeThucTe() << "," << danhSachVe[i].getMaQRCode() << ","  
                << danhSachVe[i].getTrangThaiSuDung() << "\n";
    }
    outFile.close();
}

// --- Qu?n lý Vé (Tickets) ---
void Database::docDanhSachVe(vector<Ticket>& danhSachVe) {
    danhSachVe.clear();
    ifstream inFile("DATA/lich_va_ve.csv");
    if (!inFile.is_open()) return;

    string line;
    bool reading = false;
    while (getline(inFile, line)) {
        if (line.empty()) continue;
        if (line.find("[TICKETS]") != string::npos) {
            reading = true;
            continue;
        }
        if (reading && line[0] == '[') {
            reading = false;
            break;
        }

        if (reading) {
            if (line.find("maVe") != string::npos) continue; // B? qua dòng tiêu d? c?t
            vector<string> parts = tachChuoiCSV(line);
            if (parts.size() >= 7) {
                Ticket t(parts[0], parts[1], parts[2], parts[3], atof(parts[4].c_str()), parts[5], parts[6]);
                danhSachVe.push_back(t);
            }
        }
    }
    inFile.close();
}

void Database::ghiDanhSachVe(const vector<Ticket>& danhSachVe) {
    vector<Showtime> danhSachSuatChieu;
    docDanhSachSuatChieu(danhSachSuatChieu);

    ofstream outFile("DATA/lich_va_ve.csv", ios::trunc);
    if (!outFile.is_open()) return;

    outFile << "[SHOWTIMES]\n";
    outFile << "maSuatChieu,maPhim,maPhong,ngayChieu,gioBatDau,gioKetThuc,giaVeCoBan\n";
    for (size_t i = 0; i < danhSachSuatChieu.size(); ++i) {
        outFile << danhSachSuatChieu[i].getMaSuatChieu() << "," << danhSachSuatChieu[i].getMaPhim() << "," << danhSachSuatChieu[i].getMaPhong() << ","  
                << danhSachSuatChieu[i].getNgayChieu() << "," << danhSachSuatChieu[i].getGioBatDau() << "," << danhSachSuatChieu[i].getGioKetThuc() << ","  
                << danhSachSuatChieu[i].getGiaVeCoBan() << "\n";
    }

    outFile << "\n[TICKETS]\n";
    outFile << "maVe,maHoaDon,maSuatChieu,maGhe,giaVeThucTe,maQRCode,trangThaiSuDung\n";
    for (size_t i = 0; i < danhSachVe.size(); ++i) {
        outFile << danhSachVe[i].getMaVe() << "," << danhSachVe[i].getMaHoaDon() << "," << danhSachVe[i].getMaSuatChieu() << ","  
                << danhSachVe[i].getMaGhe() << "," << danhSachVe[i].getGiaVeThucTe() << "," << danhSachVe[i].getMaQRCode() << ","  
                << danhSachVe[i].getTrangThaiSuDung() << "\n";
    }
    outFile.close();
}


// =========================================================================
// 4. DOANH THU, F&B & KHUY?N MÃI (DATA/giao_dich_fnb.csv)
// =========================================================================

// --- Qu?n lý F&B ---
void Database::docDanhSachFAndB(vector<FoodAndBeverage>& danhSachFAndB) {
    danhSachFAndB.clear();
    ifstream inFile("DATA/giao_dich_fnb.csv");
    if (!inFile.is_open()) return;

    string line;
    bool reading = false;
    while (getline(inFile, line)) {
        if (line.empty()) continue;
        if (line.find("[FOOD_AND_BEVERAGES]") != string::npos) {
            reading = true;
            continue;
        }
        if (reading && line[0] == '[') {
            reading = false;
            break;
        }

        if (reading) {
            if (line.find("maMatHang") != string::npos) continue; // B? qua dòng tiêu d? c?t
            vector<string> parts = tachChuoiCSV(line);
            if (parts.size() >= 5) {
                FoodAndBeverage fb(parts[0], parts[1], parts[2], atof(parts[3].c_str()), parts[4]);
                danhSachFAndB.push_back(fb);
            }
        }
    }
    inFile.close();
}

void Database::ghiDanhSachFAndB(const vector<FoodAndBeverage>& danhSachFAndB) {
    vector<Promotion> danhSachKhuyenMai;
    vector<Invoice> danhSachHoaDon;
    docDanhSachKhuyenMai(danhSachKhuyenMai);
    docDanhSachHoaDon(danhSachHoaDon);

    ofstream outFile("DATA/giao_dich_fnb.csv", ios::trunc);
    if (!outFile.is_open()) return;

    outFile << "[FOOD_AND_BEVERAGES]\n";
    outFile << "maMatHang,tenMatHang,loai,giaTien,trangThaiKinhDoanh\n";
    for (size_t i = 0; i < danhSachFAndB.size(); ++i) {
        outFile << danhSachFAndB[i].getMaMatHang() << "," << danhSachFAndB[i].getTenMatHang() << ","  
                << danhSachFAndB[i].getLoai() << "," << danhSachFAndB[i].getGiaTien() << "," << danhSachFAndB[i].getTrangThaiKinhDoanh() << "\n";
    }

    outFile << "\n[PROMOTIONS]\n";
    outFile << "maKhuyenMai,tenKhuyenMai,maCode,phanTramGiam,mucGiamToiDa,ngayBatDau,ngayKetThuc,phamViApDung\n";
    for (size_t i = 0; i < danhSachKhuyenMai.size(); ++i) {
        outFile << danhSachKhuyenMai[i].getMaKhuyenMai() << "," << danhSachKhuyenMai[i].getTenKhuyenMai() << ","  
                << danhSachKhuyenMai[i].getMaCode() << "," << danhSachKhuyenMai[i].getPhanTramGiam() << ","  
                << danhSachKhuyenMai[i].getMucGiamToiDa() << "," << danhSachKhuyenMai[i].getNgayBatDau() << ","  
                << danhSachKhuyenMai[i].getNgayKetThuc() << "," << danhSachKhuyenMai[i].getPhamViApDung() << "\n";
    }

    outFile << "\n[INVOICES]\n";
    outFile << "maHoaDon,maKhachHang,maNhanVien,thoiGianLap,tongTien,hinhThucThanhToan,maGiaoDich,trangThaiThanhToan\n";
    for (size_t i = 0; i < danhSachHoaDon.size(); ++i) {
        outFile << danhSachHoaDon[i].getMaHoaDon() << "," << danhSachHoaDon[i].getMaKhachHang() << ","  
                << danhSachHoaDon[i].getMaNhanVien() << "," << danhSachHoaDon[i].getThoiGianLap() << ","  
                << danhSachHoaDon[i].getTongTien() << "," << danhSachHoaDon[i].getHinhThucThanhToan() << ","  
                << danhSachHoaDon[i].getMaGiaoDich() << "," << danhSachHoaDon[i].getTrangThaiThanhToan() << "\n";
    }
    outFile.close();
}

// --- Qu?n lý Khuy?n mãi (Promotions) ---
void Database::docDanhSachKhuyenMai(vector<Promotion>& danhSachKhuyenMai) {
    danhSachKhuyenMai.clear();
    ifstream inFile("DATA/giao_dich_fnb.csv");
    if (!inFile.is_open()) return;

    string line;
    bool reading = false;
    while (getline(inFile, line)) {
        if (line.empty()) continue;
        if (line.find("[PROMOTIONS]") != string::npos) {
            reading = true;
            continue;
        }
        if (reading && line[0] == '[') {
            reading = false;
            break;
        }

        if (reading) {
            if (line.find("maKhuyenMai") != string::npos) continue; // B? qua dòng tiêu d? c?t
            vector<string> parts = tachChuoiCSV(line);
            if (parts.size() >= 8) {
                Promotion km(parts[0], parts[1], parts[2], atof(parts[3].c_str()), atof(parts[4].c_str()), parts[5], parts[6], parts[7]);
                danhSachKhuyenMai.push_back(km);
            }
        }
    }
    inFile.close();
}

void Database::ghiDanhSachKhuyenMai(const vector<Promotion>& danhSachKhuyenMai) {
    vector<FoodAndBeverage> danhSachFAndB;
    vector<Invoice> danhSachHoaDon;
    docDanhSachFAndB(danhSachFAndB);
    docDanhSachHoaDon(danhSachHoaDon);

    ofstream outFile("DATA/giao_dich_fnb.csv", ios::trunc);
    if (!outFile.is_open()) return;

    outFile << "[FOOD_AND_BEVERAGES]\n";
    outFile << "maMatHang,tenMatHang,loai,giaTien,trangThaiKinhDoanh\n";
    for (size_t i = 0; i < danhSachFAndB.size(); ++i) {
        outFile << danhSachFAndB[i].getMaMatHang() << "," << danhSachFAndB[i].getTenMatHang() << ","  
                << danhSachFAndB[i].getLoai() << "," << danhSachFAndB[i].getGiaTien() << "," << danhSachFAndB[i].getTrangThaiKinhDoanh() << "\n";
    }

    outFile << "\n[PROMOTIONS]\n";
    outFile << "maKhuyenMai,tenKhuyenMai,maCode,phanTramGiam,mucGiamToiDa,ngayBatDau,ngayKetThuc,phamViApDung\n";
    for (size_t i = 0; i < danhSachKhuyenMai.size(); ++i) {
        outFile << danhSachKhuyenMai[i].getMaKhuyenMai() << "," << danhSachKhuyenMai[i].getTenKhuyenMai() << ","  
                << danhSachKhuyenMai[i].getMaCode() << "," << danhSachKhuyenMai[i].getPhanTramGiam() << ","  
                << danhSachKhuyenMai[i].getMucGiamToiDa() << "," << danhSachKhuyenMai[i].getNgayBatDau() << ","  
                << danhSachKhuyenMai[i].getNgayKetThuc() << "," << danhSachKhuyenMai[i].getPhamViApDung() << "\n";
    }

    outFile << "\n[INVOICES]\n";
    outFile << "maHoaDon,maKhachHang,maNhanVien,thoiGianLap,tongTien,hinhThucThanhToan,maGiaoDich,trangThaiThanhToan\n";
    for (size_t i = 0; i < danhSachHoaDon.size(); ++i) {
        outFile << danhSachHoaDon[i].getMaHoaDon() << "," << danhSachHoaDon[i].getMaKhachHang() << ","  
                << danhSachHoaDon[i].getMaNhanVien() << "," << danhSachHoaDon[i].getThoiGianLap() << ","  
                << danhSachHoaDon[i].getTongTien() << "," << danhSachHoaDon[i].getHinhThucThanhToan() << ","  
                << danhSachHoaDon[i].getMaGiaoDich() << "," << danhSachHoaDon[i].getTrangThaiThanhToan() << "\n";
    }
    outFile.close();
}

// --- Qu?n lý Hóa don (Invoices) ---
void Database::docDanhSachHoaDon(vector<Invoice>& danhSachHoaDon) {
    danhSachHoaDon.clear();
    ifstream inFile("DATA/giao_dich_fnb.csv");
    if (!inFile.is_open()) return;

    string line;
    bool reading = false;
    while (getline(inFile, line)) {
        if (line.empty()) continue;
        if (line.find("[INVOICES]") != string::npos) {
            reading = true;
            continue;
        }
        if (reading && line[0] == '[') {
            reading = false;
            break;
        }

        if (reading) {
            if (line.find("maHoaDon") != string::npos) continue; // B? qua dòng tiêu d? c?t
            vector<string> parts = tachChuoiCSV(line);
            if (parts.size() >= 8) {
                Invoice hd(parts[0], parts[1], parts[2], parts[3], atof(parts[4].c_str()), parts[5], parts[6], parts[7]);
                danhSachHoaDon.push_back(hd);
            }
        }
    }
    inFile.close();
}

void Database::ghiDanhSachHoaDon(const vector<Invoice>& danhSachHoaDon) {
    vector<FoodAndBeverage> danhSachFAndB;
    vector<Promotion> danhSachKhuyenMai;
    docDanhSachFAndB(danhSachFAndB);
    docDanhSachKhuyenMai(danhSachKhuyenMai);

    ofstream outFile("DATA/giao_dich_fnb.csv", ios::trunc);
    if (!outFile.is_open()) return;

    outFile << "[FOOD_AND_BEVERAGES]\n";
    outFile << "maMatHang,tenMatHang,loai,giaTien,trangThaiKinhDoanh\n";
    for (size_t i = 0; i < danhSachFAndB.size(); ++i) {
        outFile << danhSachFAndB[i].getMaMatHang() << "," << danhSachFAndB[i].getTenMatHang() << ","  
                << danhSachFAndB[i].getLoai() << "," << danhSachFAndB[i].getGiaTien() << "," << danhSachFAndB[i].getTrangThaiKinhDoanh() << "\n";
    }

    outFile << "\n[PROMOTIONS]\n";
    outFile << "maKhuyenMai,tenKhuyenMai,maCode,phanTramGiam,mucGiamToiDa,ngayBatDau,ngayKetThuc,phamViApDung\n";
    for (size_t i = 0; i < danhSachKhuyenMai.size(); ++i) {
        outFile << danhSachKhuyenMai[i].getMaKhuyenMai() << "," << danhSachKhuyenMai[i].getTenKhuyenMai() << ","  
                << danhSachKhuyenMai[i].getMaCode() << "," << danhSachKhuyenMai[i].getPhanTramGiam() << ","  
                << danhSachKhuyenMai[i].getMucGiamToiDa() << "," << danhSachKhuyenMai[i].getNgayBatDau() << ","  
                << danhSachKhuyenMai[i].getNgayKetThuc() << "," << danhSachKhuyenMai[i].getPhamViApDung() << "\n";
    }

    outFile << "\n[INVOICES]\n";
    outFile << "maHoaDon,maKhachHang,maNhanVien,thoiGianLap,tongTien,hinhThucThanhToan,maGiaoDich,trangThaiThanhToan\n";
    for (size_t i = 0; i < danhSachHoaDon.size(); ++i) {
        outFile << danhSachHoaDon[i].getMaHoaDon() << "," << danhSachHoaDon[i].getMaKhachHang() << ","  
                << danhSachHoaDon[i].getMaNhanVien() << "," << danhSachHoaDon[i].getThoiGianLap() << ","  
                << danhSachHoaDon[i].getTongTien() << "," << danhSachHoaDon[i].getHinhThucThanhToan() << ","  
                << danhSachHoaDon[i].getMaGiaoDich() << "," << danhSachHoaDon[i].getTrangThaiThanhToan() << "\n";
    }
    outFile.close();
}
