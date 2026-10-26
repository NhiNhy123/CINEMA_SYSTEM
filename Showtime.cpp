#include "Showtime.h"
#include "Movie.h"
#include "Room.h"
#include "Database.h"
#include <iostream>
#include <iomanip>
#include <sstream>
#include <vector>
#include <cstdlib>

// --- CÁC HÀM TI?N ÍCH H? TR? CHUY?N Ð?I GI?/PHÚT ---
static int chuyenGioThanhPhut(string gio) {
    if (gio.length() < 5) return 0;
    int h = atoi(gio.substr(0, 2).c_str());
    int m = atoi(gio.substr(3, 2).c_str());
    return h * 60 + m;
}

static string chuyenPhutThanhGio(int tongPhut) {
    int h = (tongPhut / 60) % 24;
    int m = tongPhut % 60;
    stringstream ss;
    ss << setfill('0') << setw(2) << h << ":" << setfill('0') << setw(2) << m;
    return ss.str();
}

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

// Hàm hi?n th? m?t su?t chi?u don l?
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
    string maSC, maPhim, maPhong, ngay, gioBD;
    double giaVe;

    cout << "\n--- THEM SUAT CHIEU MOI ---\n";

    // 1. Ki?m tra trùng mã su?t chi?u (B?t nh?p l?i ngay n?u trùng)
    vector<Showtime> danhSach;
    while (true) {
        db.docDanhSachSuatChieu(danhSach);
        cout << "Nhap ma suat chieu (VD: SC10): "; 
        cin >> maSC;

        bool trungMa = false;
        for (size_t i = 0; i < danhSach.size(); ++i) {
            if (danhSach[i].getMaSuatChieu() == maSC) {
                trungMa = true;
                break;
            }
        }

        if (trungMa) {
            cout << "Loi: Ma suat chieu '" << maSC << "' da ton tai trong he thong! Vui long nhap lai.\n";
        } else {
            break;
        }
    }

    // 2. Nh?p mã phim và l?y th?i lu?ng
    vector<Movie> dsPhim;
    db.docDanhSachPhim(dsPhim);
    int thoiLuongPhim = 0;
    bool tonTaiPhim = false;
    
    while (true) {
        cout << "Nhap ma phim: "; 
        cin >> maPhim;

        for (size_t i = 0; i < dsPhim.size(); ++i) {
            if (dsPhim[i].getMaPhim() == maPhim) {
                thoiLuongPhim = dsPhim[i].getThoiLuong(); // L?y th?i lu?ng phim (phút)
                tonTaiPhim = true;
                break;
            }
        }

        if (!tonTaiPhim) {
            cout << "Loi: Ma phim '" << maPhim << "' khong ton tai trong he thong! Vui long nhap lai.\n";
        } else {
            cout << "-> Tim thay phim! Thoi luong: " << thoiLuongPhim << " phut.\n";
            break;
        }
    }

    // 3. Nh?p mã phòng chi?u
    vector<Room> dsPhong;
    db.docDanhSachPhong(dsPhong);
    bool tonTaiPhong = false;

    while (true) {
        cout << "Nhap ma phong chieu: "; 
        cin >> maPhong;

        for (size_t i = 0; i < dsPhong.size(); ++i) {
            if (dsPhong[i].getMaPhong() == maPhong) {
                tonTaiPhong = true;
                break;
            }
        }

        if (!tonTaiPhong) {
            cout << "Loi: Ma phong '" << maPhong << "' khong ton tai trong he thong! Vui long nhap lai.\n";
        } else {
            break;
        }
    }

    // 4. Nh?p ngày chi?u (Ð?nh d?ng DD/MM/YYYY)
    cout << "Nhap ngay chieu (DD/MM/YYYY, VD: 06/05/2026): "; 
    cin >> ngay;

    // 5. Nh?p gi? b?t d?u, t? d?ng tính gi? k?t thúc và ki?m tra xung d?t chính xác b?ng phút
    string gioKT = "";
    while (true) {
        cout << "Nhap gio bat dau (HH:MM): "; 
        cin >> gioBD;

        if (gioBD.length() < 5 || gioBD[2] != ':') {
            cout << "Loi: Dinh dang gio khong hop le (Dung dinh dang HH:MM)! Vui long nhap lai.\n";
            continue;
        }

        int startMoi = chuyenGioThanhPhut(gioBD);
        int endMoi = startMoi + thoiLuongPhim;
        gioKT = chuyenPhutThanhGio(endMoi);

        cout << "-> He thong tu dong set gio ket thuc la: " << gioKT << "\n";

        // Ki?m tra ch?ng chéo th?i gian b?ng kho?ng th?i gian (phút)
        bool trungLich = false;
        for (size_t i = 0; i < danhSach.size(); ++i) {
            if (danhSach[i].getMaPhong() == maPhong && danhSach[i].getNgayChieu() == ngay) {
                int startCu = chuyenGioThanhPhut(danhSach[i].getGioBatDau());
                int endCu = chuyenGioThanhPhut(danhSach[i].getGioKetThuc());

                // Ði?u ki?n ch?ng chéo: Su?t m?i b?t d?u tru?c khi su?t cu k?t thúc VÀ su?t m?i k?t thúc sau khi su?t cu b?t d?u
                if (startMoi < endCu && endMoi > startCu) {
                    trungLich = true;
                    break;
                }
            }
        }

        if (trungLich) {
            cout << "Loi: Da co suat chieu trong thoi gian nay tai phong " << maPhong << "! Vui long nhap lai gio khac.\n";
        } else {
            cout << "-> Khung gio hop le!\n";
            break;
        }
    }

    // 6. Nh?p giá vé co b?n
    do {
        cout << "Nhap gia ve co ban (> 0): "; 
        cin >> giaVe;
        if (giaVe <= 0) cout << "Loi: Gia ve phai lon hon 0!\n";
    } while (giaVe <= 0);

    // 7. T?o d?i tu?ng và luu xu?ng Database
    Showtime moi(maSC, maPhim, maPhong, ngay, gioBD, gioKT, giaVe);
    danhSach.push_back(moi);

    db.ghiDanhSachSuatChieu(danhSach);
    cout << "Them suat chieu thanh cong va tu dong cap nhat thoi gian ket thuc!\n";
}

