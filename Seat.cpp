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
}

Seat::Seat(string maGhe, string maPhong, string loaiGhe, double donGiaGhe) {
    this->maGhe = maGhe;
    this->maPhong = maPhong;
    this->loaiGhe = loaiGhe;
    this->donGiaGhe = donGiaGhe;
}

// Getters
string Seat::getMaGhe() const { return maGhe; }
string Seat::getMaPhong() const { return maPhong; }
string Seat::getLoaiGhe() const { return loaiGhe; }
double Seat::getDonGiaGhe() const { return donGiaGhe; }

// Setters
void Seat::setLoaiGhe(const string& loai) { loaiGhe = loai; }
void Seat::setDonGiaGhe(double gia) { if (gia > 0) donGiaGhe = gia; }

// --- XU LY CSV CHO SEATS (Ch? g?m 4 c?t v?t lý) ---

string Seat::toCSV() const {
    stringstream ss;
    ss << fixed << setprecision(0) << donGiaGhe;
    return maGhe + "," + maPhong + "," + loaiGhe + "," + ss.str();
}

Seat Seat::fromCSV(const string& line) {
    stringstream ss(line);
    string maGhe, maPhong, loaiGhe, strGia;

    getline(ss, maGhe, ',');
    getline(ss, maPhong, ',');
    getline(ss, loaiGhe, ',');
    getline(ss, strGia, ',');

    double donGia = 0.0;
    if (!strGia.empty()) {
        stringstream ssGia(strGia);
        ssGia >> donGia;
    }

    return Seat(maGhe, maPhong, loaiGhe, donGia);
}

void Seat::hienThi() const {
    cout << maGhe << " (" << loaiGhe << " - " << (int)donGiaGhe << "VND) ";
}


// --- CLASS SEAT MANAGER ---

SeatManager::SeatManager() {}

void SeatManager::taiDuLieu(Database& db) {
    db.docDanhSachGhe(danhSachGhe);
}

// 1. Phân h? Admin: T?o gh? hàng lo?t theo hàng và c?t
void SeatManager::taoGheTuDongChoPhong(const string& maPhongCanTao, Database& db) {
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

    taiDuLieu(db);

    char hang = 'A';
    int soThuTu = 1;
    int soGheMoiHang = 10;

    for (int i = 1; i <= sucChuaPhong; ++i) {
        stringstream ss;
        ss << hang << soThuTu; 
        string maGhe = ss.str();

        string loaiGhe = (hang <= 'B') ? "Standard" : "VIP";
        double donGia = (loaiGhe == "VIP") ? 90000 : 60000;

        Seat newSeat(maGhe, maPhongCanTao, loaiGhe, donGia);
        danhSachGhe.push_back(newSeat);

        soThuTu++;
        if (soThuTu > soGheMoiHang) {
            soThuTu = 1;
            hang++; 
        }
    }

    db.ghiDanhSachGhe(danhSachGhe);
    cout << "-> Da tu dong sinh thanh cong " << sucChuaPhong << " ghe cho phong " << maPhongCanTao << "!\n";
}

