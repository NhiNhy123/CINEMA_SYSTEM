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
#include "User.h"

#include <fstream>
#include <sstream>
#include <iostream>
#include <cstdlib>
#include <utility>

using namespace std;

Database::Database() {}

// Hàm ti?n ích tách chu?i CSV
static vector<string> tachChuoiCSV(const string& line) {
    vector<string> tokens;
    stringstream ss(line);
    string item;
    while (getline(ss, item, ',')) {
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
void Database::docDanhSachUsers(vector<Customer>& danhSachKH) {
    danhSachKH.clear();
    ifstream inFile("DATA/quan_ly_nhan_su.csv");
    if (!inFile.is_open()) return;

    string line;
    bool readingUsers = false, readingCustomers = false;
    vector<string> maUsers, hoTens, soDienThoais;

    while (getline(inFile, line)) {
        if (line.empty()) continue;
        if (line.find("[USERS]") != string::npos) { readingUsers = true; readingCustomers = false; continue; }
        if (line.find("[CUSTOMERS]") != string::npos) { readingUsers = false; readingCustomers = true; continue; }
        if (line[0] == '[') { readingUsers = readingCustomers = false; continue; }

        if (readingUsers && line.find("maNguoiDung") == string::npos) {
            vector<string> p = tachChuoiCSV(line);
            if (p.size() >= 4) { maUsers.push_back(p[0]); hoTens.push_back(p[1]); soDienThoais.push_back(p[2]); }
        } else if (readingCustomers && line.find("maKhachHang") == string::npos) {
            vector<string> p = tachChuoiCSV(line);
            if (p.size() >= 4) {
                string maKH = p[0];
                int diem = atoi(p[1].c_str());
                string hang = p[2];
                long long chiTieu = atoll(p[3].c_str());

                string hoTen = "", sdt = "", email = "";
                for (size_t i = 0; i < maUsers.size(); ++i) {
                    if (maUsers[i] == maKH) { hoTen = hoTens[i]; sdt = soDienThoais[i]; break; }
                }
                danhSachKH.push_back(Customer(maKH, hoTen, sdt, "", diem, hang, chiTieu));
            }
        }
    }
    inFile.close();
}

string Database::layTenNhanVienQuaMa(string maNhanVien) {
    ifstream inFile("DATA/quan_ly_nhan_su.csv");
    if (!inFile.is_open()) return "Nhan vien chua ro";

    string line;
    bool readingUsers = false;

    while (getline(inFile, line)) {
        if (line.empty()) continue;
        if (line.find("[USERS]") != string::npos) { readingUsers = true; continue; }
        if (readingUsers && line[0] == '[') break;

        if (readingUsers && line.find("maNguoiDung") == string::npos) {
            vector<string> p = tachChuoiCSV(line);
            if (p.size() >= 12) {
                // p[0] là maNguoiDung, p[1] là hoTen, p[7] là taiKhoan
                if (p[0] == maNhanVien || p[7] == maNhanVien) {
                    inFile.close();
                    return p[1]; // Tr? v? hoTen ngay khi tìm th?y
                }
            }
        }
    }
    inFile.close();
    return "Nhan vien chua ro";
}

void Database::ghiDanhSachUsers(const vector<Customer>& danhSachKH) {
    ifstream inFile("DATA/quan_ly_nhan_su.csv");
    vector<string> rawUsersLines, historyLines;
    string currentSection = "", line;
    
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
            if (currentSection == "USERS" && line.find("maNguoiDung") == string::npos) rawUsersLines.push_back(line);
            else if (currentSection == "HISTORY" && line.find("maGiaoDichDiem") == string::npos) historyLines.push_back(line);
        }
        inFile.close();
    }

    vector<string> finalCustomersLines;
    for (size_t i = 0; i < danhSachKH.size(); ++i) {
        stringstream ssDiem, ssChiTieu;
        ssDiem << danhSachKH[i].getDiemTichLuy();
        ssChiTieu << danhSachKH[i].getTongChiTieu();
        finalCustomersLines.push_back(danhSachKH[i].getMaKH() + "," + ssDiem.str() + "," + danhSachKH[i].getHangThanhVien() + "," + ssChiTieu.str());

        bool found = false;
        for (size_t j = 0; j < rawUsersLines.size(); ++j) {
            if (rawUsersLines[j].find(danhSachKH[i].getMaKH() + ",") == 0) { found = true; break; }
        }
        if (!found) {
            rawUsersLines.push_back(danhSachKH[i].getMaKH() + "," + danhSachKH[i].getHoTen() + "," + danhSachKH[i].getSoDienThoai() + "," +
                                danhSachKH[i].getEmail() + ",1/1/2000,Nam,123456789012,khach_" + danhSachKH[i].getSoDienThoai() + ",123456,Customer,DangLam,HoatDong");
        }
    }

    ofstream outFile("DATA/quan_ly_nhan_su.csv", ios::trunc);
    if (!outFile.is_open()) return;

    outFile << "[USERS]\nmaNguoiDung,hoTen,soDienThoai,email,ngaySinh,gioiTinh,cccd,taiKhoan,matKhau,vaiTro,trangThaiLamViec,trangThaiTaiKhoan\n";
    for (size_t i = 0; i < rawUsersLines.size(); ++i) outFile << rawUsersLines[i] << "\n";

    outFile << "\n[CUSTOMERS]\nmaKhachHang,diemTichLuy,hangThanhVien,tongChiTieu\n";
    for (size_t i = 0; i < finalCustomersLines.size(); ++i) outFile << finalCustomersLines[i] << "\n";

    outFile << "\n[CUSTOMER_POINTS_HISTORY]\nmaGiaoDichDiem,maKhachHang,loaiGiaoDich,soDiem,noiDung,thoiGian\n";
    for (size_t i = 0; i < historyLines.size(); ++i) outFile << historyLines[i] << "\n";
    outFile.close();
}

