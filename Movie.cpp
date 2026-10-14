#include "Movie.h"
#include "Database.h"
#include "Showtime.h"
#include <iostream>
#include <sstream>
#include <iomanip>
#include <limits>

// Constructor m?c d?nh
Movie::Movie() {
    thoiLuong = 0;
    nhanDoTuoi = 0;
}

// Constructor d?y d? tham s?
Movie::Movie(string ma, string ten, int thoiLuong, int nhanTuoi, string ngayKC, string ngayKT, string moTa, string poster) {
    this->maPhim = ma;
    this->tenPhim = ten;
    this->thoiLuong = thoiLuong;
    this->nhanDoTuoi = nhanTuoi;
    this->ngayKhoiChieu = ngayKC;
    this->ngayKetThuc = ngayKT;
    this->moTa = moTa;
    this->poster = poster;
}

// Getters
string Movie::getMaPhim() const { return maPhim; }
string Movie::getTenPhim() const { return tenPhim; }
int Movie::getThoiLuong() const { return thoiLuong; }
int Movie::getNhanDoTuoi() const { return nhanDoTuoi; }
string Movie::getNgayKhoiChieu() const { return ngayKhoiChieu; }
string Movie::getNgayKetThuc() const { return ngayKetThuc; }
string Movie::getMoTa() const { return moTa; }
string Movie::getPoster() const { return poster; }

// Setters
void Movie::setTenPhim(const string& ten) { tenPhim = ten; }
void Movie::setThoiLuong(int tl) { thoiLuong = tl; }
void Movie::setNhanDoTuoi(int tuoi) { nhanDoTuoi = tuoi; }
void Movie::setNgayKhoiChieu(const string& ngay) { ngayKhoiChieu = ngay; }
void Movie::setNgayKetThuc(const string& ngay) { ngayKetThuc = ngay; }
void Movie::setMoTa(const string& desc) { moTa = desc; }
void Movie::setPoster(const string& p) { poster = p; }

// Chuy?n d?i d?i tu?ng Movie thành dòng CSV
string Movie::toCSV() const {
    stringstream ssTL, ssTuoi;
    ssTL << thoiLuong;
    ssTuoi << nhanDoTuoi;
    return maPhim + "," + tenPhim + "," + ssTL.str() + "," + ssTuoi.str() + "," + ngayKhoiChieu + "," + ngayKetThuc + "," + moTa + "," + poster;
}

// Ð?c m?t dòng t? file Database
Movie Movie::fromCSV(const string& line) {
    stringstream ss(line);
    string ma, ten, strThoiLuong, strNhanTuoi, ngayKC, ngayKT, moTa, poster;

    getline(ss, ma, ',');
    getline(ss, ten, ',');
    getline(ss, strThoiLuong, ',');
    getline(ss, strNhanTuoi, ',');
    getline(ss, ngayKC, ',');
    getline(ss, ngayKT, ',');
    getline(ss, moTa, ',');
    getline(ss, poster, ',');

    int thoiLuong = 0;
    if (!strThoiLuong.empty()) {
        stringstream ssTL(strThoiLuong);
        ssTL >> thoiLuong;
    }

    int nhanDoTuoi = 0;
    if (!strNhanTuoi.empty()) {
        stringstream ssTuoi(strNhanTuoi);
        ssTuoi >> nhanDoTuoi;
    }

    return Movie(ma, ten, thoiLuong, nhanDoTuoi, ngayKC, ngayKT, moTa, poster);
}

// Hi?n th? m?t dòng thông tin phim
void Movie::hienThi() const {
    stringstream ssTL, ssTuoi;
    ssTL << thoiLuong << "p";
    ssTuoi << nhanDoTuoi << "+";

    cout << left << setw(10) << maPhim 
         << setw(30) << tenPhim 
         << setw(15) << ssTL.str()
         << setw(10) << ssTuoi.str()
         << setw(15) << ngayKhoiChieu 
         << setw(15) << ngayKetThuc << endl;
}

// ==========================================
// CÁC CH?C NANG QU?N LÝ PHIM (ÐÃ TÍCH H?P)
// ==========================================

void Movie::hienThiDanhSach(Database& db) {
    vector<Movie> danhSach;
    db.docDanhSachPhim(danhSach);
    if (danhSach.empty()) {
        cout << "Danh sach phim hien dang trong!\n";
        return;
    }
    cout << "\n==================================== DANH SACH PHIM CHIEU RAP ====================================\n";
    cout << left << setw(10) << "Ma Phim"  
         << setw(30) << "Ten Phim"   // Tang t? 25 lên 30 d? có kho?ng cách thoáng hon
         << setw(15) << "Thoi luong" // Tang t? 10 lên 15 d? tách bi?t h?n v?i Ð? tu?i
         << setw(10) << "Do tuoi"  
         << setw(15) << "Khoi chieu"  
         << setw(15) << "Ket thuc" << endl;
    cout << "---------------------------------------------------------------------------------------------------\n";
    for (size_t i = 0; i < danhSach.size(); ++i) {
        danhSach[i].hienThi();
    }
    cout << "===================================================================================================\n";
}

