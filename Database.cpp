#include "Database.h"
#include "Seat.h"
#include "FAndB.h"
#include "Promotion.h"
#include "Invoice.h"
#include "Room.h"
#include "Movie.h"
#include "Showtime.h"
#include "Ticket.h"
#include "ShilftManagement.h"

#include <fstream>
#include <sstream>
#include <iostream>
#include <cstdlib>
#include <utility>

using namespace std;

Database::Database() {}

// Hàm ti?n ích n?i b? d? tách chu?i CSV phân cách b?ng d?u ph?y ','
static vector<string> tachChuoiCSV(const string& line) {
    vector<string> tokens;
    stringstream ss(line);
    string item;
    while (getline(ss, item, ',')) {
        // Xóa ký t? '\r' ? cu?i chu?i n?u ch?y trên Windows (d?nh d?ng CRLF)
        if (!item.empty() && item[item.length() - 1] == '\r') {
            item.erase(item.length() - 1);
        }
        tokens.push_back(item);
    }
    return tokens;
}

// =========================================================================
// 1. QU?N LÝ NHÂN S? & KHÁCH HÀNG (DATA/quan_ly_nhan_su.csv)
// =========================================================================

// --- Ð?c danh sách Khách hàng & Users ---
void Database::docDanhSachUsers(vector<Customer>& danhSachKH) {
    danhSachKH.clear();
    ifstream inFile("DATA/quan_ly_nhan_su.csv");
    if (!inFile.is_open()) return;

    string line;
    bool readingUsers = false;
    bool readingCustomers = false;

    // Luu t?m thông tin t? b?ng [USERS] theo maNguoiDung
    // C?u trúc USERS: maNguoiDung,hoTen,soDienThoai,email,ngaySinh,gioiTinh,cccd,taiKhoan,matKhau,vaiTro,trangThaiLamViec,trangThaiTaiKhoan
    vector<string> maUsers;
    vector<string> hoTens;
    vector<string> soDienThoais;
    vector<string> emails;
    vector<string> vaiTros;

    while (getline(inFile, line)) {
        if (line.empty()) continue;

        if (line.find("[USERS]") != string::npos) {
            readingUsers = true;
            readingCustomers = false;
            continue;
        }
        if (line.find("[CUSTOMERS]") != string::npos) {
            readingUsers = false;
            readingCustomers = true;
            continue;
        }
        if (line[0] == '[') {
            readingUsers = false;
            readingCustomers = false;
            continue;
        }

        if (readingUsers) {
            if (line.find("maNguoiDung") != string::npos) continue;
            vector<string> parts = tachChuoiCSV(line);
            if (parts.size() >= 4) {
                // Ch? l?y các user có vai trò là Customer (ho?c l?c theo nhu c?u)
                maUsers.push_back(parts[0]);
                hoTens.push_back(parts[1]);
                soDienThoais.push_back(parts[2]);
                emails.push_back(parts[3]);
            }
        }

        if (readingCustomers) {
            if (line.find("maKhachHang") != string::npos) continue;
            vector<string> parts = tachChuoiCSV(line);
            // C?u trúc CUSTOMERS: maKhachHang,diemTichLuy,hangThanhVien,tongChiTieu
            if (parts.size() >= 4) {
                string maKH = parts[0];
                int diem = atoi(parts[1].c_str());
                string hang = parts[2];
                long long chiTieu = atoll(parts[3].c_str());

                // Tìm thông tin tuong ?ng trong b?ng USERS d?a vào maKH == maNguoiDung
                string hoTen = "", sdt = "", email = "";
                for (size_t i = 0; i < maUsers.size(); ++i) {
                    if (maUsers[i] == maKH) {
                        hoTen = hoTens[i];
                        sdt = soDienThoais[i];
                        email = emails[i];
                        break;
                    }
                }

                // Kh?i t?o d?i tu?ng Customer hoàn ch?nh
                Customer kh(maKH, hoTen, sdt, email, diem, hang, chiTieu);
                danhSachKH.push_back(kh);
            }
        }
    }
    inFile.close();
}

// --- Ghi d?ng b? danh sách Khách hàng & Users ---
// --- Ghi d?ng b? danh sách Khách hàng & Users (B?o toàn d? li?u cu c?a b?ng USERS) ---
void Database::ghiDanhSachUsers(const vector<Customer>& danhSachKH) {
    // 1. Ð?c toàn b? các dòng hi?n có t? file cu d? không làm m?t d? li?u h? th?ng (Admin, Staff,...)
    ifstream inFile("DATA/quan_ly_nhan_su.csv");
    vector<string> usersLines;
    vector<string> customersLines;
    vector<string> historyLines;
    
    string currentSection = "";
    string line;
    
    if (inFile.is_open()) {
        while (getline(inFile, line)) {
            if (line.empty()) continue;
            if (line[0] == '[') {
                if (line.find("[USERS]") != string::npos) currentSection = "USERS";
                else if (line.find("[CUSTOMERS]") != string::npos) currentSection = "CUSTOMERS";
                else if (line.find("[CUSTOMER_POINTS_HISTORY]") != string::npos) currentSection = "HISTORY";
                else currentSection = "";
                continue;
            }
            
            if (currentSection == "USERS") {
                if (line.find("maNguoiDung") == string::npos) usersLines.push_back(line);
            } else if (currentSection == "CUSTOMERS") {
                if (line.find("maKhachHang") == string::npos) customersLines.push_back(line);
            } else if (currentSection == "HISTORY") {
                if (line.find("maGiaoDichDiem") == string::npos) historyLines.push_back(line);
            }
        }
        inFile.close();
    }

    // 2. C?p nh?t ho?c thêm m?i thông tin khách hàng vào danh sách Users và Customers
    for (size_t i = 0; i < danhSachKH.size(); ++i) {
        string maKH = danhSachKH[i].getMaKH();
        string newUserRow = maKH + "," + danhSachKH[i].getHoTen() + "," + danhSachKH[i].getSoDienThoai() + "," +
                            danhSachKH[i].getEmail() + ",1/1/2000,Nam,123456789012,khach_" + 
                            danhSachKH[i].getSoDienThoai() + ",123456,Customer,DangLam,HoatDong";
                            
        stringstream ssDiem, ssChiTieu;

ssDiem << danhSachKH[i].getDiemTichLuy();
ssChiTieu << danhSachKH[i].getTongChiTieu();

string newCustomerRow = maKH + "," + ssDiem.str() + "," +
                        danhSachKH[i].getHangThanhVien() + "," + ssChiTieu.str();

        // Ki?m tra xem maKH dã t?n t?i trong danh sách USERS cu chua d? c?p nh?t, n?u chua thì thêm m?i
        bool foundUser = false;
        for (size_t j = 0; j < usersLines.size(); ++j) {
            if (usersLines[j].find(maKH + ",") == 0) {
                usersLines[j] = newUserRow; // C?p nh?t thông tin m?i nh?t
                foundUser = true;
                break;
            }
        }
        if (!foundUser) {
            usersLines.push_back(newUserRow); // Thêm m?i n?u là khách hàng dang ký m?i
        }

        // Tuong t? cho b?ng CUSTOMERS
        bool foundCustomer = false;
        for (size_t j = 0; j < customersLines.size(); ++j) {
            if (customersLines[j].find(maKH + ",") == 0) {
                customersLines[j] = newCustomerRow;
                foundCustomer = true;
                break;
            }
        }
        if (!foundCustomer) {
            customersLines.push_back(newCustomerRow);
        }
    }

    // 3. Ti?n hành ghi dè toàn b? t?p v?i d?y d? d? li?u du?c b?o toàn
    ofstream outFile("DATA/quan_ly_nhan_su.csv", ios::trunc);
    if (!outFile.is_open()) return;

    // Ghi b?ng [USERS]
    outFile << "[USERS]\n";
    outFile << "maNguoiDung,hoTen,soDienThoai,email,ngaySinh,gioiTinh,cccd,taiKhoan,matKhau,vaiTro,trangThaiLamViec,trangThaiTaiKhoan\n";
    for (size_t i = 0; i < usersLines.size(); ++i) {
        outFile << usersLines[i] << "\n";
    }

    // Ghi b?ng [CUSTOMERS]
    outFile << "\n[CUSTOMERS]\n";
    outFile << "maKhachHang,diemTichLuy,hangThanhVien,tongChiTieu\n";
    for (size_t i = 0; i < customersLines.size(); ++i) {
        outFile << customersLines[i] << "\n";
    }

    // Ghi b?ng [CUSTOMER_POINTS_HISTORY]
    outFile << "\n[CUSTOMER_POINTS_HISTORY]\n";
    outFile << "maGiaoDichDiem,maKhachHang,loaiGiaoDich,soDiem,noiDung,thoiGian\n";
    for (size_t i = 0; i < historyLines.size(); ++i) {
        outFile << historyLines[i] << "\n";
    }

    outFile.close();
}