void Database::docLichSuGiaoDichDiem(vector<GiaoDichDiem>& danhSachGD) {
    danhSachGD.clear();
    ifstream inFile("DATA/quan_ly_nhan_su.csv");
    if (!inFile.is_open()) return;

    string line;
    bool reading = false;
    while (getline(inFile, line)) {
        if (line.empty()) continue;
        if (line.find("[CUSTOMER_POINTS_HISTORY]") != string::npos) { reading = true; continue; }
        if (reading && line[0] == '[') { reading = false; break; }

        if (reading && line.find("maGiaoDichDiem") == string::npos) {
            vector<string> p = tachChuoiCSV(line);
            if (p.size() >= 6) danhSachGD.push_back(GiaoDichDiem(p[0], p[1], p[2], atoi(p[3].c_str()), p[4], p[5]));
        }
    }
    inFile.close();
}

void Database::ghiLichSuGiaoDichDiem(const vector<GiaoDichDiem>& danhSachGD) {
    ifstream inFile("DATA/quan_ly_nhan_su.csv");
    vector<string> rawUsersLines, customersLines;
    string currentSection = "", line;
    
    if (inFile.is_open()) {
        while (getline(inFile, line)) {
            if (line.empty()) continue;
            if (line[0] == '[') {
                if (line.find("[USERS]") != string::npos) currentSection = "USERS";
                else if (line.find("[CUSTOMERS]") != string::npos) currentSection = "CUSTOMERS";
                else currentSection = "";
                continue;
            }
            if (currentSection == "USERS" && line.find("maNguoiDung") == string::npos) rawUsersLines.push_back(line);
            else if (currentSection == "CUSTOMERS" && line.find("maKhachHang") == string::npos) customersLines.push_back(line);
        }
        inFile.close();
    }

    ofstream outFile("DATA/quan_ly_nhan_su.csv", ios::trunc);
    if (!outFile.is_open()) return;

    outFile << "[USERS]\nmaNguoiDung,hoTen,soDienThoai,email,ngaySinh,gioiTinh,cccd,taiKhoan,matKhau,vaiTro,trangThaiLamViec,trangThaiTaiKhoan\n";
    for (size_t i = 0; i < rawUsersLines.size(); ++i) outFile << rawUsersLines[i] << "\n";

    outFile << "\n[CUSTOMERS]\nmaKhachHang,diemTichLuy,hangThanhVien,tongChiTieu\n";
    for (size_t i = 0; i < customersLines.size(); ++i) outFile << customersLines[i] << "\n";

    outFile << "\n[CUSTOMER_POINTS_HISTORY]\nmaGiaoDichDiem,maKhachHang,loaiGiaoDich,soDiem,noiDung,thoiGian\n";
    for (size_t i = 0; i < danhSachGD.size(); ++i) {
        outFile << danhSachGD[i].getMaGiaoDich() << "," << danhSachGD[i].getMaKhachHang() << "," << danhSachGD[i].getLoaiGiaoDich() << ","
                << danhSachGD[i].getSoDiem() << "," << danhSachGD[i].getNoiDung() << "," << danhSachGD[i].getThoiGian() << "\n";
    }
    outFile.close();
}

// =========================================================================
// 2. KHO PHIM & H? T?NG R?P (DATA/quan_ly_phim.csv & quan_ly_phong_ghe.csv)
// =========================================================================
void Database::docDanhSachTheLoai(vector<pair<string, string> >& ds) {
    ds.clear();
    ifstream inFile("DATA/quan_ly_phim.csv");
    if (!inFile.is_open()) return;
    string line; bool reading = false;
    while (getline(inFile, line)) {
        if (line.find("[GENRES]") != string::npos) { reading = true; continue; }
        if (reading && line[0] == '[') { reading = false; break; }
        if (reading && line.find("maTheLoai") == string::npos && !line.empty()) {
            vector<string> p = tachChuoiCSV(line);
            if (p.size() >= 2) ds.push_back(make_pair(p[0], p[1]));
        }
    }
    inFile.close();
}

