#include "../include/Showtime.h"
#include "../include/Movie.h"
#include "../include/Room.h"
#include "../include/Database.h"
#include "../include/DateFormat.h"
#include "../include/Seat.h"
#include <iostream>
#include <iomanip>
#include <sstream>
#include <vector>
#include <cstdlib>
#include <ctime>
#include <cmath>

static int chuyenGioThanhPhut(string gio) {
    if (gio.length() < 5) return 0;
    int h = atoi(gio.substr(0, 2).c_str());
    int m = atoi(gio.substr(3, 2).c_str());
    return h * 60 + m;
}

static bool gioHopLe(const string& gio) {
    if (gio.length() != 5 || gio[2] != ':') return false;
    if (gio[0] < '0' || gio[0] > '9' || gio[1] < '0' || gio[1] > '9' ||
        gio[3] < '0' || gio[3] > '9' || gio[4] < '0' || gio[4] > '9') {
        return false;
    }

    int gioTrongNgay = atoi(gio.substr(0, 2).c_str());
    int phut = atoi(gio.substr(3, 2).c_str());
    return gioTrongNgay < 24 && phut < 60;
}

static string chuyenPhutThanhGio(int tongPhut) {
    int h = (tongPhut / 60) % 24;
    int m = tongPhut % 60;
    stringstream ss;
    ss << setfill('0') << setw(2) << h << ":" << setfill('0') << setw(2) << m;
    return ss.str();
}

static int chuyenNgayThanhSo(string ngay) {
    return DateFormat::dateKey(ngay);
}

static bool layGiaThapNhatCuaPhong(Database& db, const string& maPhong, double& giaThapNhat) {
    vector<Seat> danhSachGhe;
    db.docDanhSachGhe(danhSachGhe);
    bool timThayGiaHopLe = false;

    for (size_t i = 0; i < danhSachGhe.size(); ++i) {
        double giaGhe = danhSachGhe[i].getDonGiaGhe();
        if (danhSachGhe[i].getMaPhong() == maPhong &&
            giaGhe > 0 && std::isfinite(giaGhe) &&
            (!timThayGiaHopLe || giaGhe < giaThapNhat)) {
            giaThapNhat = giaGhe;
            timThayGiaHopLe = true;
        }
    }

    return timThayGiaHopLe;
}

static string dinhDangGiaThamKhao(const Showtime& suatChieu, Database& db) {
    double giaThapNhat = 0.0;
    bool coGiaGhe = layGiaThapNhatCuaPhong(db, suatChieu.getMaPhong(), giaThapNhat);
    if (!coGiaGhe) {
        giaThapNhat = suatChieu.getGiaVeCoBan();
    }
    if (giaThapNhat <= 0 || !std::isfinite(giaThapNhat)) return "Khong co";

    stringstream ssGia;
    ssGia << (coGiaGhe ? "Tu " : "Gia cu ")
          << fixed << setprecision(0) << giaThapNhat << " VND";
    return ssGia.str();
}

Showtime::Showtime() { giaVeCoBan = 0.0; }
Showtime::Showtime(string maSC, string maPhim, string maPhong, string ngay, string gioBD, string gioKT, double giaVe) {
    this->maSuatChieu = maSC;
    this->maPhim = maPhim;
    this->maPhong = maPhong;
    this->ngayChieu = DateFormat::normalizeDate(ngay);
    this->gioBatDau = gioBD;
    this->gioKetThuc = gioKT;
    this->giaVeCoBan = giaVe;
}

string Showtime::getMaSuatChieu() const { return maSuatChieu; }
string Showtime::getMaPhim() const { return maPhim; }
string Showtime::getMaPhong() const { return maPhong; }
string Showtime::getNgayChieu() const { return ngayChieu; }
string Showtime::getGioBatDau() const { return gioBatDau; }
string Showtime::getGioKetThuc() const { return gioKetThuc; }
double Showtime::getGiaVeCoBan() const { return giaVeCoBan; }

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

void Showtime::hienThi(Database& db) const {
    cout << left << setw(12) << maSuatChieu
         << setw(10) << maPhong
         << setw(12) << ngayChieu
         << setw(15) << (gioBatDau + " - " + gioKetThuc)
         << setw(22) << dinhDangGiaThamKhao(*this, db) << endl;
}

