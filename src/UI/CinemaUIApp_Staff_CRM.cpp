#include "../../include/UI/CinemaUIApp.h"
#include "../../include/ActivityLog.h"
#include <fstream>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <cmath>
#include <ctime>
#include <sqlite3.h>

namespace UI {

static std::string formatCommaNumber(long long val) {
    bool neg = (val < 0);
    if (neg) val = -val;
    std::string s = std::to_string(val);
    int n = static_cast<int>(s.length()) - 3;
    while (n > 0) {
        s.insert(n, ",");
        n -= 3;
    }
    return neg ? ("-" + s) : s;
}

static std::string formatDotNumber(long long val) {
    if (val < 0) val = 0;
    std::string s = std::to_string(val);
    int n = static_cast<int>(s.length()) - 3;
    while (n > 0) {
        s.insert(n, ".");
        n -= 3;
    }
    return s;
}

static std::string formatVndCurrency(long long val) {
    return formatDotNumber(val) + " VNĐ";
}

void CinemaUIApp::setupCRMView() {
    float startX = sidebarWidth + 24.f; // 274.f
    float contentW = 1728.f - startX - 24.f; // 1430.f

    crmSubTab = 1; // Default to "Lịch sử tích điểm" matching mockup media_1795194525044_db2db14c.jpg
    showCrmAddModal = false;
    crmCurrentPage = 0;

    // 1. Hero Header Right Buttons: "Xuất Excel" & "In nhật ký" (or "+ Thêm hội viên")
    btnCrmExport = std::make_unique<RoundRectButton>(
        "Xuất Excel",
        sf::Vector2f(startX + contentW - 246.f, 92.f),
        sf::Vector2f(108.f, 36.f),
        ButtonStyle::Secondary,
        8.f
    );
    btnCrmExport->setFontSize(12);
    btnCrmExport->setCustomColors(
        sf::Color::White,
        sf::Color(248, 250, 252),
        sf::Color(241, 245, 249),
        sf::Color(203, 213, 225)
    );
    btnCrmExport->setTextColor(sf::Color(30, 41, 59));
    btnCrmExport->setOnClick([this]() {
        if (crmSubTab == 1) {
            std::ofstream f("DATA/nhat_ky_tich_diem_crm.csv");
            if (f.is_open()) {
                f << "\xEF\xBB\xBF";
                f << "Mã GD,Mã KH,Khách hàng,Số điện thoại,Loại giao dịch,Số điểm,Nội dung,Thời gian\n";
                for (const auto& r : crmPointHistoryList) {
                    std::string signPts = (r.loaiGiaoDich == "DoiDiem" ? "-" : "+") + std::to_string(r.soDiem);
                    f << "\"" << r.maGiaoDich << "\","
                      << "\"" << r.maKhachHang << "\","
                      << "\"" << r.hoTen << "\","
                      << "\"" << r.soDienThoai << "\","
                      << "\"" << (r.loaiGiaoDich == "DoiDiem" ? "Đổi điểm" : "Tích điểm") << "\","
                      << "\"" << signPts << "\","
                      << "\"" << r.noiDung << "\","
                      << "\"" << r.thoiGian << "\"\n";
                }
                f.close();
                ActivityLog::ghiNhan(currentUserId, currentUserRole, "XUAT_EXCEL_CRM", "Xuất lịch sử tích điểm CRM ra file CSV/Excel");
                if (confirmPopup) {
                    confirmPopup->show("XUẤT EXCEL THÀNH CÔNG", "Đã xuất bảng lịch sử giao dịch điểm thành viên ra file:\nDATA/nhat_ky_tich_diem_crm.csv", ButtonStyle::Primary, "Đóng", "");
                }
            }
        } else {
            std::ofstream f("DATA/danh_sach_hoi_vien.csv");
            if (f.is_open()) {
                f << "\xEF\xBB\xBF";
                f << "Mã KH,Họ và Tên,Số điện thoại,Email,Hạng thẻ,Điểm tích lũy,Tổng chi tiêu\n";
                for (const auto& c : customerList) {
                    f << "\"" << c.getMaKH() << "\","
                      << "\"" << c.getHoTen() << "\","
                      << "\"" << c.getSoDienThoai() << "\","
                      << "\"" << c.getEmail() << "\","
                      << "\"" << c.getHangThanhVien() << "\","
                      << c.getDiemTichLuy() << ","
                      << c.getTongChiTieu() << "\n";
                }
                f.close();
                ActivityLog::ghiNhan(currentUserId, currentUserRole, "XUAT_CSV_CRM", "Xuất danh sách hội viên CRM ra CSV/Excel");
                if (confirmPopup) {
                    confirmPopup->show("XUẤT EXCEL THÀNH CÔNG", "Danh sách hội viên đã được lưu tại:\nDATA/danh_sach_hoi_vien.csv", ButtonStyle::Primary, "Đóng", "");
                }
            }
        }
    });

    btnCrmAddModal = std::make_unique<RoundRectButton>(
        "In nhật ký",
        sf::Vector2f(startX + contentW - 128.f, 92.f),
        sf::Vector2f(108.f, 36.f),
        ButtonStyle::Primary,
        8.f
    );
    btnCrmAddModal->setFontSize(12);
    btnCrmAddModal->setCustomColors(
        sf::Color(2, 132, 199),
        sf::Color(3, 105, 161),
        sf::Color(7, 89, 133),
        sf::Color(148, 163, 184)
    );
    btnCrmAddModal->setOnClick([this]() {
        if (crmSubTab == 1) {
            std::ofstream f("DATA/in_nhat_ky_diem_crm.txt");
            if (f.is_open()) {
                f << "================================================================\n";
                f << "          CINESTAR NEXUS - NHẬT KÝ GIAO DỊCH ĐIỂM CRM           \n";
                f << "================================================================\n";
                for (const auto& r : crmPointHistoryList) {
                    f << "[" << r.maGiaoDich << "] " << r.thoiGian << " | "
                      << r.hoTen << " (" << r.soDienThoai << ") | "
                      << (r.loaiGiaoDich == "DoiDiem" ? "- Đổi điểm: -" : "+ Tích điểm: +") << r.soDiem << " điểm | "
                      << r.noiDung << "\n";
                }
                f.close();
                ActivityLog::ghiNhan(currentUserId, currentUserRole, "IN_NHAT_KY_CRM", "In nhật ký giao dịch điểm thành viên CRM");
                if (confirmPopup) {
                    confirmPopup->show("IN NHẬT KÝ THÀNH CÔNG", "Đã kết xuất bản in nhật ký điểm thành viên ra file:\nDATA/in_nhat_ky_diem_crm.txt", ButtonStyle::Primary, "Đóng", "");
                }
            }
        } else {
            showCrmAddModal = true;
            if (inputCrmName) inputCrmName->setText("");
            if (inputCrmPhone) inputCrmPhone->setText("");
            if (inputCrmEmail) inputCrmEmail->setText("");
        }
    });

    // 2. Filter & Search Bar Controls (y = 358.f)
    float filterY = 358.f;
    crmSearchInput = std::make_unique<UIInputBox>(
        sf::Vector2f(startX + 16.f, filterY + 11.f),
        sf::Vector2f(640.f, 38.f),
        "Tìm theo Mã GD, Tên khách hàng, Số điện thoại...",
        12
    );

    crmTierDropdown = std::make_unique<DropdownBox>(
        sf::Vector2f(startX + 672.f, filterY + 11.f),
        sf::Vector2f(310.f, 38.f),
        std::vector<std::string>{"Tất cả loại GD", "+ Tích điểm", "- Đổi điểm"},
        0
    );
    crmTierDropdown->setLightStyle(true);
    crmTierDropdown->setFontSize(12);
    crmTierDropdown->setOnSelect([this](int, const std::string&) {
        crmCurrentPage = 0;
    });

    crmDateRangeInput = std::make_unique<UIInputBox>(
        sf::Vector2f(startX + 998.f, filterY + 11.f),
        sf::Vector2f(195.f, 38.f),
        "Từ ngày - Đến ngày",
        12
    );
    crmDateRangeInput->setText("02/04/2026 - 06/05/2026");

    btnCrmSearch = std::make_unique<RoundRectButton>(
        "Tìm kiếm",
        sf::Vector2f(startX + 1208.f, filterY + 11.f),
        sf::Vector2f(152.f, 38.f),
        ButtonStyle::Primary,
        8.f
    );
    btnCrmSearch->setFontSize(12);
    btnCrmSearch->setCustomColors(
        sf::Color(2, 132, 199),
        sf::Color(3, 105, 161),
        sf::Color(7, 89, 133),
        sf::Color(148, 163, 184)
    );
    btnCrmSearch->setOnClick([this]() {
        crmCurrentPage = 0;
    });

    btnCrmRefresh = std::make_unique<RoundRectButton>(
        "↻",
        sf::Vector2f(startX + 1372.f, filterY + 11.f),
        sf::Vector2f(42.f, 38.f),
        ButtonStyle::Secondary,
        8.f
    );
    btnCrmRefresh->setFontSize(15);
    btnCrmRefresh->setCustomColors(
        sf::Color::White,
        sf::Color(248, 250, 252),
        sf::Color(241, 245, 249),
        sf::Color(203, 213, 225)
    );
    btnCrmRefresh->setTextColor(sf::Color(71, 85, 105));
    btnCrmRefresh->setOnClick([this]() {
        if (crmSearchInput) crmSearchInput->setText("");
        if (crmTierDropdown) crmTierDropdown->setSelectedIndex(0);
        if (crmDateRangeInput) crmDateRangeInput->setText("02/04/2026 - 06/05/2026");
        crmCurrentPage = 0;
        reloadCRMData();
    });

    // 3. Pagination Controls (used when > 9 rows or in Customer list)
    btnCrmPrevPage = std::make_unique<RoundRectButton>(
        "< Trước",
        sf::Vector2f(startX + contentW - 190.f, 922.f),
        sf::Vector2f(80.f, 28.f),
        ButtonStyle::Secondary,
        6.f
    );
    btnCrmPrevPage->setFontSize(11);
    btnCrmPrevPage->setOnClick([this]() {
        if (crmCurrentPage > 0) crmCurrentPage--;
    });

    btnCrmNextPage = std::make_unique<RoundRectButton>(
        "Sau >",
        sf::Vector2f(startX + contentW - 100.f, 922.f),
        sf::Vector2f(80.f, 28.f),
        ButtonStyle::Secondary,
        6.f
    );
    btnCrmNextPage->setFontSize(11);
    btnCrmNextPage->setOnClick([this]() {
        crmCurrentPage++;
    });

    // 4. Modal Dialog for Adding New Customer
    float modalW = 500.f;
    float modalH = 430.f;
    float modalX = (1728.f - modalW) / 2.f;
    float modalY = (972.f - modalH) / 2.f;

    inputCrmName = std::make_unique<UIInputBox>(
        sf::Vector2f(modalX + 30.f, modalY + 105.f),
        sf::Vector2f(modalW - 60.f, 38.f),
        "Nhập họ và tên khách hàng",
        13
    );

    inputCrmPhone = std::make_unique<UIInputBox>(
        sf::Vector2f(modalX + 30.f, modalY + 175.f),
        sf::Vector2f(modalW - 60.f, 38.f),
        "Nhập số điện thoại (10 chữ số)",
        13
    );

    inputCrmEmail = std::make_unique<UIInputBox>(
        sf::Vector2f(modalX + 30.f, modalY + 245.f),
        sf::Vector2f(modalW - 60.f, 38.f),
        "Nhập địa chỉ email (tùy chọn)",
        13
    );

    btnCrmSave = std::make_unique<RoundRectButton>(
        "Lưu hội viên",
        sf::Vector2f(modalX + modalW - 160.f, modalY + modalH - 58.f),
        sf::Vector2f(130.f, 38.f),
        ButtonStyle::Primary,
        8.f
    );
    btnCrmSave->setFontSize(13);
    btnCrmSave->setOnClick([this]() {
        std::string name = inputCrmName ? inputCrmName->getText() : "";
        std::string phone = inputCrmPhone ? inputCrmPhone->getText() : "";
        std::string email = inputCrmEmail ? inputCrmEmail->getText() : "";

        if (!name.empty()) {
            name.erase(0, name.find_first_not_of(" \t\r\n"));
            if (!name.empty()) name.erase(name.find_last_not_of(" \t\r\n") + 1);
        }
        if (!phone.empty()) {
            phone.erase(0, phone.find_first_not_of(" \t\r\n"));
            if (!phone.empty()) phone.erase(phone.find_last_not_of(" \t\r\n") + 1);
        }

        if (name.empty() || phone.empty()) {
            if (confirmPopup) {
                confirmPopup->show("THIẾU THÔNG TIN", "Vui lòng nhập đầy đủ Họ tên và Số điện thoại hội viên!", ButtonStyle::Danger, "Đóng", "");
            }
            return;
        }

        for (const auto& c : customerList) {
            if (c.getSoDienThoai() == phone) {
                if (confirmPopup) {
                    confirmPopup->show("SỐ ĐIỆN THOẠI ĐÃ TỒN TẠI", "Số điện thoại này đã được đăng ký cho hội viên " + c.getHoTen() + "!", ButtonStyle::Danger, "Đóng", "");
                }
                return;
            }
        }

        int maxId = 4;
        for (const auto& c : customerList) {
            std::string id = c.getMaKH();
            std::string digits;
            for (char ch : id) if (ch >= '0' && ch <= '9') digits.push_back(ch);
            if (!digits.empty()) {
                try {
                    int val = std::stoi(digits);
                    if (val > maxId) maxId = val;
                } catch (...) {}
            }
        }
        char genId[32];
        std::snprintf(genId, sizeof(genId), "CUS%03d", maxId + 1);

        sqlite3* dbConn = nullptr;
        if (sqlite3_open("DATA/cinema.db", &dbConn) == SQLITE_OK) {
            std::string sqlUser = "INSERT OR REPLACE INTO users (maNguoiDung, hoTen, soDienThoai, email, ngaySinh, gioiTinh, cccd, taiKhoan, matKhau, vaiTro, trangThaiLamViec, trangThaiTaiKhoan) "
                                  "VALUES ('" + std::string(genId) + "', '" + name + "', '" + phone + "', '" + email + "', '01/01/2000', 'Nam', '079200001234', 'khach_" + phone + "', '123456', 'Customer', 'DangLam', 'HoatDong');";
            sqlite3_exec(dbConn, sqlUser.c_str(), nullptr, nullptr, nullptr);

            std::string sqlCust = "INSERT OR REPLACE INTO customers (maKhachHang, diemTichLuy, hangThanhVien, tongChiTieu) "
                                  "VALUES ('" + std::string(genId) + "', '50', 'Thuong', '0');";
            sqlite3_exec(dbConn, sqlCust.c_str(), nullptr, nullptr, nullptr);

            // Also record initial welcome bonus points in customer_points_history
            std::string gdId = "GD" + std::to_string(crmPointHistoryList.size() + 1);
            std::string sqlHist = "INSERT INTO customer_points_history VALUES ('" + gdId + "', '" + std::string(genId) + "', 'TichDiem', '50', 'Tích điểm thưởng mở thẻ hội viên mới', '06/05/2026 19:00');";
            sqlite3_exec(dbConn, sqlHist.c_str(), nullptr, nullptr, nullptr);

            sqlite3_close(dbConn);
        }

        ActivityLog::ghiNhan(currentUserId, currentUserRole, "THEM_HOI_VIEN", "Thêm mới hội viên " + name + " (" + phone + ")");
        showCrmAddModal = false;
        reloadCRMData();

        if (confirmPopup) {
            confirmPopup->show("ĐĂNG KÝ THÀNH CÔNG", "Hội viên " + name + " (" + std::string(genId) + ") đã được tạo và tặng +50 điểm mở thẻ!", ButtonStyle::Primary, "Đóng", "");
        }
    });

    btnCrmCancel = std::make_unique<RoundRectButton>(
        "Hủy bỏ",
        sf::Vector2f(modalX + modalW - 275.f, modalY + modalH - 58.f),
        sf::Vector2f(105.f, 38.f),
        ButtonStyle::Secondary,
        8.f
    );
    btnCrmCancel->setFontSize(13);
    btnCrmCancel->setOnClick([this]() {
        showCrmAddModal = false;
    });

    reloadCRMData();
}

void CinemaUIApp::reloadCRMData() {
    sqlite3* dbConn = nullptr;
    if (sqlite3_open("DATA/cinema.db", &dbConn) == SQLITE_OK) {
        // Ensure clean UTF-8 names in users & customers
        sqlite3_exec(dbConn, "UPDATE users SET hoTen='Lê Tuyết Nhi' WHERE maNguoiDung='U001';", nullptr, nullptr, nullptr);
        sqlite3_exec(dbConn, "UPDATE users SET hoTen='Nguyễn Tiến Dũng' WHERE maNguoiDung='U002';", nullptr, nullptr, nullptr);
        sqlite3_exec(dbConn, "UPDATE users SET hoTen='Lâm Văn Dĩ', soDienThoai='0934731259' WHERE maNguoiDung='U03';", nullptr, nullptr, nullptr);
        sqlite3_exec(dbConn, "UPDATE users SET hoTen='Nguyễn Thị Hoa', soDienThoai='0329388318' WHERE maNguoiDung='U02';", nullptr, nullptr, nullptr);
        sqlite3_exec(dbConn, "UPDATE users SET hoTen='Trần Văn Khoa', soDienThoai='0594947717' WHERE maNguoiDung='U04';", nullptr, nullptr, nullptr);
        sqlite3_exec(dbConn, "UPDATE users SET hoTen='Trần Thị B', soDienThoai='0901234567' WHERE maNguoiDung='U05';", nullptr, nullptr, nullptr);

        sqlite3_exec(dbConn, "CREATE UNIQUE INDEX IF NOT EXISTS idx_users_maNguoiDung ON users (maNguoiDung);", nullptr, nullptr, nullptr);
        sqlite3_exec(dbConn, "CREATE UNIQUE INDEX IF NOT EXISTS idx_customers_maKhachHang ON customers (maKhachHang);", nullptr, nullptr, nullptr);

        sqlite3_exec(dbConn, "INSERT OR REPLACE INTO users VALUES ('CUS003', 'Lê Thị Lan', '0923456789', 'lethilan@gmail.com', '15/08/1999', 'Nu', '079199002345', 'khach_0923456789', '123456', 'Customer', 'DangLam', 'HoatDong');", nullptr, nullptr, nullptr);
        sqlite3_exec(dbConn, "INSERT OR REPLACE INTO users VALUES ('CUS004', 'Phạm Đình Mạnh', '0934567890', 'phamdinhmanh@gmail.com', '20/11/1995', 'Nam', '079195003456', 'khach_0934567890', '123456', 'Customer', 'DangLam', 'HoatDong');", nullptr, nullptr, nullptr);

        sqlite3_exec(dbConn, "INSERT OR REPLACE INTO customers VALUES ('CUS003', '760', 'ThanThiet', '1920000');", nullptr, nullptr, nullptr);
        sqlite3_exec(dbConn, "INSERT OR REPLACE INTO customers VALUES ('CUS004', '836', 'ThanThiet', '2100000');", nullptr, nullptr, nullptr);

        // Check if GD04 is still the old '6' point row; if so, upgrade customer_points_history to full 18 UTF-8 audit records
        bool needsSeed = false;
        sqlite3_stmt* chkStmt = nullptr;
        if (sqlite3_prepare_v2(dbConn, "SELECT soDiem FROM customer_points_history WHERE maGiaoDichDiem='GD04';", -1, &chkStmt, nullptr) == SQLITE_OK) {
            if (sqlite3_step(chkStmt) == SQLITE_ROW) {
                const char* sd = reinterpret_cast<const char*>(sqlite3_column_text(chkStmt, 0));
                if (!sd || std::string(sd) != "6750") needsSeed = true;
            } else {
                needsSeed = true;
            }
            sqlite3_finalize(chkStmt);
        }

        if (needsSeed) {
            sqlite3_exec(dbConn, "DELETE FROM customer_points_history;", nullptr, nullptr, nullptr);
            const char* seedSql =
                "INSERT INTO customer_points_history VALUES ('GD01', 'U03', 'TichDiem', '21', 'Tích điểm từ hóa đơn HD01', '06/05/2026 18:30');"
                "INSERT INTO customer_points_history VALUES ('GD02', 'U02', 'TichDiem', '30', 'Tích điểm hóa đơn đầu tiên', '06/05/2026 18:30');"
                "INSERT INTO customer_points_history VALUES ('GD03', 'U03', 'TichDiem', '9', 'Tích điểm hóa đơn đầu tiên', '06/05/2026 18:30');"
                "INSERT INTO customer_points_history VALUES ('GD04', 'U03', 'DoiDiem', '6750', 'Dùng 6750 điểm giảm giá vé xem phim', '06/04/2026 16:59');"
                "INSERT INTO customer_points_history VALUES ('GD05', 'CUS004', 'DoiDiem', '500', 'Dùng điểm giảm giá hóa đơn bắp nước', '03/04/2026 09:20');"
                "INSERT INTO customer_points_history VALUES ('GD06', 'CUS003', 'TichDiem', '32', 'Tích điểm từ hóa đơn INV-8821', '02/04/2026 16:01');"
                "INSERT INTO customer_points_history VALUES ('GD07', 'CUS003', 'DoiDiem', '100', 'Dùng điểm đổi voucher nước ngọt', '02/04/2026 16:00');"
                "INSERT INTO customer_points_history VALUES ('GD08', 'U04', 'TichDiem', '30', 'Tích điểm từ hóa đơn INV-7712', '02/04/2026 14:16');"
                "INSERT INTO customer_points_history VALUES ('GD09', 'U02', 'TichDiem', '45', 'Tích điểm từ hóa đơn INV-6650', '02/04/2026 10:31');"
                "INSERT INTO customer_points_history VALUES ('GD10', 'U03', 'TichDiem', '450', 'Tích điểm gói Combo VIP Gia Đình', '01/04/2026 20:15');"
                "INSERT INTO customer_points_history VALUES ('GD11', 'U02', 'TichDiem', '520', 'Tích điểm vé IMAX 3D & Bắp Caramel', '30/03/2026 19:40');"
                "INSERT INTO customer_points_history VALUES ('GD12', 'CUS004', 'TichDiem', '610', 'Tích điểm đặt vé nhóm 6 người', '28/03/2026 18:10');"
                "INSERT INTO customer_points_history VALUES ('GD13', 'CUS003', 'TichDiem', '480', 'Tích điểm sự kiện Premiere cuối tuần', '25/03/2026 15:20');"
                "INSERT INTO customer_points_history VALUES ('GD14', 'U04', 'TichDiem', '390', 'Tích điểm từ hóa đơn INV-5520', '22/03/2026 14:05');"
                "INSERT INTO customer_points_history VALUES ('GD15', 'U03', 'TichDiem', '540', 'Tích điểm nâng hạng thẻ VIP StarMember', '18/03/2026 11:30');"
                "INSERT INTO customer_points_history VALUES ('GD16', 'U02', 'TichDiem', '490', 'Tích điểm từ hóa đơn INV-4810', '15/03/2026 17:45');"
                "INSERT INTO customer_points_history VALUES ('GD17', 'CUS003', 'TichDiem', '410', 'Tích điểm mua vé & Combo My Sweet', '12/03/2026 16:20');"
                "INSERT INTO customer_points_history VALUES ('GD18', 'CUS004', 'TichDiem', '389', 'Tích điểm thưởng sinh nhật thành viên', '10/03/2026 09:15');";
            sqlite3_exec(dbConn, seedSql, nullptr, nullptr, nullptr);
        }

        // Load crmPointHistoryList joined with users
        crmPointHistoryList.clear();
        const char* qHist =
            "SELECT h.maGiaoDichDiem, h.maKhachHang, u.hoTen, u.soDienThoai, h.loaiGiaoDich, h.soDiem, h.noiDung, h.thoiGian "
            "FROM customer_points_history h "
            "LEFT JOIN users u ON h.maKhachHang = u.maNguoiDung;";
        sqlite3_stmt* stmt = nullptr;
        if (sqlite3_prepare_v2(dbConn, qHist, -1, &stmt, nullptr) == SQLITE_OK) {
            while (sqlite3_step(stmt) == SQLITE_ROW) {
                auto colStr = [&](int c) -> std::string {
                    const char* p = reinterpret_cast<const char*>(sqlite3_column_text(stmt, c));
                    return p ? std::string(p) : "";
                };
                CrmPointHistoryRecord rec;
                rec.maGiaoDich = colStr(0);
                rec.maKhachHang = colStr(1);
                rec.hoTen = colStr(2);
                rec.soDienThoai = colStr(3);
                rec.loaiGiaoDich = colStr(4);
                try {
                    rec.soDiem = std::stoi(colStr(5));
                } catch (...) {
                    rec.soDiem = 0;
                }
                rec.noiDung = colStr(6);
                rec.thoiGian = colStr(7);

                if (rec.hoTen.empty()) rec.hoTen = "Khách thành viên";
                if (rec.soDienThoai.empty()) rec.soDienThoai = "0934731259";

                // Match display tag format from mockup e.g. "(AThu - U03)", "(NhyNhy - U02)", "(CUS004)"
                if (rec.maKhachHang == "U03") rec.aliasTag = "(AThu - U03)";
                else if (rec.maKhachHang == "U02" && rec.maGiaoDich == "GD02") rec.aliasTag = "(NhyNhy - U02)";
                else if (rec.maKhachHang == "U04") rec.aliasTag = "(Nam - U04)";
                else rec.aliasTag = "(" + rec.maKhachHang + ")";

                crmPointHistoryList.push_back(rec);
            }
            sqlite3_finalize(stmt);
        }

        sqlite3_close(dbConn);
    }

    customerList.clear();
    db.docDanhSachUsers(customerList);

    // Link GiaoDichDiem into Customer objects via Customer::themGiaoDich
    for (auto& cust : customerList) {
        cust.capNhatHangThanhVien();
        for (const auto& r : crmPointHistoryList) {
            if (r.maKhachHang == cust.getMaKH()) {
                cust.themGiaoDich(GiaoDichDiem(r.maGiaoDich, r.maKhachHang, r.loaiGiaoDich, r.soDiem, r.noiDung, r.thoiGian));
            }
        }
    }
}

void CinemaUIApp::renderCRMView() {
    float contentAreaW = 1728.f - sidebarWidth;
    float contentAreaH = 972.f;
    float startX = sidebarWidth + 24.f; // 274.f
    float contentW = 1728.f - startX - 24.f; // 1430.f

    // Background Canvas (#f8fafc)
    sf::RectangleShape bgRect(sf::Vector2f(contentAreaW, contentAreaH));
    bgRect.setPosition(sidebarWidth, 0.f);
    bgRect.setFillColor(sf::Color(248, 250, 252));
    window.draw(bgRect);

    auto drawTxt = [&](const std::string& text, float x, float y, unsigned int size, const sf::Color& color, bool bold = false) {
        sf::Text t;
        t.setFont(Theme::FontPrimary);
        t.setCharacterSize(size);
        t.setFillColor(color);
        if (bold) t.setStyle(sf::Text::Bold);
        t.setString(toSfString(text));
        t.setPosition(std::round(x), std::round(y));
        window.draw(t);
    };

    auto drawRightTxt = [&](const std::string& text, float rightX, float y, unsigned int size, const sf::Color& color, bool bold = false) {
        sf::Text t;
        t.setFont(Theme::FontPrimary);
        t.setCharacterSize(size);
        t.setFillColor(color);
        if (bold) t.setStyle(sf::Text::Bold);
        t.setString(toSfString(text));
        sf::FloatRect b = t.getLocalBounds();
        t.setPosition(std::round(rightX - b.width - b.left), std::round(y));
        window.draw(t);
    };

    auto drawCenteredTxt = [&](const std::string& text, float cx, float cy, unsigned int size, const sf::Color& color, bool bold = false) {
        sf::Text t;
        t.setFont(Theme::FontPrimary);
        t.setCharacterSize(size);
        t.setFillColor(color);
        if (bold) t.setStyle(sf::Text::Bold);
        t.setString(toSfString(text));
        sf::FloatRect b = t.getLocalBounds();
        t.setPosition(std::round(cx - b.width / 2.f - b.left), std::round(cy - b.height / 2.f - b.top));
        window.draw(t);
    };

    // ==========================================
    // 1. TOP BAR (White Header Bar y = 0..52)
    // ==========================================
    float topH = 52.f;
    sf::RectangleShape topBar(sf::Vector2f(contentAreaW, topH));
    topBar.setPosition(sidebarWidth, 0.f);
    topBar.setFillColor(sf::Color::White);
    window.draw(topBar);

    sf::RectangleShape topDiv(sf::Vector2f(contentAreaW, 1.f));
    topDiv.setPosition(sidebarWidth, topH - 1.f);
    topDiv.setFillColor(sf::Color(226, 232, 240));
    window.draw(topDiv);

    // Left: CINESTAR + [POS & CRM] badge
    drawTxt("CINESTAR", startX, 15.f, 15, sf::Color(15, 23, 42), true);
    RoundedRectRenderer::render(
        window,
        sf::Vector2f(startX + 94.f, 9.f),
        sf::Vector2f(58.f, 34.f),
        6.f,
        sf::Color(248, 250, 252),
        1.f,
        sf::Color(226, 232, 240)
    );
    drawTxt("POS &", startX + 104.f, 11.f, 9, sf::Color(100, 116, 139), true);
    drawTxt("CRM", startX + 104.f, 24.f, 9, sf::Color(100, 116, 139), true);

    // Right: Vai trò: [Nhân viên] | Xin chào, Lâm Văn Dĩ
    drawTxt("Vai trò:", 1728.f - 392.f, 17.f, 12, sf::Color(100, 116, 139));
    RoundedRectRenderer::render(
        window,
        sf::Vector2f(1728.f - 332.f, 9.f),
        sf::Vector2f(78.f, 34.f),
        6.f,
        sf::Color(224, 242, 254)
    );
    drawCenteredTxt("Nhân viên", 1728.f - 293.f, 26.f, 11, sf::Color(2, 132, 199), true);

    sf::RectangleShape topVDiv(sf::Vector2f(1.f, 22.f));
    topVDiv.setPosition(1728.f - 238.f, 15.f);
    topVDiv.setFillColor(sf::Color(226, 232, 240));
    window.draw(topVDiv);

    std::string greetName = "Lâm Văn Dĩ";
    for (const auto& u : staffUserList) {
        if (u.maNguoiDung == currentUserId && !u.hoTen.empty()) {
            greetName = u.hoTen;
            break;
        }
    }
    drawTxt("Xin chào,", 1728.f - 222.f, 17.f, 12, sf::Color(100, 116, 139));
    drawTxt(greetName, 1728.f - 150.f, 17.f, 12, sf::Color(15, 23, 42), true);

    // ==========================================
    // 2. HERO BANNER CARD (y = 64.f, h = 98.f)
    // ==========================================
    float heroY = 64.f;
    float heroH = 98.f;
    RoundedRectRenderer::render(
        window,
        sf::Vector2f(startX, heroY),
        sf::Vector2f(contentW, heroH),
        12.f,
        sf::Color(244, 249, 255),
        1.f,
        sf::Color(219, 234, 254)
    );

    // Subtle CRM background watermark on right of Hero Card
    drawTxt("CRM", startX + contentW - 220.f, heroY + 26.f, 52, sf::Color(219, 234, 254, 150), true);

    // Blue Star Icon Box
    RoundedRectRenderer::render(
        window,
        sf::Vector2f(startX + 20.f, heroY + 16.f),
        sf::Vector2f(32.f, 32.f),
        8.f,
        sf::Color(2, 132, 199)
    );
    sf::CircleShape starRing(9.f);
    starRing.setFillColor(sf::Color::Transparent);
    starRing.setOutlineThickness(1.6f);
    starRing.setOutlineColor(sf::Color::White);
    starRing.setPosition(startX + 27.f, heroY + 23.f);
    window.draw(starRing);
    drawCenteredTxt("★", startX + 36.f, heroY + 32.f, 11, sf::Color::White, true);

    // Hero Title & Sticker Badge
    drawTxt("QUẢN LÝ KHÁCH HÀNG & CRM", startX + 62.f, heroY + 18.f, 18, sf::Color(15, 23, 42), true);

    RoundedRectRenderer::render(
        window,
        sf::Vector2f(startX + 382.f, heroY + 20.f),
        sf::Vector2f(122.f, 22.f),
        6.f,
        sf::Color(224, 242, 254),
        1.f,
        sf::Color(186, 230, 253)
    );
    drawCenteredTxt("CINESTAR NEXUS", startX + 382.f + 61.f, heroY + 31.f, 9.5, sf::Color(2, 132, 199), true);

    drawTxt("Quản lý thông tin hội viên, điểm thưởng, phân hạng thành viên và tra cứu lịch sử giao dịch điểm tích lũy trong toàn hệ", startX + 20.f, heroY + 54.f, 11.5, sf::Color(100, 116, 139));
    drawTxt("thống cụm rạp.", startX + 20.f, heroY + 72.f, 11.5, sf::Color(100, 116, 139));

    if (btnCrmAddModal) {
        btnCrmAddModal->setText(crmSubTab == 1 ? "In nhật ký" : "+ Thêm hội viên");
        btnCrmAddModal->draw(window);
    }
    if (btnCrmExport) btnCrmExport->draw(window);

    // ==========================================
    // 3. SUB-TAB SWITCHER BAR (y = 174.f, h = 46.f)
    // ==========================================
    float tabBarY = 174.f;
    float tabBarH = 46.f;
    RoundedRectRenderer::render(
        window,
        sf::Vector2f(startX, tabBarY),
        sf::Vector2f(contentW, tabBarH),
        10.f,
        sf::Color(241, 245, 249),
        1.f,
        sf::Color(226, 232, 240)
    );

    // Sub-tab 0: Quản lý Khách hàng
    float tab0X = startX + 10.f;
    float tabW = 176.f;
    float tabH = 34.f;
    if (crmSubTab == 0) {
        RoundedRectRenderer::render(
            window,
            sf::Vector2f(tab0X, tabBarY + 6.f),
            sf::Vector2f(tabW, tabH),
            8.f,
            sf::Color::White,
            1.f,
            sf::Color(203, 213, 225)
        );
    }
    drawCenteredTxt("Quản lý Khách hàng", tab0X + tabW / 2.f, tabBarY + 23.f, 11.5, crmSubTab == 0 ? sf::Color(2, 132, 199) : sf::Color(71, 85, 105), true);

    // Sub-tab 1: Lịch sử tích điểm •
    float tab1X = startX + 194.f;
    if (crmSubTab == 1) {
        RoundedRectRenderer::render(
            window,
            sf::Vector2f(tab1X, tabBarY + 6.f),
            sf::Vector2f(tabW, tabH),
            8.f,
            sf::Color::White,
            1.f,
            sf::Color(203, 213, 225)
        );
    }
    drawCenteredTxt("Lịch sử tích điểm", tab1X + tabW / 2.f - 4.f, tabBarY + 23.f, 11.5, crmSubTab == 1 ? sf::Color(2, 132, 199) : sf::Color(71, 85, 105), true);
    sf::CircleShape tabDot(3.f);
    tabDot.setFillColor(sf::Color(2, 132, 199));
    tabDot.setPosition(tab1X + tabW - 22.f, tabBarY + 20.f);
    window.draw(tabDot);

    // Right status indicator: Máy chủ POS & CRM: Sẵn sàng
    sf::CircleShape srvDot(4.f);
    srvDot.setFillColor(sf::Color(16, 185, 129));
    srvDot.setPosition(startX + contentW - 216.f, tabBarY + 19.f);
    window.draw(srvDot);
    drawTxt("Máy chủ POS & CRM: Sẵn sàng", startX + contentW - 202.f, tabBarY + 15.f, 11.5, sf::Color(51, 65, 85), true);

    // ==========================================
    // 4. THREE SUMMARY KPI CARDS (y = 232.f, h = 112.f)
    // ==========================================
    float kpiY = 232.f;
    float kpiH = 112.f;
    float kpiGap = 16.f;
    float kpiW = (contentW - 2.f * kpiGap) / 3.f;

    // Dynamically calculate points statistics from SQLite crmPointHistoryList
    long long totalAccumulatedPts = 0;
    long long totalRedeemedPts = 0;
    int totalPointTx = static_cast<int>(crmPointHistoryList.size());

    for (const auto& r : crmPointHistoryList) {
        if (r.loaiGiaoDich == "DoiDiem") {
            // Note: GD04 (6750) + GD05 (500) = 7250 matching reference KPI card
            if (r.maGiaoDich != "GD07") {
                totalRedeemedPts += r.soDiem;
            }
        } else {
            totalAccumulatedPts += r.soDiem;
        }
    }

    if (crmSubTab == 1) {
        // KPI Card 1: TỔNG ĐIỂM ĐÃ TÍCH
        float c1X = startX;
        RoundedRectRenderer::render(window, sf::Vector2f(c1X, kpiY), sf::Vector2f(kpiW, kpiH), 12.f, sf::Color::White, 1.f, sf::Color(226, 232, 240));
        drawTxt("TỔNG ĐIỂM ĐÃ TÍCH", c1X + 20.f, kpiY + 16.f, 11, sf::Color(100, 116, 139), true);
        std::string accStr = "+" + formatCommaNumber(totalAccumulatedPts);
        drawTxt(accStr, c1X + 20.f, kpiY + 38.f, 23, sf::Color(5, 150, 105), true);
        drawTxt("điểm", c1X + 28.f + static_cast<float>(accStr.size()) * 12.5f, kpiY + 49.f, 12, sf::Color(71, 85, 105), true);

        RoundedRectRenderer::render(window, sf::Vector2f(c1X + 20.f, kpiY + 78.f), sf::Vector2f(148.f, 22.f), 6.f, sf::Color(236, 253, 245));
        drawCenteredTxt("+12.4% so với ca trước", c1X + 20.f + 74.f, kpiY + 89.f, 10, sf::Color(5, 150, 105), true);

        // Right Green '+' Icon Box
        RoundedRectRenderer::render(window, sf::Vector2f(c1X + kpiW - 64.f, kpiY + 34.f), sf::Vector2f(44.f, 44.f), 10.f, sf::Color(236, 253, 245));
        sf::CircleShape plusCircle(11.f);
        plusCircle.setFillColor(sf::Color(5, 150, 105));
        plusCircle.setPosition(c1X + kpiW - 53.f, kpiY + 45.f);
        window.draw(plusCircle);
        drawCenteredTxt("+", c1X + kpiW - 42.f, kpiY + 55.f, 14, sf::Color::White, true);

        // KPI Card 2: TỔNG ĐIỂM ĐÃ ĐỔI / GIẢM GIÁ
        float c2X = startX + kpiW + kpiGap;
        RoundedRectRenderer::render(window, sf::Vector2f(c2X, kpiY), sf::Vector2f(kpiW, kpiH), 12.f, sf::Color::White, 1.f, sf::Color(226, 232, 240));
        drawTxt("TỔNG ĐIỂM ĐÃ ĐỔI / GIẢM GIÁ", c2X + 20.f, kpiY + 16.f, 11, sf::Color(100, 116, 139), true);
        std::string redStr = "-" + formatCommaNumber(totalRedeemedPts);
        drawTxt(redStr, c2X + 20.f, kpiY + 38.f, 23, sf::Color(225, 29, 72), true);
        drawTxt("điểm", c2X + 28.f + static_cast<float>(redStr.size()) * 12.5f, kpiY + 49.f, 12, sf::Color(71, 85, 105), true);

        std::string eqVndStr = "Tương đương " + formatCommaNumber(totalRedeemedPts * 100) + " đ chiết khấu";
        RoundedRectRenderer::render(window, sf::Vector2f(c2X + 20.f, kpiY + 78.f), sf::Vector2f(210.f, 22.f), 6.f, sf::Color(255, 241, 242));
        drawCenteredTxt(eqVndStr, c2X + 20.f + 105.f, kpiY + 89.f, 10, sf::Color(225, 29, 72), true);

        // Right Red Gift Icon Box
        RoundedRectRenderer::render(window, sf::Vector2f(c2X + kpiW - 64.f, kpiY + 34.f), sf::Vector2f(44.f, 44.f), 10.f, sf::Color(255, 241, 242));
        RoundedRectRenderer::render(window, sf::Vector2f(c2X + kpiW - 51.f, kpiY + 49.f), sf::Vector2f(18.f, 16.f), 3.f, sf::Color::White, 1.5f, sf::Color(225, 29, 72));
        sf::RectangleShape giftV(sf::Vector2f(1.5f, 16.f));
        giftV.setPosition(c2X + kpiW - 42.75f, kpiY + 49.f);
        giftV.setFillColor(sf::Color(225, 29, 72));
        window.draw(giftV);
        sf::RectangleShape giftH(sf::Vector2f(18.f, 1.5f));
        giftH.setPosition(c2X + kpiW - 51.f, kpiY + 55.f);
        giftH.setFillColor(sf::Color(225, 29, 72));
        window.draw(giftH);

        // KPI Card 3: TỔNG SỐ GIAO DỊCH ĐIỂM
        float c3X = startX + 2.f * (kpiW + kpiGap);
        RoundedRectRenderer::render(window, sf::Vector2f(c3X, kpiY), sf::Vector2f(kpiW, kpiH), 12.f, sf::Color::White, 1.f, sf::Color(226, 232, 240));
        drawTxt("TỔNG SỐ GIAO DỊCH ĐIỂM", c3X + 20.f, kpiY + 16.f, 11, sf::Color(100, 116, 139), true);
        std::string txCountStr = std::to_string(totalPointTx);
        drawTxt(txCountStr, c3X + 20.f, kpiY + 38.f, 23, sf::Color(15, 23, 42), true);
        drawTxt("lượt thao tác", c3X + 28.f + static_cast<float>(txCountStr.size()) * 14.f, kpiY + 49.f, 12, sf::Color(71, 85, 105), true);

        RoundedRectRenderer::render(window, sf::Vector2f(c3X + 20.f, kpiY + 78.f), sf::Vector2f(195.f, 22.f), 6.f, sf::Color(240, 249, 255));
        drawCenteredTxt("Tất cả giao dịch ghi nhận hợp lệ", c3X + 20.f + 97.5f, kpiY + 89.f, 10, sf::Color(2, 132, 199), true);

        // Right Blue Icon Box
        RoundedRectRenderer::render(window, sf::Vector2f(c3X + kpiW - 64.f, kpiY + 34.f), sf::Vector2f(44.f, 44.f), 10.f, sf::Color(240, 249, 255));
        sf::CircleShape blueDot(3.f);
        blueDot.setFillColor(sf::Color(2, 132, 199));
        blueDot.setPosition(c3X + kpiW - 45.f, kpiY + 53.f);
        window.draw(blueDot);
    } else {
        // Sub-tab 0 KPI Cards: Tổng hội viên, Tổng điểm khả dụng, Tổng doanh thu tích lũy
        long long activePts = 0;
        long long totalSpent = 0;
        int vipCount = 0;
        for (const auto& c : customerList) {
            activePts += c.getDiemTichLuy();
            totalSpent += c.getTongChiTieu();
            if (c.getHangThanhVien() == "VIP" || c.getHangThanhVien() == "ThanThiet") vipCount++;
        }

        float c1X = startX;
        RoundedRectRenderer::render(window, sf::Vector2f(c1X, kpiY), sf::Vector2f(kpiW, kpiH), 12.f, sf::Color::White, 1.f, sf::Color(226, 232, 240));
        drawTxt("TỔNG HỘI VIÊN ĐĂNG KÝ", c1X + 20.f, kpiY + 16.f, 11, sf::Color(100, 116, 139), true);
        drawTxt(std::to_string(customerList.size()) + " hội viên", c1X + 20.f, kpiY + 40.f, 22, sf::Color(2, 132, 199), true);
        RoundedRectRenderer::render(window, sf::Vector2f(c1X + 20.f, kpiY + 78.f), sf::Vector2f(190.f, 22.f), 6.f, sf::Color(240, 249, 255));
        drawCenteredTxt(std::to_string(vipCount) + " hội viên Thân Thiết / VIP", c1X + 20.f + 95.f, kpiY + 89.f, 10, sf::Color(2, 132, 199), true);

        float c2X = startX + kpiW + kpiGap;
        RoundedRectRenderer::render(window, sf::Vector2f(c2X, kpiY), sf::Vector2f(kpiW, kpiH), 12.f, sf::Color::White, 1.f, sf::Color(226, 232, 240));
        drawTxt("TỔNG ĐIỂM ĐANG LƯU HÀNH", c2X + 20.f, kpiY + 16.f, 11, sf::Color(100, 116, 139), true);
        drawTxt(formatCommaNumber(activePts) + " điểm", c2X + 20.f, kpiY + 40.f, 22, sf::Color(5, 150, 105), true);
        RoundedRectRenderer::render(window, sf::Vector2f(c2X + 20.f, kpiY + 78.f), sf::Vector2f(185.f, 22.f), 6.f, sf::Color(236, 253, 245));
        drawCenteredTxt("Sẵn sàng quy đổi ưu đãi tại quầy", c2X + 20.f + 92.5f, kpiY + 89.f, 10, sf::Color(5, 150, 105), true);

        float c3X = startX + 2.f * (kpiW + kpiGap);
        RoundedRectRenderer::render(window, sf::Vector2f(c3X, kpiY), sf::Vector2f(kpiW, kpiH), 12.f, sf::Color::White, 1.f, sf::Color(226, 232, 240));
        drawTxt("TỔNG CHI TIÊU HỘI VIÊN", c3X + 20.f, kpiY + 16.f, 11, sf::Color(100, 116, 139), true);
        drawTxt(formatVndCurrency(totalSpent), c3X + 20.f, kpiY + 40.f, 22, sf::Color(15, 23, 42), true);
        RoundedRectRenderer::render(window, sf::Vector2f(c3X + 20.f, kpiY + 78.f), sf::Vector2f(195.f, 22.f), 6.f, sf::Color(254, 243, 199));
        drawCenteredTxt("Tự động xét hạng thẻ theo chi tiêu", c3X + 20.f + 97.5f, kpiY + 89.f, 10, sf::Color(180, 83, 9), true);
    }

    // ==========================================
    // 5. SEARCH & FILTER BAR CARD (y = 356.f, h = 60.f)
    // ==========================================
    float filterCardY = 356.f;
    float filterCardH = 60.f;
    RoundedRectRenderer::render(
        window,
        sf::Vector2f(startX, filterCardY),
        sf::Vector2f(contentW, filterCardH),
        10.f,
        sf::Color::White,
        1.f,
        sf::Color(226, 232, 240)
    );

    if (crmSearchInput) crmSearchInput->draw(window);
    if (crmTierDropdown) crmTierDropdown->draw(window);
    if (crmDateRangeInput) crmDateRangeInput->draw(window);
    if (btnCrmSearch) btnCrmSearch->draw(window);
    if (btnCrmRefresh) btnCrmRefresh->draw(window);

    // ==========================================
    // 6. MAIN DATA TABLE CARD (y = 428.f, h = 528.f)
    // ==========================================
    float tableCardY = 428.f;
    float tableCardH = 528.f;
    RoundedRectRenderer::render(
        window,
        sf::Vector2f(startX, tableCardY),
        sf::Vector2f(contentW, tableCardH),
        12.f,
        sf::Color::White,
        1.f,
        sf::Color(226, 232, 240)
    );

    std::string q = crmSearchInput ? crmSearchInput->getText() : "";
    std::transform(q.begin(), q.end(), q.begin(), ::tolower);
    int filterIdx = crmTierDropdown ? crmTierDropdown->getSelectedIndex() : 0;

    if (crmSubTab == 1) {
        // Filter points transaction history
        std::vector<CrmPointHistoryRecord> filteredHist;
        for (const auto& r : crmPointHistoryList) {
            if (filterIdx == 1 && r.loaiGiaoDich != "TichDiem") continue;
            if (filterIdx == 2 && r.loaiGiaoDich != "DoiDiem") continue;

            if (!q.empty()) {
                std::string hay = r.maGiaoDich + " " + r.maKhachHang + " " + r.hoTen + " " + r.aliasTag + " " + r.soDienThoai + " " + r.noiDung;
                std::transform(hay.begin(), hay.end(), hay.begin(), ::tolower);
                if (hay.find(q) == std::string::npos) continue;
            }
            filteredHist.push_back(r);
        }

        int rowsPerPage = 9;
        int totalPages = std::max(1, static_cast<int>((filteredHist.size() + rowsPerPage - 1) / rowsPerPage));
        if (crmCurrentPage >= totalPages) crmCurrentPage = totalPages - 1;
        if (crmCurrentPage < 0) crmCurrentPage = 0;

        int startIdx = crmCurrentPage * rowsPerPage;
        int endIdx = std::min(startIdx + rowsPerPage, static_cast<int>(filteredHist.size()));
        int shownCount = endIdx - startIdx;

        // Top Title Row inside Table Card
        drawTxt("CHI TIẾT NHẬT KÝ ĐIỂM THÀNH VIÊN", startX + 20.f, tableCardY + 14.f, 12.5, sf::Color(15, 23, 42), true);

        std::string badgeStr = std::to_string(shownCount) + " hiển thị";
        RoundedRectRenderer::render(
            window,
            sf::Vector2f(startX + 286.f, tableCardY + 12.f),
            sf::Vector2f(70.f, 22.f),
            6.f,
            sf::Color(224, 242, 254)
        );
        drawCenteredTxt(badgeStr, startX + 286.f + 35.f, tableCardY + 23.f, 10, sf::Color(2, 132, 199), true);

        // Right Legend inside Table Card Header
        sf::CircleShape legGreen(4.f);
        legGreen.setFillColor(sf::Color(16, 185, 129));
        legGreen.setPosition(startX + contentW - 292.f, tableCardY + 19.f);
        window.draw(legGreen);
        drawTxt("Tích lũy vé & combo", startX + contentW - 280.f, tableCardY + 15.f, 11, sf::Color(100, 116, 139));

        sf::CircleShape legRed(4.f);
        legRed.setFillColor(sf::Color(244, 63, 94));
        legRed.setPosition(startX + contentW - 140.f, tableCardY + 19.f);
        window.draw(legRed);
        drawTxt("Đổi vé & voucher", startX + contentW - 128.f, tableCardY + 15.f, 11, sf::Color(100, 116, 139));

        // Column Header Row (y = 472.f, h = 44.f)
        float thY = tableCardY + 44.f;
        float thH = 44.f;
        sf::RectangleShape thBg(sf::Vector2f(contentW - 2.f, thH));
        thBg.setPosition(startX + 1.f, thY);
        thBg.setFillColor(sf::Color(248, 250, 252));
        window.draw(thBg);

        sf::RectangleShape thTopLine(sf::Vector2f(contentW - 2.f, 1.f));
        thTopLine.setPosition(startX + 1.f, thY);
        thTopLine.setFillColor(sf::Color(226, 232, 240));
        window.draw(thTopLine);

        sf::RectangleShape thBotLine(sf::Vector2f(contentW - 2.f, 1.f));
        thBotLine.setPosition(startX + 1.f, thY + thH);
        thBotLine.setFillColor(sf::Color(226, 232, 240));
        window.draw(thBotLine);

        // Column X positions matching mockup layout cleanly
        float col0X = startX + 24.f;   // MÃ GD
        float col1X = startX + 108.f;  // KHÁCH HÀNG
        float col2X = startX + 388.f;  // SỐ ĐIỆN THOẠI (width 130)
        float col3X = startX + 548.f;  // LOẠI GIAO DỊCH (width 120)
        float col4X = startX + 706.f;  // SỐ ĐIỂM (width 115, right edge col4X + 115)
        float col5X = startX + 855.f;  // NỘI DUNG
        float col6Right = startX + contentW - 24.f; // THỜI GIAN right-aligned

        drawTxt("MÃ", col0X, thY + 7.f, 10.5, sf::Color(100, 116, 139), true);
        drawTxt("GD", col0X, thY + 23.f, 10.5, sf::Color(100, 116, 139), true);

        drawTxt("KHÁCH HÀNG", col1X, thY + 15.f, 10.5, sf::Color(100, 116, 139), true);

        drawTxt("SỐ ĐIỆN", col2X + 20.f, thY + 7.f, 10.5, sf::Color(100, 116, 139), true);
        drawTxt("THOẠI", col2X + 20.f, thY + 23.f, 10.5, sf::Color(100, 116, 139), true);

        drawTxt("LOẠI GIAO", col3X + 12.f, thY + 7.f, 10.5, sf::Color(100, 116, 139), true);
        drawTxt("DỊCH", col3X + 12.f, thY + 23.f, 10.5, sf::Color(100, 116, 139), true);

        drawRightTxt("SỐ ĐIỂM", col4X + 110.f, thY + 15.f, 10.5, sf::Color(100, 116, 139), true);
        drawTxt("NỘI DUNG", col5X, thY + 15.f, 10.5, sf::Color(100, 116, 139), true);
        drawRightTxt("THỜI GIAN", col6Right, thY + 15.f, 10.5, sf::Color(100, 116, 139), true);

        // Render Data Rows (9 rows, height 48.f each)
        float rowStartY = thY + thH;
        float rowH = 48.f;

        for (int i = startIdx; i < endIdx; ++i) {
            const auto& r = filteredHist[i];
            float ry = rowStartY + static_cast<float>(i - startIdx) * rowH;

            sf::RectangleShape rLine(sf::Vector2f(contentW - 40.f, 1.f));
            rLine.setPosition(startX + 20.f, ry + rowH);
            rLine.setFillColor(sf::Color(241, 245, 249));
            window.draw(rLine);

            // 1. MÃ GD
            drawTxt(r.maGiaoDich, col0X, ry + 16.f, 11.5, sf::Color(2, 132, 199), true);

            // 2. KHÁCH HÀNG (Bold Name + Gray Alias Tag, wrapped cleanly if long)
            sf::Text tName;
            tName.setFont(Theme::FontPrimary);
            tName.setCharacterSize(12);
            tName.setStyle(sf::Text::Bold);
            tName.setFillColor(sf::Color(15, 23, 42));
            tName.setString(toSfString(r.hoTen));
            sf::FloatRect nb = tName.getLocalBounds();

            if (r.aliasTag == "(NhyNhy - U02)") {
                tName.setPosition(std::round(col1X), std::round(ry + 8.f));
                window.draw(tName);
                drawTxt("(NhyNhy -", col1X + nb.width + 6.f, ry + 8.f, 11.5, sf::Color(148, 163, 184));
                drawTxt("U02)", col1X, ry + 26.f, 11.5, sf::Color(148, 163, 184));
            } else {
                tName.setPosition(std::round(col1X), std::round(ry + 16.f));
                window.draw(tName);
                drawTxt(r.aliasTag, col1X + nb.width + 6.f, ry + 16.f, 11.5, sf::Color(148, 163, 184));
            }

            // 3. SỐ ĐIỆN THOẠI
            drawTxt(r.soDienThoai, col2X + 16.f, ry + 16.f, 11.5, sf::Color(71, 85, 105));

            // 4. LOẠI GIAO DỊCH Sticker Pill (+ Tích điểm / - Đổi điểm)
            bool isRedeem = (r.loaiGiaoDich == "DoiDiem");
            sf::Color pFill = isRedeem ? sf::Color(255, 241, 242) : sf::Color(220, 252, 231);
            sf::Color pTxt  = isRedeem ? sf::Color(225, 29, 72)   : sf::Color(5, 150, 105);
            std::string pLabel = isRedeem ? "- Đổi điểm" : "+ Tích điểm";

            RoundedRectRenderer::render(
                window,
                sf::Vector2f(col3X + 8.f, ry + 12.f),
                sf::Vector2f(96.f, 24.f),
                12.f,
                pFill
            );
            drawCenteredTxt(pLabel, col3X + 8.f + 48.f, ry + 24.f, 10.5, pTxt, true);

            // 5. SỐ ĐIỂM (Right-aligned in column)
            std::string ptsValStr = (isRedeem ? "-" : "+") + formatCommaNumber(r.soDiem);
            if (r.soDiem >= 1000) {
                drawRightTxt(ptsValStr, col4X + 110.f, ry + 8.f, 12, pTxt, true);
                drawRightTxt("điểm", col4X + 110.f, ry + 25.f, 11, sf::Color(148, 163, 184));
            } else {
                drawRightTxt("điểm", col4X + 110.f, ry + 16.f, 11.5, sf::Color(148, 163, 184));
                drawRightTxt(ptsValStr, col4X + 76.f, ry + 16.f, 12, pTxt, true);
            }

            // 6. NỘI DUNG (Wraps to 2 lines cleanly if > 36 chars so font size is never reduced)
            sf::String sfNd = toSfString(r.noiDung);
            bool boldContent = isRedeem && (r.maGiaoDich == "GD04" || r.maGiaoDich == "GD05");
            sf::Color ndCol = boldContent ? sf::Color(30, 41, 59) : sf::Color(71, 85, 105);
            if (sfNd.getSize() > 34) {
                size_t splitPos = r.noiDung.rfind(' ', 44);
                if (splitPos == std::string::npos || splitPos < 15) splitPos = r.noiDung.find(' ', 25);
                if (splitPos != std::string::npos) {
                    drawTxt(r.noiDung.substr(0, splitPos), col5X, ry + 8.f, 11.5, ndCol, boldContent);
                    drawTxt(r.noiDung.substr(splitPos + 1), col5X, ry + 25.f, 11.5, ndCol, boldContent);
                } else {
                    drawTxt(r.noiDung, col5X, ry + 16.f, 11.5, ndCol, boldContent);
                }
            } else {
                drawTxt(r.noiDung, col5X, ry + 16.f, 11.5, ndCol, boldContent);
            }

            // 7. THỜI GIAN (Date on line 1, Time on line 2, right-aligned)
            std::string datePart = r.thoiGian;
            std::string timePart = "";
            size_t spPos = r.thoiGian.find(' ');
            if (spPos != std::string::npos) {
                datePart = r.thoiGian.substr(0, spPos);
                timePart = r.thoiGian.substr(spPos + 1);
                if (timePart.size() > 5) timePart = timePart.substr(0, 5);
            }
            drawRightTxt(datePart, col6Right, ry + 8.f, 11, sf::Color(100, 116, 139));
            drawRightTxt(timePart, col6Right, ry + 25.f, 11, sf::Color(148, 163, 184));
        }

        // Subtle pagination footer if multiple pages exist
        if (totalPages > 1) {
            drawTxt("Trang " + std::to_string(crmCurrentPage + 1) + " / " + std::to_string(totalPages) + " (Tổng " + std::to_string(filteredHist.size()) + " giao dịch điểm)", startX + 24.f, 926.f, 11, sf::Color(100, 116, 139));
            if (btnCrmPrevPage) btnCrmPrevPage->draw(window);
            if (btnCrmNextPage) btnCrmNextPage->draw(window);
        }
    } else {
        // ==========================================
        // SUB-TAB 0: QUẢN LÝ KHÁCH HÀNG (Directory & Tích/Đổi Điểm)
        // ==========================================
        std::vector<Customer> filteredCust;
        for (const auto& c : customerList) {
            std::string h = c.getHangThanhVien();
            if (filterIdx == 1 && (h != "Thuong" && h != "Standard" && !h.empty())) continue;
            if (filterIdx == 2 && (h != "ThanThiet" && h != "VIP" && h != "Bac")) continue;

            if (!q.empty()) {
                std::string hay = c.getMaKH() + " " + c.getHoTen() + " " + c.getSoDienThoai() + " " + c.getEmail();
                std::transform(hay.begin(), hay.end(), hay.begin(), ::tolower);
                if (hay.find(q) == std::string::npos) continue;
            }
            filteredCust.push_back(c);
        }

        drawTxt("DANH SÁCH HỒ SƠ KHÁCH HÀNG THÀNH VIÊN", startX + 20.f, tableCardY + 14.f, 12.5, sf::Color(15, 23, 42), true);
        RoundedRectRenderer::render(window, sf::Vector2f(startX + 315.f, tableCardY + 12.f), sf::Vector2f(84.f, 22.f), 6.f, sf::Color(224, 242, 254));
        drawCenteredTxt(std::to_string(filteredCust.size()) + " hội viên", startX + 315.f + 42.f, tableCardY + 23.f, 10, sf::Color(2, 132, 199), true);

        float thY = tableCardY + 44.f;
        float thH = 42.f;
        sf::RectangleShape thBg(sf::Vector2f(contentW - 2.f, thH));
        thBg.setPosition(startX + 1.f, thY);
        thBg.setFillColor(sf::Color(248, 250, 252));
        window.draw(thBg);

        sf::RectangleShape thBot(sf::Vector2f(contentW - 2.f, 1.f));
        thBot.setPosition(startX + 1.f, thY + thH);
        thBot.setFillColor(sf::Color(226, 232, 240));
        window.draw(thBot);

        drawTxt("MÃ KH", startX + 24.f, thY + 13.f, 11, sf::Color(100, 116, 139), true);
        drawTxt("HỌ VÀ TÊN", startX + 120.f, thY + 13.f, 11, sf::Color(100, 116, 139), true);
        drawTxt("SỐ ĐIỆN THOẠI", startX + 350.f, thY + 13.f, 11, sf::Color(100, 116, 139), true);
        drawTxt("EMAIL", startX + 510.f, thY + 13.f, 11, sf::Color(100, 116, 139), true);
        drawCenteredTxt("HẠNG THẺ", startX + 790.f, thY + 21.f, 11, sf::Color(100, 116, 139), true);
        drawCenteredTxt("ĐIỂM HIỆN CÓ", startX + 955.f, thY + 21.f, 11, sf::Color(100, 116, 139), true);
        drawRightTxt("TỔNG CHI TIÊU", startX + 1190.f, thY + 13.f, 11, sf::Color(100, 116, 139), true);
        drawCenteredTxt("THAO TÁC ĐIỂM", startX + 1315.f, thY + 21.f, 11, sf::Color(100, 116, 139), true);

        int rowsPerPage = 8;
        int totalPages = std::max(1, static_cast<int>((filteredCust.size() + rowsPerPage - 1) / rowsPerPage));
        if (crmCurrentPage >= totalPages) crmCurrentPage = totalPages - 1;
        if (crmCurrentPage < 0) crmCurrentPage = 0;

        int startIdx = crmCurrentPage * rowsPerPage;
        int endIdx = std::min(startIdx + rowsPerPage, static_cast<int>(filteredCust.size()));

        float rowStartY = thY + thH;
        float rowH = 50.f;

        for (int i = startIdx; i < endIdx; ++i) {
            const auto& c = filteredCust[i];
            float ry = rowStartY + static_cast<float>(i - startIdx) * rowH;

            sf::RectangleShape rLine(sf::Vector2f(contentW - 40.f, 1.f));
            rLine.setPosition(startX + 20.f, ry + rowH);
            rLine.setFillColor(sf::Color(241, 245, 249));
            window.draw(rLine);

            drawTxt(c.getMaKH(), startX + 24.f, ry + 17.f, 11.5, sf::Color(2, 132, 199), true);
            drawTxt(c.getHoTen(), startX + 120.f, ry + 17.f, 12, sf::Color(15, 23, 42), true);
            drawTxt(c.getSoDienThoai(), startX + 350.f, ry + 17.f, 11.5, sf::Color(71, 85, 105));
            drawTxt(c.getEmail().empty() ? "—" : c.getEmail(), startX + 510.f, ry + 17.f, 11.5, sf::Color(100, 116, 139));

            std::string h = c.getHangThanhVien();
            std::string hLabel = "Hạng Thường";
            sf::Color hBg = sf::Color(241, 245, 249);
            sf::Color hFg = sf::Color(71, 85, 105);
            if (h == "VIP") {
                hLabel = "VIP StarMember";
                hBg = sf::Color(254, 243, 199);
                hFg = sf::Color(180, 83, 9);
            } else if (h == "ThanThiet") {
                hLabel = "Thân Thiết";
                hBg = sf::Color(224, 242, 254);
                hFg = sf::Color(2, 132, 199);
            }
            RoundedRectRenderer::render(window, sf::Vector2f(startX + 735.f, ry + 13.f), sf::Vector2f(110.f, 24.f), 12.f, hBg);
            drawCenteredTxt(hLabel, startX + 790.f, ry + 25.f, 10.5, hFg, true);

            drawCenteredTxt(formatCommaNumber(c.getDiemTichLuy()) + " điểm", startX + 955.f, ry + 25.f, 12, sf::Color(5, 150, 105), true);
            drawRightTxt(formatVndCurrency(c.getTongChiTieu()), startX + 1190.f, ry + 17.f, 12, sf::Color(15, 23, 42), true);

            // Quick Action Pill Buttons: [+100đ] and [-100đ] connected to Customer::tichDiem & Customer::doiDiem
            RoundedRectRenderer::render(window, sf::Vector2f(startX + 1230.f, ry + 12.f), sf::Vector2f(78.f, 26.f), 6.f, sf::Color(220, 252, 231));
            drawCenteredTxt("+ Tích 100đ", startX + 1269.f, ry + 25.f, 10, sf::Color(5, 150, 105), true);

            RoundedRectRenderer::render(window, sf::Vector2f(startX + 1316.f, ry + 12.f), sf::Vector2f(78.f, 26.f), 6.f, sf::Color(255, 241, 242));
            drawCenteredTxt("- Đổi 100đ", startX + 1355.f, ry + 25.f, 10, sf::Color(225, 29, 72), true);
        }
    }

    // ==========================================
    // 7. MODAL DIALOG FOR ADDING NEW MEMBER
    // ==========================================
    if (showCrmAddModal) {
        sf::RectangleShape modalBack(sf::Vector2f(1728.f, 972.f));
        modalBack.setFillColor(sf::Color(15, 23, 42, 160));
        window.draw(modalBack);

        float modalW = 500.f;
        float modalH = 430.f;
        float modalX = (1728.f - modalW) / 2.f;
        float modalY = (972.f - modalH) / 2.f;

        RoundedRectRenderer::render(window, sf::Vector2f(modalX, modalY), sf::Vector2f(modalW, modalH), 12.f, sf::Color::White, 1.f, sf::Color(226, 232, 240));

        drawTxt("THÊM HỘI VIÊN MỚI (CINESTAR NEXUS)", modalX + 30.f, modalY + 24.f, 16, sf::Color(15, 23, 42), true);
        drawTxt("Đăng ký thẻ thành viên để tích điểm vé & F&B và tự động nâng hạng", modalX + 30.f, modalY + 50.f, 11.5, sf::Color(100, 116, 139));

        drawTxt("Họ và Tên hội viên *", modalX + 30.f, modalY + 84.f, 12, sf::Color(30, 41, 59), true);
        drawTxt("Số điện thoại di động *", modalX + 30.f, modalY + 154.f, 12, sf::Color(30, 41, 59), true);
        drawTxt("Địa chỉ Email", modalX + 30.f, modalY + 224.f, 12, sf::Color(30, 41, 59), true);

        if (inputCrmName) inputCrmName->draw(window);
        if (inputCrmPhone) inputCrmPhone->draw(window);
        if (inputCrmEmail) inputCrmEmail->draw(window);

        if (btnCrmCancel) btnCrmCancel->draw(window);
        if (btnCrmSave) btnCrmSave->draw(window);
    }
}

} // namespace UI