void Database::docMovieGenres(vector<pair<string, string> >& mg) {
    mg.clear();
    ifstream inFile("DATA/quan_ly_phim.csv");
    if (!inFile.is_open()) return;
    string line; bool reading = false;
    while (getline(inFile, line)) {
        if (line.find("[MOVIE_GENRES]") != string::npos) { reading = true; continue; }
        if (reading && line[0] == '[') { reading = false; break; }
        if (reading && line.find("maPhim") == string::npos && !line.empty()) {
            vector<string> p = tachChuoiCSV(line);
            if (p.size() >= 2) mg.push_back(make_pair(p[0], p[1]));
        }
    }
    inFile.close();
}

string Database::layTheLoaiCuaPhim(string maPhimToFind) {
    vector<pair<string, string> > dsTheLoai; vector<pair<string, string> > movieGenres;
    docDanhSachTheLoai(dsTheLoai); docMovieGenres(movieGenres);
    string ketQua = "";
    for (size_t i = 0; i < movieGenres.size(); ++i) {
        if (movieGenres[i].first == maPhimToFind) {
            for (size_t j = 0; j < dsTheLoai.size(); ++j) {
                if (dsTheLoai[j].first == movieGenres[i].second) {
                    if (!ketQua.empty()) ketQua += ", ";
                    ketQua += dsTheLoai[j].second;
                }
            }
        }
    }
    return ketQua.empty() ? "Chua cap nhat" : ketQua;
}

void Database::themTheLoaiChoPhim(string maPhim, string tenTheLoaiMoi) {
    vector<pair<string, string> > dsTheLoai; vector<pair<string, string> > movieGenres; vector<Movie> danhSachPhim;
    docDanhSachTheLoai(dsTheLoai); docMovieGenres(movieGenres); docDanhSachPhim(danhSachPhim);

    string maTheLoai = "";
    for (size_t i = 0; i < dsTheLoai.size(); ++i) {
        if (dsTheLoai[i].second == tenTheLoaiMoi) { maTheLoai = dsTheLoai[i].first; break; }
    }
    if (maTheLoai.empty()) {
        stringstream ss; ss << "TL0" << dsTheLoai.size() + 1;
        maTheLoai = ss.str();
        dsTheLoai.push_back(make_pair(maTheLoai, tenTheLoaiMoi));
    }

    bool daCo = false;
    for (size_t i = 0; i < movieGenres.size(); ++i) {
        if (movieGenres[i].first == maPhim && movieGenres[i].second == maTheLoai) { daCo = true; break; }
    }
    if (!daCo) movieGenres.push_back(make_pair(maPhim, maTheLoai));

    ofstream outFile("DATA/quan_ly_phim.csv", ios::trunc);
    if (!outFile.is_open()) return;
    outFile << "[GENRES]\nmaTheLoai,tenTheLoai\n";
    for (size_t i = 0; i < dsTheLoai.size(); ++i) outFile << dsTheLoai[i].first << "," << dsTheLoai[i].second << "\n";
    outFile << "\n[MOVIE_GENRES]\nmaPhim,maTheLoai\n";
    for (size_t i = 0; i < movieGenres.size(); ++i) outFile << movieGenres[i].first << "," << movieGenres[i].second << "\n";
    outFile << "\n[MOVIES]\nmaPhim,tenPhim,thoiLuong,nhanDoTuoi,ngayKhoiChieu,ngayKetThuc,moTa,poster\n";
    for (size_t i = 0; i < danhSachPhim.size(); ++i) {
        outFile << danhSachPhim[i].getMaPhim() << "," << danhSachPhim[i].getTenPhim() << "," << danhSachPhim[i].getThoiLuong() << ","  
                << danhSachPhim[i].getNhanDoTuoi() << "," << danhSachPhim[i].getNgayKhoiChieu() << "," << danhSachPhim[i].getNgayKetThuc() << ","  
                << danhSachPhim[i].getMoTa() << "," << danhSachPhim[i].getPoster() << "\n";
    }
    outFile.close();
}

