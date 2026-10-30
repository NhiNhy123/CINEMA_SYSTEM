#include "../include/Seat.h"
#include "../include/Database.h"
#include "../include/Room.h"
#include <iostream>
#include <iomanip>
#include <sstream>
#include <limits>

using namespace std;

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

string Seat::getMaGhe() const { return maGhe; }
string Seat::getMaPhong() const { return maPhong; }
string Seat::getLoaiGhe() const { return loaiGhe; }
double Seat::getDonGiaGhe() const { return donGiaGhe; }
string Seat::getTrangThai() const { return trangThai; }

void Seat::setLoaiGhe(const string& loai) { loaiGhe = loai; }
void Seat::setDonGiaGhe(double gia) { if (gia > 0) donGiaGhe = gia; }
void Seat::setTrangThai(const string& tt) { trangThai = tt; }

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
    string hienThiTT = "[ ]";
    if (trangThai == "DangChon" || trangThai.find("DangChon|") == 0) hienThiTT = "[?]";
    else if (trangThai == "DaDat") hienThiTT = "[X]";
    else if (trangThai == "KhongSuDung") hienThiTT = "[-]";

    cout << maGhe << hienThiTT << "(" << loaiGhe << ": "
         << fixed << setprecision(0) << donGiaGhe << " VND) ";
}

SeatManager::SeatManager() {}

void SeatManager::taiDuLieu(Database& db) {
    db.docDanhSachGhe(danhSachGhe);
}

void SeatManager::taoGheTuDongChoPhong(const string& maPhong, Database& db) {
    vector<Room> danhSachPhong;
    db.docDanhSachPhong(danhSachPhong);

    for (size_t i = 0; i < danhSachPhong.size(); ++i) {
        if (danhSachPhong[i].getMaPhong() == maPhong) {
            db.taoGheTuDongChoPhong(maPhong, danhSachPhong[i].getSucChua());
            taiDuLieu(db);
            return;
        }
    }

    cout << "Khong tim thay phong " << maPhong << "!\n";
}

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
        cout << "Nhap gia ban day du cho moi ghe (Phai > 0): ";
        cin >> donGia;
        if (donGia <= 0) cout << "Loi: Don gia khong duoc am hoac bang 0!\n";
    } while (donGia <= 0);

    for (char row = batDauHang; row <= ketThucHang; ++row) {
        for (int col = 1; col <= soGheMoiHang; ++col) {
            stringstream ss;
            ss << row << col;
            string maGhe = ss.str();

            Seat newSeat(maGhe, maPhong, loaiGhe, donGia, "Trong");
            danhSachGhe.push_back(newSeat);
        }
    }

    db.ghiDanhSachGhe(danhSachGhe);
    cout << "Da khoi tao va luu thanh cong so do ghe cho phong " << maPhong << "!\n";
}

void SeatManager::saoChepBucLuc(const string& phongDich, const string& phongNguon, Database& db) {
    int count = 0;

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

void SeatManager::hienThiSoDoPhong(const string& maPhong) const {
    cout << "\n==================== SO DO GHE PHONG CHIEU: " << maPhong << " ====================\n";
    cout << " [MAN HINH CHIEU PHIM]\n";
    cout << "-----------------------------------------------------------------------\n";

    bool found = false;

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

void SeatManager::hienThiDanhSachLoaiGhe(Database& db) const {
    vector<pair<string, double> > danhSachLoaiGhe;
    db.docDanhSachLoaiGhe(danhSachLoaiGhe);
    if (danhSachLoaiGhe.empty()) {
        cout << "Danh muc loai ghe dang trong!\n";
        return;
    }

    cout << "\n========== DANH MUC LOAI GHE ==========\n";
    cout << left << setw(6) << "STT" << setw(24) << "Loai ghe" << "Gia ban day du (VND)\n";
    for (size_t i = 0; i < danhSachLoaiGhe.size(); ++i) {
        cout << left << setw(6) << (i + 1) << setw(24) << danhSachLoaiGhe[i].first
             << fixed << setprecision(0) << danhSachLoaiGhe[i].second << "\n";
    }
    cout << "=======================================\n";
}

void SeatManager::themLoaiGhe(Database& db) {
    string tenLoaiGhe;
    double donGia;
    cin.ignore(numeric_limits<streamsize>::max(), '\n');
    cout << "Nhap ten loai ghe moi: ";
    getline(cin, tenLoaiGhe);
    cout << "Nhap gia ban day du cho moi ghe (VND, > 0): ";
    if (!(cin >> donGia) || donGia <= 0) {
        cin.clear();
        cin.ignore(numeric_limits<streamsize>::max(), '\n');
        cout << "Loi: Don gia phai la so lon hon 0!\n";
        return;
    }
    if (!db.themLoaiGhe(tenLoaiGhe, donGia)) {
        cout << "Loi: Ten loai ghe da ton tai, du lieu khong hop le hoac khong the luu file!\n";
        return;
    }
    cout << "Da them loai ghe thanh cong!\n";
}

void SeatManager::suaLoaiGhe(Database& db) {
    vector<pair<string, double> > danhSachLoaiGhe;
    db.docDanhSachLoaiGhe(danhSachLoaiGhe);
    hienThiDanhSachLoaiGhe(db);
    if (danhSachLoaiGhe.empty()) return;

    size_t stt;
    cout << "Chon STT loai ghe can sua (0 de huy): ";
    if (!(cin >> stt) || stt > danhSachLoaiGhe.size()) {
        cin.clear();
        cin.ignore(numeric_limits<streamsize>::max(), '\n');
        cout << "Lua chon khong hop le!\n";
        return;
    }
    if (stt == 0) return;

    string tenMoi;
    double donGiaMoi;
    cin.ignore(numeric_limits<streamsize>::max(), '\n');
    cout << "Nhap ten loai ghe moi: ";
    getline(cin, tenMoi);
    cout << "Nhap gia ban day du moi (VND, > 0): ";
    if (!(cin >> donGiaMoi) || donGiaMoi <= 0) {
        cin.clear();
        cin.ignore(numeric_limits<streamsize>::max(), '\n');
        cout << "Loi: Don gia phai la so lon hon 0!\n";
        return;
    }
    if (!db.suaLoaiGhe(danhSachLoaiGhe[stt - 1].first, tenMoi, donGiaMoi)) {
        cout << "Loi: Ten loai ghe bi trung, khong ton tai hoac khong the luu file!\n";
        return;
    }
    taiDuLieu(db);
    cout << "Da cap nhat loai ghe va gia ghe dang su dung!\n";
}

void SeatManager::xoaLoaiGhe(Database& db) {
    vector<pair<string, double> > danhSachLoaiGhe;
    db.docDanhSachLoaiGhe(danhSachLoaiGhe);
    hienThiDanhSachLoaiGhe(db);
    if (danhSachLoaiGhe.empty()) return;

    size_t stt;
    cout << "Chon STT loai ghe can xoa (0 de huy): ";
    if (!(cin >> stt) || stt > danhSachLoaiGhe.size()) {
        cin.clear();
        cin.ignore(numeric_limits<streamsize>::max(), '\n');
        cout << "Lua chon khong hop le!\n";
        return;
    }
    if (stt == 0) return;
    if (!db.xoaLoaiGhe(danhSachLoaiGhe[stt - 1].first)) {
        cout << "Loi: Khong tim thay loai ghe, khong the luu, hoac van con ghe dang su dung loai nay!\n";
        return;
    }
    cout << "Da xoa loai ghe!\n";
}

void SeatManager::chonGheTamThoi(const string& maGhe, const string& maPhong, Database& db) {
    bool found = false;

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