// --- Ð?c l?ch s? giao d?ch di?m ---
void Database::docLichSuGiaoDichDiem(vector<GiaoDichDiem>& danhSachGD) {
    danhSachGD.clear();
    ifstream inFile("DATA/quan_ly_nhan_su.csv");
    if (!inFile.is_open()) return;

    string line;
    bool reading = false;
    while (getline(inFile, line)) {
        if (line.empty()) continue;
        if (line.find("[CUSTOMER_POINTS_HISTORY]") != string::npos) {
            reading = true;
            continue;
        }
        if (reading && line[0] == '[') {
            reading = false;
            break;
        }

        if (reading) {
            if (line.find("maGiaoDichDiem") != string::npos) continue;
            vector<string> parts = tachChuoiCSV(line);
            // C?u trúc: maGiaoDichDiem,maKhachHang,loaiGiaoDich,soDiem,noiDung,thoiGian
            if (parts.size() >= 6) {
                GiaoDichDiem gd(parts[0], "", parts[1], parts[2], atoi(parts[3].c_str()), parts[4], parts[5]);
                danhSachGD.push_back(gd);
            }
        }
    }
    inFile.close();
}

// --- Ghi l?ch s? giao d?ch di?m ---
void Database::ghiLichSuGiaoDichDiem(const vector<GiaoDichDiem>& danhSachGD) {
    vector<Customer> danhSachKH;
    docDanhSachUsers(danhSachKH);

    ofstream outFile("DATA/quan_ly_nhan_su.csv", ios::trunc);
    if (!outFile.is_open()) return;

    // Ghi l?i b?ng [USERS]
    outFile << "[USERS]\n";
    outFile << "maNguoiDung,hoTen,soDienThoai,email,ngaySinh,gioiTinh,cccd,taiKhoan,matKhau,vaiTro,trangThaiLamViec,trangThaiTaiKhoan\n";
    for (size_t i = 0; i < danhSachKH.size(); ++i) {
        outFile << danhSachKH[i].getMaKH() << ","
                << danhSachKH[i].getHoTen() << ","
                << danhSachKH[i].getSoDienThoai() << ","
                << danhSachKH[i].getEmail() << ","
                << "1/1/2000,Nam,123456789012,"
                << "khach_" << danhSachKH[i].getSoDienThoai() << ","
                << "123456,Customer,DangLam,HoatDong\n";
    }

    // Ghi l?i b?ng [CUSTOMERS]
    outFile << "\n[CUSTOMERS]\n";
    outFile << "maKhachHang,diemTichLuy,hangThanhVien,tongChiTieu\n";
    for (size_t i = 0; i < danhSachKH.size(); ++i) {
        outFile << danhSachKH[i].getMaKH() << ","
                << danhSachKH[i].getDiemTichLuy() << ","
                << danhSachKH[i].getHangThanhVien() << ","
                << danhSachKH[i].getTongChiTieu() << "\n";
    }

    // Ghi l?i b?ng [CUSTOMER_POINTS_HISTORY] v?i danh sách m?i nh?t
    outFile << "\n[CUSTOMER_POINTS_HISTORY]\n";
    outFile << "maGiaoDichDiem,maKhachHang,loaiGiaoDich,soDiem,noiDung,thoiGian\n";
    for (size_t i = 0; i < danhSachGD.size(); ++i) {
        outFile << danhSachGD[i].getMaGiaoDich() << ","
                << danhSachGD[i].getSoDienThoai() << ","
                << danhSachGD[i].getLoaiGiaoDich() << ","
                << danhSachGD[i].getSoDiem() << ","
                << danhSachGD[i].getNoiDung() << ","
                << danhSachGD[i].getThoiGian() << "\n";
    }

    outFile.close();
}

// --- Qu?n lý Phòng chi?u (Rooms) ---
// =========================================================================
// 2. KHO PHIM & H? T?NG R?P (DATA/quan_ly_phim.csv)
// =========================================================================

// --- Qu?n lý Th? lo?i (Genres) ---
// =========================================================================
// B? SUNG: QU?N LÝ TH? LO?I & LIÊN K?T PHIM - TH? LO?I (DATA/quan_ly_phim.csv)
// =========================================================================

void Database::docDanhSachTheLoai(vector<pair<string, string> >& danhSachTheLoai) {
    danhSachTheLoai.clear();
    ifstream inFile("DATA/quan_ly_phim.csv");
    if (!inFile.is_open()) return;

    string line;
    bool reading = false;
    while (getline(inFile, line)) {
        if (line.empty()) continue;
        if (line.find("[GENRES]") != string::npos) {
            reading = true;
            continue;
        }
        if (reading && line[0] == '[') {
            reading = false;
            break;
        }

        if (reading) {
            if (line.find("maTheLoai") != string::npos) continue;
            vector<string> parts = tachChuoiCSV(line);
            if (parts.size() >= 2) {
                danhSachTheLoai.push_back(make_pair(parts[0], parts[1]));
            }
        }
    }
    inFile.close();
}

void Database::docMovieGenres(vector<pair<string, string> >& movieGenres) {
    movieGenres.clear();
    ifstream inFile("DATA/quan_ly_phim.csv");
    if (!inFile.is_open()) return;

    string line;
    bool reading = false;
    while (getline(inFile, line)) {
        if (line.empty()) continue;
        if (line.find("[MOVIE_GENRES]") != string::npos) {
            reading = true;
            continue;
        }
        if (reading && line[0] == '[') {
            reading = false;
            break;
        }

        if (reading) {
            if (line.find("maPhim") != string::npos) continue;
            vector<string> parts = tachChuoiCSV(line);
            if (parts.size() >= 2) {
                movieGenres.push_back(make_pair(parts[0], parts[1]));
            }
        }
    }
    inFile.close();
}

string Database::layTheLoaiCuaPhim(string maPhimToFind) {
    vector<pair<string, string> > dsTheLoai;
    vector<pair<string, string> > movieGenres;
    docDanhSachTheLoai(dsTheLoai);
    docMovieGenres(movieGenres);

    string ketQua = "";
    for (size_t i = 0; i < movieGenres.size(); ++i) {
        if (movieGenres[i].first == maPhimToFind) {
            for (size_t j = 0; j < dsTheLoai.size(); ++j) {
                if (dsTheLoai[j].first == movieGenres[i].second) {
                    if (!ketQua.empty()) {
                        ketQua += ", ";
                    }
                    ketQua += dsTheLoai[j].second;
                }
            }
        }
    }
    return ketQua.empty() ? "Chua cap nhat" : ketQua;
}

