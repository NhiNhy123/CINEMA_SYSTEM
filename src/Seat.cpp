#include "../include/Seat.h"
#include "../include/Database.h"
#include "../include/Room.h"
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

// --- XU LY CSV CHO SEATS (Ch? g?m 4 c?t v?t l�) ---

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

// 1. Ph�n h? Admin: T?o gh? h�ng lo?t theo h�ng v� c?t
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

    // BU?C QUAN TR?NG: L?c b? to�n b? gh? cu c?a ph�ng n�y ra kh?i danh s�ch tru?c khi t?o l?i
    vector<Seat> danhSachConLai;
    for (size_t i = 0; i < danhSachGhe.size(); ++i) {
        if (danhSachGhe[i].getMaPhong() != maPhongCanTao) {
            danhSachConLai.push_back(danhSachGhe[i]);
        }
    }
    danhSachGhe = danhSachConLai;

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
    cout << "-> Da khoi tao lai thanh cong " << sucChuaPhong << " ghe cho phong " << maPhongCanTao << "!\n";
}

// 2. T�nh nang th�ng minh: Sao ch�p b? c?c gh? t? ph�ng cu sang ph�ng m?i
void SeatManager::saoChepBucLuc(const string& phongDich, const string& phongNguon, Database& db) {
    taiDuLieu(db); // �?m b?o d� load d? li?u m?i nh?t

    // Ki?m tra xem ph�ng ngu?n c� t?n t?i gh? kh�ng
    vector<Seat> ghePhongNguon;
    for (size_t i = 0; i < danhSachGhe.size(); ++i) {
        if (danhSachGhe[i].getMaPhong() == phongNguon) {
            ghePhongNguon.push_back(danhSachGhe[i]);
        }
    }

    if (ghePhongNguon.empty()) {
        cout << "Khong tim thay du lieu ghe cua phong nguon " << phongNguon << " de sao chep!\n";
        return;
    }

    // L?c b? to�n b? gh? cu c?a ph�ng d�ch d? tr�nh b? c?ng d?n
    vector<Seat> danhSachConLai;
    for (size_t i = 0; i < danhSachGhe.size(); ++i) {
        if (danhSachGhe[i].getMaPhong() != phongDich) {
            danhSachConLai.push_back(danhSachGhe[i]);
        }
    }
    danhSachGhe = danhSachConLai;

    // Ti?n h�nh sao ch�p gh? t? ph�ng ngu?n sang ph�ng d�ch
    int count = 0;
    for (size_t i = 0; i < ghePhongNguon.size(); ++i) {
        string maGheMoi = ghePhongNguon[i].getMaGhe();
        Seat copiedSeat(maGheMoi, phongDich, ghePhongNguon[i].getLoaiGhe(), ghePhongNguon[i].getDonGiaGhe());
        danhSachGhe.push_back(copiedSeat);
        count++;
    }

    db.ghiDanhSachGhe(danhSachGhe);
    cout << "Da sao chep thanh cong " << count << " ghe tu phong " << phongNguon << " sang phong " << phongDich << "!\n";
}

// ==========================================
// D�NH CHO ADMIN: XEM SO �? B? C?C V?T L� (Kh�ng c?n su?t chi?u)
// ==========================================
void SeatManager::hienThiSoDoPhong(const string& maPhong) const {
    cout << "\n==================== SO DO GHE VAT LY - PHONG: " << maPhong << " ====================\n";
    cout << "                                     [MAN HINH CHIEU PHIM]\n";
    cout << "---------------------------------------------------------------------------------\n";

    // 1. L?c ra danh s�ch c�c gh? v?t l� thu?c ri�ng ph�ng n�y
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

    // 2. Duy?t qua c�c h�ng t? 'A' d?n 'Z' d? in so d? b? c?c
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
                    // In tr?c ti?p m� gh? v?t l�
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

// Hi?n th? so d? ph�ng tr?c quan (Load tr?ng th�i d?ng t? su?t chi?u)
void SeatManager::hienThiSoDoPhong(const string& maPhong, const string& maSuatChieu, Database& db) const {
    cout << "\n==================== SO DO GHE PHONG CHIEU: " << maPhong << " (Suat: " << maSuatChieu << ") ====================\n";
    cout << "                                     [MAN HINH CHIEU PHIM]\n";
    cout << "---------------------------------------------------------------------------------\n";

    // 1. L?c ra danh s�ch c�c gh? v?t l� thu?c ri�ng ph�ng n�y
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

    // 2. �?c tr?ng th�i gh? theo su?t chi?u t? b?ng [SEAT_STATUS] c?a lich_va_ve.csv
    vector<pair<string, string> > trangThaiGheList;
    db.docTrangThaiGheSuatChieu(maSuatChieu, trangThaiGheList);

    // 3. Duy?t qua c�c h�ng t? 'A' d?n 'Z' d? in so d? tr?c quan
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
                    
                    // Ki?m tra tr?ng th�i d?ng c?a gh? trong su?t chi?u c? th? n�y
                    string trangThaiGhe = "Trong"; // M?c d?nh l� tr?ng n?u chua c� record
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
                        cout << "[ X ] "; // �� d?t ho?c d� kh�a ? su?t chi?u n�y
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
