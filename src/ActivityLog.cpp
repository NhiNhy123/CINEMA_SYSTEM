#include "../include/ActivityLog.h"

#include <ctime>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <sqlite3.h>

namespace {
bool currentTimestamp(std::string& timestamp) {
    const std::time_t now = std::time(0);
    const std::tm* local = std::localtime(&now);
    if (local == NULL) return false;

    std::ostringstream output;
    output << std::put_time(local, "%d/%m/%Y %H:%M:%S");
    timestamp = output.str();
    return !timestamp.empty();
}
}

void ActivityLog::ghiNhan(const std::string& maNguoiDung,
                          const std::string& vaiTro,
                          const std::string& hanhDong,
                          const std::string& chiTiet) {
    std::string timestamp;
    if (!currentTimestamp(timestamp)) {
        std::cerr << "Khong the xac dinh thoi gian ghi nhat ky hoat dong.\n";
        return;
    }

    sqlite3* database = NULL;
    if (sqlite3_open("DATA/cinema.db", &database) != SQLITE_OK) {
        std::cerr << "Khong the mo SQLite de ghi nhat ky hoat dong: "
                  << (database != NULL ? sqlite3_errmsg(database) : "khong du bo nho") << "\n";
        if (database != NULL) sqlite3_close(database);
        return;
    }

    sqlite3_stmt* statement = NULL;
    const char* sql = "INSERT INTO nhat_ky_hoat_dong "
                      "(thoiGian,maNguoiDung,vaiTro,hanhDong,chiTiet) VALUES (?,?,?,?,?)";
    if (sqlite3_prepare_v2(database, sql, -1, &statement, NULL) != SQLITE_OK) {
        std::cerr << "Khong the chuan bi ghi nhat ky hoat dong: "
                  << sqlite3_errmsg(database) << "\n";
        sqlite3_close(database);
        return;
    }
    sqlite3_bind_text(statement, 1, timestamp.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(statement, 2, maNguoiDung.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(statement, 3, vaiTro.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(statement, 4, hanhDong.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(statement, 5, chiTiet.c_str(), -1, SQLITE_TRANSIENT);
    if (sqlite3_step(statement) != SQLITE_DONE) {
        std::cerr << "Loi khi ghi nhat ky hoat dong: " << sqlite3_errmsg(database) << "\n";
    }
    sqlite3_finalize(statement);
    sqlite3_close(database);
}

void ActivityLog::hienThi(const std::string& maNguoiDung) {
    sqlite3* database = NULL;
    if (sqlite3_open("DATA/cinema.db", &database) != SQLITE_OK) {
        std::cerr << "Khong the mo SQLite de doc nhat ky hoat dong: "
                  << (database != NULL ? sqlite3_errmsg(database) : "khong du bo nho") << "\n";
        if (database != NULL) sqlite3_close(database);
        return;
    }

    sqlite3_stmt* statement = NULL;
    const char* sql = maNguoiDung.empty()
        ? "SELECT thoiGian,maNguoiDung,vaiTro,hanhDong,chiTiet FROM nhat_ky_hoat_dong ORDER BY rowid"
        : "SELECT thoiGian,maNguoiDung,vaiTro,hanhDong,chiTiet FROM nhat_ky_hoat_dong WHERE maNguoiDung=? ORDER BY rowid";
    if (sqlite3_prepare_v2(database, sql, -1, &statement, NULL) != SQLITE_OK) {
        std::cerr << "Khong the doc nhat ky hoat dong: " << sqlite3_errmsg(database) << "\n";
        sqlite3_close(database);
        return;
    }
    if (!maNguoiDung.empty()) {
        sqlite3_bind_text(statement, 1, maNguoiDung.c_str(), -1, SQLITE_TRANSIENT);
    }

    std::cout << "\n====================== NHAT KY HOAT DONG ======================\n";
    std::cout << std::left << std::setw(22) << "Thoi gian"
              << std::setw(14) << "Nguoi dung"
              << std::setw(10) << "Vai tro"
              << std::setw(20) << "Hanh dong"
              << "Chi tiet\n";
    std::cout << "---------------------------------------------------------------\n";

    bool found = false;
    int stepResult = SQLITE_OK;
    while ((stepResult = sqlite3_step(statement)) == SQLITE_ROW) {
        const char* fields[5];
        for (int i = 0; i < 5; ++i) {
            const unsigned char* text = sqlite3_column_text(statement, i);
            fields[i] = text != NULL ? reinterpret_cast<const char*>(text) : "";
        }
        std::cout << std::left << std::setw(22) << fields[0]
                  << std::setw(14) << fields[1]
                  << std::setw(10) << fields[2]
                  << std::setw(20) << fields[3]
                  << fields[4] << "\n";
        found = true;
    }
    if (stepResult != SQLITE_DONE) {
        std::cerr << "Loi khi truy van nhat ky hoat dong: " << sqlite3_errmsg(database) << "\n";
    }

    if (!found) {
        std::cout << "Khong co hoat dong nao phu hop.\n";
    }
    std::cout << "===============================================================\n";
    sqlite3_finalize(statement);
    sqlite3_close(database);
}