// Thêm th? lo?i m?i cho phim (gi? th? lo?i cu, g?n thêm th? lo?i m?i)
void Database::themTheLoaiChoPhim(string maPhim, string tenTheLoaiMoi) {
    vector<pair<string, string> > dsTheLoai;
    vector<pair<string, string> > movieGenres;
    vector<Room> danhSachPhong;
    vector<Seat> danhSachGhe;
    vector<Movie> danhSachPhim;

    docDanhSachTheLoai(dsTheLoai);
    docMovieGenres(movieGenres);
    docDanhSachPhong(danhSachPhong);
    docDanhSachGhe(danhSachGhe);
    docDanhSachPhim(danhSachPhim);

    // 1. Ki?m tra th? lo?i dã t?n t?i chua, n?u chua thì t?o mã m?i (TL05, TL06,...)
    string maTheLoai = "";
    for (size_t i = 0; i < dsTheLoai.size(); ++i) {
        if (dsTheLoai[i].second == tenTheLoaiMoi) {
            maTheLoai = dsTheLoai[i].first;
            break;
        }
    }

    if (maTheLoai.empty()) {
        stringstream ss;
        ss << "TL0" << dsTheLoai.size() + 1;
        maTheLoai = ss.str();
        dsTheLoai.push_back(make_pair(maTheLoai, tenTheLoaiMoi));
    }

    // 2. Thêm liên k?t vào movieGenres n?u chua có
    bool daCo = false;
    for (size_t i = 0; i < movieGenres.size(); ++i) {
        if (movieGenres[i].first == maPhim && movieGenres[i].second == maTheLoai) {
            daCo = true;
            break;
        }
    }
    if (!daCo) {
        movieGenres.push_back(make_pair(maPhim, maTheLoai));
    }

    // 3. Ghi l?i toàn b? file v?i d?y d? 5 b?ng
    ofstream outFile("DATA/quan_ly_phim.csv", ios::trunc);
    if (!outFile.is_open()) return;

    outFile << "[GENRES]\n";
    outFile << "maTheLoai,tenTheLoai\n";
    for (size_t i = 0; i < dsTheLoai.size(); ++i) {
        outFile << dsTheLoai[i].first << "," << dsTheLoai[i].second << "\n";
    }

    outFile << "\n[MOVIE_GENRES]\n";
    outFile << "maPhim,maTheLoai\n";
    for (size_t i = 0; i < movieGenres.size(); ++i) {
        outFile << movieGenres[i].first << "," << movieGenres[i].second << "\n";
    }

    outFile << "\n[ROOMS]\n";
    outFile << "maPhong,tenPhong,loaiPhong,sucChua,trangThaiPhong\n";
    for (size_t i = 0; i < danhSachPhong.size(); ++i) {
        outFile << danhSachPhong[i].getMaPhong() << "," << danhSachPhong[i].getTenPhong() << ","  
                << danhSachPhong[i].getLoaiPhong() << "," << danhSachPhong[i].getSucChua() << "," << danhSachPhong[i].getTrangThaiPhong() << "\n";
    }

    outFile << "\n[SEATS]\n";
    outFile << "maGhe,maPhong,loaiGhe,donGiaGhe,trangThai\n";
    for (size_t i = 0; i < danhSachGhe.size(); ++i) {
        outFile << danhSachGhe[i].getMaGhe() << "," << danhSachGhe[i].getMaPhong() << ","  
                << danhSachGhe[i].getLoaiGhe() << "," << danhSachGhe[i].getDonGiaGhe() << "," << danhSachGhe[i].getTrangThai() << "\n";
    }

    outFile << "\n[MOVIES]\n";
    outFile << "maPhim,tenPhim,thoiLuong,nhanDoTuoi,ngayKhoiChieu,ngayKetThuc,moTa,poster\n";
    for (size_t i = 0; i < danhSachPhim.size(); ++i) {
        outFile << danhSachPhim[i].getMaPhim() << "," << danhSachPhim[i].getTenPhim() << "," << danhSachPhim[i].getThoiLuong() << ","  
                << danhSachPhim[i].getNhanDoTuoi() << "," << danhSachPhim[i].getNgayKhoiChieu() << "," << danhSachPhim[i].getNgayKetThuc() << ","  
                << danhSachPhim[i].getMoTa() << "," << danhSachPhim[i].getPoster() << "\n";
    }
    outFile.close();
}

// C?p nh?t th? lo?i cho phim (Xóa th? lo?i cu, gán th? lo?i m?i hoàn toàn)
void Database::capNhatTheLoaiChoPhim(string maPhim, string tenTheLoaiMoi) {
    vector<pair<string, string> > dsTheLoai;
    vector<pair<string, string> > movieGenres;
    vector<Room> danhSachPhong;
    vector<Seat> danhSachGhe;
    vector<Movie> danhSachPhim;

    docDanhSachTheLoai(dsTheLoai);
    docMovieGenres(movieGenres);
    docDanhSachPhong(danhSachPhong);
    docDanhSachGhe(danhSachGhe);
    docDanhSachPhim(danhSachPhim);

    // 1. Xóa các liên k?t cu c?a mã phim này trong movieGenres
    for (size_t i = 0; i < movieGenres.size(); ) {
        if (movieGenres[i].first == maPhim) {
            movieGenres.erase(movieGenres.begin() + i);
        } else {
            ++i;
        }
    }

    // 2. Ki?m tra xem th? lo?i m?i dã t?n t?i trong [GENRES] chua, n?u chua thì t?o mã m?i
    string maTheLoai = "";
    for (size_t i = 0; i < dsTheLoai.size(); ++i) {
        if (dsTheLoai[i].second == tenTheLoaiMoi) {
            maTheLoai = dsTheLoai[i].first;
            break;
        }
    }

    if (maTheLoai.empty()) {
        stringstream ss;
        ss << "TL0" << dsTheLoai.size() + 1;
        maTheLoai = ss.str();
        dsTheLoai.push_back(make_pair(maTheLoai, tenTheLoaiMoi));
    }

    // 3. Thêm liên k?t m?i vào movieGenres
    movieGenres.push_back(make_pair(maPhim, maTheLoai));

    // 4. Ghi l?i toàn b? file v?i d?y d? 5 b?ng
    ofstream outFile("DATA/quan_ly_phim.csv", ios::trunc);
    if (!outFile.is_open()) return;

    outFile << "[GENRES]\n";
    outFile << "maTheLoai,tenTheLoai\n";
    for (size_t i = 0; i < dsTheLoai.size(); ++i) {
        outFile << dsTheLoai[i].first << "," << dsTheLoai[i].second << "\n";
    }

    outFile << "\n[MOVIE_GENRES]\n";
    outFile << "maPhim,maTheLoai\n";
    for (size_t i = 0; i < movieGenres.size(); ++i) {
        outFile << movieGenres[i].first << "," << movieGenres[i].second << "\n";
    }

    outFile << "\n[ROOMS]\n";
    outFile << "maPhong,tenPhong,loaiPhong,sucChua,trangThaiPhong\n";
    for (size_t i = 0; i < danhSachPhong.size(); ++i) {
        outFile << danhSachPhong[i].getMaPhong() << "," << danhSachPhong[i].getTenPhong() << ","  
                << danhSachPhong[i].getLoaiPhong() << "," << danhSachPhong[i].getSucChua() << "," << danhSachPhong[i].getTrangThaiPhong() << "\n";
    }

    outFile << "\n[SEATS]\n";
    outFile << "maGhe,maPhong,loaiGhe,donGiaGhe,trangThai\n";
    for (size_t i = 0; i < danhSachGhe.size(); ++i) {
        outFile << danhSachGhe[i].getMaGhe() << "," << danhSachGhe[i].getMaPhong() << ","  
                << danhSachGhe[i].getLoaiGhe() << "," << danhSachGhe[i].getDonGiaGhe() << "," << danhSachGhe[i].getTrangThai() << "\n";
    }

    outFile << "\n[MOVIES]\n";
    outFile << "maPhim,tenPhim,thoiLuong,nhanDoTuoi,ngayKhoiChieu,ngayKetThuc,moTa,poster\n";
    for (size_t i = 0; i < danhSachPhim.size(); ++i) {
        outFile << danhSachPhim[i].getMaPhim() << "," << danhSachPhim[i].getTenPhim() << "," << danhSachPhim[i].getThoiLuong() << ","  
                << danhSachPhim[i].getNhanDoTuoi() << "," << danhSachPhim[i].getNgayKhoiChieu() << "," << danhSachPhim[i].getNgayKetThuc() << ","  
                << danhSachPhim[i].getMoTa() << "," << danhSachPhim[i].getPoster() << "\n";
    }
    outFile.close();
}

