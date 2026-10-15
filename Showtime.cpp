#include "Showtime.h"
#include "Movie.h"
#include "Room.h"
#include "Database.h"
#include <iostream>
#include <iomanip>
#include <sstream>
#include <vector>

// Constructor m?c d?nh & d?y d? tham s?
Showtime::Showtime() { giaVeCoBan = 0.0; }
Showtime::Showtime(string maSC, string maPhim, string maPhong, string ngay, string gioBD, string gioKT, double giaVe) {
    this->maSuatChieu = maSC;
    this->maPhim = maPhim;
    this->maPhong = maPhong;
    this->ngayChieu = ngay;
    this->gioBatDau = gioBD;
    this->gioKetThuc = gioKT;
    this->giaVeCoBan = giaVe;
}

// Getters
string Showtime::getMaSuatChieu() const { return maSuatChieu; }
string Showtime::getMaPhim() const { return maPhim; }
string Showtime::getMaPhong() const { return maPhong; }
string Showtime::getNgayChieu() const { return ngayChieu; }
string Showtime::getGioBatDau() const { return gioBatDau; }
string Showtime::getGioKetThuc() const { return gioKetThuc; }
double Showtime::getGiaVeCoBan() const { return giaVeCoBan; }

// Chuy?n d?i CSV
string Showtime::toCSV() const {
    stringstream ss;
    ss << maSuatChieu << "," << maPhim << "," << maPhong << "," << ngayChieu << "," << gioBatDau << "," << gioKetThuc << "," << giaVeCoBan;
    return ss.str();
}

Showtime Showtime::fromCSV(const string& line) {
    stringstream ss(line);
    string maSC, maPhim, maPhong, ngay, gioBD, gioKT, strGia;
    getline(ss, maSC, ',');
    getline(ss, maPhim, ',');
    getline(ss, maPhong, ',');
    getline(ss, ngay, ',');
    getline(ss, gioBD, ',');
    getline(ss, gioKT, ',');
    getline(ss, strGia, ',');

    double gia = 0.0;
    if (!strGia.empty()) {
        stringstream ssGia(strGia);
        ssGia >> gia;
    }
    return Showtime(maSC, maPhim, maPhong, ngay, gioBD, gioKT, gia);
}

// Hàm hi?n th? m?t su?t chi?u don l? (Dùng cho hienThiSuatChieuTheoPhim vì dã có tiêu d? ch?a tên phim)
void Showtime::hienThi() const {
    stringstream ssGia;
    ssGia << fixed << setprecision(0) << giaVeCoBan << " VND";
    cout << left << setw(12) << maSuatChieu 
         << setw(10) << maPhong 
         << setw(12) << ngayChieu 
         << setw(15) << (gioBatDau + " - " + gioKetThuc)
         << setw(15) << ssGia.str() << endl;
}

// ==========================================
// CÁC HÀM X? LÝ NGHI?P V? (QU?N LÝ SU?T CHI?U)
// ==========================================

void Showtime::themSuatChieu(Database& db) {
    string maSC, maPhim, maPhong, ngay, gioBD, gioKT;
    double giaVe;

    cout << "\n--- THEM SUAT CHIEU MOI ---\n";
    cout << "Nhap ma suat chieu (VD: SC10): "; cin >> maSC;

    // L?y danh sách su?t chi?u t? Database tru?c d? ki?m tra trùng mã
    vector<Showtime> danhSach;
    db.docDanhSachSuatChieu(danhSach);

    // Ki?m tra trùng mã su?t chi?u
    for (size_t i = 0; i < danhSach.size(); ++i) {
        if (danhSach[i].getMaSuatChieu() == maSC) {
            cout << "Loi: Ma suat chieu '" << maSC << "' da ton tai trong he thong!\n";
            return;
        }
    }

    cout << "Nhap ma phim: "; cin >> maPhim;

    // Ki?m tra mã phim có t?n t?i hay không
    vector<Movie> dsPhim;
    db.docDanhSachPhim(dsPhim);
    bool tonTaiPhim = false;
    for (size_t i = 0; i < dsPhim.size(); ++i) {
        if (dsPhim[i].getMaPhim() == maPhim) {
            tonTaiPhim = true;
            break;
        }
    }
    if (!tonTaiPhim) {
        cout << "Loi: Ma phim '" << maPhim << "' khong ton tai trong he thong!\n";
        return;
    }

    cout << "Nhap ma phong chieu: "; cin >> maPhong;

    // Ki?m tra mã phòng có t?n t?i hay không
    vector<Room> dsPhong;
    db.docDanhSachPhong(dsPhong);
    bool tonTaiPhong = false;
    for (size_t i = 0; i < dsPhong.size(); ++i) {
        if (dsPhong[i].getMaPhong() == maPhong) {
            tonTaiPhong = true;
            break;
        }
    }
    if (!tonTaiPhong) {
        cout << "Loi: Ma phong '" << maPhong << "' khong ton tai trong he thong!\n";
        return;
    }

    cout << "Nhap ngay chieu (YYYY-MM-DD): "; cin >> ngay;
    cout << "Nhap gio bat dau (HH:MM): "; cin >> gioBD;
    cout << "Nhap gio ket thuc (HH:MM): "; cin >> gioKT;
    
    do {
        cout << "Nhap gia ve co ban (> 0): "; cin >> giaVe;
        if (giaVe <= 0) cout << "Loi: Gia ve phai lon hon 0!\n";
    } while (giaVe <= 0);

    // RÀNG BU?C LOGIC: Ki?m tra trùng phòng và ch?ng chéo th?i gian
    for (size_t i = 0; i < danhSach.size(); ++i) {
        if (danhSach[i].getMaPhong() == maPhong && danhSach[i].getNgayChieu() == ngay) {
            // Ki?m tra xung d?t khung gi?
            if (!(gioKT <= danhSach[i].getGioBatDau() || gioBD >= danhSach[i].getGioKetThuc())) {
                cout << "Loi: Da co suat chieu trong thoi gian nay tai phong " << maPhong << "! Vui long chon thoi gian khac.\n";
                return;
            }
        }
    }

    // N?u h?p l?, t?o d?i tu?ng và thêm vào danh sách
    Showtime moi(maSC, maPhim, maPhong, ngay, gioBD, gioKT, giaVe);
    danhSach.push_back(moi);

    // Ghi xu?ng file Database
    db.ghiDanhSachSuatChieu(danhSach);
    cout << "Them suat chieu thanh công!\n";
}