void Database::capNhatTheLoaiChoPhim(string maPhim, string tenTheLoaiMoi) {
    vector<pair<string, string> > dsTheLoai; vector<pair<string, string> > movieGenres; vector<Movie> danhSachPhim;
    docDanhSachTheLoai(dsTheLoai); docMovieGenres(movieGenres); docDanhSachPhim(danhSachPhim);

    for (size_t i = 0; i < movieGenres.size(); ) {
        if (movieGenres[i].first == maPhim) movieGenres.erase(movieGenres.begin() + i);
        else ++i;
    }

    string maTheLoai = "";
    for (size_t i = 0; i < dsTheLoai.size(); ++i) {
        if (dsTheLoai[i].second == tenTheLoaiMoi) { maTheLoai = dsTheLoai[i].first; break; }
    }
    if (maTheLoai.empty()) {
        stringstream ss; ss << "TL0" << dsTheLoai.size() + 1;
        maTheLoai = ss.str();
        dsTheLoai.push_back(make_pair(maTheLoai, tenTheLoaiMoi));
    }
    movieGenres.push_back(make_pair(maPhim, maTheLoai));

    ofstream outFile("DATA/quan_ly_phim.csv", ios::trunc);
    if (!outFile.is_open()) return;
    outFile << "[GENRES]\nmaTheLoai,tenTheLoai\n";
    for (size_t i = 0; i < dsTheLoai.size(); ++i) outFile << dsTheLoai[i].first << "," << dsTheLoai[i].second << "\n";
    outFile << "\n[MOVIE_GENRES]\nmaPhim,maTheLoai\n";
    for (size_t i = 0; i < movieGenres.size(); ++i) outFile << movieGenres[i].first << "," << movieGenres[i].second << "\n";
    outFile << "\n[MOVIES]\nmaPhim,tenPhim,thoiLuong,nhanDoTuoi,ngayKhoiChieu,ngayKetThuc,moTa,poster\n";
    for (size_t i = 0; i < danhSachPhim.size(); ++i) {
        outFile << danhSachPhim[i].getMaPhim() << "," << danhSachPhim[i].getTenPhim() << "," << danhSachPhim[i].getThoiLuong() << ","  
                << danhSachPhim[i].getNhanDoTuoi() << "," << danhSachPhim[i].getNgayKhoiChieu() << "," << danhSachPhim[i].getNgayKetThuc() << ","  
                << danhSachPhim[i].getMoTa() << "," << danhSachPhim[i].getPoster() << "\n";
    }
    outFile.close();
}

void Database::docDanhSachPhong(vector<Room>& ds) {
    ds.clear(); ifstream inFile("DATA/quan_ly_phong_ghe.csv");
    if (!inFile.is_open()) return; string line; bool reading = false;
    while (getline(inFile, line)) {
        if (line.find("[ROOMS]") != string::npos) { reading = true; continue; }
        if (reading && line[0] == '[') { reading = false; break; }
        if (reading && line.find("maPhong") == string::npos && !line.empty()) {
            vector<string> p = tachChuoiCSV(line);
            if (p.size() >= 5) ds.push_back(Room(p[0], p[1], p[2], atoi(p[3].c_str()), p[4]));
        }
    }
    inFile.close();
}

void Database::ghiDanhSachPhong(const vector<Room>& danhSachPhong) {
    vector<Seat> danhSachGhe; docDanhSachGhe(danhSachGhe);
    ofstream outFile("DATA/quan_ly_phong_ghe.csv", ios::trunc);
    if (!outFile.is_open()) return;
    outFile << "[ROOMS]\nmaPhong,tenPhong,loaiPhong,sucChua,trangThaiPhong\n";
    for (size_t i = 0; i < danhSachPhong.size(); ++i)
        outFile << danhSachPhong[i].getMaPhong() << "," << danhSachPhong[i].getTenPhong() << "," << danhSachPhong[i].getLoaiPhong() << "," << danhSachPhong[i].getSucChua() << "," << danhSachPhong[i].getTrangThaiPhong() << "\n";
    outFile << "\n[SEATS]\nmaGhe,maPhong,loaiGhe,donGiaGhe\n";
    for (size_t i = 0; i < danhSachGhe.size(); ++i)
        outFile << danhSachGhe[i].getMaGhe() << "," << danhSachGhe[i].getMaPhong() << "," << danhSachGhe[i].getLoaiGhe() << "," << danhSachGhe[i].getDonGiaGhe() << "\n";
    outFile.close();
}

void Database::docDanhSachGhe(vector<Seat>& ds) {
    ds.clear(); ifstream inFile("DATA/quan_ly_phong_ghe.csv");
    if (!inFile.is_open()) return; string line; bool reading = false;
    while (getline(inFile, line)) {
        if (line.find("[SEATS]") != string::npos) { reading = true; continue; }
        if (reading && line[0] == '[') { reading = false; break; }
        if (reading && line.find("maGhe") == string::npos && !line.empty()) {
            vector<string> p = tachChuoiCSV(line);
            if (p.size() >= 4) ds.push_back(Seat(p[0], p[1], p[2], atof(p[3].c_str())));
        }
    }
    inFile.close();
}