// --- Qu?n lý Phòng chi?u (Rooms) ---
void Database::docDanhSachPhong(vector<Room>& danhSachPhong) {
    danhSachPhong.clear();
    ifstream inFile("DATA/quan_ly_phim.csv");
    if (!inFile.is_open()) return;

    string line;
    bool reading = false;
    while (getline(inFile, line)) {
        if (line.empty()) continue;
        if (line.find("[ROOMS]") != string::npos) {
            reading = true;
            continue;
        }
        if (reading && line[0] == '[') {
            reading = false;
            break;
        }

        if (reading) {
            if (line.find("maPhong") != string::npos) continue;
            vector<string> parts = tachChuoiCSV(line);
            if (parts.size() >= 5) {
                Room r(parts[0], parts[1], parts[2], atoi(parts[3].c_str()), parts[4]);
                danhSachPhong.push_back(r);
            }
        }
    }
    inFile.close();
}

void Database::ghiDanhSachPhong(const vector<Room>& danhSachPhong) {
    vector<Seat> danhSachGhe;
    vector<Movie> danhSachPhim;
    vector<pair<string, string> > dsTheLoai;
    vector<pair<string, string> > movieGenres;
    docDanhSachGhe(danhSachGhe);
    docDanhSachPhim(danhSachPhim);
    docDanhSachTheLoai(dsTheLoai);
    docMovieGenres(movieGenres);

    ofstream outFile("DATA/quan_ly_phim.csv", ios::trunc);
    if (!outFile.is_open()) return;

    outFile << "[GENRES]\n";
    outFile << "maTheLoai,tenTheLoai\n";
for (size_t i = 0; i < dsTheLoai.size(); ++i) {
    outFile << dsTheLoai[i].first << ","
            << dsTheLoai[i].second << "\n";
}

    outFile << "\n[MOVIE_GENRES]\n";
    outFile << "maPhim,maTheLoai\n";
for (size_t i = 0; i < movieGenres.size(); ++i) {
    outFile << movieGenres[i].first << ","
            << movieGenres[i].second << "\n";
}

    outFile << "\n[ROOMS]\n";
    outFile << "maPhong,tenPhong,loaiPhong,sucChua,trangThaiPhong\n";
    for (size_t i = 0; i < danhSachPhong.size(); ++i) {
        outFile << danhSachPhong[i].getMaPhong() << "," << danhSachPhong[i].getTenPhong() << ","  
                << danhSachPhong[i].getLoaiPhong() << "," << danhSachPhong[i].getSucChua() << "," << danhSachPhong[i].getTrangThaiPhong() << "\n";
    }

    outFile << "\n[SEATS]\n";
    outFile << "maGhe,maPhong,loaiGhe,donGiaGhe,trangThai\n";
    for (size_t i = 0; i < danhSachGhe.size(); ++i) {
        outFile << danhSachGhe[i].getMaGhe() << "," << danhSachGhe[i].getMaPhong() << ","  
                << danhSachGhe[i].getLoaiGhe() << "," << danhSachGhe[i].getDonGiaGhe() << "," << danhSachGhe[i].getTrangThai() << "\n";
    }

    outFile << "\n[MOVIES]\n";
    outFile << "maPhim,tenPhim,thoiLuong,nhanDoTuoi,ngayKhoiChieu,ngayKetThuc,moTa,poster\n";
    for (size_t i = 0; i < danhSachPhim.size(); ++i) {
        outFile << danhSachPhim[i].getMaPhim() << "," << danhSachPhim[i].getTenPhim() << "," << danhSachPhim[i].getThoiLuong() << ","  
                << danhSachPhim[i].getNhanDoTuoi() << "," << danhSachPhim[i].getNgayKhoiChieu() << "," << danhSachPhim[i].getNgayKetThuc() << ","  
                << danhSachPhim[i].getMoTa() << "," << danhSachPhim[i].getPoster() << "\n";
    }
    outFile.close();
}

// --- Qu?n lý Gh? (Seats) ---
void Database::docDanhSachGhe(vector<Seat>& danhSachGhe) {
    danhSachGhe.clear();
    ifstream inFile("DATA/quan_ly_phim.csv");
    if (!inFile.is_open()) return;

    string line;
    bool reading = false;
    while (getline(inFile, line)) {
        if (line.empty()) continue;
        if (line.find("[SEATS]") != string::npos) {
            reading = true;
            continue;
        }
        if (reading && line[0] == '[') {
            reading = false;
            break;
        }

        if (reading) {
            if (line.find("maGhe") != string::npos) continue;
            vector<string> parts = tachChuoiCSV(line);
            if (parts.size() >= 5) {
                Seat s(parts[0], parts[1], parts[2], atof(parts[3].c_str()), parts[4]);
                danhSachGhe.push_back(s);
            }
        }
    }
    inFile.close();
}

void Database::ghiDanhSachGhe(const vector<Seat>& danhSachGhe) {
    vector<Room> danhSachPhong;
    vector<Movie> danhSachPhim;
    vector<pair<string, string> > dsTheLoai;
    vector<pair<string, string> > movieGenres;
    docDanhSachPhong(danhSachPhong);
    docDanhSachPhim(danhSachPhim);
    docDanhSachTheLoai(dsTheLoai);
    docMovieGenres(movieGenres);

    ofstream outFile("DATA/quan_ly_phim.csv", ios::trunc);
    if (!outFile.is_open()) return;

    outFile << "[GENRES]\n";
    outFile << "maTheLoai,tenTheLoai\n";
for (size_t i = 0; i < dsTheLoai.size(); ++i) {
    outFile << dsTheLoai[i].first << ","
            << dsTheLoai[i].second << "\n";
}

    outFile << "\n[MOVIE_GENRES]\n";
    outFile << "maPhim,maTheLoai\n";
    for (size_t i = 0; i < movieGenres.size(); ++i) {
    	outFile << movieGenres[i].first << ","
        	    << movieGenres[i].second << "\n";
	}

    outFile << "\n[ROOMS]\n";
    outFile << "maPhong,tenPhong,loaiPhong,sucChua,trangThaiPhong\n";
    for (size_t i = 0; i < danhSachPhong.size(); ++i) {
        outFile << danhSachPhong[i].getMaPhong() << "," << danhSachPhong[i].getTenPhong() << ","  
                << danhSachPhong[i].getLoaiPhong() << "," << danhSachPhong[i].getSucChua() << "," << danhSachPhong[i].getTrangThaiPhong() << "\n";
    }

    outFile << "\n[SEATS]\n";
    outFile << "maGhe,maPhong,loaiGhe,donGiaGhe,trangThai\n";
    for (size_t i = 0; i < danhSachGhe.size(); ++i) {
        outFile << danhSachGhe[i].getMaGhe() << "," << danhSachGhe[i].getMaPhong() << ","  
                << danhSachGhe[i].getLoaiGhe() << "," << danhSachGhe[i].getDonGiaGhe() << "," << danhSachGhe[i].getTrangThai() << "\n";
    }

    outFile << "\n[MOVIES]\n";
    outFile << "maPhim,tenPhim,thoiLuong,nhanDoTuoi,ngayKhoiChieu,ngayKetThuc,moTa,poster\n";
    for (size_t i = 0; i < danhSachPhim.size(); ++i) {
        outFile << danhSachPhim[i].getMaPhim() << "," << danhSachPhim[i].getTenPhim() << "," << danhSachPhim[i].getThoiLuong() << ","  
                << danhSachPhim[i].getNhanDoTuoi() << "," << danhSachPhim[i].getNgayKhoiChieu() << "," << danhSachPhim[i].getNgayKetThuc() << ","  
                << danhSachPhim[i].getMoTa() << "," << danhSachPhim[i].getPoster() << "\n";
    }
    outFile.close();
}

