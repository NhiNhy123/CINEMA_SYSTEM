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

#include <sstream>
#include <iostream>
#include <cstdlib>
#include <utility>
#include <ctime>
#include <iomanip>
#include <limits>
#include <map>
#include <set>
#include <algorithm>
#include <windows.h>
#include <sqlite3.h>

using namespace std;

namespace {
sqlite3* databaseHandle = NULL;

struct TableDefinition {
    const char* name;
    const char* createSql;
    int columnCount;
};

const TableDefinition tables[] = {
    {"food_and_beverages", "CREATE TABLE IF NOT EXISTS food_and_beverages (maMatHang TEXT,tenMatHang TEXT,loai TEXT,giaTien TEXT,trangThaiKinhDoanh TEXT)", 5},
    {"promotions", "CREATE TABLE IF NOT EXISTS promotions (maKhuyenMai TEXT,tenKhuyenMai TEXT,maCode TEXT,phanTramGiam TEXT,mucGiamToiDa TEXT,ngayBatDau TEXT,ngayKetThuc TEXT,phamViApDung TEXT)", 8},
    {"invoices", "CREATE TABLE IF NOT EXISTS invoices (maHoaDon TEXT,maKhachHang TEXT,maNhanVien TEXT,thoiGianLap TEXT,tongTien TEXT,hinhThucThanhToan TEXT,maGiaoDich TEXT,trangThaiThanhToan TEXT)", 8},
    {"invoice_fb_items", "CREATE TABLE IF NOT EXISTS invoice_fb_items (maHoaDon TEXT,maMatHang TEXT,tenMatHangTaiThoiDiem TEXT,soLuong TEXT,donGiaTaiThoiDiem TEXT,thanhTien TEXT)", 6},
    {"showtimes", "CREATE TABLE IF NOT EXISTS showtimes (maSuatChieu TEXT,maPhim TEXT,maPhong TEXT,ngayChieu TEXT,gioBatDau TEXT,gioKetThuc TEXT,giaVeCoBan TEXT)", 7},
    {"tickets", "CREATE TABLE IF NOT EXISTS tickets (maVe TEXT,maHoaDon TEXT,maSuatChieu TEXT,maGhe TEXT,giaVeThucTe TEXT,maQRCode TEXT,trangThaiSuDung TEXT)", 7},
    {"seat_status", "CREATE TABLE IF NOT EXISTS seat_status (maSuatChieu TEXT,maGhe TEXT,trangThai TEXT)", 3},
    {"nhat_ky_hoat_dong", "CREATE TABLE IF NOT EXISTS nhat_ky_hoat_dong (thoiGian TEXT,maNguoiDung TEXT,vaiTro TEXT,hanhDong TEXT,chiTiet TEXT)", 5},
    {"users", "CREATE TABLE IF NOT EXISTS users (maNguoiDung TEXT,hoTen TEXT,soDienThoai TEXT,email TEXT,ngaySinh TEXT,gioiTinh TEXT,cccd TEXT,taiKhoan TEXT,matKhau TEXT,vaiTro TEXT,trangThaiLamViec TEXT,trangThaiTaiKhoan TEXT)", 12},
    {"customers", "CREATE TABLE IF NOT EXISTS customers (maKhachHang TEXT,diemTichLuy TEXT,hangThanhVien TEXT,tongChiTieu TEXT)", 4},
    {"customer_points_history", "CREATE TABLE IF NOT EXISTS customer_points_history (maGiaoDichDiem TEXT,maKhachHang TEXT,loaiGiaoDich TEXT,soDiem TEXT,noiDung TEXT,thoiGian TEXT)", 6},
    {"genres", "CREATE TABLE IF NOT EXISTS genres (maTheLoai TEXT,tenTheLoai TEXT)", 2},
    {"movie_genres", "CREATE TABLE IF NOT EXISTS movie_genres (maPhim TEXT,maTheLoai TEXT)", 2},
    {"movies", "CREATE TABLE IF NOT EXISTS movies (maPhim TEXT,tenPhim TEXT,thoiLuong TEXT,nhanDoTuoi TEXT,ngayKhoiChieu TEXT,ngayKetThuc TEXT,moTa TEXT,poster TEXT)", 8},
    {"rooms", "CREATE TABLE IF NOT EXISTS rooms (maPhong TEXT,tenPhong TEXT,loaiPhong TEXT,sucChua TEXT,trangThaiPhong TEXT)", 5},
    {"seats", "CREATE TABLE IF NOT EXISTS seats (maGhe TEXT,maPhong TEXT,loaiGhe TEXT,donGiaGhe TEXT)", 4},
    {"seat_types", "CREATE TABLE IF NOT EXISTS seat_types (tenLoaiGhe TEXT,donGia TEXT)", 2},
    {"shift_managements", "CREATE TABLE IF NOT EXISTS shift_managements (maCa TEXT,maNhanVien TEXT,tenNhanVien TEXT,ngayLam TEXT,gioBatDau TEXT,gioKetThuc TEXT,doanhThuHeThong TEXT,tienThucTe TEXT,chenhLech TEXT,trangThai TEXT,daKhoaBaoCao TEXT)", 11}
};

const size_t tableCount = sizeof(tables) / sizeof(tables[0]);

bool openDatabase();

string quoteSqlIdentifier(const string& value) {
    string result = "\"";
    for (size_t i = 0; i < value.size(); ++i) {
        if (value[i] == '"') result += '"';
        result += value[i];
    }
    return result + "\"";
}

bool executeSql(const string& sql) {
    if (databaseHandle == NULL) return false;
    char* errorMessage = NULL;
    const int result = sqlite3_exec(databaseHandle, sql.c_str(), NULL, NULL, &errorMessage);
    if (result != SQLITE_OK) {
        cerr << "[LOI SQLite]: " << (errorMessage != NULL ? errorMessage : sqlite3_errmsg(databaseHandle)) << "\n";
        sqlite3_free(errorMessage);
        return false;
    }
    return true;
}

const TableDefinition* tableDefinition(const string& name) {
    for (size_t i = 0; i < tableCount; ++i) {
        if (name == tables[i].name) return &tables[i];
    }
    return NULL;
}

bool readTable(const string& table, vector<vector<string> >& rows) {
    rows.clear();
    const TableDefinition* definition = tableDefinition(table);
    if (definition == NULL || !openDatabase()) return false;
    const string sql = "SELECT * FROM " + quoteSqlIdentifier(table) + " ORDER BY rowid";
    sqlite3_stmt* statement = NULL;
    if (sqlite3_prepare_v2(databaseHandle, sql.c_str(), -1, &statement, NULL) != SQLITE_OK) {
        cerr << "[LOI SQLite]: " << sqlite3_errmsg(databaseHandle) << "\n";
        return false;
    }
    int result;
    while ((result = sqlite3_step(statement)) == SQLITE_ROW) {
        vector<string> row;
        for (int column = 0; column < definition->columnCount; ++column) {
            const unsigned char* text = sqlite3_column_text(statement, column);
            row.push_back(text != NULL ? reinterpret_cast<const char*>(text) : "");
        }
        rows.push_back(row);
    }
    sqlite3_finalize(statement);
    if (result != SQLITE_DONE) {
        cerr << "[LOI SQLite]: " << sqlite3_errmsg(databaseHandle) << "\n";
        return false;
    }
    return true;
}

bool writeTable(const string& table, const vector<vector<string> >& rows) {
    const TableDefinition* definition = tableDefinition(table);
    if (definition == NULL || !openDatabase()) return false;
    if (!executeSql("SAVEPOINT single_table_write")) return false;
    if (!executeSql("DELETE FROM " + quoteSqlIdentifier(table))) {
        executeSql("ROLLBACK TO single_table_write");
        executeSql("RELEASE single_table_write");
        return false;
    }

    ostringstream sql;
    sql << "INSERT INTO " << quoteSqlIdentifier(table) << " VALUES (";
    for (int i = 0; i < definition->columnCount; ++i) {
        if (i > 0) sql << ",";
        sql << "?";
    }
    sql << ")";
    sqlite3_stmt* statement = NULL;
    if (sqlite3_prepare_v2(databaseHandle, sql.str().c_str(), -1, &statement, NULL) != SQLITE_OK) {
        cerr << "[LOI SQLite]: " << sqlite3_errmsg(databaseHandle) << "\n";
        executeSql("ROLLBACK TO single_table_write");
        executeSql("RELEASE single_table_write");
        return false;
    }
    for (size_t row = 0; row < rows.size(); ++row) {
        if (static_cast<int>(rows[row].size()) != definition->columnCount) {
            sqlite3_finalize(statement);
            cerr << "[LOI SQLite]: So cot du lieu khong hop le cho bang " << table << ".\n";
            executeSql("ROLLBACK TO single_table_write");
            executeSql("RELEASE single_table_write");
            return false;
        }
        sqlite3_reset(statement);
        sqlite3_clear_bindings(statement);
        for (int column = 0; column < definition->columnCount; ++column) {
            sqlite3_bind_text(statement, column + 1, rows[row][column].c_str(),
                              -1, SQLITE_TRANSIENT);
        }
        if (sqlite3_step(statement) != SQLITE_DONE) {
            cerr << "[LOI SQLite]: " << sqlite3_errmsg(databaseHandle) << "\n";
            sqlite3_finalize(statement);
            executeSql("ROLLBACK TO single_table_write");
            executeSql("RELEASE single_table_write");
            return false;
        }
    }
    sqlite3_finalize(statement);
    return executeSql("RELEASE single_table_write");
}

bool writeTables(const vector<pair<string, vector<vector<string> > > >& updates) {
    if (!executeSql("SAVEPOINT data_write")) return false;
    for (size_t i = 0; i < updates.size(); ++i) {
        if (!writeTable(updates[i].first, updates[i].second)) {
            executeSql("ROLLBACK TO data_write");
            executeSql("RELEASE data_write");
            return false;
        }
    }
    return executeSql("RELEASE data_write");
}

template <typename T>
string toText(const T& value) {
    ostringstream output;
    output << value;
    return output.str();
}

string toPreciseText(double value) {
    ostringstream output;
    output << setprecision(numeric_limits<double>::max_digits10) << value;
    return output.str();
}

vector<string> makeRow(const string& a, const string& b) {
    vector<string> row;
    row.push_back(a);
    row.push_back(b);
    return row;
}

vector<string> makeRow(const string& a, const string& b, const string& c) {
    vector<string> row = makeRow(a, b);
    row.push_back(c);
    return row;
}

vector<string> makeRow(const string& a, const string& b, const string& c,
                       const string& d) {
    vector<string> row = makeRow(a, b, c);
    row.push_back(d);
    return row;
}

vector<string> makeRow(const string& a, const string& b, const string& c,
                       const string& d, const string& e) {
    vector<string> row = makeRow(a, b, c, d);
    row.push_back(e);
    return row;
}

bool openDatabase() {
    if (databaseHandle != NULL) return true;
    const int result = sqlite3_open("DATA/cinema.db", &databaseHandle);
    if (result != SQLITE_OK) {
        cerr << "[LOI SQLite]: Khong the mo DATA/cinema.db: "
             << (databaseHandle != NULL ? sqlite3_errmsg(databaseHandle) : "khong du bo nho") << "\n";
        if (databaseHandle != NULL) sqlite3_close(databaseHandle);
        databaseHandle = NULL;
        return false;
    }
    sqlite3_busy_timeout(databaseHandle, 5000);
    if (!executeSql("PRAGMA journal_mode=WAL;")) return false;

    for (size_t i = 0; i < tableCount; ++i) {
        if (!executeSql(tables[i].createSql)) return false;
    }
    return true;
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

Database::Database() : giaoDichBanVeDangMo(false) {
    if (!openDatabase()) {
        cerr << "[LOI]: Khong the khoi tao co so du lieu SQLite.\n";
    }
}

bool Database::batDauGiaoDichBanVe() {
    if (giaoDichBanVeDangMo || !executeSql("SAVEPOINT sale_transaction")) {
        cerr << "[LOI]: Khong the bat dau giao dich ban ve.\n";
        return false;
    }
    giaoDichBanVeDangMo = true;
    return true;
}

bool Database::khoiPhucGiaoDichBanVe() {
    if (!giaoDichBanVeDangMo) return false;
    if (!executeSql("ROLLBACK TO sale_transaction")) return false;
    if (!executeSql("RELEASE sale_transaction")) return false;
    giaoDichBanVeDangMo = false;
    return true;
}

void Database::hoanTatGiaoDichBanVe() {
    if (!giaoDichBanVeDangMo || !executeSql("RELEASE sale_transaction")) return;
    giaoDichBanVeDangMo = false;
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
    vector<vector<string> > users, customers;
    if (!readTable("users", users) || !readTable("customers", customers)) return;
    map<string, vector<string> > usersById;
    set<string> customerIds;
    for (size_t i = 0; i < users.size(); ++i) {
        if (users[i].empty() || users[i][0].empty()) continue;
        if (!usersById.insert(make_pair(users[i][0], users[i])).second) {
            cerr << "[CANH BAO]: Ma nguoi dung bi trung trong SQLite: "
                 << users[i][0] << ". Su dung ho so xuat hien dau tien.\n";
        }
    }
    for (size_t i = 0; i < customers.size(); ++i) {
        const vector<string>& row = customers[i];
        if (row.size() < 4 || row[0].empty()) continue;
        const string& maKH = row[0];
        if (!customerIds.insert(maKH).second) {
            continue;
        }
        map<string, vector<string> >::const_iterator user = usersById.find(maKH);
        const vector<string>* profile = user == usersById.end() ? NULL : &user->second;
        Customer customer(maKH,
            profile != NULL && profile->size() > 1 ? (*profile)[1] : "",
            profile != NULL && profile->size() > 2 ? (*profile)[2] : "",
            profile != NULL && profile->size() > 3 ? (*profile)[3] : "",
            atoi(row[1].c_str()), row[2], atoll(row[3].c_str()));
        customer.setNgaySinh(profile != NULL && profile->size() > 4 ? (*profile)[4] : "");
        customer.setGioiTinh(profile != NULL && profile->size() > 5 ? (*profile)[5] : "");
        customer.setCCCD(profile != NULL && profile->size() > 6 ? (*profile)[6] : "");
        danhSachKH.push_back(customer);
    }
}

string Database::layTenNhanVienQuaMa(string maNhanVien) {
    vector<vector<string> > users;
    if (!readTable("users", users)) return "Nhan vien chua ro";
    for (size_t i = 0; i < users.size(); ++i) {
        const vector<string>& row = users[i];
        if (row.size() >= 8 && (row[0] == maNhanVien || row[7] == maNhanVien)) {
            return row[1];
        }
    }
    return "Nhan vien chua ro";
}

bool Database::ghiDanhSachUsers(const vector<Customer>& danhSachKH) {
    vector<vector<string> > users, customers;
    if (!readTable("users", users)) return false;
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

    set<string> matchedUserIds;
    set<string> existingUserIds;
    for (size_t i = 0; i < users.size(); ++i) {
        vector<string>& userFields = users[i];
        if (userFields.empty() || userFields[0].empty() ||
            !existingUserIds.insert(userFields[0]).second) {
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
        matchedUserIds.insert(userFields[0]);
    }

    for (map<string, const Customer*>::const_iterator it = customersById.begin();
         it != customersById.end(); ++it) {
        if (matchedUserIds.find(it->first) != matchedUserIds.end()) continue;
        const Customer& profile = *it->second;
        vector<string> row;
        row.push_back(profile.getMaKH());
        row.push_back(profile.getHoTen());
        row.push_back(profile.getSoDienThoai());
        row.push_back(profile.getEmail());
        row.push_back(profile.getNgaySinh());
        row.push_back(profile.getGioiTinh());
        row.push_back(profile.getCCCD());
        row.push_back("khach_" + profile.getSoDienThoai());
        row.push_back("123456");
        row.push_back("Customer");
        row.push_back("DangLam");
        row.push_back("HoatDong");
        users.push_back(row);
    }
    for (size_t i = 0; i < danhSachKH.size(); ++i) {
        const Customer& customer = danhSachKH[i];
        customers.push_back(makeRow(customer.getMaKH(),
                                    toText(customer.getDiemTichLuy()),
                                    customer.getHangThanhVien(),
                                    toText(customer.getTongChiTieu())));
    }
    return writeTables(vector<pair<string, vector<vector<string> > > >{
        make_pair("users", users), make_pair("customers", customers)
    });
}

void Database::docLichSuGiaoDichDiem(vector<GiaoDichDiem>& danhSachGD) {
    danhSachGD.clear();
    vector<vector<string> > rows;
    if (!readTable("customer_points_history", rows)) return;
    for (size_t i = 0; i < rows.size(); ++i) {
        const vector<string>& row = rows[i];
        if (row.size() >= 6) {
            danhSachGD.push_back(GiaoDichDiem(row[0], row[1], row[2],
                                              atoi(row[3].c_str()), row[4], row[5]));
        }
    }
}

bool Database::ghiLichSuGiaoDichDiem(const vector<GiaoDichDiem>& danhSachGD) {
    vector<vector<string> > rows;
    for (size_t i = 0; i < danhSachGD.size(); ++i) {
        const GiaoDichDiem& item = danhSachGD[i];
        vector<string> row;
        row.push_back(item.getMaGiaoDich());
        row.push_back(item.getMaKhachHang());
        row.push_back(item.getLoaiGiaoDich());
        row.push_back(toText(item.getSoDiem()));
        row.push_back(item.getNoiDung());
        row.push_back(item.getThoiGian());
        rows.push_back(row);
    }
    return writeTable("customer_points_history", rows);
}

static bool ghiDuLieuPhim(const vector<pair<string, string> >& dsTheLoai,
                          const vector<pair<string, string> >& movieGenres,
                          const vector<Movie>& danhSachPhim) {
    vector<vector<string> > genresRows, movieGenreRows, movieRows;
    for (size_t i = 0; i < dsTheLoai.size(); ++i) {
        genresRows.push_back(makeRow(dsTheLoai[i].first, dsTheLoai[i].second));
    }
    for (size_t i = 0; i < movieGenres.size(); ++i) {
        movieGenreRows.push_back(makeRow(movieGenres[i].first, movieGenres[i].second));
    }
    for (size_t i = 0; i < danhSachPhim.size(); ++i) {
        const Movie& movie = danhSachPhim[i];
        vector<string> row;
        row.push_back(movie.getMaPhim());
        row.push_back(movie.getTenPhim());
        row.push_back(toText(movie.getThoiLuong()));
        row.push_back(toText(movie.getNhanDoTuoi()));
        row.push_back(movie.getNgayKhoiChieu());
        row.push_back(movie.getNgayKetThuc());
        row.push_back(movie.getMoTa());
        row.push_back(movie.getPoster());
        movieRows.push_back(row);
    }
    return writeTables(vector<pair<string, vector<vector<string> > > >{
        make_pair("genres", genresRows),
        make_pair("movie_genres", movieGenreRows),
        make_pair("movies", movieRows)
    });
}

void Database::docDanhSachTheLoai(vector<pair<string, string> >& ds) {
    ds.clear();
    vector<vector<string> > rows;
    if (!readTable("genres", rows)) return;
    bool upgraded = false;
    for (size_t i = 0; i < rows.size(); ++i) {
        if (rows[i].size() >= 2) {
            string code = rows[i][0];
            string name = rows[i][1];
            if (code == "TL01" && name == "Hanh Dong") { name = "Hành Động"; rows[i][1] = name; upgraded = true; }
            else if (code == "TL02" && name == "Kinh Di") { name = "Kinh Dị"; rows[i][1] = name; upgraded = true; }
            else if (code == "TL03" && name == "Tinh Cam") { name = "Tình Cảm"; rows[i][1] = name; upgraded = true; }
            else if (code == "TL04" && name == "Hoat Hinh") { name = "Hoạt Hình"; rows[i][1] = name; upgraded = true; }
            else if (code == "TL05" && name == "Tam Ly") { name = "Tâm Lý"; rows[i][1] = name; upgraded = true; }
            else if (code == "TL06" && name == "Gia Dinh") { name = "Gia Đình"; rows[i][1] = name; upgraded = true; }
            else if (code == "TL07" && (name == "Hoat Hinh" || name == "Vien Tuong")) { name = "Viễn Tưởng"; rows[i][1] = name; upgraded = true; }
            else if (code == "TL08" && name == "Kich Tinh") { name = "Kịch Tính"; rows[i][1] = name; upgraded = true; }
            else if (code == "TL09" && name == "Gay Can") { name = "Gay Cấn"; rows[i][1] = name; upgraded = true; }
            ds.push_back(make_pair(code, name));
        }
    }
    if (upgraded) {
        writeTable("genres", rows);
    }
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
        ss << "TL" << setw(2) << setfill('0') << nextId++;
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

    for (size_t i = movieGenres.size(); i > 0; --i) {
        if (movieGenres[i - 1].second == maTheLoai) {
            movieGenres.erase(movieGenres.begin() + (i - 1));
        }
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
    vector<vector<string> > rows;
    if (!readTable("movie_genres", rows)) return;
    for (size_t i = 0; i < rows.size(); ++i) {
        if (rows[i].size() >= 2) mg.push_back(make_pair(rows[i][0], rows[i][1]));
    }
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
    vector<vector<string> > roomRows, seatRows, typeRows;
    for (size_t i = 0; i < danhSachPhong.size(); ++i) {
        const Room& room = danhSachPhong[i];
        vector<string> row = makeRow(room.getMaPhong(), room.getTenPhong(),
                                     room.getLoaiPhong(),
                                     toText(room.getSucChua()),
                                     room.getTrangThaiPhong());
        roomRows.push_back(row);
    }
    for (size_t i = 0; i < danhSachGhe.size(); ++i) {
        const Seat& seat = danhSachGhe[i];
        seatRows.push_back(makeRow(seat.getMaGhe(), seat.getMaPhong(),
                                   seat.getLoaiGhe(), toText(seat.getDonGiaGhe())));
    }
    for (size_t i = 0; i < danhSachLoaiGhe.size(); ++i) {
        typeRows.push_back(makeRow(danhSachLoaiGhe[i].first,
                                   toText(danhSachLoaiGhe[i].second)));
    }
    return writeTables(vector<pair<string, vector<vector<string> > > >{
        make_pair("rooms", roomRows), make_pair("seats", seatRows),
        make_pair("seat_types", typeRows)
    });
}

void Database::docDanhSachPhong(vector<Room>& ds) {
    ds.clear();
    vector<vector<string> > rows;
    if (!readTable("rooms", rows)) return;
    for (size_t i = 0; i < rows.size(); ++i) {
        const vector<string>& p = rows[i];
        if (p.size() >= 5) ds.push_back(Room(p[0], p[1], p[2], atoi(p[3].c_str()), p[4]));
    }
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
    ds.clear();
    vector<vector<string> > rows;
    if (!readTable("seats", rows)) return;
    for (size_t i = 0; i < rows.size(); ++i) {
        const vector<string>& p = rows[i];
        if (p.size() >= 4) ds.push_back(Seat(p[0], p[1], p[2], atof(p[3].c_str()), "Trong"));
    }
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

bool Database::ghiSoDoGhe(const vector<Seat>& danhSachGhe,
                          const vector<pair<string, double> >& danhSachLoaiGhe) {
    vector<Room> danhSachPhong;
    docDanhSachPhong(danhSachPhong);
    return ghiDuLieuPhongGhe(danhSachPhong, danhSachGhe, danhSachLoaiGhe);
}

void Database::docDanhSachLoaiGhe(vector<pair<string, double> >& ds) {
    ds.clear();
    vector<vector<string> > rows;
    if (!readTable("seat_types", rows)) return;
    for (size_t i = 0; i < rows.size(); ++i) {
        if (rows[i].size() >= 2) {
            ds.push_back(make_pair(rows[i][0], atof(rows[i][1].c_str())));
        }
    }
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

void Database::docDanhSachPhim(vector<Movie>& ds) {
    ds.clear();
    vector<vector<string> > rows;
    if (!readTable("movies", rows)) return;
    for (size_t i = 0; i < rows.size(); ++i) {
        const vector<string>& p = rows[i];
        if (p.size() >= 8) {
            ds.push_back(Movie(p[0], p[1], atoi(p[2].c_str()), atoi(p[3].c_str()),
                               p[4], p[5], p[6], p[7]));
        }
    }
}

void Database::ghiDanhSachPhim(const vector<Movie>& danhSachPhim) {
    vector<pair<string, string> > dsTheLoai; vector<pair<string, string> > movieGenres;
    docDanhSachTheLoai(dsTheLoai); docMovieGenres(movieGenres);
    ghiDuLieuPhim(dsTheLoai, movieGenres, danhSachPhim);
}

void Database::docDanhSachSuatChieu(vector<Showtime>& ds) {
    ds.clear();
    vector<vector<string> > rows;
    if (!readTable("showtimes", rows)) return;
    for (size_t i = 0; i < rows.size(); ++i) {
        const vector<string>& p = rows[i];
        if (p.size() >= 7) {
            ds.push_back(Showtime(p[0], p[1], p[2], p[3], p[4], p[5],
                                  atof(p[6].c_str())));
        }
    }
}

void Database::ghiDanhSachSuatChieu(const vector<Showtime>& danhSachSuatChieu) {
    vector<vector<string> > rows;
    for (size_t i = 0; i < danhSachSuatChieu.size(); ++i) {
        const Showtime& item = danhSachSuatChieu[i];
        vector<string> row;
        row.push_back(item.getMaSuatChieu());
        row.push_back(item.getMaPhim());
        row.push_back(item.getMaPhong());
        row.push_back(item.getNgayChieu());
        row.push_back(item.getGioBatDau());
        row.push_back(item.getGioKetThuc());
        row.push_back(toText(item.getGiaVeCoBan()));
        rows.push_back(row);
    }
    writeTable("showtimes", rows);
}

void Database::docTrangThaiGheSuatChieu(const string& maSC, vector<pair<string, string> >& list) {
    list.clear();
    vector<vector<string> > rows;
    if (!readTable("seat_status", rows)) return;
    bool changed = false;
    const time_t hienTai = time(NULL);
    for (size_t i = 0; i < rows.size(); ++i) {
        vector<string>& row = rows[i];
        if (row.size() < 3 || row[0] != maSC) continue;
        if (row[2] == "DangChon" ||
            (row[2].find("DangChon|") == 0 && !giuGheConHan(row[2], hienTai))) {
            row[2] = "Trong";
            changed = true;
        }
        list.push_back(make_pair(row[1], row[2]));
    }
    if (changed) writeTable("seat_status", rows);
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

    vector<vector<string> > rows;
    if (!readTable("seat_status", rows)) return false;
    for (size_t i = 0; i < rows.size(); ++i) {
        if (rows[i].size() < 3 || rows[i][0] != maSC) continue;
        if (find(danhSachGhe.begin(), danhSachGhe.end(), rows[i][1]) != danhSachGhe.end()) {
            rows[i][2] = "DaDat";
        }
    }
    return writeTable("seat_status", rows);
}

bool Database::capNhatTrangThaiGheSuatChieu(const string& maSC, const string& maGhe,
                                             const string& trangThaiMoi, const string& maPhienGiu) {
    vector<vector<string> > rows;
    if (!readTable("seat_status", rows)) return false;
    bool found = false, daCapNhat = false;
    const time_t hienTai = time(NULL);
    for (size_t i = 0; i < rows.size(); ++i) {
        if (rows[i].size() < 3 || rows[i][0] != maSC || rows[i][1] != maGhe) continue;
        found = true;
        bool duocCapNhat = maPhienGiu.empty();
        if (!duocCapNhat) {
            string maPhienDangGiu;
            duocCapNhat = giuGheConHan(rows[i][2], hienTai, &maPhienDangGiu) &&
                          maPhienDangGiu == maPhienGiu;
        }
        if (duocCapNhat && !daCapNhat) {
            rows[i][2] = trangThaiMoi;
            daCapNhat = true;
        }
    }
    if (!found && maPhienGiu.empty()) {
        rows.push_back(makeRow(maSC, maGhe, trangThaiMoi));
        daCapNhat = true;
    }
    return daCapNhat && writeTable("seat_status", rows);
}

void Database::docDanhSachVe(vector<Ticket>& ds) {
    ds.clear();
    vector<vector<string> > rows;
    if (!readTable("tickets", rows)) return;
    for (size_t i = 0; i < rows.size(); ++i) {
        const vector<string>& p = rows[i];
        if (p.size() >= 7) {
            ds.push_back(Ticket(p[0], p[1], p[2], p[3], atof(p[4].c_str()),
                                p[5], p[6]));
        }
    }
}

bool Database::ghiDanhSachVe(const vector<Ticket>& danhSachVe) {
    vector<vector<string> > showtimeRows, seatStatusRows, ticketRows;
    if (!readTable("showtimes", showtimeRows) ||
        !readTable("seat_status", seatStatusRows)) return false;
    for (size_t i = 0; i < danhSachVe.size(); ++i) {
        const Ticket& item = danhSachVe[i];
        vector<string> row;
        row.push_back(item.getMaVe());
        row.push_back(item.getMaHoaDon());
        row.push_back(item.getMaSuatChieu());
        row.push_back(item.getMaGhe());
        row.push_back(toText(item.getGiaVeThucTe()));
        row.push_back(item.getMaQRCode());
        row.push_back(item.getTrangThaiSuDung());
        ticketRows.push_back(row);
    }
    return writeTable("tickets", ticketRows);
}

static bool ghiFileGiaoDichFNB(const vector<FoodAndBeverage>& fnb, const vector<Promotion>& km, const vector<Invoice>& hd) {
    vector<vector<string> > foodRows, promotionRows, invoiceRows, itemRows;
    for (size_t i = 0; i < fnb.size(); ++i) {
        foodRows.push_back(makeRow(fnb[i].getMaMatHang(), fnb[i].getTenMatHang(),
            fnb[i].getLoai(), toText(fnb[i].getGiaTien()),
            fnb[i].getTrangThaiKinhDoanh()));
    }
    for (size_t i = 0; i < km.size(); ++i) {
        vector<string> row;
        row.push_back(km[i].getMaKhuyenMai());
        row.push_back(km[i].getTenKhuyenMai());
        row.push_back(km[i].getMaCode());
        row.push_back(toText(km[i].getPhanTramGiam()));
        row.push_back(toText(km[i].getMucGiamToiDa()));
        row.push_back(DateFormat::normalizeDate(km[i].getNgayBatDau()));
        row.push_back(DateFormat::normalizeDate(km[i].getNgayKetThuc()));
        row.push_back(km[i].getPhamViApDung());
        promotionRows.push_back(row);
    }
    for (size_t i = 0; i < hd.size(); ++i) {
        const Invoice& invoice = hd[i];
        vector<string> row;
        row.push_back(invoice.getMaHoaDon());
        row.push_back(invoice.getMaKhachHang());
        row.push_back(invoice.getMaNhanVien());
        row.push_back(DateFormat::normalizeDateTime(invoice.getThoiGianLap()));
        row.push_back(toText(invoice.getTongTien()));
        row.push_back(invoice.getHinhThucThanhToan());
        row.push_back(invoice.getMaGiaoDich());
        row.push_back(invoice.getTrangThaiThanhToan());
        invoiceRows.push_back(row);
        const vector<InvoiceFBItem>& items = hd[i].getChiTietFB();
        for (size_t j = 0; j < items.size(); ++j) {
            vector<string> itemRow;
            itemRow.push_back(invoice.getMaHoaDon());
            itemRow.push_back(items[j].maMatHang);
            itemRow.push_back(items[j].tenMatHangTaiThoiDiem);
            itemRow.push_back(toText(items[j].soLuong));
            itemRow.push_back(toPreciseText(items[j].donGiaTaiThoiDiem));
            itemRow.push_back(toPreciseText(items[j].thanhTien));
            itemRows.push_back(itemRow);
        }
    }
    return writeTables(vector<pair<string, vector<vector<string> > > >{
        make_pair("food_and_beverages", foodRows),
        make_pair("promotions", promotionRows),
        make_pair("invoices", invoiceRows),
        make_pair("invoice_fb_items", itemRows)
    });
}

void Database::docDanhSachFAndB(vector<FoodAndBeverage>& ds) {
    ds.clear();
    vector<vector<string> > rows;
    if (!readTable("food_and_beverages", rows)) return;
    for (size_t i = 0; i < rows.size(); ++i) {
        const vector<string>& p = rows[i];
        if (p.size() >= 5) {
            ds.push_back(FoodAndBeverage(p[0], p[1], p[2], atof(p[3].c_str()), p[4]));
        }
    }
}

void Database::ghiDanhSachFAndB(const vector<FoodAndBeverage>& fnb) {
    vector<Promotion> km; vector<Invoice> hd; docDanhSachKhuyenMai(km); docDanhSachHoaDon(hd);
    ghiFileGiaoDichFNB(fnb, km, hd);
}

void Database::docDanhSachKhuyenMai(vector<Promotion>& ds) {
    ds.clear();
    vector<vector<string> > rows;
    if (!readTable("promotions", rows)) return;
    for (size_t i = 0; i < rows.size(); ++i) {
        const vector<string>& p = rows[i];
        if (p.size() >= 8) {
            ds.push_back(Promotion(p[0], p[1], p[2], atof(p[3].c_str()),
                                   atof(p[4].c_str()), p[5], p[6], p[7]));
        }
    }
}

void Database::ghiDanhSachKhuyenMai(const vector<Promotion>& km) {
    vector<FoodAndBeverage> fnb; vector<Invoice> hd; docDanhSachFAndB(fnb); docDanhSachHoaDon(hd);
    ghiFileGiaoDichFNB(fnb, km, hd);
}

void Database::docDanhSachHoaDon(vector<Invoice>& ds) {
    ds.clear();
    vector<vector<string> > invoices, items;
    if (!readTable("invoices", invoices) ||
        !readTable("invoice_fb_items", items)) return;
    for (size_t i = 0; i < invoices.size(); ++i) {
        const vector<string>& row = invoices[i];
        if (row.size() >= 8) {
            ds.push_back(Invoice(row[0], row[1], row[2], row[3],
                                 atof(row[4].c_str()), row[5], row[6], row[7]));
        }
    }
    for (size_t i = 0; i < items.size(); ++i) {
            const vector<string>& p = items[i];
            int quantity;
            double unitPrice, total;
            if (p.size() != 6 ||
                !parsePositiveInteger(p[3], quantity) ||
                !parseNonNegativeDouble(p[4], unitPrice) ||
                !parseNonNegativeDouble(p[5], total)) {
                cerr << "[CANH BAO]: Bo qua dong chi tiet F&B khong hop le trong SQLite.\n";
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

bool Database::ghiDanhSachHoaDon(const vector<Invoice>& hd) {
    vector<FoodAndBeverage> fnb; vector<Promotion> km; docDanhSachFAndB(fnb); docDanhSachKhuyenMai(km);
    return ghiFileGiaoDichFNB(fnb, km, hd);
}

void Database::docDanhSachCa(vector<ShiftReport>& danhSachCa) {
    danhSachCa.clear();
    vector<vector<string> > rows;
    if (!readTable("shift_managements", rows)) return;
    for (size_t i = 0; i < rows.size(); ++i) {
        const vector<string>& p = rows[i];
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

void Database::ghiDanhSachCa(const vector<ShiftReport>& danhSachCa) {
    vector<vector<string> > rows;
    for (size_t i = 0; i < danhSachCa.size(); ++i) {
        const ShiftReport& sr = danhSachCa[i];
        vector<string> row;
        row.push_back(sr.getMaCa());
        row.push_back(sr.getMaNhanVien());
        row.push_back(sr.getTenNhanVien());
        row.push_back(sr.getNgayLam());
        row.push_back(sr.getGioBatDau());
        row.push_back(sr.getGioKetThuc());
        row.push_back(toText(sr.getDoanhThuHeThong()));
        row.push_back(toText(sr.getTienThucTe()));
        row.push_back(toText(sr.getChenhLech()));
        row.push_back(sr.getTrangThai());
        row.push_back(sr.getDaKhoaBaoCao() ? "true" : "false");
        rows.push_back(row);
    }
    writeTable("shift_managements", rows);
}
