#include "Seat.h"
#include "Database.h"
#include "Room.h"
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
void SeatManager::taoGheTuDongChoPhong(const string& maPhongCanTao, Database& db) {
    // 1. Ð?c danh sách phòng d? l?y dúng "sucChua" c?a phòng này
    vector<Room> danhSachPhong;
    db.docDanhSachPhong(danhSachPhong);

    int sucChuaPhong = 0;
    bool timThay = false;
    for (size_t i = 0; i < danhSachPhong.size(); ++i) {
        if (danhSachPhong[i].getMaPhong() == maPhongCanTao) {
            sucChuaPhong = danhSachPhong[i].getSucChua();
            timThay = true;
            break;
        }
    }

    if (!timThay) {
        cout << "Khong tim thay phong co ma: " << maPhongCanTao << "!\n";
        return;
    }

    // 2. T?i danh sách gh? hi?n t?i lên b? nh?
    taiDuLieu(db);

    // 3. T? d?ng thu?t toán sinh gh? v?a v?n v?i s?c ch?a (M?i hàng 10 gh?)
    char hang = 'A';
    int soThuTu = 1;
    int soGheMoiHang = 10;

    for (int i = 1; i <= sucChuaPhong; ++i) {
        stringstream ss;
        ss << hang << soThuTu; // T?o mã A1, A2... A10, B1...
        string maGhe = ss.str();

        // Phân lo?i gh? t? d?ng: Hàng A, B là Standard (giá 60k), t? hàng C là VIP (giá 90k)
        string loaiGhe = (hang <= 'B') ? "Standard" : "VIP";
        double donGia = (loaiGhe == "VIP") ? 90000 : 60000;

        Seat newSeat(maGhe, maPhongCanTao, loaiGhe, donGia, "Trong");
        danhSachGhe.push_back(newSeat);

        soThuTu++;
        if (soThuTu > soGheMoiHang) {
            soThuTu = 1;
            hang++; // Ð? 10 gh? thì t? d?ng xu?ng hàng ti?p theo
        }
    }

    // 4. Ghi l?i toàn b? database an toàn qua Database
    db.ghiDanhSachGhe(danhSachGhe);
    cout << "-> Da tu dong sinh thanh cong " << sucChuaPhong << " ghe cho phong " << maPhongCanTao << "!\n";
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
    cout << "                         [MAN HINH CHIEU PHIM]\n";
    cout << "---------------------------------------------------------------------------------\n";

    // 1. L?c ra danh sách các gh? thu?c riêng phòng này
    vector<Seat> gheCuaPhong;
    for (size_t i = 0; i < danhSachGhe.size(); ++i) {
        if (danhSachGhe[i].getMaPhong() == maPhong) {
            gheCuaPhong.push_back(danhSachGhe[i]);
        }
    }

    if (gheCuaPhong.empty()) {
        cout << "Phong nay chua duoc thiet lap so do ghe!\n";
        cout << "---------------------------------------------------------------------------------\n";
        cout << "=================================================================================\n";
        return;
    }

    // 2. Duy?t qua các hàng t? 'A' d?n 'Z' d? phân lo?i và in t?ng hàng xu?ng dòng riêng
    for (char hang = 'A'; hang <= 'Z'; ++hang) {
        bool coHangNay = false;
        
        // Ki?m tra xem hàng ch? cái này có t?n t?i trong phòng không
        for (size_t i = 0; i < gheCuaPhong.size(); ++i) {
            string maGhe = gheCuaPhong[i].getMaGhe();
            if (!maGhe.empty() && toupper(maGhe[0]) == hang) {
                coHangNay = true;
                break;
            }
        }

        // N?u hàng có t?n t?i, in nhãn tên hàng và các gh? thu?c hàng dó trên m?t dòng
        if (coHangNay) {
            cout << " Hang " << hang << " : ";
            for (size_t i = 0; i < gheCuaPhong.size(); ++i) {
                string maGhe = gheCuaPhong[i].getMaGhe();
                if (!maGhe.empty() && toupper(maGhe[0]) == hang) {
                    string trangThai = gheCuaPhong[i].getTrangThai();
                    
                    // Hi?n th? tr?c quan theo tr?ng thái gh?
                    if (trangThai == "Trong" || trangThai == "Available" || trangThai == "0") {
                        cout << "[" << maGhe << "] ";
                    } else {
                        cout << "[ X ] "; // Gh? dã d?t ho?c tr?ng thái khác
                    }
                }
            }
            cout << "\n"; // Xu?ng dòng sau khi in xong h?t các gh? c?a hàng hi?n t?i
        }
    }

    cout << "---------------------------------------------------------------------------------\n";
    cout << "Chu thich: [MaGhe] Ghe trong  |  [ X ] Ghe da dat\n";
    cout << "=================================================================================\n";
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