void Database::taoGheTuDongChoPhong(const string& maPhongCanTao, int sucChuaPhong) {
    vector<Room> danhSachPhong;
    vector<Seat> danhSachGhe;
    vector<Movie> danhSachPhim;
    vector<pair<string, string> > dsTheLoai;
    vector<pair<string, string> > movieGenres;

    docDanhSachPhong(danhSachPhong);
    docDanhSachGhe(danhSachGhe);
    docDanhSachPhim(danhSachPhim);
    docDanhSachTheLoai(dsTheLoai);
    docMovieGenres(movieGenres);

    char hang = 'A';
    int soThuTu = 1;
    int soGheMoiHang = 10;

    for (int i = 1; i <= sucChuaPhong; ++i) {
        stringstream ss;
        ss << hang << soThuTu;
        string maGhe = ss.str();

        string loaiGhe = (hang <= 'B') ? "Standard" : "VIP";
        double donGia = (loaiGhe == "VIP") ? 90000 : 60000;

        Seat newSeat(maGhe, maPhongCanTao, loaiGhe, donGia, "Trong");
        danhSachGhe.push_back(newSeat);

        soThuTu++;
        if (soThuTu > soGheMoiHang) {
            soThuTu = 1;
            hang++;
        }
    }

    // Ghi l?i toàn b? 5 b?ng chu?n xác
    ofstream outFile("DATA/quan_ly_phim.csv", ios::trunc);
    if (!outFile.is_open()) return;

    outFile << "[GENRES]\n";
    outFile << "maTheLoai,tenTheLoai\n";
    for (size_t i = 0; i < dsTheLoai.size(); ++i) {
        outFile << dsTheLoai[i].first << "," << dsTheLoai[i].second << "\n";
    }

    outFile << "\n[MOVIE_GENRES]\n";
    outFile << "maPhim,maTheLoai\n";
    for (size_t i = 0; i < movieGenres.size(); ++i) {
        outFile << movieGenres[i].first << "," << movieGenres[i].second << "\n";
    }

    outFile << "\n[ROOMS]\n";
    outFile << "maPhong,tenPhong,loaiPhong,sucChua,trangThaiPhong\n";
    for (size_t i = 0; i < danhSachPhong.size(); ++i) {
        outFile << danhSachPhong[i].getMaPhong() << "," << danhSachPhong[i].getTenPhong() << ","  
                << danhSachPhong[i].getLoaiPhong() << "," << danhSachPhong[i].getSucChua() << "," << danhSachPhong[i].getTrangThaiPhong() << "\n";
    }

    outFile << "\n[SEATS]\n";
    outFile << "maGhe,maPhong,loaiGhe,donGiaGhe,trangThai\n";
    for (size_t i = 0; i < danhSachGhe.size(); ++i) {
        outFile << danhSachGhe[i].getMaGhe() << "," << danhSachGhe[i].getMaPhong() << ","  
                << danhSachGhe[i].getLoaiGhe() << "," << danhSachGhe[i].getDonGiaGhe() << "," << danhSachGhe[i].getTrangThai() << "\n";
    }

    outFile << "\n[MOVIES]\n";
    outFile << "maPhim,tenPhim,thoiLuong,nhanDoTuoi,ngayKhoiChieu,ngayKetThuc,moTa,poster\n";
    for (size_t i = 0; i < danhSachPhim.size(); ++i) {
        outFile << danhSachPhim[i].getMaPhim() << "," << danhSachPhim[i].getTenPhim() << "," << danhSachPhim[i].getThoiLuong() << ","  
                << danhSachPhim[i].getNhanDoTuoi() << "," << danhSachPhim[i].getNgayKhoiChieu() << "," << danhSachPhim[i].getNgayKetThuc() << ","  
                << danhSachPhim[i].getMoTa() << "," << danhSachPhim[i].getPoster() << "\n";
    }
    outFile.close();
}

// --- Qu?n lý Phim (Movies) ---
void Database::docDanhSachPhim(vector<Movie>& danhSachPhim) {
    danhSachPhim.clear();
    ifstream inFile("DATA/quan_ly_phim.csv");
    if (!inFile.is_open()) return;

    string line;
    bool reading = false;
    while (getline(inFile, line)) {
        if (line.empty()) continue;
        if (line.find("[MOVIES]") != string::npos) {
            reading = true;
            continue;
        }
        if (reading && line[0] == '[') {
            reading = false;
            break;
        }

        if (reading) {
            if (line.find("maPhim") != string::npos) continue;
            vector<string> parts = tachChuoiCSV(line);
            if (parts.size() >= 8) {
                Movie m(parts[0], parts[1], atoi(parts[2].c_str()), atoi(parts[3].c_str()), parts[4], parts[5], parts[6], parts[7]);
                danhSachPhim.push_back(m);
            }
        }
    }
    inFile.close();
}

void Database::ghiDanhSachPhim(const vector<Movie>& danhSachPhim) {
    vector<Room> danhSachPhong;
    vector<Seat> danhSachGhe;
    vector<pair<string, string> > dsTheLoai;
    vector<pair<string, string> > movieGenres;
    docDanhSachPhong(danhSachPhong);
    docDanhSachGhe(danhSachGhe);
    docDanhSachTheLoai(dsTheLoai);
    docMovieGenres(movieGenres);

    ofstream outFile("DATA/quan_ly_phim.csv", ios::trunc);
    if (!outFile.is_open()) return;

    outFile << "[GENRES]\n";
    outFile << "maTheLoai,tenTheLoai\n";
    for (size_t i = 0; i < dsTheLoai.size(); ++i) {
    outFile << dsTheLoai[i].first << ","
            << dsTheLoai[i].second << "\n";
}

    outFile << "\n[MOVIE_GENRES]\n";
    outFile << "maPhim,maTheLoai\n";
    for (size_t i = 0; i < movieGenres.size(); ++i) {
    outFile << movieGenres[i].first << ","
            << movieGenres[i].second << "\n";
}

    outFile << "\n[ROOMS]\n";
    outFile << "maPhong,tenPhong,loaiPhong,sucChua,trangThaiPhong\n";
    for (size_t i = 0; i < danhSachPhong.size(); ++i) {
        outFile << danhSachPhong[i].getMaPhong() << "," << danhSachPhong[i].getTenPhong() << ","  
                << danhSachPhong[i].getLoaiPhong() << "," << danhSachPhong[i].getSucChua() << "," << danhSachPhong[i].getTrangThaiPhong() << "\n";
    }

    outFile << "\n[SEATS]\n";
    outFile << "maGhe,maPhong,loaiGhe,donGiaGhe,trangThai\n";
    for (size_t i = 0; i < danhSachGhe.size(); ++i) {
        outFile << danhSachGhe[i].getMaGhe() << "," << danhSachGhe[i].getMaPhong() << ","  
                << danhSachGhe[i].getLoaiGhe() << "," << danhSachGhe[i].getDonGiaGhe() << "," << danhSachGhe[i].getTrangThai() << "\n";
    }

    outFile << "\n[MOVIES]\n";
    outFile << "maPhim,tenPhim,thoiLuong,nhanDoTuoi,ngayKhoiChieu,ngayKetThuc,moTa,poster\n";
    for (size_t i = 0; i < danhSachPhim.size(); ++i) {
        outFile << danhSachPhim[i].getMaPhim() << "," << danhSachPhim[i].getTenPhim() << "," << danhSachPhim[i].getThoiLuong() << ","  
                << danhSachPhim[i].getNhanDoTuoi() << "," << danhSachPhim[i].getNgayKhoiChieu() << "," << danhSachPhim[i].getNgayKetThuc() << ","  
                << danhSachPhim[i].getMoTa() << "," << danhSachPhim[i].getPoster() << "\n";
    }
    outFile.close();
}


