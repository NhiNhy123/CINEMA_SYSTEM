#include "Room.h"
#include "Database.h"
#include "Showtime.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <string>
#include <limits>
using namespace std;

Room::Room() {
    this->sucChua = 0;
    this->trangThaiPhong = "HoatDong";
}

Room::Room(string ma, string ten, string loai, int sucChua, string trangThai) {
    this->maPhong = ma;
    this->tenPhong = ten;
    this->loaiPhong = loai;
    this->sucChua = sucChua;
    this->trangThaiPhong = trangThai;
}

string Room::getMaPhong() const { return maPhong; }
string Room::getTenPhong() const { return tenPhong; }
string Room::getLoaiPhong() const { return loaiPhong; }
int Room::getSucChua() const { return sucChua; }
string Room::getTrangThaiPhong() const { return trangThaiPhong; }

void Room::setTenPhong(const string& ten) { tenPhong = ten; }
void Room::setLoaiPhong(const string& loai) { loaiPhong = loai; }
void Room::setSucChua(int sc) { sucChua = sc; }
void Room::setTrangThaiPhong(const string& tt) { trangThaiPhong = tt; }

void Room::hienThi() const {
    cout << left << setw(10) << maPhong 
         << setw(20) << tenPhong 
         << setw(12) << loaiPhong 
         << setw(12) << sucChua 
         << setw(15) << trangThaiPhong << endl;
}

string Room::toCSV() const {
    stringstream ss;
    ss << sucChua;
    return maPhong + "," + tenPhong + "," + loaiPhong + "," + ss.str() + "," + trangThaiPhong;
}

Room Room::fromCSV(const string& line) {
    stringstream ss(line);
    string ma, ten, loai, strSucChua, trangThai;

    getline(ss, ma, ',');
    getline(ss, ten, ',');
    getline(ss, loai, ',');
    getline(ss, strSucChua, ',');
    getline(ss, trangThai, ',');

    int sucChua = 0;
    if (!strSucChua.empty()) {
        stringstream ssSucChua(strSucChua);
        ssSucChua >> sucChua; // Ð?c giá tr? t? stringstream vào int
    }
    
    return Room(ma, ten, loai, sucChua, trangThai);
}

void RoomManager::taiDuLieu(Database& db) {
    db.docDanhSachPhong(danhSachPhong);
}

void RoomManager::hienThiDanhSach() const {
    if (danhSachPhong.empty()) {
        cout << "Danh sach phong chieu dang trong!" << endl;
        return;
    }
    cout << "\n==================== DANH SACH PHONG CHIEU ====================\n";
    cout << left << setw(10) << "Ma Phong" 
         << setw(20) << "Ten Phong" 
         << setw(12) << "Loai Phong" 
         << setw(12) << "Suc Chua" 
         << setw(15) << "Trang Thai" << endl;
    cout << "---------------------------------------------------------------\n";
    for (size_t i = 0; i < danhSachPhong.size(); ++i) {
        danhSachPhong[i].hienThi();
    }
    cout << "===============================================================\n";
}

void RoomManager::themPhong(Database& db) {
    string ma, ten, loai, trangThai;
    int sucChua;

    cout << "\n--- THEM PHONG CHIEU MOI ---\n";
    cout << "Nhap ma phong (VD: P01): ";
    cin >> ma;

    // Kiem tra ma phong da ton tai chua
    for (size_t i = 0; i < danhSachPhong.size(); ++i) {
        if (danhSachPhong[i].getMaPhong() == ma) {
            cout << "Loi: Ma phong nay da ton tai trong co so du lieu!" << endl;
            return;
        }
    }

    cin.ignore(numeric_limits<streamsize>::max(), '\n');

    cout << "Nhap ten phong (VD: Phong 01, Phong IMAX): ";
    getline(cin, ten);
    
    cout << "Nhap loai phong (2D, 3D, IMAX, 4DX...): ";
    cin >> loai;
    
    cout << "Nhap suc chua tong the: ";
    cin >> sucChua;

    Room newRoom(ma, ten, loai, sucChua, "HoatDong");
    danhSachPhong.push_back(newRoom);
    db.ghiDanhSachPhong(danhSachPhong);

    cout << "Them phong chieu thanh cong va da luu vao file he thong!" << endl;
}

void RoomManager::suaPhong(Database& db) {
    string ma;
    cout << "\n--- SUA THONG TIN PHONG CHIEU ---\n";
    cout << "Nhap ma phong can sua: ";
    cin >> ma;

    bool found = false;
    // Dùng vòng l?p for truy?n th?ng thay cho range-based for
    for (size_t i = 0; i < danhSachPhong.size(); ++i) {
        if (danhSachPhong[i].getMaPhong() == ma) {
            found = true;
            string ten, loai, trangThai;
            int sucChua;

            cin.ignore();
            cout << "Nhap ten phong moi (" << danhSachPhong[i].getTenPhong() << "): ";
            getline(cin, ten);
            cout << "Nhap loai phong moi (2D, 3D, IMAX, 4DX...) (" << danhSachPhong[i].getLoaiPhong() << "): ";
            cin >> loai;
            cout << "Nhap suc chua moi (" << danhSachPhong[i].getSucChua() << "): ";
            cin >> sucChua;
            cout << "Nhap trang thai moi (HoatDong / TamNgung) (" << danhSachPhong[i].getTrangThaiPhong() << "): ";
            cin >> trangThai;

            danhSachPhong[i].setTenPhong(ten);
            danhSachPhong[i].setLoaiPhong(loai);
            danhSachPhong[i].setSucChua(sucChua);
            danhSachPhong[i].setTrangThaiPhong(trangThai);

            db.ghiDanhSachPhong(danhSachPhong);
            cout << "Cap nhat thong tin phong thanh cong!" << endl;
            break;
        }
    }
    if (!found) {
        cout << "Khong tim thay phong co ma: " << ma << endl;
    }
}

bool RoomManager::kiemTraPhongDangChieu(const string& maPhong) const {
    Database db; // Kh?i t?o d?i tu?ng Database t?m th?i
    vector<Showtime> dsShowtime;
    db.docDanhSachSuatChieu(dsShowtime); // Ð?c danh sách su?t chi?u
    
    for (size_t i = 0; i < dsShowtime.size(); ++i) {
        if (dsShowtime[i].getMaPhong() == maPhong) {
            return true; // Phòng dang có su?t chi?u, không du?c xóa
        }
    }
    return false;  
}

void RoomManager::xoaPhong(Database& db) {
    string ma;
    cout << "\n--- XOA PHONG CHIEU ---\n";
    cout << "Nhap ma phong can xoa: ";
    cin >> ma;

    if (kiemTraPhongDangChieu(ma)) {
        cout << "LOI: Khong the xoa phong nay vi dang co phim chieu/suat chieu hoat dong!" << endl;
        return;
    }

    for (size_t i = 0; i < danhSachPhong.size(); ++i) {
        if (danhSachPhong[i].getMaPhong() == ma) {
            danhSachPhong.erase(danhSachPhong.begin() + i);
            db.ghiDanhSachPhong(danhSachPhong);
            cout << "Xoa phong chieu thanh cong!" << endl;
            return;
        }
    }
    cout << "Khong tim thay phong co ma: " << ma << endl;
}
