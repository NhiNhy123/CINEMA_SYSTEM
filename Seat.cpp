#include "Seat.h"
#include "Database.h"
#include <iostream>
#include <iomanip>
#include <sstream>

using namespace std;

// --- CLASS SEAT ---

Seat::Seat() {
    this->donGiaGhe = 0.0;
    this->trangThai = "Trong";
}

Seat::Seat(string maGhe, string maPhong, string loaiGhe, double donGiaGhe, string trangThai) {
    this->maGhe = maGhe;
    this->maPhong = maPhong;
    this->loaiGhe = loaiGhe;
    this->donGiaGhe = donGiaGhe;
    this->trangThai = trangThai;
}

// Getters
string Seat::getMaGhe() const { return maGhe; }
string Seat::getMaPhong() const { return maPhong; }
string Seat::getLoaiGhe() const { return loaiGhe; }
double Seat::getDonGiaGhe() const { return donGiaGhe; }
string Seat::getTrangThai() const { return trangThai; }

// Setters
void Seat::setLoaiGhe(const string& loai) { loaiGhe = loai; }
void Seat::setDonGiaGhe(double gia) { if (gia > 0) donGiaGhe = gia; }
void Seat::setTrangThai(const string& tt) { trangThai = tt; }
// --- B? SUNG VÀO CLASS SEAT TRONG Seat.cpp ---

string Seat::toCSV() const {
    stringstream ss;
    ss << fixed << setprecision(0) << donGiaGhe;
    return maGhe + "," + maPhong + "," + loaiGhe + "," + ss.str() + "," + trangThai;
}

Seat Seat::fromCSV(const string& line) {
    stringstream ss(line);
    string maGhe, maPhong, loaiGhe, strGia, trangThai;

    getline(ss, maGhe, ',');
    getline(ss, maPhong, ',');
    getline(ss, loaiGhe, ',');
    getline(ss, strGia, ',');
    getline(ss, trangThai, ',');

    double donGia = 0.0;
    if (!strGia.empty()) {
        stringstream ssGia(strGia);
        ssGia >> donGia;
    }

    return Seat(maGhe, maPhong, loaiGhe, donGia, trangThai);
}

void Seat::hienThi() const {
    string hienThiTT = "[ ]"; // Tr?ng
    if (trangThai == "DangChon") hienThiTT = "[?]"; // Ðang ch?n (Khóa t?m 15p)
    else if (trangThai == "DaDat") hienThiTT = "[X]"; // Ðã d?t
    else if (trangThai == "KhongSuDung") hienThiTT = "[-]";

    cout << maGhe << hienThiTT << "(" << loaiGhe << ") ";
}


// --- CLASS SEAT MANAGER ---

SeatManager::SeatManager() {}

void SeatManager::taiDuLieu(Database& db) {
    db.docDanhSachGhe(danhSachGhe);
}

// 1. Phân h? Admin: T?o gh? hàng lo?t theo hàng và c?t
void SeatManager::taoGheHangLoat(const string& maPhong, Database& db) {
    char batDauHang, ketThucHang;
    int soGheMoiHang;
    string loaiGhe;
    double donGia;

    cout << "\n--- TAO SO DO GHE HANG LOAT CHO PHONG: " << maPhong << " ---\n";
    cout << "Nhap hang bat dau (VD: A): ";
    cin >> batDauHang;
    cout << "Nhap hang ket thuc (VD: D): ";
    cin >> ketThucHang;
    cout << "Nhap so luong ghe moi hang (VD: 10): ";
    cin >> soGheMoiHang;
    cout << "Nhap loai ghe chung (Standard / VIP / Couple): ";
    cin >> loaiGhe;
    
    do {
        cout << "Nhap don gia co ban (Phai > 0): ";
        cin >> donGia;
        if (donGia <= 0) cout << "Loi: Don gia khong duoc am hoac bang 0!\n";
    } while (donGia <= 0);

    // Sinh danh sach ghe moi vao bo nho
    for (char row = batDauHang; row <= ketThucHang; ++row) {
        for (int col = 1; col <= soGheMoiHang; ++col) {
            stringstream ss;
            ss << row << col;
            string maGhe = ss.str();
        
            Seat newSeat(maGhe, maPhong, loaiGhe, donGia, "Trong");
            danhSachGhe.push_back(newSeat);
        }
    }

    // Uy quyen cho Database ghi toan bo xuong file he thong
    db.ghiDanhSachGhe(danhSachGhe);
    cout << "Da khoi tao va luu thanh cong so do ghe cho phong " << maPhong << "!\n";
}