// =========================================================================
// 3. L?CH CHI?U & VÉ (DATA/lich_va_ve.csv)
// =========================================================================

// --- Qu?n lý Su?t chi?u (Showtimes) ---
void Database::docDanhSachSuatChieu(vector<Showtime>& danhSachSuatChieu) {
    danhSachSuatChieu.clear();
    ifstream inFile("DATA/lich_va_ve.csv");
    if (!inFile.is_open()) return;

    string line;
    bool reading = false;

    while (getline(inFile, line)) {
        if (line.empty()) continue;

        if (line.find("[SHOWTIMES]") != string::npos) {
            reading = true;
            continue;
        }

        if (reading && line[0] == '[') {
            reading = false;
            break;
        }

        if (reading) {
            if (line.find("maSuatChieu") != string::npos) continue; // B? qua dòng tiêu d? c?t
            vector<string> parts = tachChuoiCSV(line);
            if (parts.size() >= 7) {
                Showtime st(parts[0], parts[1], parts[2], parts[3], parts[4], parts[5], atof(parts[6].c_str()));
                danhSachSuatChieu.push_back(st);
            }
        }
    }
    inFile.close();
}

void Database::ghiDanhSachSuatChieu(const vector<Showtime>& danhSachSuatChieu) {
    vector<Ticket> danhSachVe;
    docDanhSachVe(danhSachVe);

    ofstream outFile("DATA/lich_va_ve.csv", ios::trunc);
    if (!outFile.is_open()) return;

    outFile << "[SHOWTIMES]\n";
    outFile << "maSuatChieu,maPhim,maPhong,ngayChieu,gioBatDau,gioKetThuc,giaVeCoBan\n";
    for (size_t i = 0; i < danhSachSuatChieu.size(); ++i) {
        outFile << danhSachSuatChieu[i].getMaSuatChieu() << "," << danhSachSuatChieu[i].getMaPhim() << "," << danhSachSuatChieu[i].getMaPhong() << ","  
                << danhSachSuatChieu[i].getNgayChieu() << "," << danhSachSuatChieu[i].getGioBatDau() << "," << danhSachSuatChieu[i].getGioKetThuc() << ","  
                << danhSachSuatChieu[i].getGiaVeCoBan() << "\n";
    }

    outFile << "\n[TICKETS]\n";
    outFile << "maVe,maHoaDon,maSuatChieu,maGhe,giaVeThucTe,maQRCode,trangThaiSuDung\n";
    for (size_t i = 0; i < danhSachVe.size(); ++i) {
        outFile << danhSachVe[i].getMaVe() << "," << danhSachVe[i].getMaHoaDon() << "," << danhSachVe[i].getMaSuatChieu() << ","  
                << danhSachVe[i].getMaGhe() << "," << danhSachVe[i].getGiaVeThucTe() << "," << danhSachVe[i].getMaQRCode() << ","  
                << danhSachVe[i].getTrangThaiSuDung() << "\n";
    }
    outFile.close();
}

// --- Qu?n lý Vé (Tickets) ---
void Database::docDanhSachVe(vector<Ticket>& danhSachVe) {
    danhSachVe.clear();
    ifstream inFile("DATA/lich_va_ve.csv");
    if (!inFile.is_open()) return;

    string line;
    bool reading = false;
    while (getline(inFile, line)) {
        if (line.empty()) continue;
        if (line.find("[TICKETS]") != string::npos) {
            reading = true;
            continue;
        }
        if (reading && line[0] == '[') {
            reading = false;
            break;
        }

        if (reading) {
            if (line.find("maVe") != string::npos) continue; // B? qua dòng tiêu d? c?t
            vector<string> parts = tachChuoiCSV(line);
            if (parts.size() >= 7) {
                Ticket t(parts[0], parts[1], parts[2], parts[3], atof(parts[4].c_str()), parts[5], parts[6]);
                danhSachVe.push_back(t);
            }
        }
    }
    inFile.close();
}

void Database::ghiDanhSachVe(const vector<Ticket>& danhSachVe) {
    vector<Showtime> danhSachSuatChieu;
    docDanhSachSuatChieu(danhSachSuatChieu);

    ofstream outFile("DATA/lich_va_ve.csv", ios::trunc);
    if (!outFile.is_open()) return;

    outFile << "[SHOWTIMES]\n";
    outFile << "maSuatChieu,maPhim,maPhong,ngayChieu,gioBatDau,gioKetThuc,giaVeCoBan\n";
    for (size_t i = 0; i < danhSachSuatChieu.size(); ++i) {
        outFile << danhSachSuatChieu[i].getMaSuatChieu() << "," << danhSachSuatChieu[i].getMaPhim() << "," << danhSachSuatChieu[i].getMaPhong() << ","  
                << danhSachSuatChieu[i].getNgayChieu() << "," << danhSachSuatChieu[i].getGioBatDau() << "," << danhSachSuatChieu[i].getGioKetThuc() << ","  
                << danhSachSuatChieu[i].getGiaVeCoBan() << "\n";
    }

    outFile << "\n[TICKETS]\n";
    outFile << "maVe,maHoaDon,maSuatChieu,maGhe,giaVeThucTe,maQRCode,trangThaiSuDung\n";
    for (size_t i = 0; i < danhSachVe.size(); ++i) {
        outFile << danhSachVe[i].getMaVe() << "," << danhSachVe[i].getMaHoaDon() << "," << danhSachVe[i].getMaSuatChieu() << ","  
                << danhSachVe[i].getMaGhe() << "," << danhSachVe[i].getGiaVeThucTe() << "," << danhSachVe[i].getMaQRCode() << ","  
                << danhSachVe[i].getTrangThaiSuDung() << "\n";
    }
    outFile.close();
}


// =========================================================================
// 4. DOANH THU, F&B & KHUY?N MÃI (DATA/giao_dich_fnb.csv)
// =========================================================================

// --- Qu?n lý F&B ---
void Database::docDanhSachFAndB(vector<FoodAndBeverage>& danhSachFAndB) {
    danhSachFAndB.clear();
    ifstream inFile("DATA/giao_dich_fnb.csv");
    if (!inFile.is_open()) return;

    string line;
    bool reading = false;
    while (getline(inFile, line)) {
        if (line.empty()) continue;
        if (line.find("[FOOD_AND_BEVERAGES]") != string::npos) {
            reading = true;
            continue;
        }
        if (reading && line[0] == '[') {
            reading = false;
            break;
        }

        if (reading) {
            if (line.find("maMatHang") != string::npos) continue; // B? qua dòng tiêu d? c?t
            vector<string> parts = tachChuoiCSV(line);
            if (parts.size() >= 5) {
                FoodAndBeverage fb(parts[0], parts[1], parts[2], atof(parts[3].c_str()), parts[4]);
                danhSachFAndB.push_back(fb);
            }
        }
    }
    inFile.close();
}

