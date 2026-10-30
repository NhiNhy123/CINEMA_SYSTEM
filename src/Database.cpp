#include "../include/Database.h"
#include "../include/Seat.h"
#include "../include/FAndB.h"
#include "../include/Promotion.h"
#include "../include/Invoice.h"
#include "../include/Room.h"
#include "../include/Movie.h"
#include "../include/Showtime.h"
#include "../include/Ticket.h"
#include "../include/ShilftManagement.h"
#include "../include/User.h"
#include "../include/DateFormat.h"

#include <fstream>
#include <sstream>
#include <iostream>
#include <cstdlib>
#include <utility>
#include <ctime>
#include <iomanip>
#include <limits>
#include <map>
#include <set>
#include <windows.h>

using namespace std;

namespace {
bool docToanBoFile(const string& path, string& content) {
    ifstream input(path.c_str(), ios::binary);
    if (!input.is_open()) return false;
    ostringstream buffer;
    buffer << input.rdbuf();
    if (input.bad()) return false;
    content = buffer.str();
    return true;
}

bool ghiToanBoFile(const string& path, const string& content) {
    const string tempPath = path + ".tmp";
    ofstream output(tempPath.c_str(), ios::binary | ios::trunc);
    if (!output.is_open()) return false;
    output.write(content.data(), static_cast<streamsize>(content.size()));
    output.close();
    if (!output) {
        DeleteFileA(tempPath.c_str());
        return false;
    }

    if (!MoveFileExA(tempPath.c_str(), path.c_str(),
                     MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH)) {
        DeleteFileA(tempPath.c_str());
        return false;
    }
    return true;
}

bool tachDongCSV(const string& line, vector<string>& fields) {
    fields.clear();
    string field;
    bool inQuotes = false;
    bool closedQuote = false;
    for (size_t i = 0; i < line.size(); ++i) {
        const char ch = line[i];
        if (inQuotes) {
            if (ch == '"') {
                if (i + 1 < line.size() && line[i + 1] == '"') {
                    field += '"';
                    ++i;
                } else {
                    inQuotes = false;
                    closedQuote = true;
                }
            } else {
                field += ch;
            }
        } else if (ch == ',') {
            fields.push_back(field);
            field.clear();
            closedQuote = false;
        } else if (ch == '"' && field.empty() && !closedQuote) {
            inQuotes = true;
        } else if (closedQuote) {
            return false;
        } else {
            field += ch;
        }
    }
    if (inQuotes) return false;
    if (!field.empty() && field[field.size() - 1] == '\r') field.erase(field.size() - 1);
    fields.push_back(field);
    return true;
}

string quoteCSVField(const string& value) {
    string field;
    bool needsQuotes = false;
    for (size_t i = 0; i < value.size(); ++i) {
        const char ch = value[i];
        if (ch == ',' || ch == '"') needsQuotes = true;
        if (ch == '\r' || ch == '\n') {
            field += ' ';
        } else {
            if (ch == '"') field += '"';
            field += ch;
        }
    }
    return needsQuotes ? "\"" + field + "\"" : field;
}

string serializeCSVFields(const vector<string>& fields) {
    ostringstream output;
    for (size_t i = 0; i < fields.size(); ++i) {
        if (i > 0) output << ",";
        output << quoteCSVField(fields[i]);
    }
    return output.str();
}

bool parsePositiveInteger(const string& value, int& result) {
    stringstream stream(value);
    if (!(stream >> result)) return false;
    stream >> ws;
    return stream.eof() && result > 0;
}

bool parseNonNegativeDouble(const string& value, double& result) {
    stringstream stream(value);
    if (!(stream >> result)) return false;
    stream >> ws;
    return stream.eof() &&
           result >= 0.0 && result <= numeric_limits<double>::max();
}
}

Database::Database() : giaoDichBanVeDangMo(false) {}

bool Database::batDauGiaoDichBanVe() {
    if (giaoDichBanVeDangMo ||
        !docToanBoFile("DATA/lich_va_ve.csv", banSaoLichVaVe) ||
        !docToanBoFile("DATA/giao_dich_fnb.csv", banSaoGiaoDichFnb) ||
        !docToanBoFile("DATA/quan_ly_nhan_su.csv", banSaoNhanSu)) {
        cerr << "[LOI]: Khong the tao ban sao du lieu de bat dau giao dich ban ve.\n";
        return false;
    }
    giaoDichBanVeDangMo = true;
    return true;
}

bool Database::khoiPhucGiaoDichBanVe() {
    if (!giaoDichBanVeDangMo) return false;

    const bool restoredLichVaVe = ghiToanBoFile("DATA/lich_va_ve.csv", banSaoLichVaVe);
    const bool restoredFnb = ghiToanBoFile("DATA/giao_dich_fnb.csv", banSaoGiaoDichFnb);
    const bool restoredNhanSu = ghiToanBoFile("DATA/quan_ly_nhan_su.csv", banSaoNhanSu);
    if (!restoredLichVaVe || !restoredFnb || !restoredNhanSu) {
        cerr << "[LOI NGHIEM TRONG]: Khong the khoi phuc day du du lieu giao dich ban ve.\n";
        return false;
    }

    hoanTatGiaoDichBanVe();
    return true;
}

void Database::hoanTatGiaoDichBanVe() {
    giaoDichBanVeDangMo = false;
    banSaoLichVaVe.clear();
    banSaoGiaoDichFnb.clear();
    banSaoNhanSu.clear();
}

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

static bool phanTichGiuGhe(const string& trangThai, time_t& hetHan, string& maPhienGiu) {
    const string tienTo = "DangChon|";
    if (trangThai.find(tienTo) != 0) return false;
    const size_t dauPhanCach = trangThai.find('|', tienTo.length());
    if (dauPhanCach == string::npos) return false;

    stringstream ss(trangThai.substr(tienTo.length(), dauPhanCach - tienTo.length()));
    long long mocHetHan = 0;
    if (!(ss >> mocHetHan) || mocHetHan <= 0) return false;
    hetHan = static_cast<time_t>(mocHetHan);
    maPhienGiu = trangThai.substr(dauPhanCach + 1);
    return !maPhienGiu.empty();
}