void Database::ghiDanhSachGhe(const vector<Seat>& danhSachGhe) {
    vector<Room> danhSachPhong; docDanhSachPhong(danhSachPhong);
    ofstream outFile("DATA/quan_ly_phong_ghe.csv", ios::trunc);
    if (!outFile.is_open()) return;
    outFile << "[ROOMS]\nmaPhong,tenPhong,loaiPhong,sucChua,trangThaiPhong\n";
    for (size_t i = 0; i < danhSachPhong.size(); ++i)
        outFile << danhSachPhong[i].getMaPhong() << "," << danhSachPhong[i].getTenPhong() << "," << danhSachPhong[i].getLoaiPhong() << "," << danhSachPhong[i].getSucChua() << "," << danhSachPhong[i].getTrangThaiPhong() << "\n";
    outFile << "\n[SEATS]\nmaGhe,maPhong,loaiGhe,donGiaGhe\n";
    for (size_t i = 0; i < danhSachGhe.size(); ++i)
        outFile << danhSachGhe[i].getMaGhe() << "," << danhSachGhe[i].getMaPhong() << "," << danhSachGhe[i].getLoaiGhe() << "," << danhSachGhe[i].getDonGiaGhe() << "\n";
    outFile.close();
}

void Database::taoGheTuDongChoPhong(const string& maPhongCanTao, int sucChuaPhong) {
    vector<Room> danhSachPhong; vector<Seat> danhSachGhe;
    docDanhSachPhong(danhSachPhong); docDanhSachGhe(danhSachGhe);
    char hang = 'A'; int soThuTu = 1;
    for (int i = 1; i <= sucChuaPhong; ++i) {
        stringstream ss; ss << hang << soThuTu;
        string loaiGhe = (hang <= 'B') ? "Standard" : "VIP";
        danhSachGhe.push_back(Seat(ss.str(), maPhongCanTao, loaiGhe, (loaiGhe == "VIP") ? 90000 : 60000));
        if (++soThuTu > 10) { soThuTu = 1; hang++; }
    }
    ghiDanhSachGhe(danhSachGhe); // Tái s? d?ng hàm ghi có s?n d? code g?n hon
}

void Database::docDanhSachPhim(vector<Movie>& ds) {
    ds.clear(); ifstream inFile("DATA/quan_ly_phim.csv");
    if (!inFile.is_open()) return; string line; bool reading = false;
    while (getline(inFile, line)) {
        if (line.find("[MOVIES]") != string::npos) { reading = true; continue; }
        if (reading && line[0] == '[') { reading = false; break; }
        if (reading && line.find("maPhim") == string::npos && !line.empty()) {
            vector<string> p = tachChuoiCSV(line);
            if (p.size() >= 8) ds.push_back(Movie(p[0], p[1], atoi(p[2].c_str()), atoi(p[3].c_str()), p[4], p[5], p[6], p[7]));
        }
    }
    inFile.close();
}