void Database::ghiDanhSachFAndB(const vector<FoodAndBeverage>& danhSachFAndB) {
    vector<Promotion> danhSachKhuyenMai;
    vector<Invoice> danhSachHoaDon;
    docDanhSachKhuyenMai(danhSachKhuyenMai);
    docDanhSachHoaDon(danhSachHoaDon);

    ofstream outFile("DATA/giao_dich_fnb.csv", ios::trunc);
    if (!outFile.is_open()) return;

    outFile << "[FOOD_AND_BEVERAGES]\n";
    outFile << "maMatHang,tenMatHang,loai,giaTien,trangThaiKinhDoanh\n";
    for (size_t i = 0; i < danhSachFAndB.size(); ++i) {
        outFile << danhSachFAndB[i].getMaMatHang() << "," << danhSachFAndB[i].getTenMatHang() << ","  
                << danhSachFAndB[i].getLoai() << "," << danhSachFAndB[i].getGiaTien() << "," << danhSachFAndB[i].getTrangThaiKinhDoanh() << "\n";
    }

    outFile << "\n[PROMOTIONS]\n";
    outFile << "maKhuyenMai,tenKhuyenMai,maCode,phanTramGiam,mucGiamToiDa,ngayBatDau,ngayKetThuc,phamViApDung\n";
    for (size_t i = 0; i < danhSachKhuyenMai.size(); ++i) {
        outFile << danhSachKhuyenMai[i].getMaKhuyenMai() << "," << danhSachKhuyenMai[i].getTenKhuyenMai() << ","  
                << danhSachKhuyenMai[i].getMaCode() << "," << danhSachKhuyenMai[i].getPhanTramGiam() << ","  
                << danhSachKhuyenMai[i].getMucGiamToiDa() << "," << danhSachKhuyenMai[i].getNgayBatDau() << ","  
                << danhSachKhuyenMai[i].getNgayKetThuc() << "," << danhSachKhuyenMai[i].getPhamViApDung() << "\n";
    }

    outFile << "\n[INVOICES]\n";
    outFile << "maHoaDon,maKhachHang,maNhanVien,thoiGianLap,tongTien,hinhThucThanhToan,maGiaoDich,trangThaiThanhToan\n";
    for (size_t i = 0; i < danhSachHoaDon.size(); ++i) {
        outFile << danhSachHoaDon[i].getMaHoaDon() << "," << danhSachHoaDon[i].getMaKhachHang() << ","  
                << danhSachHoaDon[i].getMaNhanVien() << "," << danhSachHoaDon[i].getThoiGianLap() << ","  
                << danhSachHoaDon[i].getTongTien() << "," << danhSachHoaDon[i].getHinhThucThanhToan() << ","  
                << danhSachHoaDon[i].getMaGiaoDich() << "," << danhSachHoaDon[i].getTrangThaiThanhToan() << "\n";
    }
    outFile.close();
}

// --- Qu?n lý Khuy?n mãi (Promotions) ---
void Database::docDanhSachKhuyenMai(vector<Promotion>& danhSachKhuyenMai) {
    danhSachKhuyenMai.clear();
    ifstream inFile("DATA/giao_dich_fnb.csv");
    if (!inFile.is_open()) return;

    string line;
    bool reading = false;
    while (getline(inFile, line)) {
        if (line.empty()) continue;
        if (line.find("[PROMOTIONS]") != string::npos) {
            reading = true;
            continue;
        }
        if (reading && line[0] == '[') {
            reading = false;
            break;
        }

        if (reading) {
            if (line.find("maKhuyenMai") != string::npos) continue; // B? qua dòng tiêu d? c?t
            vector<string> parts = tachChuoiCSV(line);
            if (parts.size() >= 8) {
                Promotion km(parts[0], parts[1], parts[2], atof(parts[3].c_str()), atof(parts[4].c_str()), parts[5], parts[6], parts[7]);
                danhSachKhuyenMai.push_back(km);
            }
        }
    }
    inFile.close();
}

void Database::ghiDanhSachKhuyenMai(const vector<Promotion>& danhSachKhuyenMai) {
    vector<FoodAndBeverage> danhSachFAndB;
    vector<Invoice> danhSachHoaDon;
    docDanhSachFAndB(danhSachFAndB);
    docDanhSachHoaDon(danhSachHoaDon);

    ofstream outFile("DATA/giao_dich_fnb.csv", ios::trunc);
    if (!outFile.is_open()) return;

    outFile << "[FOOD_AND_BEVERAGES]\n";
    outFile << "maMatHang,tenMatHang,loai,giaTien,trangThaiKinhDoanh\n";
    for (size_t i = 0; i < danhSachFAndB.size(); ++i) {
        outFile << danhSachFAndB[i].getMaMatHang() << "," << danhSachFAndB[i].getTenMatHang() << ","  
                << danhSachFAndB[i].getLoai() << "," << danhSachFAndB[i].getGiaTien() << "," << danhSachFAndB[i].getTrangThaiKinhDoanh() << "\n";
    }

    outFile << "\n[PROMOTIONS]\n";
    outFile << "maKhuyenMai,tenKhuyenMai,maCode,phanTramGiam,mucGiamToiDa,ngayBatDau,ngayKetThuc,phamViApDung\n";
    for (size_t i = 0; i < danhSachKhuyenMai.size(); ++i) {
        outFile << danhSachKhuyenMai[i].getMaKhuyenMai() << "," << danhSachKhuyenMai[i].getTenKhuyenMai() << ","  
                << danhSachKhuyenMai[i].getMaCode() << "," << danhSachKhuyenMai[i].getPhanTramGiam() << ","  
                << danhSachKhuyenMai[i].getMucGiamToiDa() << "," << danhSachKhuyenMai[i].getNgayBatDau() << ","  
                << danhSachKhuyenMai[i].getNgayKetThuc() << "," << danhSachKhuyenMai[i].getPhamViApDung() << "\n";
    }

    outFile << "\n[INVOICES]\n";
    outFile << "maHoaDon,maKhachHang,maNhanVien,thoiGianLap,tongTien,hinhThucThanhToan,maGiaoDich,trangThaiThanhToan\n";
    for (size_t i = 0; i < danhSachHoaDon.size(); ++i) {
        outFile << danhSachHoaDon[i].getMaHoaDon() << "," << danhSachHoaDon[i].getMaKhachHang() << ","  
                << danhSachHoaDon[i].getMaNhanVien() << "," << danhSachHoaDon[i].getThoiGianLap() << ","  
                << danhSachHoaDon[i].getTongTien() << "," << danhSachHoaDon[i].getHinhThucThanhToan() << ","  
                << danhSachHoaDon[i].getMaGiaoDich() << "," << danhSachHoaDon[i].getTrangThaiThanhToan() << "\n";
    }
    outFile.close();
}

// --- Qu?n lý Hóa don (Invoices) ---
void Database::docDanhSachHoaDon(vector<Invoice>& danhSachHoaDon) {
    danhSachHoaDon.clear();
    ifstream inFile("DATA/giao_dich_fnb.csv");
    if (!inFile.is_open()) return;

    string line;
    bool reading = false;
    while (getline(inFile, line)) {
        if (line.empty()) continue;
        if (line.find("[INVOICES]") != string::npos) {
            reading = true;
            continue;
        }
        if (reading && line[0] == '[') {
            reading = false;
            break;
        }

        if (reading) {
            if (line.find("maHoaDon") != string::npos) continue; // B? qua dòng tiêu d? c?t
            vector<string> parts = tachChuoiCSV(line);
            if (parts.size() >= 8) {
                Invoice hd(parts[0], parts[1], parts[2], parts[3], atof(parts[4].c_str()), parts[5], parts[6], parts[7]);
                danhSachHoaDon.push_back(hd);
            }
        }
    }
    inFile.close();
}

