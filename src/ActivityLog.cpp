#include "../include/ActivityLog.h"

#include <ctime>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>

namespace {
const char* const activityLogPath = "DATA/nhat_ky_hoat_dong.csv";

std::string csvSafe(std::string value) {
    for (std::size_t i = 0; i < value.size(); ++i) {
        if (value[i] == ',' || value[i] == '\r' || value[i] == '\n') {
            value[i] = ' ';
        }
    }
    return value;
}

std::string currentTimestamp() {
    const std::time_t now = std::time(0);
    const std::tm* local = std::localtime(&now);
    if (local == NULL) {
        return "";
    }
    std::ostringstream output;
    output << std::put_time(local, "%d/%m/%Y %H:%M:%S");
    return output.str();
}
}

void ActivityLog::ghiNhan(const std::string& maNguoiDung,
                          const std::string& vaiTro,
                          const std::string& hanhDong,
                          const std::string& chiTiet) {
    std::ifstream existing(activityLogPath);
    const bool needsHeader = !existing.good() || existing.peek() == std::ifstream::traits_type::eof();
    existing.close();

    std::ofstream output(activityLogPath, std::ios::app);
    if (!output.is_open()) {
        std::cerr << "Khong the ghi nhat ky hoat dong vao "
                  << activityLogPath << ".\n";
        return;
    }
    if (needsHeader) {
        output << "thoiGian,maNguoiDung,vaiTro,hanhDong,chiTiet\n";
    }
    output << csvSafe(currentTimestamp()) << ","
           << csvSafe(maNguoiDung) << ","
           << csvSafe(vaiTro) << ","
           << csvSafe(hanhDong) << ","
           << csvSafe(chiTiet) << "\n";
    if (!output.good()) {
        std::cerr << "Loi khi ghi nhat ky hoat dong vao "
                  << activityLogPath << ".\n";
    }
}

void ActivityLog::hienThi(const std::string& maNguoiDung) {
    std::ifstream input(activityLogPath);
    if (!input.is_open()) {
        std::cout << "Chua co nhat ky hoat dong hoac khong the mo "
                  << activityLogPath << ".\n";
        return;
    }

    std::string line;
    std::getline(input, line);
    std::cout << "\n====================== NHAT KY HOAT DONG ======================\n";
    std::cout << std::left << std::setw(22) << "Thoi gian"
              << std::setw(14) << "Nguoi dung"
              << std::setw(10) << "Vai tro"
              << std::setw(20) << "Hanh dong"
              << "Chi tiet\n";
    std::cout << "---------------------------------------------------------------\n";

    bool found = false;
    while (std::getline(input, line)) {
        std::stringstream row(line);
        std::string timestamp, userId, role, action, details;
        std::getline(row, timestamp, ',');
        std::getline(row, userId, ',');
        std::getline(row, role, ',');
        std::getline(row, action, ',');
        std::getline(row, details);

        if (!maNguoiDung.empty() && userId != maNguoiDung) {
            continue;
        }
        std::cout << std::left << std::setw(22) << timestamp
                  << std::setw(14) << userId
                  << std::setw(10) << role
                  << std::setw(20) << action
                  << details << "\n";
        found = true;
    }
    if (!found) {
        std::cout << "Khong co hoat dong nao phu hop.\n";
    }
    std::cout << "===============================================================\n";
}