void Database::ghiDanhSachPhim(const vector<Movie>& danhSachPhim) {
    vector<pair<string, string> > dsTheLoai; vector<pair<string, string> > movieGenres;
    docDanhSachTheLoai(dsTheLoai); docMovieGenres(movieGenres);
    ofstream outFile("DATA/quan_ly_phim.csv", ios::trunc);
    if (!outFile.is_open()) return;
    outFile << "[GENRES]\nmaTheLoai,tenTheLoai\n";
    for (size_t i = 0; i < dsTheLoai.size(); ++i) outFile << dsTheLoai[i].first << "," << dsTheLoai[i].second << "\n";
    outFile << "\n[MOVIE_GENRES]\nmaPhim,maTheLoai\n";
    for (size_t i = 0; i < movieGenres.size(); ++i) outFile << movieGenres[i].first << "," << movieGenres[i].second << "\n";
    outFile << "\n[MOVIES]\nmaPhim,tenPhim,thoiLuong,nhanDoTuoi,ngayKhoiChieu,ngayKetThuc,moTa,poster\n";
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
void Database::docDanhSachSuatChieu(vector<Showtime>& ds) {
    ds.clear(); ifstream inFile("DATA/lich_va_ve.csv");
    if (!inFile.is_open()) return; string line; bool reading = false;
    while (getline(inFile, line)) {
        if (line.empty()) continue;
        if (line[0] == '[') { reading = (line.find("[SHOWTIMES]") != string::npos); continue; }
        if (reading && line.find("maSuatChieu") == string::npos) {
            vector<string> p = tachChuoiCSV(line);
            if (p.size() >= 7) ds.push_back(Showtime(p[0], p[1], p[2], p[3], p[4], p[5], atof(p[6].c_str())));
        }
    }
    inFile.close();
}

void Database::ghiDanhSachSuatChieu(const vector<Showtime>& danhSachSuatChieu) {
    vector<string> ticketLines, seatStatusLines;
    ifstream inFile("DATA/lich_va_ve.csv");
    if (inFile.is_open()) {
        string line; int sec = 0;
        while (getline(inFile, line)) {
            if (line.empty()) continue;
            if (line[0] == '[') {
                if (line.find("[TICKETS]") != string::npos) sec = 2;
                else if (line.find("[SEAT_STATUS]") != string::npos) sec = 3;
                else sec = 0;
                continue;
            }
            if (sec == 2 && line.find("maVe") == string::npos) ticketLines.push_back(line);
            else if (sec == 3 && line.find("maSuatChieu") == string::npos) seatStatusLines.push_back(line);
        }
        inFile.close();
    }

    ofstream outFile("DATA/lich_va_ve.csv", ios::trunc);
    if (!outFile.is_open()) return;
    outFile << "[SHOWTIMES]\nmaSuatChieu,maPhim,maPhong,ngayChieu,gioBatDau,gioKetThuc,giaVeCoBan\n";
    for (size_t i = 0; i < danhSachSuatChieu.size(); ++i) {
        outFile << danhSachSuatChieu[i].getMaSuatChieu() << "," << danhSachSuatChieu[i].getMaPhim() << "," << danhSachSuatChieu[i].getMaPhong() << ","  
                << danhSachSuatChieu[i].getNgayChieu() << "," << danhSachSuatChieu[i].getGioBatDau() << "," << danhSachSuatChieu[i].getGioKetThuc() << ","  
                << danhSachSuatChieu[i].getGiaVeCoBan() << "\n";
    }
    outFile << "\n[TICKETS]\nmaVe,maHoaDon,maSuatChieu,maGhe,giaVeThucTe,maQRCode,trangThaiSuDung\n";
    for (size_t i = 0; i < ticketLines.size(); ++i) outFile << ticketLines[i] << "\n";
    outFile << "\n[SEAT_STATUS]\nmaSuatChieu,maGhe,trangThai\n";
    for (size_t i = 0; i < seatStatusLines.size(); ++i) outFile << seatStatusLines[i] << "\n";
    outFile.close();
}

void Database::docTrangThaiGheSuatChieu(const string& maSC, vector<pair<string, string> >& list) {
    list.clear(); ifstream inFile("DATA/lich_va_ve.csv");
    if (!inFile.is_open()) return; string line; bool reading = false;
    while (getline(inFile, line)) {
        if (line.empty()) continue;
        if (line[0] == '[') { reading = (line.find("[SEAT_STATUS]") != string::npos); continue; }
        if (reading && line.find("maSuatChieu") == string::npos) {
            vector<string> p = tachChuoiCSV(line);
            if (p.size() >= 3 && p[0] == maSC) list.push_back(make_pair(p[1], p[2]));
        }
    }
    inFile.close();
}

void Database::capNhatTrangThaiGheSuatChieu(const string& maSC, const string& maGhe, const string& trangThaiMoi) {
    ifstream inFile("DATA/lich_va_ve.csv");
    vector<string> lines; string line; bool inSec = false, found = false;
    while (getline(inFile, line)) {
        if (line.empty()) continue;
        if (line[0] == '[') { inSec = (line.find("[SEAT_STATUS]") != string::npos); lines.push_back(line); continue; }
        if (inSec && line.find("maSuatChieu") == string::npos) {
            vector<string> p = tachChuoiCSV(line);
            if (p.size() >= 3 && p[0] == maSC && p[1] == maGhe) {
                lines.push_back(maSC + "," + maGhe + "," + trangThaiMoi);
                found = true; continue;
            }
        }
        lines.push_back(line);
    }
    inFile.close();

    if (!found) {
        bool hasSec = false; vector<string> newLines;
        for (size_t i = 0; i < lines.size(); ++i) {
            newLines.push_back(lines[i]);
            if (lines[i].find("[SEAT_STATUS]") != string::npos) hasSec = true;
        }
        if (!hasSec) { newLines.push_back("\n[SEAT_STATUS]"); newLines.push_back("maSuatChieu,maGhe,trangThai"); }
        newLines.push_back(maSC + "," + maGhe + "," + trangThaiMoi);
        lines = newLines;
    }

    ofstream outFile("DATA/lich_va_ve.csv", ios::trunc);
    if (!outFile.is_open()) return;
    for (size_t i = 0; i < lines.size(); ++i) outFile << lines[i] << "\n";
    outFile.close();
}

void Database::docDanhSachVe(vector<Ticket>& ds) {
    ds.clear(); ifstream inFile("DATA/lich_va_ve.csv");
    if (!inFile.is_open()) return; string line; bool reading = false;
    while (getline(inFile, line)) {
        if (line.empty()) continue;
        if (line[0] == '[') { reading = (line.find("[TICKETS]") != string::npos); continue; }
        if (reading && line.find("maVe") == string::npos) {
            vector<string> p = tachChuoiCSV(line);
            if (p.size() >= 7) ds.push_back(Ticket(p[0], p[1], p[2], p[3], atof(p[4].c_str()), p[5], p[6]));
        }
    }
    inFile.close();
}

void Database::ghiDanhSachVe(const vector<Ticket>& danhSachVe) {
    vector<string> showtimeLines, seatStatusLines;
    ifstream inFile("DATA/lich_va_ve.csv");
    if (inFile.is_open()) {
        string line; int sec = 0;
        while (getline(inFile, line)) {
            if (line.empty()) continue;
            if (line[0] == '[') {
                if (line.find("[SHOWTIMES]") != string::npos) sec = 1;
                else if (line.find("[SEAT_STATUS]") != string::npos) sec = 3;
                else sec = 0;
                continue;
            }
            if (sec == 1 && line.find("maSuatChieu") == string::npos) showtimeLines.push_back(line);
            else if (sec == 3 && line.find("maSuatChieu") == string::npos) seatStatusLines.push_back(line);
        }
        inFile.close();
    }

    ofstream outFile("DATA/lich_va_ve.csv", ios::trunc);
    if (!outFile.is_open()) return;
    outFile << "[SHOWTIMES]\nmaSuatChieu,maPhim,maPhong,ngayChieu,gioBatDau,gioKetThuc,giaVeCoBan\n";
    for (size_t i = 0; i < showtimeLines.size(); ++i) outFile << showtimeLines[i] << "\n";
    
    outFile << "\n[TICKETS]\nmaVe,maHoaDon,maSuatChieu,maGhe,giaVeThucTe,maQRCode,trangThaiSuDung\n";
    for (size_t i = 0; i < danhSachVe.size(); ++i) {
        outFile << danhSachVe[i].getMaVe() << "," << danhSachVe[i].getMaHoaDon() << "," << danhSachVe[i].getMaSuatChieu() << ","  
                << danhSachVe[i].getMaGhe() << "," << danhSachVe[i].getGiaVeThucTe() << "," << danhSachVe[i].getMaQRCode() << ","  
                << danhSachVe[i].getTrangThaiSuDung() << "\n";
    }
    
    outFile << "\n[SEAT_STATUS]\nmaSuatChieu,maGhe,trangThai\n";
    for (size_t i = 0; i < seatStatusLines.size(); ++i) outFile << seatStatusLines[i] << "\n";
    outFile.close();
}

// =========================================================================
// 4. DOANH THU, F&B & KHUY?N MÃI (DATA/giao_dich_fnb.csv)
// =========================================================================
// Hàm h? tr? ghi chung file giao_dich_fnb.csv d? tránh l?p code gi?a F&B, Khuy?n mãi, Hóa don
static void ghiFileGiaoDichFNB(const vector<FoodAndBeverage>& fnb, const vector<Promotion>& km, const vector<Invoice>& hd) {
    ofstream outFile("DATA/giao_dich_fnb.csv", ios::trunc);
    if (!outFile.is_open()) return;

    outFile << "[FOOD_AND_BEVERAGES]\nmaMatHang,tenMatHang,loai,giaTien,trangThaiKinhDoanh\n";
    for (size_t i = 0; i < fnb.size(); ++i)
        outFile << fnb[i].getMaMatHang() << "," << fnb[i].getTenMatHang() << "," << fnb[i].getLoai() << "," << fnb[i].getGiaTien() << "," << fnb[i].getTrangThaiKinhDoanh() << "\n";

    outFile << "\n[PROMOTIONS]\nmaKhuyenMai,tenKhuyenMai,maCode,phanTramGiam,mucGiamToiDa,ngayBatDau,ngayKetThuc,phamViApDung\n";
    for (size_t i = 0; i < km.size(); ++i)
        outFile << km[i].getMaKhuyenMai() << "," << km[i].getTenKhuyenMai() << "," << km[i].getMaCode() << "," << km[i].getPhanTramGiam() << "," << km[i].getMucGiamToiDa() << "," << km[i].getNgayBatDau() << "," << km[i].getNgayKetThuc() << "," << km[i].getPhamViApDung() << "\n";

    outFile << "\n[INVOICES]\nmaHoaDon,maKhachHang,maNhanVien,thoiGianLap,tongTien,hinhThucThanhToan,maGiaoDich,trangThaiThanhToan\n";
    for (size_t i = 0; i < hd.size(); ++i)
        outFile << hd[i].getMaHoaDon() << "," << hd[i].getMaKhachHang() << "," << hd[i].getMaNhanVien() << "," << hd[i].getThoiGianLap() << "," << hd[i].getTongTien() << "," << hd[i].getHinhThucThanhToan() << "," << hd[i].getMaGiaoDich() << "," << hd[i].getTrangThaiThanhToan() << "\n";
    outFile.close();
}

void Database::docDanhSachFAndB(vector<FoodAndBeverage>& ds) {
    ds.clear(); ifstream inFile("DATA/giao_dich_fnb.csv");
    if (!inFile.is_open()) return; string line; bool reading = false;
    while (getline(inFile, line)) {
        if (line.find("[FOOD_AND_BEVERAGES]") != string::npos) { reading = true; continue; }
        if (reading && line[0] == '[') { reading = false; break; }
        if (reading && line.find("maMatHang") == string::npos && !line.empty()) {
            vector<string> p = tachChuoiCSV(line);
            if (p.size() >= 5) ds.push_back(FoodAndBeverage(p[0], p[1], p[2], atof(p[3].c_str()), p[4]));
        }
    }
    inFile.close();
}

void Database::ghiDanhSachFAndB(const vector<FoodAndBeverage>& fnb) {
    vector<Promotion> km; vector<Invoice> hd; docDanhSachKhuyenMai(km); docDanhSachHoaDon(hd);
    ghiFileGiaoDichFNB(fnb, km, hd);
}

void Database::docDanhSachKhuyenMai(vector<Promotion>& ds) {
    ds.clear(); ifstream inFile("DATA/giao_dich_fnb.csv");
    if (!inFile.is_open()) return; string line; bool reading = false;
    while (getline(inFile, line)) {
        if (line.find("[PROMOTIONS]") != string::npos) { reading = true; continue; }
        if (reading && line[0] == '[') { reading = false; break; }
        if (reading && line.find("maKhuyenMai") == string::npos && !line.empty()) {
            vector<string> p = tachChuoiCSV(line);
            if (p.size() >= 8) ds.push_back(Promotion(p[0], p[1], p[2], atof(p[3].c_str()), atof(p[4].c_str()), p[5], p[6], p[7]));
        }
    }
    inFile.close();
}

void Database::ghiDanhSachKhuyenMai(const vector<Promotion>& km) {
    vector<FoodAndBeverage> fnb; vector<Invoice> hd; docDanhSachFAndB(fnb); docDanhSachHoaDon(hd);
    ghiFileGiaoDichFNB(fnb, km, hd);
}

void Database::docDanhSachHoaDon(vector<Invoice>& ds) {
    ds.clear(); ifstream inFile("DATA/giao_dich_fnb.csv");
    if (!inFile.is_open()) return; string line; bool reading = false;
    while (getline(inFile, line)) {
        if (line.find("[INVOICES]") != string::npos) { reading = true; continue; }
        if (reading && line[0] == '[') { reading = false; break; }
        if (reading && line.find("maHoaDon") == string::npos && !line.empty()) {
            vector<string> p = tachChuoiCSV(line);
            if (p.size() >= 8) ds.push_back(Invoice(p[0], p[1], p[2], p[3], atof(p[4].c_str()), p[5], p[6], p[7]));
        }
    }
    inFile.close();
}

void Database::ghiDanhSachHoaDon(const vector<Invoice>& hd) {
    vector<FoodAndBeverage> fnb; vector<Promotion> km; docDanhSachFAndB(fnb); docDanhSachKhuyenMai(km);
    ghiFileGiaoDichFNB(fnb, km, hd);
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
        if (reading && line[0] == '[') break;

        if (reading && line.find("maCa") == string::npos) {
            vector<string> p = tachChuoiCSV(line);
            if (p.size() >= 11) {
                danhSachCa.push_back(ShiftReport(
                    p[0], p[1], p[2], p[3], p[4], p[5],
                    atoll(p[6].c_str()),
                    atoll(p[7].c_str()),
                    p[9],
                    (p[10] == "true" || p[10] == "1")
                ));
            }
        }
    }
    inFile.close();
}

void Database::ghiDanhSachCa(const vector<ShiftReport>& danhSachCa) {
    ofstream outFile("DATA/van_hanh_he_thong.csv", ios::trunc);
    if (!outFile.is_open()) return;

    outFile << "[SHIFT_MANAGEMENTS]\nmaCa,maNhanVien,tenNhanVien,ngayLam,gioBatDau,gioKetThuc,doanhThuHeThong,tienThucTe,chenhLech,trangThai,daKhoaBaoCao\n";
    for (size_t i = 0; i < danhSachCa.size(); ++i) {
        const ShiftReport& sr = danhSachCa[i];
        outFile << sr.getMaCa() << ","
                << sr.getMaNhanVien() << ","
                << sr.getTenNhanVien() << ","
                << sr.getNgayLam() << ","
                << sr.getGioBatDau() << ","
                << sr.getGioKetThuc() << ","
                << sr.getDoanhThuHeThong() << ","
                << sr.getTienThucTe() << ","
                << sr.getChenhLech() << ","
                << sr.getTrangThai() << ","
                << (sr.getDaKhoaBaoCao() ? "true" : "false")
                << "\n";
    }
    outFile.close();
}