static bool giuGheConHan(const string& trangThai, time_t hienTai, string* maPhienGiu = NULL) {
    time_t hetHan;
    string maPhien;
    if (!phanTichGiuGhe(trangThai, hetHan, maPhien) || hetHan <= hienTai) return false;
    if (maPhienGiu != NULL) *maPhienGiu = maPhien;
    return true;
}

void Database::docDanhSachUsers(vector<Customer>& danhSachKH) {
    danhSachKH.clear();
    ifstream inFile("DATA/quan_ly_nhan_su.csv");
    if (!inFile.is_open()) return;

    string line;
    int section = 0;
    map<string, vector<string> > usersById;
    set<string> customerIds;

    while (getline(inFile, line)) {
        if (line.empty()) continue;
        if (line[0] == '[') {
            if (line.find("[USERS]") == 0) section = 1;
            else if (line.find("[CUSTOMERS]") == 0) section = 2;
            else section = 0;
            continue;
        }

        vector<string> fields;
        if (!tachDongCSV(line, fields)) {
            cerr << "[CANH BAO]: Bo qua dong CSV khong hop le trong du lieu CRM.\n";
            continue;
        }

        if (section == 1 && !fields.empty() && fields[0] != "maNguoiDung" &&
            fields.size() >= 1 && !fields[0].empty()) {
            if (usersById.find(fields[0]) != usersById.end()) {
                cerr << "[CANH BAO]: Ma nguoi dung bi trung trong CSV: "
                     << fields[0] << ". Su dung ho so xuat hien dau tien.\n";
                continue;
            }
            usersById[fields[0]] = fields;
        } else if (section == 2 && fields.size() >= 4 &&
                   fields[0] != "maKhachHang" && !fields[0].empty()) {
            const string& maKH = fields[0];
            if (!customerIds.insert(maKH).second) {
                cerr << "[CANH BAO]: Ma khach hang bi trung trong CSV: "
                     << maKH << ". Giu nguyen ban ghi de tranh mat du lieu.\n";
            }

            const map<string, vector<string> >::const_iterator user = usersById.find(maKH);
            const vector<string>* profile = user == usersById.end() ? NULL : &user->second;
            const string hoTen = profile != NULL && profile->size() > 1 ? (*profile)[1] : "";
            const string soDienThoai = profile != NULL && profile->size() > 2 ? (*profile)[2] : "";
            const string email = profile != NULL && profile->size() > 3 ? (*profile)[3] : "";
            const string ngaySinh = profile != NULL && profile->size() > 4 ? (*profile)[4] : "";
            const string gioiTinh = profile != NULL && profile->size() > 5 ? (*profile)[5] : "";
            const string cccd = profile != NULL && profile->size() > 6 ? (*profile)[6] : "";

            Customer customer(maKH, hoTen, soDienThoai, email,
                              atoi(fields[1].c_str()), fields[2], atoll(fields[3].c_str()));
            customer.setNgaySinh(ngaySinh);
            customer.setGioiTinh(gioiTinh);
            customer.setCCCD(cccd);
            danhSachKH.push_back(customer);
        }
    }
    if (inFile.bad()) cerr << "[LOI]: Khong doc duoc day du du lieu CRM.\n";
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

                if (p[0] == maNhanVien || p[7] == maNhanVien) {
                    inFile.close();
                    return p[1];
                }
            }
        }
    }
    inFile.close();
    return "Nhan vien chua ro";
}