void Movie::themPhim(Database& db) {
    string ma, ten, ngayKC, ngayKT, moTa, poster;
    int thoiLuong, nhanDoTuoi;

    cout << "\n--- THEM PHIM MOI ---\n";
    cout << "Nhap ma phim (VD: M01): ";
    cin >> ma;

    vector<Movie> danhSach;
	db.docDanhSachPhim(danhSach);
    for (size_t i = 0; i < danhSach.size(); ++i) {
        if (danhSach[i].getMaPhim() == ma) {
            cout << "Loi: Ma phim nay da ton tai!\n";
            return;
        }
    }

    cin.ignore(numeric_limits<streamsize>::max(), '\n');
    cout << "Nhap ten phim: ";
    getline(cin, ten);
    cout << "Nhap thoi luong (phut): ";
    cin >> thoiLuong;
    cout << "Nhap nhan do tuoi (13, 16, 18...): ";
    cin >> nhanDoTuoi;
    
    cout << "Nhap ngay khoi chieu (YYYY-MM-DD): ";
    cin >> ngayKC;
    
    // Ràng bu?c logic: Ki?m tra d?nh d?ng/th?i gian phát hành so b?
    if (ngayKC.empty()) {
        cout << "Loi: Ngay phat hanh khong hop le!\n";
        return;
    }

    cout << "Nhap ngay ket thuc (YYYY-MM-DD): ";
    cin >> ngayKT;

    cin.ignore(numeric_limits<streamsize>::max(), '\n');
    cout << "Nhap mo ta phim: ";
    getline(cin, moTa);
    cout << "Nhap duong dan poster/trailer: ";
    getline(cin, poster);

    Movie moi(ma, ten, thoiLuong, nhanDoTuoi, ngayKC, ngayKT, moTa, poster);
    danhSach.push_back(moi);
    db.ghiDanhSachPhim(danhSach);

    cout << "Them phim moi thanh cong va da luu vao he thong!\n";
}

void Movie::suaPhim(Database& db) {
    string ma;
    cout << "\n--- SUA THONG TIN PHIM ---\n";
    cout << "Nhap ma phim can sua: ";
    cin >> ma;

    vector<Movie> danhSach;
	db.docDanhSachPhim(danhSach);
    bool found = false;

    for (size_t i = 0; i < danhSach.size(); ++i) {
        if (danhSach[i].getMaPhim() == ma) {
            found = true;
            string ten, ngayKC, ngayKT, moTa, poster;
            int thoiLuong, nhanDoTuoi;

            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            cout << "Nhap ten phim moi (" << danhSach[i].getTenPhim() << "): ";
            getline(cin, ten);
            cout << "Nhap thoi luong moi (phut): ";
            cin >> thoiLuong;
            cout << "Nhap nhan do tuoi moi: ";
            cin >> nhanDoTuoi;
            cout << "Nhap ngay khoi chieu moi (YYYY-MM-DD): ";
            cin >> ngayKC;
            cout << "Nhap ngay ket thuc moi (YYYY-MM-DD): ";
            cin >> ngayKT;

            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            cout << "Nhap mo ta moi: ";
            getline(cin, moTa);
            cout << "Nhap poster moi: ";
            getline(cin, poster);

            danhSach[i].setTenPhim(ten);
            danhSach[i].setThoiLuong(thoiLuong);
            danhSach[i].setNhanDoTuoi(nhanDoTuoi);
            danhSach[i].setNgayKhoiChieu(ngayKC);
            danhSach[i].setNgayKetThuc(ngayKT);
            danhSach[i].setMoTa(moTa);
            danhSach[i].setPoster(poster);

            db.ghiDanhSachPhim(danhSach);
            cout << "Cap nhat thong tin phim thanh cong!\n";
            break;
        }
    }

    if (!found) {
        cout << "Khong tim thay phim co ma: " << ma << endl;
    }
}

bool Movie::kiemTraPhimDangChieu(const string& maPhim, Database& db) {
    // Ki?m tra thông qua danh sách su?t chi?u trong h? th?ng
    vector<Showtime> dsShowtime;
	db.docDanhSachSuatChieu(dsShowtime);
    for (size_t i = 0; i < dsShowtime.size(); ++i) {
        if (dsShowtime[i].getMaPhim() == maPhim) {
            return true; // Phim dang có su?t chi?u, không du?c xóa
        }
    }
    return false;
}

void Movie::xoaPhim(Database& db) {
    string ma;
    cout << "\n--- XOA PHIM ---\n";
    cout << "Nhap ma phim can xoa: ";
    cin >> ma;

    // Rang buoc logic: Tuyet doi khong the xoa phim neu dang co suat chieu
    if (kiemTraPhimDangChieu(ma, db)) {
        cout << "Loi: Khong the xoa phim nay vi dang co suat chieu hoac dang trong dot chieu hoat dong!\n";
        return;
    }

    // DOC DU LIEU TU DATABASE LEN
    vector<Movie> danhSach;
	db.docDanhSachPhim(danhSach);
    
    for (size_t i = 0; i < danhSach.size(); ++i) {
        if (danhSach[i].getMaPhim() == ma) {
            danhSach.erase(danhSach.begin() + i);
            
            // GHI LAI DANH SACH MOI XUONG DATABASE
            db.ghiDanhSachPhim(danhSach);
            
            cout << "Xoa phim thanh cong!\n";
            return;
        }
    }
    cout << "Khong tim thay phim co ma: " << ma << endl;
}
