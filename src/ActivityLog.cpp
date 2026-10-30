#include "../include/ActivityLog.h"

#include <ctime>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <vector>

namespace {
const char* const activityLogPath = "DATA/nhat_ky_hoat_dong.csv";

std::string csvField(const std::string& value) {
    std::string field;
    bool needsQuotes = false;
    for (std::size_t i = 0; i < value.size(); ++i) {
        if (value[i] == '\r' || value[i] == '\n') {
            field += ' ';
        } else {
            if (value[i] == ',' || value[i] == '"') needsQuotes = true;
            if (value[i] == '"') field += '"';
            field += value[i];
        }
    }

    if (!needsQuotes) return field;
    return "\"" + field + "\"";
}

bool parseCsvRow(const std::string& line, std::vector<std::string>& fields) {
    fields.clear();
    std::string field;
    bool inQuotes = false;
    bool closedQuote = false;

    for (std::size_t i = 0; i < line.size(); ++i) {
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
    fields.push_back(field);
    return true;
}

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
    output << csvField(timestamp) << ","
           << csvField(maNguoiDung) << ","
           << csvField(vaiTro) << ","
           << csvField(hanhDong) << ","
           << csvField(chiTiet) << "\n";
    output.close();
    if (!output) {
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

    std::cout << "\n====================== NHAT KY HOAT DONG ======================\n";
    std::cout << std::left << std::setw(22) << "Thoi gian"
              << std::setw(14) << "Nguoi dung"
              << std::setw(10) << "Vai tro"
              << std::setw(20) << "Hanh dong"
              << "Chi tiet\n";
    std::cout << "---------------------------------------------------------------\n";

    bool found = false;
    std::string line;
    std::vector<std::string> fields;
    while (std::getline(input, line)) {
        if (!parseCsvRow(line, fields) || fields.size() != 5 ||
            fields[0] == "thoiGian") {
            continue;
        }
        if (!maNguoiDung.empty() && fields[1] != maNguoiDung) continue;

        std::cout << std::left << std::setw(22) << fields[0]
                  << std::setw(14) << fields[1]
                  << std::setw(10) << fields[2]
                  << std::setw(20) << fields[3]
                  << fields[4] << "\n";
        found = true;
    }

    if (!found) {
        std::cout << "Khong co hoat dong nao phu hop.\n";
    }
    std::cout << "===============================================================\n";
}
