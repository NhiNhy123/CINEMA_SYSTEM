#include "Room.h"
#include "Seat.h"      // B? sung include này d? g?i SeatManager
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
        ssSucChua >> sucChua;
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
    cout << "Nhap ma phong (VD: R01): ";
    cin >> ma;

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
    hienThiDanhSach();
    cout << "Nhap ma phong can sua: ";
    cin >> ma;

    for (size_t i = 0; i < danhSachPhong.size(); ++i) {
        if (danhSachPhong[i].getMaPhong() != ma) continue;

        bool daThayDoi = false;
        int luaChon = -1;
        do {
            cout << "\n--- CHINH SUA PHONG " << danhSachPhong[i].getMaPhong() << " ---\n";
            cout << "Ten phong   : " << danhSachPhong[i].getTenPhong() << "\n";
            cout << "Loai phong  : " << danhSachPhong[i].getLoaiPhong() << "\n";
            cout << "Suc chua    : " << danhSachPhong[i].getSucChua() << "\n";
            cout << "Trang thai  : " << danhSachPhong[i].getTrangThaiPhong() << "\n";
            cout << "1. Sua ten phong\n";
            cout << "2. Sua loai phong\n";
            cout << "3. Sua suc chua\n";
            cout << "4. Sua trang thai\n";
            cout << "0. Hoan tat va luu\n";
            cout << "Chon thuoc tinh can sua: ";

            if (!(cin >> luaChon)) {
                cin.clear();
                cin.ignore(numeric_limits<streamsize>::max(), '\n');
                cout << "Loi: Vui long nhap lua chon bang so!\n";
                continue;
            }

            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            switch (luaChon) {
                case 1: {
                    string tenMoi;
                    cout << "Nhap ten phong moi: ";
                    getline(cin, tenMoi);
                    if (tenMoi.empty()) {
                        cout << "Ten phong khong duoc de trong!\n";
                    } else {
                        danhSachPhong[i].setTenPhong(tenMoi);
                        daThayDoi = true;
                    }
                    break;
                }
                case 2: {
                    string loaiMoi;
                    cout << "Nhap loai phong moi (2D, 3D, IMAX, 4DX...): ";
                    cin >> loaiMoi;
                    danhSachPhong[i].setLoaiPhong(loaiMoi);
                    daThayDoi = true;
                    break;
                }
                case 3: {
                    int sucChuaMoi;
                    cout << "Nhap suc chua moi (> 0): ";
                    if (!(cin >> sucChuaMoi) || sucChuaMoi <= 0) {
                        cin.clear();
                        cin.ignore(numeric_limits<streamsize>::max(), '\n');
                        cout << "Loi: Suc chua phai la so nguyen lon hon 0!\n";
                    } else {
                        danhSachPhong[i].setSucChua(sucChuaMoi);
                        daThayDoi = true;
                    }
                    break;
                }
                case 4: {
                    int trangThai;
                    cout << "1. Hoat dong\n";
                    cout << "2. Tam ngung\n";
                    cout << "Chon trang thai moi: ";
                    if (!(cin >> trangThai) || (trangThai != 1 && trangThai != 2)) {
                        cin.clear();
                        cin.ignore(numeric_limits<streamsize>::max(), '\n');
                        cout << "Lua chon trang thai khong hop le!\n";
                    } else {
                        danhSachPhong[i].setTrangThaiPhong(
                            trangThai == 1 ? "HoatDong" : "TamNgung");
                        daThayDoi = true;
                    }
                    break;
                }
                case 0:
                    if (daThayDoi) {
                        db.ghiDanhSachPhong(danhSachPhong);
                        cout << "Cap nhat thong tin phong thanh cong!\n";
                    } else {
                        cout << "Khong co thay doi nao.\n";
                    }
                    break;
                default:
                    cout << "Lua chon khong hop le!\n";
                    break;
            }
        } while (luaChon != 0);
        return;
    }
    cout << "Khong tim thay phong co ma: " << ma << endl;
}

bool RoomManager::kiemTraPhongDangChieu(const string& maPhong) const {
    Database db;
    vector<Showtime> dsShowtime;
    db.docDanhSachSuatChieu(dsShowtime);

    for (size_t i = 0; i < dsShowtime.size(); ++i) {
        if (dsShowtime[i].getMaPhong() == maPhong) {
            return true;
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