// 2. Tính nang thông minh: Sao chép b? c?c gh? t? phòng cu sang phòng m?i
void SeatManager::saoChepBucLuc(const string& phongDich, const string& phongNguon, Database& db) {
    int count = 0;
    
    size_t n = danhSachGhe.size();
    for (size_t i = 0; i < n; ++i) {
        if (danhSachGhe[i].getMaPhong() == phongNguon) {
            string maGheMoi = danhSachGhe[i].getMaGhe();
            Seat copiedSeat(maGheMoi, phongDich, danhSachGhe[i].getLoaiGhe(), danhSachGhe[i].getDonGiaGhe());
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

// ==========================================
// DÀNH CHO ADMIN: XEM SO Ð? B? C?C V?T LÝ (Không c?n su?t chi?u)
// ==========================================
void SeatManager::hienThiSoDoPhong(const string& maPhong) const {
    cout << "\n==================== SO DO GHE VAT LY - PHONG: " << maPhong << " ====================\n";
    cout << "                                     [MAN HINH CHIEU PHIM]\n";
    cout << "---------------------------------------------------------------------------------\n";

    // 1. L?c ra danh sách các gh? v?t lý thu?c riêng phòng này
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

    // 2. Duy?t qua các hàng t? 'A' d?n 'Z' d? in so d? b? c?c
    for (char hang = 'A'; hang <= 'Z'; ++hang) {
        bool coHangNay = false;
        
        for (size_t i = 0; i < gheCuaPhong.size(); ++i) {
            string maGhe = gheCuaPhong[i].getMaGhe();
            if (!maGhe.empty() && toupper(maGhe[0]) == hang) {
                coHangNay = true;
                break;
            }
        }

        if (coHangNay) {
            cout << " Hang " << hang << " : ";
            for (size_t i = 0; i < gheCuaPhong.size(); ++i) {
                string maGhe = gheCuaPhong[i].getMaGhe();
                if (!maGhe.empty() && toupper(maGhe[0]) == hang) {
                    // In tr?c ti?p mã gh? v?t lý
                    cout << "[" << maGhe << "] ";
                }
            }
            cout << "\n";
        }
    }

    cout << "---------------------------------------------------------------------------------\n";
    cout << "Chu thich: [MaGhe] Ghe vat ly co san trong phong chieu\n";
    cout << "=================================================================================\n";
}

// Hi?n th? so d? phòng tr?c quan (Load tr?ng thái d?ng t? su?t chi?u)
void SeatManager::hienThiSoDoPhong(const string& maPhong, const string& maSuatChieu, Database& db) const {
    cout << "\n==================== SO DO GHE PHONG CHIEU: " << maPhong << " (Suat: " << maSuatChieu << ") ====================\n";
    cout << "                                     [MAN HINH CHIEU PHIM]\n";
    cout << "---------------------------------------------------------------------------------\n";

    // 1. L?c ra danh sách các gh? v?t lý thu?c riêng phòng này
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

    // 2. Ð?c tr?ng thái gh? theo su?t chi?u t? b?ng [SEAT_STATUS] c?a lich_va_ve.csv
    vector<pair<string, string> > trangThaiGheList;
    db.docTrangThaiGheSuatChieu(maSuatChieu, trangThaiGheList);

    // 3. Duy?t qua các hàng t? 'A' d?n 'Z' d? in so d? tr?c quan
    for (char hang = 'A'; hang <= 'Z'; ++hang) {
        bool coHangNay = false;
        
        for (size_t i = 0; i < gheCuaPhong.size(); ++i) {
            string maGhe = gheCuaPhong[i].getMaGhe();
            if (!maGhe.empty() && toupper(maGhe[0]) == hang) {
                coHangNay = true;
                break;
            }
        }

        if (coHangNay) {
            cout << " Hang " << hang << " : ";
            for (size_t i = 0; i < gheCuaPhong.size(); ++i) {
                string maGhe = gheCuaPhong[i].getMaGhe();
                if (!maGhe.empty() && toupper(maGhe[0]) == hang) {
                    
                    // Ki?m tra tr?ng thái d?ng c?a gh? trong su?t chi?u c? th? này
                    string trangThaiGhe = "Trong"; // M?c d?nh là tr?ng n?u chua có record
                    for (size_t k = 0; k < trangThaiGheList.size(); ++k) {
                        if (trangThaiGheList[k].first == maGhe) {
                            trangThaiGhe = trangThaiGheList[k].second;
                            break;
                        }
                    }
                    
                    // Hi?n th? tr?c quan
                    if (trangThaiGhe == "Trong") {
                        cout << "[" << maGhe << "] ";
                    } else {
                        cout << "[ X ] "; // Ðã d?t ho?c dã khóa ? su?t chi?u này
                    }
                }
            }
            cout << "\n";
        }
    }

    cout << "---------------------------------------------------------------------------------\n";
    cout << "Chu thich: [MaGhe] Ghe trong  |  [ X ] Ghe da dat\n";
    cout << "=================================================================================\n";
}