// 2. Tính nang thông minh: Sao chép b? c?c gh? t? phòng cu sang phòng m?i
void SeatManager::saoChepBucLuc(const string& phongDich, const string& phongNguon, Database& db) {
    int count = 0;
    
    // Luu y: Dung kich thuoc ban dau de trinh vong lap vo han neu vector tu dong cap phat lai bo nho
    size_t n = danhSachGhe.size();
    for (size_t i = 0; i < n; ++i) {
        if (danhSachGhe[i].getMaPhong() == phongNguon) {
            string maGheMoi = danhSachGhe[i].getMaGhe();
            Seat copiedSeat(maGheMoi, phongDich, danhSachGhe[i].getLoaiGhe(), danhSachGhe[i].getDonGiaGhe(), "Trong");
            danhSachGhe.push_back(copiedSeat);
            count++;
        }
    }

    if (count > 0) {
        db.ghiDanhSachGhe(danhSachGhe);
        cout << "Da sao chep thanh cong " << count << " ghe tu phong " << phongNguon << " sang phong " << phongDich << "!\n";
    } else {
        cout << "Khong tim thay du lieu ghe cua phong nguon " << phongNguon << " de sao chep!\n";
    }
}

// Hi?n th? so d? phòng d?ng tr?c quan
void SeatManager::hienThiSoDoPhong(const string& maPhong) const {
    cout << "\n==================== SO DO GHE PHONG CHIEU: " << maPhong << " ====================\n";
    cout << " [MAN HINH CHIEU PHIM]\n";
    cout << "-----------------------------------------------------------------------\n";
    
    bool found = false;
    // Dung vong lap for truyen thong thay cho range-based for
    for (size_t i = 0; i < danhSachGhe.size(); ++i) {
        if (danhSachGhe[i].getMaPhong() == maPhong) {
            danhSachGhe[i].hienThi();
            found = true;
        }
    }
    
    if (!found) {
        cout << "Phong nay chua duoc thiet lap so do ghe!\n";
    }
    cout << "\n-----------------------------------------------------------------------\n";
    cout << "Chu thich: [ ] Trong | [?] Dang chon (Khoa tam 15p) | [X] Da dat\n";
    cout << "=======================================================================\n";
}

// 3. Phân h? Staff: Ch?n gh? tuong tác (Khóa t?m)
void SeatManager::chonGheTamThoi(const string& maGhe, const string& maPhong, Database& db) {
    bool found = false;
    
    // Dung vong lap for truyen thong thay cho range-based for
    for (size_t i = 0; i < danhSachGhe.size(); ++i) {
        if (danhSachGhe[i].getMaPhong() == maPhong && danhSachGhe[i].getMaGhe() == maGhe) {
            found = true;
            if (danhSachGhe[i].getTrangThai() == "DaDat") {
                cout << "Ghe " << maGhe << " da co khach dat! Vui long chon ghe khac.\n";
                return;
            }
            if (danhSachGhe[i].getTrangThai() == "DangChon") {
                cout << "Ghe " << maGhe << " dang duoc giu cho tam thoi o quay khac!\n";
                return;
            }
            danhSachGhe[i].setTrangThai("DangChon");
            db.ghiDanhSachGhe(danhSachGhe);
            cout << "Da giu cho thanh cong ghe " << maGhe << " (Khoa tam 15 phut).\n";
            return;
        }
    }
    
    if (!found) {
        cout << "Khong tim thay ghe " << maGhe << " trong phong nay!\n";
    }
}