void Database::ghiDanhSachHoaDon(const vector<Invoice>& danhSachHoaDon) {
    vector<FoodAndBeverage> danhSachFAndB;
    vector<Promotion> danhSachKhuyenMai;
    docDanhSachFAndB(danhSachFAndB);
    docDanhSachKhuyenMai(danhSachKhuyenMai);

    ofstream outFile("DATA/giao_dich_fnb.csv", ios::trunc);
    if (!outFile.is_open()) return;

    outFile << "[FOOD_AND_BEVERAGES]\n";
    outFile << "maMatHang,tenMatHang,loai,giaTien,trangThaiKinhDoanh\n";
    for (size_t i = 0; i < danhSachFAndB.size(); ++i) {
        outFile << danhSachFAndB[i].getMaMatHang() << "," << danhSachFAndB[i].getTenMatHang() << ","  
                << danhSachFAndB[i].getLoai() << "," << danhSachFAndB[i].getGiaTien() << "," << danhSachFAndB[i].getTrangThaiKinhDoanh() << "\n";
    }

    outFile << "\n[PROMOTIONS]\n";
    outFile << "maKhuyenMai,tenKhuyenMai,maCode,phanTramGiam,mucGiamToiDa,ngayBatDau,ngayKetThuc,phamViApDung\n";
    for (size_t i = 0; i < danhSachKhuyenMai.size(); ++i) {
        outFile << danhSachKhuyenMai[i].getMaKhuyenMai() << "," << danhSachKhuyenMai[i].getTenKhuyenMai() << ","  
                << danhSachKhuyenMai[i].getMaCode() << "," << danhSachKhuyenMai[i].getPhanTramGiam() << ","  
                << danhSachKhuyenMai[i].getMucGiamToiDa() << "," << danhSachKhuyenMai[i].getNgayBatDau() << ","  
                << danhSachKhuyenMai[i].getNgayKetThuc() << "," << danhSachKhuyenMai[i].getPhamViApDung() << "\n";
    }

    outFile << "\n[INVOICES]\n";
    outFile << "maHoaDon,maKhachHang,maNhanVien,thoiGianLap,tongTien,hinhThucThanhToan,maGiaoDich,trangThaiThanhToan\n";
    for (size_t i = 0; i < danhSachHoaDon.size(); ++i) {
        outFile << danhSachHoaDon[i].getMaHoaDon() << "," << danhSachHoaDon[i].getMaKhachHang() << ","  
                << danhSachHoaDon[i].getMaNhanVien() << "," << danhSachHoaDon[i].getThoiGianLap() << ","  
                << danhSachHoaDon[i].getTongTien() << "," << danhSachHoaDon[i].getHinhThucThanhToan() << ","  
                << danhSachHoaDon[i].getMaGiaoDich() << "," << danhSachHoaDon[i].getTrangThaiThanhToan() << "\n";
    }
    outFile.close();
}
// =========================================================================
// 5. V?N HÀNH H? TH?NG & CA LÀM VI?C (DATA/van_hanh_he_thong.csv)
// =========================================================================

void Database::docDanhSachCa(vector<ShiftReport>& danhSachCa) {
    danhSachCa.clear();
    ifstream inFile("DATA/van_hanh_he_thong.csv");
    if (!inFile.is_open()) return;

    string line;
    bool reading = false;
    while (getline(inFile, line)) {
        if (line.empty()) continue;
        if (line.find("[SHIFT_MANAGEMENTS]") != string::npos) {
            reading = true;
            continue;
        }
        if (reading && line[0] == '[') {
            reading = false;
            break;
        }

        if (reading) {
            if (line.find("maCa") != string::npos) continue; // B? qua dòng tiêu d? c?t
            vector<string> parts = tachChuoiCSV(line);
            
            // C?u trúc c?t t? file van_hanh_he_thong.csv:
            // [0]: maCa, [1]: maNhanVien, [2]: thoiGianBatDau, [3]: thoiGianKetThuc, 
            // [4]: tongTienDauCa, [5]: doanhThuVe, [6]: doanhThuFnB, [7]: doanhThuKhac, 
            // [8]: tongDoanhThu (doanhThuHeThong), [9]: tienMatDauCa, [10]: tienMatThucTe, 
            // [11]: doLechDoanhThu, [12]: tienXuatQuy, [13]: tienGiulai, [14]: trangThaiCa
            if (parts.size() >= 15) {
                string maCa = parts[0];
                string maNV = parts[1];
                string thoiGianBD = parts[2];
                string thoiGianKT = parts[3];
                
                // Tách l?y ph?n ngày (yyyy-mm-dd ho?c dd/mm/yyyy) t? chu?i th?i gian b?t d?u
                string ngayLam = (thoiGianBD.length() >= 10) ? thoiGianBD.substr(0, 10) : "";
                
                long long doanhThuHeThong = atoll(parts[8].c_str());
                long long tienThucTe = atoll(parts[10].c_str());
                string trangThai = parts[14];

                // Kh?i t?o d?i tu?ng ShiftReport kh?p v?i các tru?ng trong class c?a b?n
                ShiftReport sr(maCa, maNV, "Nhan Vien", ngayLam, thoiGianBD, thoiGianKT, doanhThuHeThong, tienThucTe, trangThai, false);
                danhSachCa.push_back(sr);
            }
        }
    }
    inFile.close();
}

void Database::ghiDanhSachCa(const vector<ShiftReport>& danhSachCa) {
    ofstream outFile("DATA/van_hanh_he_thong.csv", ios::trunc);
    if (!outFile.is_open()) return;

    // Ghi kh?i SHIFT_MANAGEMENTS v?i d?y d? tiêu d? c?t theo dúng m?u th?c t? c?a b?n
    outFile << "[SHIFT_MANAGEMENTS]\n";
    outFile << "maCa,maNhanVien,thoiGianBatDau,thoiGianKetThuc,tongTienDauCa,doanhThuVe,doanhThuFnB,doanhThuKhac,tongDoanhThu,tienMatDauCa,tienMatThucTe,doLechDoanhThu,tienXuatQuy,tienGiulai,trangThaiCa\n";
    
    for (size_t i = 0; i < danhSachCa.size(); ++i) {
    	outFile << danhSachCa[i].getMaCa() << ","
            << danhSachCa[i].getMaNhanVien() << ","
            << danhSachCa[i].getTenNhanVien() << ","
            << danhSachCa[i].getNgayLam() << ","
            << danhSachCa[i].getGioBatDau() << ","
            << danhSachCa[i].getGioKetThuc() << ","
            << danhSachCa[i].getDoanhThuHeThong() << ","
            << danhSachCa[i].getTienThucTe() << ","
            << danhSachCa[i].getChenhLech() << ","
            << danhSachCa[i].getTrangThai() << ","
            << danhSachCa[i].getDaKhoaBaoCao()
            << "\n";
	}

    // B?o toàn ghi l?i kh?i [SYSTEM_LOGS] bên du?i d? file không b? m?t c?u trúc log h? th?ng
    outFile << "\n[SYSTEM_LOGS]\n";
    outFile << "maLog,thoiGian,maNguoiDung,loaiHanhDong,chiTietThayDoi\n";
    outFile << "LOG1,2026-10-14,U02,Them,Tao hoa don moi HD01 cho khach hang U03\n";

    outFile.close();
}