void Showtime::hienThiSuatChieuTheoPhim(const string& maPhimCanXem, Database& db) {
    vector<Movie> dsPhim;
    db.docDanhSachPhim(dsPhim);
    string tenPhim = maPhimCanXem; 
    for (size_t i = 0; i < dsPhim.size(); ++i) {
        if (dsPhim[i].getMaPhim() == maPhimCanXem) {
            tenPhim = dsPhim[i].getTenPhim();
            break;
        }
    }

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
    vector<Movie> dsPhim;
    db.docDanhSachPhim(dsPhim);

    vector<Showtime> danhSach;
    db.docDanhSachSuatChieu(danhSach);

    cout << "\n================================ TAT CA SUAT CHIEU TRONG HE THONG =================================\n";
    cout << left << setw(12) << "Ma SC" << setw(30) << "Ten Phim" << setw(10) << "Phong" << setw(12) << "Ngay" << setw(15) << "Khung gio" << setw(15) << "Gia ve" << endl;
    cout << "---------------------------------------------------------------------------------------------------\n";
    
    for (size_t i = 0; i < danhSach.size(); ++i) {
        string tenPhimHienThi = danhSach[i].getMaPhim();
        for (size_t j = 0; j < dsPhim.size(); ++j) {
            if (dsPhim[j].getMaPhim() == danhSach[i].getMaPhim()) {
                tenPhimHienThi = dsPhim[j].getTenPhim();
                break;
            }
        }

        stringstream ssGia;
        ssGia << fixed << setprecision(0) << danhSach[i].getGiaVeCoBan() << " VND";

        cout << left << setw(12) << danhSach[i].getMaSuatChieu() 
             << setw(30) << tenPhimHienThi 
             << setw(10) << danhSach[i].getMaPhong() 
             << setw(12) << danhSach[i].getNgayChieu() 
             << setw(15) << (danhSach[i].getGioBatDau() + " - " + danhSach[i].getGioKetThuc())
             << setw(15) << ssGia.str() << endl;
    }
    cout << "===================================================================================================\n";
}
