#ifndef ACTIVITY_LOG_H
#define ACTIVITY_LOG_H

#include <string>

class ActivityLog {
public:
    static void ghiNhan(const std::string& maNguoiDung,
                        const std::string& vaiTro,
                        const std::string& hanhDong,
                        const std::string& chiTiet);
    static void hienThi(const std::string& maNguoiDung = "");
};

#endif