bool Database::ghiDanhSachUsers(const vector<Customer>& danhSachKH) {
    string originalContent;
    if (!docToanBoFile("DATA/quan_ly_nhan_su.csv", originalContent) ||
        originalContent.find("[USERS]") == string::npos ||
        originalContent.find("[CUSTOMERS]") == string::npos ||
        originalContent.find("[CUSTOMER_POINTS_HISTORY]") == string::npos) {
        return false;
    }

    ifstream inFile("DATA/quan_ly_nhan_su.csv");
    if (!inFile.is_open()) return false;
    vector<string> rawUsersLines, historyLines;
    string currentSection = "", line;

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
    if (inFile.bad()) return false;
    inFile.close();

    map<string, const Customer*> customersById;
    for (size_t i = 0; i < danhSachKH.size(); ++i) {
        const string maKH = danhSachKH[i].getMaKH();
        if (maKH.empty()) {
            cerr << "[LOI]: Ma khach hang rong; khong ghi du lieu CRM.\n";
            return false;
        }
        if (!customersById.insert(make_pair(maKH, &danhSachKH[i])).second)
            cerr << "[CANH BAO]: Ma khach hang bi trung khi ghi CRM: "
                 << maKH << ". Giu nguyen cac ban ghi thanh vien.\n";
    }

    vector<string> finalCustomersLines;
    for (size_t i = 0; i < danhSachKH.size(); ++i) {
        vector<string> customerFields;
        stringstream ssDiem, ssChiTieu;
        ssDiem << danhSachKH[i].getDiemTichLuy();
        ssChiTieu << danhSachKH[i].getTongChiTieu();
        customerFields.push_back(danhSachKH[i].getMaKH());
        customerFields.push_back(ssDiem.str());
        customerFields.push_back(danhSachKH[i].getHangThanhVien());
        customerFields.push_back(ssChiTieu.str());
        finalCustomersLines.push_back(serializeCSVFields(customerFields));
    }

    set<string> matchedUserIds;
    set<string> existingUserIds;
    for (size_t i = 0; i < rawUsersLines.size(); ++i) {
        vector<string> userFields;
        if (!tachDongCSV(rawUsersLines[i], userFields) || userFields.empty()) {
            cerr << "[LOI]: Dong USERS khong hop le; khong ghi de du lieu CRM.\n";
            return false;
        }
        if (userFields[0].empty() || !existingUserIds.insert(userFields[0]).second) {
            cerr << "[LOI]: Ma nguoi dung rong hoac bi trung; khong ghi de du lieu CRM.\n";
            return false;
        }

        const map<string, const Customer*>::const_iterator customer = customersById.find(userFields[0]);
        if (customer == customersById.end()) continue;
        if (userFields.size() > 9 && !userFields[9].empty() &&
            userFields[9] != "Customer") {
            cerr << "[LOI]: Ma khach hang " << userFields[0]
                 << " dang thuoc ve tai khoan khong phai Customer.\n";
            return false;
        }

        const Customer& profile = *customer->second;
        if (userFields.size() < 7) userFields.resize(7);
        userFields[1] = profile.getHoTen();
        userFields[2] = profile.getSoDienThoai();
        userFields[3] = profile.getEmail();
        userFields[4] = profile.getNgaySinh();
        userFields[5] = profile.getGioiTinh();
        userFields[6] = profile.getCCCD();
        if (userFields.size() < 12) userFields.resize(12);
        if (userFields[9].empty()) userFields[9] = "Customer";
        if (userFields[10].empty()) userFields[10] = "DangLam";
        if (userFields[11].empty()) userFields[11] = "HoatDong";
        rawUsersLines[i] = serializeCSVFields(userFields);
        matchedUserIds.insert(userFields[0]);
    }

    for (map<string, const Customer*>::const_iterator it = customersById.begin();
         it != customersById.end(); ++it) {
        if (matchedUserIds.find(it->first) != matchedUserIds.end()) continue;
        const Customer& profile = *it->second;
        vector<string> userFields;
        userFields.push_back(profile.getMaKH());
        userFields.push_back(profile.getHoTen());
        userFields.push_back(profile.getSoDienThoai());
        userFields.push_back(profile.getEmail());
        userFields.push_back(profile.getNgaySinh());
        userFields.push_back(profile.getGioiTinh());
        userFields.push_back(profile.getCCCD());
        userFields.push_back("khach_" + profile.getSoDienThoai());
        userFields.push_back("123456");
        userFields.push_back("Customer");
        userFields.push_back("DangLam");
        userFields.push_back("HoatDong");
        rawUsersLines.push_back(serializeCSVFields(userFields));
    }

    ostringstream output;
    output << "[USERS]\nmaNguoiDung,hoTen,soDienThoai,email,ngaySinh,gioiTinh,cccd,taiKhoan,matKhau,vaiTro,trangThaiLamViec,trangThaiTaiKhoan\n";
    for (size_t i = 0; i < rawUsersLines.size(); ++i) output << rawUsersLines[i] << "\n";

    output << "\n[CUSTOMERS]\nmaKhachHang,diemTichLuy,hangThanhVien,tongChiTieu\n";
    for (size_t i = 0; i < finalCustomersLines.size(); ++i) output << finalCustomersLines[i] << "\n";

    output << "\n[CUSTOMER_POINTS_HISTORY]\nmaGiaoDichDiem,maKhachHang,loaiGiaoDich,soDiem,noiDung,thoiGian\n";
    for (size_t i = 0; i < historyLines.size(); ++i) output << historyLines[i] << "\n";
    return ghiToanBoFile("DATA/quan_ly_nhan_su.csv", output.str());
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

bool Database::ghiLichSuGiaoDichDiem(const vector<GiaoDichDiem>& danhSachGD) {
    string originalContent;
    if (!docToanBoFile("DATA/quan_ly_nhan_su.csv", originalContent) ||
        originalContent.find("[USERS]") == string::npos ||
        originalContent.find("[CUSTOMERS]") == string::npos ||
        originalContent.find("[CUSTOMER_POINTS_HISTORY]") == string::npos) {
        return false;
    }

    ifstream inFile("DATA/quan_ly_nhan_su.csv");
    if (!inFile.is_open()) return false;
    vector<string> rawUsersLines, customersLines;
    string currentSection = "", line;

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
    if (inFile.bad()) return false;
    inFile.close();

    ostringstream output;
    output << "[USERS]\nmaNguoiDung,hoTen,soDienThoai,email,ngaySinh,gioiTinh,cccd,taiKhoan,matKhau,vaiTro,trangThaiLamViec,trangThaiTaiKhoan\n";
    for (size_t i = 0; i < rawUsersLines.size(); ++i) output << rawUsersLines[i] << "\n";

    output << "\n[CUSTOMERS]\nmaKhachHang,diemTichLuy,hangThanhVien,tongChiTieu\n";
    for (size_t i = 0; i < customersLines.size(); ++i) output << customersLines[i] << "\n";

    output << "\n[CUSTOMER_POINTS_HISTORY]\nmaGiaoDichDiem,maKhachHang,loaiGiaoDich,soDiem,noiDung,thoiGian\n";
    for (size_t i = 0; i < danhSachGD.size(); ++i) {
        output << danhSachGD[i].getMaGiaoDich() << "," << danhSachGD[i].getMaKhachHang() << "," << danhSachGD[i].getLoaiGiaoDich() << ","
               << danhSachGD[i].getSoDiem() << "," << danhSachGD[i].getNoiDung() << "," << danhSachGD[i].getThoiGian() << "\n";
    }
    return ghiToanBoFile("DATA/quan_ly_nhan_su.csv", output.str());
}

static bool ghiDuLieuPhim(const vector<pair<string, string> >& dsTheLoai,
                          const vector<pair<string, string> >& movieGenres,
                          const vector<Movie>& danhSachPhim) {
    ofstream outFile("DATA/quan_ly_phim.csv", ios::trunc);
    if (!outFile.is_open()) return false;
    outFile << "[GENRES]\nmaTheLoai,tenTheLoai\n";
    for (size_t i = 0; i < dsTheLoai.size(); ++i) {
        outFile << dsTheLoai[i].first << "," << dsTheLoai[i].second << "\n";
    }
    outFile << "\n[MOVIE_GENRES]\nmaPhim,maTheLoai\n";
    for (size_t i = 0; i < movieGenres.size(); ++i) {
        outFile << movieGenres[i].first << "," << movieGenres[i].second << "\n";
    }
    outFile << "\n[MOVIES]\nmaPhim,tenPhim,thoiLuong,nhanDoTuoi,ngayKhoiChieu,ngayKetThuc,moTa,poster\n";
    for (size_t i = 0; i < danhSachPhim.size(); ++i) {
        outFile << danhSachPhim[i].getMaPhim() << "," << danhSachPhim[i].getTenPhim() << ","
                << danhSachPhim[i].getThoiLuong() << "," << danhSachPhim[i].getNhanDoTuoi() << ","
                << danhSachPhim[i].getNgayKhoiChieu() << "," << danhSachPhim[i].getNgayKetThuc() << ","
                << danhSachPhim[i].getMoTa() << "," << danhSachPhim[i].getPoster() << "\n";
    }
    return true;
}

void Database::docDanhSachTheLoai(vector<pair<string, string> >& ds) {
    ds.clear();
    ifstream inFile("DATA/quan_ly_phim.csv");
    if (!inFile.is_open()) return;
    string line; bool reading = false;
    while (getline(inFile, line)) {
        if (line.empty()) continue;
        if (line.find("[GENRES]") != string::npos) { reading = true; continue; }
        if (reading && line[0] == '[') { reading = false; break; }
        if (reading && line.find("maTheLoai") == string::npos && !line.empty()) {
            vector<string> p = tachChuoiCSV(line);
            if (p.size() >= 2) ds.push_back(make_pair(p[0], p[1]));
        }
    }
    inFile.close();
}

bool Database::themLoaiTheLoai(const string& tenTheLoai) {
    if (tenTheLoai.find_first_not_of(" \t") == string::npos ||
        tenTheLoai.find_first_of(",\r\n") != string::npos) return false;
    vector<pair<string, string> > dsTheLoai, movieGenres;
    vector<Movie> danhSachPhim;
    docDanhSachTheLoai(dsTheLoai);
    docMovieGenres(movieGenres);
    docDanhSachPhim(danhSachPhim);
    for (size_t i = 0; i < dsTheLoai.size(); ++i) {
        if (dsTheLoai[i].second == tenTheLoai) return false;
    }

    size_t nextId = 1;
    string maTheLoai;
    bool daTonTai;
    do {
        stringstream ss;
        ss << "TL0" << nextId++;
        maTheLoai = ss.str();
        daTonTai = false;
        for (size_t i = 0; i < dsTheLoai.size(); ++i) {
            if (dsTheLoai[i].first == maTheLoai) {
                daTonTai = true;
                break;
            }
        }
    } while (daTonTai);

    dsTheLoai.push_back(make_pair(maTheLoai, tenTheLoai));
    return ghiDuLieuPhim(dsTheLoai, movieGenres, danhSachPhim);
}

bool Database::suaLoaiTheLoai(const string& maTheLoai, const string& tenMoi) {
    if (tenMoi.find_first_not_of(" \t") == string::npos ||
        tenMoi.find_first_of(",\r\n") != string::npos) return false;
    vector<pair<string, string> > dsTheLoai, movieGenres;
    vector<Movie> danhSachPhim;
    docDanhSachTheLoai(dsTheLoai);
    docMovieGenres(movieGenres);
    docDanhSachPhim(danhSachPhim);

    size_t index = dsTheLoai.size();
    for (size_t i = 0; i < dsTheLoai.size(); ++i) {
        if (dsTheLoai[i].first == maTheLoai) index = i;
        else if (dsTheLoai[i].second == tenMoi) return false;
    }
    if (index == dsTheLoai.size()) return false;
    dsTheLoai[index].second = tenMoi;
    return ghiDuLieuPhim(dsTheLoai, movieGenres, danhSachPhim);
}

bool Database::xoaLoaiTheLoai(const string& maTheLoai) {
    vector<pair<string, string> > dsTheLoai, movieGenres;
    vector<Movie> danhSachPhim;
    docDanhSachTheLoai(dsTheLoai);
    docMovieGenres(movieGenres);
    docDanhSachPhim(danhSachPhim);

    for (size_t i = 0; i < movieGenres.size(); ++i) {
        if (movieGenres[i].second == maTheLoai) return false;
    }
    for (size_t i = 0; i < dsTheLoai.size(); ++i) {
        if (dsTheLoai[i].first == maTheLoai) {
            dsTheLoai.erase(dsTheLoai.begin() + i);
            return ghiDuLieuPhim(dsTheLoai, movieGenres, danhSachPhim);
        }
    }
    return false;
}

void Database::docMovieGenres(vector<pair<string, string> >& mg) {
    mg.clear();
    ifstream inFile("DATA/quan_ly_phim.csv");
    if (!inFile.is_open()) return;
    string line; bool reading = false;
    while (getline(inFile, line)) {
        if (line.empty()) continue;
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

static bool luuTheLoaiChoPhim(Database& db, const string& maPhim,
                              const string& tenTheLoai, bool thayThe) {
    if (tenTheLoai.find_first_not_of(" \t") == string::npos ||
        tenTheLoai.find_first_of(",\r\n") != string::npos) {
        return false;
    }

    vector<pair<string, string> > dsTheLoai;
    vector<pair<string, string> > movieGenres;
    vector<Movie> danhSachPhim;
    db.docDanhSachTheLoai(dsTheLoai);
    db.docMovieGenres(movieGenres);
    db.docDanhSachPhim(danhSachPhim);

    bool coPhim = false;
    for (size_t i = 0; i < danhSachPhim.size(); ++i) {
        if (danhSachPhim[i].getMaPhim() == maPhim) {
            coPhim = true;
            break;
        }
    }
    if (!coPhim) return false;

    string maTheLoai;
    for (size_t i = 0; i < dsTheLoai.size(); ++i) {
        if (dsTheLoai[i].second == tenTheLoai) {
            maTheLoai = dsTheLoai[i].first;
            break;
        }
    }

    if (maTheLoai.empty()) {
        size_t nextId = 1;
        bool daTonTai;
        do {
            stringstream ss;
            ss << "TL0" << nextId++;
            maTheLoai = ss.str();
            daTonTai = false;
            for (size_t i = 0; i < dsTheLoai.size(); ++i) {
                if (dsTheLoai[i].first == maTheLoai) {
                    daTonTai = true;
                    break;
                }
            }
        } while (daTonTai);
        dsTheLoai.push_back(make_pair(maTheLoai, tenTheLoai));
    }

    if (thayThe) {
        for (size_t i = 0; i < movieGenres.size();) {
            if (movieGenres[i].first == maPhim) {
                movieGenres.erase(movieGenres.begin() + i);
            } else {
                ++i;
            }
        }
    }

    for (size_t i = 0; i < movieGenres.size(); ++i) {
        if (movieGenres[i].first == maPhim && movieGenres[i].second == maTheLoai) {
            return ghiDuLieuPhim(dsTheLoai, movieGenres, danhSachPhim);
        }
    }
    movieGenres.push_back(make_pair(maPhim, maTheLoai));
    return ghiDuLieuPhim(dsTheLoai, movieGenres, danhSachPhim);
}

bool Database::themTheLoaiChoPhim(string maPhim, string tenTheLoaiMoi) {
    return luuTheLoaiChoPhim(*this, maPhim, tenTheLoaiMoi, false);
}

bool Database::capNhatTheLoaiChoPhim(string maPhim, string tenTheLoaiMoi) {
    return luuTheLoaiChoPhim(*this, maPhim, tenTheLoaiMoi, true);
}

static bool ghiDuLieuPhongGhe(const vector<Room>& danhSachPhong,
                              const vector<Seat>& danhSachGhe,
                              const vector<pair<string, double> >& danhSachLoaiGhe) {
    ofstream outFile("DATA/quan_ly_phong_ghe.csv", ios::trunc);
    if (!outFile.is_open()) return false;
    outFile << "[ROOMS]\nmaPhong,tenPhong,loaiPhong,sucChua,trangThaiPhong\n";
    for (size_t i = 0; i < danhSachPhong.size(); ++i) {
        outFile << danhSachPhong[i].getMaPhong() << "," << danhSachPhong[i].getTenPhong() << ","
                << danhSachPhong[i].getLoaiPhong() << "," << danhSachPhong[i].getSucChua() << ","
                << danhSachPhong[i].getTrangThaiPhong() << "\n";
    }
    outFile << "\n[SEATS]\nmaGhe,maPhong,loaiGhe,donGiaGhe\n";
    for (size_t i = 0; i < danhSachGhe.size(); ++i) {
        outFile << danhSachGhe[i].getMaGhe() << "," << danhSachGhe[i].getMaPhong() << ","
                << danhSachGhe[i].getLoaiGhe() << "," << danhSachGhe[i].getDonGiaGhe() << "\n";
    }
    outFile << "\n[SEAT_TYPES]\ntenLoaiGhe,donGia\n";
    for (size_t i = 0; i < danhSachLoaiGhe.size(); ++i) {
        outFile << danhSachLoaiGhe[i].first << "," << danhSachLoaiGhe[i].second << "\n";
    }
    return true;
}

void Database::docDanhSachPhong(vector<Room>& ds) {
    ds.clear(); ifstream inFile("DATA/quan_ly_phong_ghe.csv");
    if (!inFile.is_open()) return; string line; bool reading = false;
    while (getline(inFile, line)) {
        if (line.empty()) continue;
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
    vector<Seat> danhSachGhe;
    vector<pair<string, double> > danhSachLoaiGhe;
    docDanhSachGhe(danhSachGhe);
    docDanhSachLoaiGhe(danhSachLoaiGhe);
    if (!ghiDuLieuPhongGhe(danhSachPhong, danhSachGhe, danhSachLoaiGhe)) {
        cout << "Loi: Khong the luu danh sach phong va ghe!\n";
    }
}

void Database::docDanhSachGhe(vector<Seat>& ds) {
    ds.clear(); ifstream inFile("DATA/quan_ly_phong_ghe.csv");
    if (!inFile.is_open()) return; string line; bool reading = false;
    while (getline(inFile, line)) {
        if (line.empty()) continue;
        if (line.find("[SEATS]") != string::npos) { reading = true; continue; }
        if (reading && line[0] == '[') { reading = false; break; }
        if (reading && line.find("maGhe") == string::npos && !line.empty()) {
            vector<string> p = tachChuoiCSV(line);
            if (p.size() >= 4) ds.push_back(Seat(p[0], p[1], p[2], atof(p[3].c_str()), "Trong"));
        }
    }
    inFile.close();
}

void Database::ghiDanhSachGhe(const vector<Seat>& danhSachGhe) {
    vector<Room> danhSachPhong;
    vector<pair<string, double> > danhSachLoaiGhe;
    docDanhSachPhong(danhSachPhong);
    docDanhSachLoaiGhe(danhSachLoaiGhe);
    if (!ghiDuLieuPhongGhe(danhSachPhong, danhSachGhe, danhSachLoaiGhe)) {
        cout << "Loi: Khong the luu danh sach phong va ghe!\n";
    }
}

void Database::docDanhSachLoaiGhe(vector<pair<string, double> >& ds) {
    ds.clear();
    ifstream inFile("DATA/quan_ly_phong_ghe.csv");
    if (!inFile.is_open()) return;
    string line;
    bool reading = false;
    while (getline(inFile, line)) {
        if (line.find("[SEAT_TYPES]") != string::npos) {
            reading = true;
            continue;
        }
        if (!line.empty() && line[0] == '[') {
            reading = false;
            continue;
        }
        if (reading && !line.empty() && line.find("tenLoaiGhe") == string::npos) {
            vector<string> p = tachChuoiCSV(line);
            if (p.size() >= 2) ds.push_back(make_pair(p[0], atof(p[1].c_str())));
        }
    }
    inFile.close();
}

bool Database::themLoaiGhe(const string& tenLoaiGhe, double donGia) {
    if (tenLoaiGhe.find_first_not_of(" \t") == string::npos ||
        tenLoaiGhe.find_first_of(",\r\n") != string::npos || donGia <= 0) return false;
    vector<pair<string, double> > danhSachLoaiGhe;
    docDanhSachLoaiGhe(danhSachLoaiGhe);
    for (size_t i = 0; i < danhSachLoaiGhe.size(); ++i) {
        if (danhSachLoaiGhe[i].first == tenLoaiGhe) return false;
    }
    danhSachLoaiGhe.push_back(make_pair(tenLoaiGhe, donGia));

    vector<Room> danhSachPhong;
    vector<Seat> danhSachGhe;
    docDanhSachPhong(danhSachPhong);
    docDanhSachGhe(danhSachGhe);
    return ghiDuLieuPhongGhe(danhSachPhong, danhSachGhe, danhSachLoaiGhe);
}

bool Database::suaLoaiGhe(const string& tenLoaiGhe, const string& tenMoi, double donGiaMoi) {
    if (tenMoi.find_first_not_of(" \t") == string::npos ||
        tenMoi.find_first_of(",\r\n") != string::npos || donGiaMoi <= 0) return false;
    vector<pair<string, double> > danhSachLoaiGhe;
    vector<Room> danhSachPhong;
    vector<Seat> danhSachGhe;
    docDanhSachLoaiGhe(danhSachLoaiGhe);
    docDanhSachPhong(danhSachPhong);
    docDanhSachGhe(danhSachGhe);

    size_t index = danhSachLoaiGhe.size();
    for (size_t i = 0; i < danhSachLoaiGhe.size(); ++i) {
        if (danhSachLoaiGhe[i].first == tenLoaiGhe) index = i;
        else if (danhSachLoaiGhe[i].first == tenMoi) return false;
    }
    if (index == danhSachLoaiGhe.size()) return false;
    danhSachLoaiGhe[index] = make_pair(tenMoi, donGiaMoi);

    for (size_t i = 0; i < danhSachGhe.size(); ++i) {
        if (danhSachGhe[i].getLoaiGhe() == tenLoaiGhe) {
            danhSachGhe[i].setLoaiGhe(tenMoi);
            danhSachGhe[i].setDonGiaGhe(donGiaMoi);
        }
    }
    return ghiDuLieuPhongGhe(danhSachPhong, danhSachGhe, danhSachLoaiGhe);
}

bool Database::xoaLoaiGhe(const string& tenLoaiGhe) {
    vector<pair<string, double> > danhSachLoaiGhe;
    vector<Room> danhSachPhong;
    vector<Seat> danhSachGhe;
    docDanhSachLoaiGhe(danhSachLoaiGhe);
    docDanhSachPhong(danhSachPhong);
    docDanhSachGhe(danhSachGhe);
    for (size_t i = 0; i < danhSachGhe.size(); ++i) {
        if (danhSachGhe[i].getLoaiGhe() == tenLoaiGhe) return false;
    }
    for (size_t i = 0; i < danhSachLoaiGhe.size(); ++i) {
        if (danhSachLoaiGhe[i].first == tenLoaiGhe) {
            danhSachLoaiGhe.erase(danhSachLoaiGhe.begin() + i);
            return ghiDuLieuPhongGhe(danhSachPhong, danhSachGhe, danhSachLoaiGhe);
        }
    }
    return false;
}

void Database::taoGheTuDongChoPhong(const string& maPhongCanTao, int sucChuaPhong) {
    vector<Room> danhSachPhong; vector<Seat> danhSachGhe;
    vector<pair<string, double> > danhSachLoaiGhe;
    docDanhSachPhong(danhSachPhong); docDanhSachGhe(danhSachGhe);
    docDanhSachLoaiGhe(danhSachLoaiGhe);
    if (danhSachLoaiGhe.empty()) {
        cout << "Loi: Chua co loai ghe trong danh muc. Hay tao loai ghe truoc!\n";
        return;
    }

    string loaiChuan = danhSachLoaiGhe[0].first;
    string loaiVIP = loaiChuan;
    double giaChuan = danhSachLoaiGhe[0].second;
    double giaVIP = giaChuan;
    for (size_t i = 0; i < danhSachLoaiGhe.size(); ++i) {
        if (danhSachLoaiGhe[i].first == "Standard") {
            loaiChuan = danhSachLoaiGhe[i].first;
            giaChuan = danhSachLoaiGhe[i].second;
        }
        if (danhSachLoaiGhe[i].first == "VIP") {
            loaiVIP = danhSachLoaiGhe[i].first;
            giaVIP = danhSachLoaiGhe[i].second;
        }
    }
    char hang = 'A'; int soThuTu = 1;
    for (int i = 1; i <= sucChuaPhong; ++i) {
        stringstream ss; ss << hang << soThuTu;
        bool laGheVIP = hang > 'B';
        string loaiGhe = laGheVIP ? loaiVIP : loaiChuan;
        double donGia = laGheVIP ? giaVIP : giaChuan;
        danhSachGhe.push_back(Seat(ss.str(), maPhongCanTao, loaiGhe, donGia, "Trong"));
        if (++soThuTu > 10) { soThuTu = 1; hang++; }
    }
    ghiDanhSachGhe(danhSachGhe);
}

void Database::docDanhSachPhim(vector<Movie>& ds) {
    ds.clear(); ifstream inFile("DATA/quan_ly_phim.csv");
    if (!inFile.is_open()) return; string line; bool reading = false;
    while (getline(inFile, line)) {
        if (line.empty()) continue;
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
    list.clear();
    ifstream inFile("DATA/lich_va_ve.csv");
    if (!inFile.is_open()) return;

    vector<string> lines;
    string line;
    bool reading = false, changed = false;
    const time_t hienTai = time(NULL);
    while (getline(inFile, line)) {
        if (!line.empty() && line[0] == '[') {
            reading = (line.find("[SEAT_STATUS]") != string::npos);
            lines.push_back(line);
            continue;
        }
        if (reading && line.find("maSuatChieu") == string::npos && !line.empty()) {
            vector<string> p = tachChuoiCSV(line);
            if (p.size() >= 3 && p[0] == maSC) {
                string trangThai = p[2];
                if (trangThai == "DangChon" || (trangThai.find("DangChon|") == 0 && !giuGheConHan(trangThai, hienTai))) {
                    trangThai = "Trong";
                    line = p[0] + "," + p[1] + "," + trangThai;
                    changed = true;
                }
                list.push_back(make_pair(p[1], trangThai));
            }
        }
        lines.push_back(line);
    }
    inFile.close();

    if (changed) {
        ofstream outFile("DATA/lich_va_ve.csv", ios::trunc);
        if (outFile.is_open()) {
            for (size_t i = 0; i < lines.size(); ++i) outFile << lines[i] << "\n";
        }
    }
}

bool Database::giuGheSuatChieu(const string& maSC, const string& maGhe, const string& maPhienGiu) {
    if (maPhienGiu.empty() || maPhienGiu.find_first_of(",|") != string::npos) return false;
    vector<pair<string, string> > trangThaiGhe;
    docTrangThaiGheSuatChieu(maSC, trangThaiGhe);
    for (size_t i = 0; i < trangThaiGhe.size(); ++i) {
        if (trangThaiGhe[i].first == maGhe && trangThaiGhe[i].second != "Trong") return false;
    }

    const time_t hetHan = time(NULL) + 15 * 60;
    stringstream trangThaiMoi;
    trangThaiMoi << "DangChon|" << static_cast<long long>(hetHan) << "|" << maPhienGiu;
    return capNhatTrangThaiGheSuatChieu(maSC, maGhe, trangThaiMoi.str());
}

bool Database::kiemTraGiuGheSuatChieu(const string& maSC, const string& maGhe, const string& maPhienGiu) {
    vector<pair<string, string> > trangThaiGhe;
    docTrangThaiGheSuatChieu(maSC, trangThaiGhe);
    for (size_t i = 0; i < trangThaiGhe.size(); ++i) {
        if (trangThaiGhe[i].first == maGhe) {
            string maPhienDangGiu;
            return giuGheConHan(trangThaiGhe[i].second, time(NULL), &maPhienDangGiu) &&
                   maPhienDangGiu == maPhienGiu;
        }
    }
    return false;
}

bool Database::giaiPhongGheSuatChieu(const string& maSC, const string& maGhe, const string& maPhienGiu) {
    return capNhatTrangThaiGheSuatChieu(maSC, maGhe, "Trong", maPhienGiu);
}

bool Database::chotGheSuatChieu(const string& maSC, const vector<string>& danhSachGhe,
                                const string& maPhienGiu) {
    if (danhSachGhe.empty() || maPhienGiu.empty()) return false;
    vector<pair<string, string> > trangThaiGhe;
    docTrangThaiGheSuatChieu(maSC, trangThaiGhe);
    const time_t hienTai = time(NULL);
    for (size_t i = 0; i < danhSachGhe.size(); ++i) {
        bool hopLe = false;
        for (size_t j = 0; j < trangThaiGhe.size(); ++j) {
            string maPhienDangGiu;
            if (trangThaiGhe[j].first == danhSachGhe[i] &&
                giuGheConHan(trangThaiGhe[j].second, hienTai, &maPhienDangGiu) &&
                maPhienDangGiu == maPhienGiu) {
                hopLe = true;
                break;
            }
        }
        if (!hopLe) return false;
    }

    ifstream inFile("DATA/lich_va_ve.csv");
    if (!inFile.is_open()) return false;
    vector<string> lines;
    string line;
    bool inSec = false;
    while (getline(inFile, line)) {
        if (!line.empty() && line[0] == '[') {
            inSec = (line.find("[SEAT_STATUS]") != string::npos);
            lines.push_back(line);
            continue;
        }
        if (inSec && !line.empty() && line.find("maSuatChieu") == string::npos) {
            vector<string> p = tachChuoiCSV(line);
            if (p.size() >= 3 && p[0] == maSC) {
                for (size_t i = 0; i < danhSachGhe.size(); ++i) {
                    if (p[1] == danhSachGhe[i]) {
                        line = maSC + "," + p[1] + ",DaDat";
                        break;
                    }
                }
            }
        }
        lines.push_back(line);
    }
    if (inFile.bad()) return false;
    inFile.close();

    ostringstream output;
    for (size_t i = 0; i < lines.size(); ++i) output << lines[i] << "\n";
    return ghiToanBoFile("DATA/lich_va_ve.csv", output.str());
}

bool Database::capNhatTrangThaiGheSuatChieu(const string& maSC, const string& maGhe,
                                             const string& trangThaiMoi, const string& maPhienGiu) {
    ifstream inFile("DATA/lich_va_ve.csv");
    if (!inFile.is_open()) return false;
    vector<string> lines;
    string line;
    bool inSec = false, found = false, daCapNhat = false;
    const time_t hienTai = time(NULL);
    while (getline(inFile, line)) {
        if (!line.empty() && line[0] == '[') { inSec = (line.find("[SEAT_STATUS]") != string::npos); lines.push_back(line); continue; }
        if (inSec && !line.empty() && line.find("maSuatChieu") == string::npos) {
            vector<string> p = tachChuoiCSV(line);
            if (p.size() >= 3 && p[0] == maSC && p[1] == maGhe) {
                found = true;
                bool duocCapNhat = maPhienGiu.empty();
                if (!duocCapNhat) {
                    string maPhienDangGiu;
                    duocCapNhat = giuGheConHan(p[2], hienTai, &maPhienDangGiu) && maPhienDangGiu == maPhienGiu;
                }
                if (duocCapNhat && !daCapNhat) {
                    lines.push_back(maSC + "," + maGhe + "," + trangThaiMoi);
                    daCapNhat = true;
                } else {
                    lines.push_back(line);
                }
                continue;
            }
        }
        lines.push_back(line);
    }
    if (inFile.bad()) return false;
    inFile.close();

    if (!found && maPhienGiu.empty()) {
        bool hasSec = false; vector<string> newLines;
        for (size_t i = 0; i < lines.size(); ++i) {
            newLines.push_back(lines[i]);
            if (lines[i].find("[SEAT_STATUS]") != string::npos) hasSec = true;
        }
        if (!hasSec) { newLines.push_back("\n[SEAT_STATUS]"); newLines.push_back("maSuatChieu,maGhe,trangThai"); }
        newLines.push_back(maSC + "," + maGhe + "," + trangThaiMoi);
        lines = newLines;
        daCapNhat = true;
    }

    if (!daCapNhat) return false;
    ostringstream output;
    for (size_t i = 0; i < lines.size(); ++i) output << lines[i] << "\n";
    return ghiToanBoFile("DATA/lich_va_ve.csv", output.str());
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

bool Database::ghiDanhSachVe(const vector<Ticket>& danhSachVe) {
    string originalContent;
    if (!docToanBoFile("DATA/lich_va_ve.csv", originalContent) ||
        originalContent.find("[SHOWTIMES]") == string::npos ||
        originalContent.find("[TICKETS]") == string::npos ||
        originalContent.find("[SEAT_STATUS]") == string::npos) {
        return false;
    }

    vector<string> showtimeLines, seatStatusLines;
    ifstream inFile("DATA/lich_va_ve.csv");
    if (!inFile.is_open()) return false;
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
    if (inFile.bad()) return false;
    inFile.close();

    ostringstream output;
    output << "[SHOWTIMES]\nmaSuatChieu,maPhim,maPhong,ngayChieu,gioBatDau,gioKetThuc,giaVeCoBan\n";
    for (size_t i = 0; i < showtimeLines.size(); ++i) output << showtimeLines[i] << "\n";

    output << "\n[TICKETS]\nmaVe,maHoaDon,maSuatChieu,maGhe,giaVeThucTe,maQRCode,trangThaiSuDung\n";
    for (size_t i = 0; i < danhSachVe.size(); ++i) {
        output << danhSachVe[i].getMaVe() << "," << danhSachVe[i].getMaHoaDon() << "," << danhSachVe[i].getMaSuatChieu() << ","
               << danhSachVe[i].getMaGhe() << "," << danhSachVe[i].getGiaVeThucTe() << "," << danhSachVe[i].getMaQRCode() << ","
               << danhSachVe[i].getTrangThaiSuDung() << "\n";
    }

    output << "\n[SEAT_STATUS]\nmaSuatChieu,maGhe,trangThai\n";
    for (size_t i = 0; i < seatStatusLines.size(); ++i) output << seatStatusLines[i] << "\n";
    return ghiToanBoFile("DATA/lich_va_ve.csv", output.str());
}

static bool ghiFileGiaoDichFNB(const vector<FoodAndBeverage>& fnb, const vector<Promotion>& km, const vector<Invoice>& hd) {
    ostringstream output;

    output << "[FOOD_AND_BEVERAGES]\nmaMatHang,tenMatHang,loai,giaTien,trangThaiKinhDoanh\n";
    for (size_t i = 0; i < fnb.size(); ++i)
        output << fnb[i].getMaMatHang() << "," << fnb[i].getTenMatHang() << "," << fnb[i].getLoai() << "," << fnb[i].getGiaTien() << "," << fnb[i].getTrangThaiKinhDoanh() << "\n";

    output << "\n[PROMOTIONS]\nmaKhuyenMai,tenKhuyenMai,maCode,phanTramGiam,mucGiamToiDa,ngayBatDau,ngayKetThuc,phamViApDung\n";
    for (size_t i = 0; i < km.size(); ++i)
        output << km[i].getMaKhuyenMai() << "," << km[i].getTenKhuyenMai() << "," << km[i].getMaCode() << "," << km[i].getPhanTramGiam() << "," << km[i].getMucGiamToiDa() << "," << DateFormat::normalizeDate(km[i].getNgayBatDau()) << "," << DateFormat::normalizeDate(km[i].getNgayKetThuc()) << "," << km[i].getPhamViApDung() << "\n";

    output << "\n[INVOICES]\nmaHoaDon,maKhachHang,maNhanVien,thoiGianLap,tongTien,hinhThucThanhToan,maGiaoDich,trangThaiThanhToan\n";
    for (size_t i = 0; i < hd.size(); ++i)
        output << hd[i].getMaHoaDon() << "," << hd[i].getMaKhachHang() << "," << hd[i].getMaNhanVien() << "," << DateFormat::normalizeDateTime(hd[i].getThoiGianLap()) << "," << hd[i].getTongTien() << "," << hd[i].getHinhThucThanhToan() << "," << hd[i].getMaGiaoDich() << "," << hd[i].getTrangThaiThanhToan() << "\n";

    output << "\n[INVOICE_FB_ITEMS]\nmaHoaDon,maMatHang,tenMatHangTaiThoiDiem,soLuong,donGiaTaiThoiDiem,thanhTien\n";
    output << setprecision(numeric_limits<double>::max_digits10);
    for (size_t i = 0; i < hd.size(); ++i) {
        const vector<InvoiceFBItem>& items = hd[i].getChiTietFB();
        for (size_t j = 0; j < items.size(); ++j) {
            output << quoteCSVField(hd[i].getMaHoaDon()) << ","
                   << quoteCSVField(items[j].maMatHang) << ","
                   << quoteCSVField(items[j].tenMatHangTaiThoiDiem) << ","
                   << items[j].soLuong << ","
                   << items[j].donGiaTaiThoiDiem << ","
                   << items[j].thanhTien << "\n";
        }
    }
    return ghiToanBoFile("DATA/giao_dich_fnb.csv", output.str());
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
    if (!inFile.is_open()) return;
    string line;
    int section = 0;
    while (getline(inFile, line)) {
        if (line.empty()) continue;
        if (line[0] == '[') {
            if (line.find("[INVOICES]") == 0) section = 1;
            else if (line.find("[INVOICE_FB_ITEMS]") == 0) section = 2;
            else section = 0;
            continue;
        }
        if (section == 1 && line.find("maHoaDon") == string::npos) {
            vector<string> p = tachChuoiCSV(line);
            if (p.size() >= 8) ds.push_back(Invoice(p[0], p[1], p[2], p[3], atof(p[4].c_str()), p[5], p[6], p[7]));
        } else if (section == 2 && line.find("maHoaDon") != 0) {
            vector<string> p;
            int quantity;
            double unitPrice, total;
            if (!tachDongCSV(line, p) || p.size() != 6 ||
                !parsePositiveInteger(p[3], quantity) ||
                !parseNonNegativeDouble(p[4], unitPrice) ||
                !parseNonNegativeDouble(p[5], total)) {
                cerr << "[CANH BAO]: Bo qua dong chi tiet F&B khong hop le trong giao_dich_fnb.csv.\n";
                continue;
            }

            size_t invoiceIndex = ds.size();
            for (size_t i = 0; i < ds.size(); ++i) {
                if (ds[i].getMaHoaDon() == p[0]) {
                    invoiceIndex = i;
                    break;
                }
            }
            if (invoiceIndex == ds.size()) {
                cerr << "[CANH BAO]: Chi tiet F&B tham chieu hoa don khong ton tai: "
                     << p[0] << "\n";
                continue;
            }

            InvoiceFBItem item;
            item.maHoaDon = p[0];
            item.maMatHang = p[1];
            item.tenMatHangTaiThoiDiem = p[2];
            item.soLuong = quantity;
            item.donGiaTaiThoiDiem = unitPrice;
            item.thanhTien = total;
            ds[invoiceIndex].themChiTietFB(item);
        }
    }
    if (inFile.bad()) cerr << "[LOI]: Khong doc duoc day du du lieu hoa don va chi tiet F&B.\n";
    inFile.close();
}

bool Database::ghiDanhSachHoaDon(const vector<Invoice>& hd) {
    string originalContent;
    if (!docToanBoFile("DATA/giao_dich_fnb.csv", originalContent) ||
        originalContent.find("[FOOD_AND_BEVERAGES]") == string::npos ||
        originalContent.find("[PROMOTIONS]") == string::npos ||
        originalContent.find("[INVOICES]") == string::npos) {
        return false;
    }

    vector<FoodAndBeverage> fnb; vector<Promotion> km; docDanhSachFAndB(fnb); docDanhSachKhuyenMai(km);
    return ghiFileGiaoDichFNB(fnb, km, hd);
}

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
