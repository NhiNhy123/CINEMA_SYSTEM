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
    string ma, ten, loai;
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

    // 1. Thêm phòng m?i vào danh sách và luu xu?ng Database
    Room newRoom(ma, ten, loai, sucChua, "HoatDong");
    danhSachPhong.push_back(newRoom);
    db.ghiDanhSachPhong(danhSachPhong);

    // 2. T? Ð?NG SINH GH?: G?i SeatManager d? t? d?ng t?o so d? gh? kh?p tuy?t d?i v?i `sucChua`
    SeatManager seatMgr;
    seatMgr.taiDuLieu(db);
    seatMgr.taoGheTuDongChoPhong(ma, db); // Hàm t? d?ng chia hàng/c?t và ghi vào file

    cout << "Them phong chieu va tu dong sinh so do ghe thanh cong!" << endl;
}

void RoomManager::suaPhong(Database& db) {
    cout << "\n--- SUA THONG TIN PHONG CHIEU ---\n";
    
    // 1. Hi?n th? danh sách phòng d? ngu?i dùng xem mã phòng
    hienThiDanhSach();

    string ma;
    cout << "\nNhap ma phong can sua (nhap 0 de quay lai): ";
    cin >> ma;
    if (ma == "0") return;

    // T?i l?i danh sách phòng m?i nh?t t? database
    taiDuLieu(db);
    bool found = false;

    for (size_t i = 0; i < danhSachPhong.size(); ++i) {
        if (danhSachPhong[i].getMaPhong() == ma) {
            found = true;
            int choice;
            do {
                // 2. In ra chi ti?t toàn b? thông tin hi?n t?i c?a phòng d? theo dõi
                cout << "\n==================================================\n";
                cout << "         THONG TIN CHI TIET PHONG DANG SUA        \n";
                cout << "==================================================\n";
                cout << "Ma phong    : " << danhSachPhong[i].getMaPhong() << "\n";
                cout << "Ten phong   : " << danhSachPhong[i].getTenPhong() << "\n";
                cout << "Loai phong  : " << danhSachPhong[i].getLoaiPhong() << "\n";
                cout << "Suc chua    : " << danhSachPhong[i].getSucChua() << " ghe\n";
                cout << "Trang thai  : " << danhSachPhong[i].getTrangThaiPhong() << "\n";
                cout << "==================================================\n";

                // Menu tùy ch?n s?a phòng
                cout << "1. Sua ten phong\n";
                cout << "2. Sua loai phong (2D, 3D, IMAX, 4DX...)\n";
                cout << "3. Sua suc chua (Tong so ghe)\n";
                cout << "4. Sua trang thai phong (HoatDong / TamNgung)\n";
                cout << "0. Hoan tat va luu thay doi\n";
                cout << "Chon thuoc tinh can sua (0-4): ";
                cin >> choice;

                cin.ignore(numeric_limits<streamsize>::max(), '\n'); // Xóa b? d?m sau khi nh?p s?

                switch (choice) {
                    case 1: {
                        string tenMoi;
                        cout << "Nhap ten phong moi (" << danhSachPhong[i].getTenPhong() << "): ";
                        getline(cin, tenMoi);
                        if (!tenMoi.empty()) {
                            danhSachPhong[i].setTenPhong(tenMoi);
                            cout << "-> Da cap nhat ten phong!\n";
                        }
                        break;
                    }
                    case 2: {
                        string loaiMoi;
                        cout << "Nhap loai phong moi (" << danhSachPhong[i].getLoaiPhong() << "): ";
                        getline(cin, loaiMoi);
                        if (!loaiMoi.empty()) {
                            danhSachPhong[i].setLoaiPhong(loaiMoi);
                            cout << "-> Da cap nhat loai phong!\n";
                        }
                        break;
                    }
                    case 3: {
                        int sucChuaCu = danhSachPhong[i].getSucChua();
                        int sucChuaMoi;
                        cout << "Nhap suc chua moi (" << sucChuaCu << "): ";
                        cin >> sucChuaMoi;
                        cin.ignore(numeric_limits<streamsize>::max(), '\n');

                        if (sucChuaMoi > 0) {
                            danhSachPhong[i].setSucChua(sucChuaMoi);
                            cout << "-> Da cap nhat suc chua thanh " << sucChuaMoi << "!\n";

                            // N?u s?c ch?a thay d?i, t? d?ng c?p nh?t l?i so d? gh? tuong ?ng
                            if (sucChuaMoi != sucChuaCu) {
                                cout << "Phat hien thay doi suc chua. Dang cap nhat lai so do ghe...\n";
                                
                                vector<Seat> danhSachGhe;
                                db.docDanhSachGhe(danhSachGhe);

                                // Xóa b? gh? cu c?a phòng này
                                for (size_t j = 0; j < danhSachGhe.size(); ) {
                                    if (danhSachGhe[j].getMaPhong() == ma) {
                                        danhSachGhe.erase(danhSachGhe.begin() + j);
                                    } else {
                                        ++j;
                                    }
                                }
                                db.ghiDanhSachGhe(danhSachGhe);

                                // Sinh l?i so d? gh? m?i kh?p v?i s?c ch?a m?i
                                SeatManager seatMgr;
                                seatMgr.taiDuLieu(db);
                                seatMgr.taoGheTuDongChoPhong(ma, db);
                            }
                        } else {
                            cout << "Loi: Suc chua phai lon hon 0!\n";
                        }
                        break;
                    }
                    case 4: {
                        string trangThaiMoi;
                        cout << "Nhap trang thai moi (HoatDong / TamNgung) (" << danhSachPhong[i].getTrangThaiPhong() << "): ";
                        getline(cin, trangThaiMoi);
                        if (!trangThaiMoi.empty()) {
                            danhSachPhong[i].setTrangThaiPhong(trangThaiMoi);
                            cout << "-> Da cap nhat trang thai phong!\n";
                        }
                        break;
                    }
                    case 0: {
                        // Luu toàn b? danh sách phòng sau khi ch?nh s?a vào database
                        db.ghiDanhSachPhong(danhSachPhong);
                        cout << "\nLuu thay doi thong tin phong thanh cong!\n";
                        break;
                    }
                    default:
                        cout << "Lua chon khong hop le! Vui long chon lai.\n";
                }
            } while (choice != 0);

            break;
        }
    }

    if (!found) {
        cout << "Khong tim thay phong co ma: " << ma << endl;
    }
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