void Showtime::themSuatChieu(Database& db) {
    string maSC, maPhim, maPhong, ngay, gioBD;
    double giaThamKhao;

    cout << "\n--- THEM SUAT CHIEU MOI ---\n";

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

    vector<Movie> dsPhim;
    db.docDanhSachPhim(dsPhim);
    int thoiLuongPhim = 0;
    string ngayKC = "", ngayKT = "";
    bool tonTaiPhim = false;

    while (true) {
        cout << "Nhap ma phim: ";
        cin >> maPhim;

        for (size_t i = 0; i < dsPhim.size(); ++i) {
            if (dsPhim[i].getMaPhim() == maPhim) {
                thoiLuongPhim = dsPhim[i].getThoiLuong();
                ngayKC = dsPhim[i].getNgayKhoiChieu();
                ngayKT = dsPhim[i].getNgayKetThuc();
                tonTaiPhim = true;
                break;
            }
        }

        if (!tonTaiPhim) {
            cout << "Loi: Ma phim '" << maPhim << "' khong ton tai trong he thong! Vui long nhap lai.\n";
        } else {
            cout << "-> Tim thay phim! Thoi luong: " << thoiLuongPhim << " phut.\n";
            cout << "-> Dot chieu cua phim tu: " << ngayKC << " den " << ngayKT << "\n";
            break;
        }
    }

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

    if (!layGiaThapNhatCuaPhong(db, maPhong, giaThamKhao)) {
        cout << "Loi: Phong " << maPhong
             << " chua co ghe voi gia hop le. Vui long tao so do ghe truoc khi them suat chieu.\n";
        return;
    }

    int soNgayKC = chuyenNgayThanhSo(ngayKC);
    int soNgayKT = chuyenNgayThanhSo(ngayKT);

    time_t nowTime = time(0);
    tm* ltmNow = localtime(&nowTime);
    if (ltmNow == NULL) {
        cout << "Loi: Khong the xac dinh ngay gio hien tai!\n";
        return;
    }
    int soNgayHomNay = (1900 + ltmNow->tm_year) * 10000
                      + (1 + ltmNow->tm_mon) * 100
                      + ltmNow->tm_mday;

    while (true) {
        cout << "Nhap ngay chieu (DD/MM/YYYY, VD: 06/05/2026): ";
        cin >> ngay;

        if (!DateFormat::isValidDate(ngay)) {
            cout << "Loi: Ngay khong hop le, vui long nhap lai!\n";
            continue;
        }
        ngay = DateFormat::normalizeDate(ngay);
        int soNgayChieu = chuyenNgayThanhSo(ngay);

        if (soNgayChieu < soNgayHomNay) {
            cout << "Loi: Ngay chieu (" << ngay << ") da trong qua khu! Vui long nhap ngay tu hom nay tro di.\n";
        } else if (soNgayKC == 0 || soNgayKT == 0 || soNgayChieu < soNgayKC || soNgayChieu > soNgayKT) {
            cout << "Loi: Ngay chieu phai nam trong khoang thoi gian phat hanh cua phim (" << ngayKC << " den " << ngayKT << ")! Vui long nhap lai.\n";
        } else {
            cout << "-> Ngay chieu hop le!\n";
            break;
        }
    }

    string gioKT = "";
    while (true) {
        cout << "Nhap gio bat dau (HH:MM): ";
        cin >> gioBD;

        if (!gioHopLe(gioBD)) {
            cout << "Loi: Dinh dang gio khong hop le (Dung dinh dang HH:MM)! Vui long nhap lai.\n";
            continue;
        }

        int startMoi = chuyenGioThanhPhut(gioBD);
        nowTime = time(0);
        ltmNow = localtime(&nowTime);
        if (ltmNow == NULL) {
            cout << "Loi: Khong the xac dinh ngay gio hien tai!\n";
            return;
        }
        soNgayHomNay = (1900 + ltmNow->tm_year) * 10000
                       + (1 + ltmNow->tm_mon) * 100
                       + ltmNow->tm_mday;
        if (chuyenNgayThanhSo(ngay) == soNgayHomNay &&
            startMoi <= ltmNow->tm_hour * 60 + ltmNow->tm_min) {
            cout << "Loi: Gio chieu da qua trong hom nay! Vui long nhap gio muon hon.\n";
            continue;
        }

        int endMoi = startMoi + thoiLuongPhim;
        gioKT = chuyenPhutThanhGio(endMoi);

        cout << "-> He thong tu dong set gio ket thuc la: " << gioKT << "\n";

        bool trungLich = false;
        for (size_t i = 0; i < danhSach.size(); ++i) {
            if (danhSach[i].getMaPhong() == maPhong && danhSach[i].getNgayChieu() == ngay) {
                int startCu = chuyenGioThanhPhut(danhSach[i].getGioBatDau());
                int endCu = chuyenGioThanhPhut(danhSach[i].getGioKetThuc());

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

    Showtime moi(maSC, maPhim, maPhong, ngay, gioBD, gioKT, giaThamKhao);
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
    cout << left << setw(12) << "Ma SC" << setw(10) << "Phong" << setw(12) << "Ngay" << setw(15) << "Khung gio" << setw(22) << "Gia tu (tham khao)" << endl;
    cout << "(Gia tham khao lay tu gia ghe thap nhat; gia thanh toan lay truc tiep tu ghe.)\n";
    cout << "-------------------------------------------------------------\n";

    bool found = false;
    for (size_t i = 0; i < danhSach.size(); ++i) {
        if (danhSach[i].getMaPhim() == maPhimCanXem) {
            danhSach[i].hienThi(db);
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
    cout << left << setw(12) << "Ma SC" << setw(30) << "Ten Phim" << setw(10) << "Phong" << setw(12) << "Ngay" << setw(15) << "Khung gio" << setw(22) << "Gia tu (tham khao)" << endl;
    cout << "(Gia tham khao lay tu gia ghe thap nhat; gia thanh toan lay truc tiep tu ghe.)\n";
    cout << "---------------------------------------------------------------------------------------------------\n";

    for (size_t i = 0; i < danhSach.size(); ++i) {
        string tenPhimHienThi = danhSach[i].getMaPhim();
        for (size_t j = 0; j < dsPhim.size(); ++j) {
            if (dsPhim[j].getMaPhim() == danhSach[i].getMaPhim()) {
                tenPhimHienThi = dsPhim[j].getTenPhim();
                break;
            }
        }

        cout << left << setw(12) << danhSach[i].getMaSuatChieu()
             << setw(30) << tenPhimHienThi
             << setw(10) << danhSach[i].getMaPhong()
             << setw(12) << danhSach[i].getNgayChieu()
             << setw(15) << (danhSach[i].getGioBatDau() + " - " + danhSach[i].getGioKetThuc())
             << setw(22) << dinhDangGiaThamKhao(danhSach[i], db) << endl;
    }
    cout << "===================================================================================================\n";
}