void Showtime::hienThiSuatChieuTheoPhim(const string& maPhimCanXem, Database& db) {
    // 1. L?y tên phim t? danh sách phim d? hi?n th? tiêu d? tr?c quan
    vector<Movie> dsPhim;
    db.docDanhSachPhim(dsPhim);
    string tenPhim = maPhimCanXem; 
    for (size_t i = 0; i < dsPhim.size(); ++i) {
        if (dsPhim[i].getMaPhim() == maPhimCanXem) {
            tenPhim = dsPhim[i].getTenPhim();
            break;
        }
    }

    // 2. Ð?c danh sách su?t chi?u
    vector<Showtime> danhSach;
    db.docDanhSachSuatChieu(danhSach);

    cout << "\n--- DANH SACH SUAT CHIEU CUA PHIM: " << tenPhim << " ---\n";
    cout << left << setw(12) << "Ma SC" << setw(10) << "Phong" << setw(12) << "Ngay" << setw(15) << "Khung gio" << setw(15) << "Gia ve" << endl;
    cout << "-------------------------------------------------------------\n";

    bool found = false;
    for (size_t i = 0; i < danhSach.size(); ++i) {
        if (danhSach[i].getMaPhim() == maPhimCanXem) {
            danhSach[i].hienThi();
            found = true;
        }
    }

    if (!found) {
        cout << "Hien tai khong co suat chieu nao cho bo phim nay!\n";
    }
}

void Showtime::hienThiTatCa(Database& db) {
    // L?y danh sách phim d? ánh x? mã phim sang tên phim
    vector<Movie> dsPhim;
    db.docDanhSachPhim(dsPhim);

    vector<Showtime> danhSach;
    db.docDanhSachSuatChieu(danhSach);

    cout << "\n================================ TAT CA SUAT CHIEU TRONG HE THONG =================================\n";
    cout << left << setw(12) << "Ma SC" << setw(30) << "Ten Phim" << setw(10) << "Phong" << setw(12) << "Ngay" << setw(15) << "Khung gio" << setw(15) << "Gia ve" << endl;
    cout << "---------------------------------------------------------------------------------------------------\n";
    
    for (size_t i = 0; i < danhSach.size(); ++i) {
        // Tra c?u tên phim tuong ?ng v?i mã phim c?a su?t chi?u
        string tenPhimHienThi = danhSach[i].getMaPhim();
        for (size_t j = 0; j < dsPhim.size(); ++j) {
            if (dsPhim[j].getMaPhim() == danhSach[i].getMaPhim()) {
                tenPhimHienThi = dsPhim[j].getTenPhim();
                break;
            }
        }

        stringstream ssGia;
        ssGia << fixed << setprecision(0) << danhSach[i].getGiaVeCoBan() << " VND";

        // In ra tên phim thay vì mã phim ? c?t th? 2
        cout << left << setw(12) << danhSach[i].getMaSuatChieu() 
             << setw(30) << tenPhimHienThi 
             << setw(10) << danhSach[i].getMaPhong() 
             << setw(12) << danhSach[i].getNgayChieu() 
             << setw(15) << (danhSach[i].getGioBatDau() + " - " + danhSach[i].getGioKetThuc())
             << setw(15) << ssGia.str() << endl;
    }
    cout << "===================================================================================================\n";
}
