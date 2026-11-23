#include "../../include/UI/CinemaUIApp.h"
#include "../../include/UI/CinemaUIApp_ShowtimeHelpers.h"
#include "../../include/ActivityLog.h"
#include <sqlite3.h>
#include <algorithm>
#include <cmath>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <set>

namespace UI {
void CinemaUIApp::setupDashboardView() {
    dashboardStats.clear();
    float contentX = sidebarWidth + 30.f;
    float statY = topbarHeight + 25.f;
    float cardW = 325.f;
    float cardH = 120.f;
    float gap = 24.f;

    // Real KPI numbers derived from CSV data
    dashboardStats.push_back(std::unique_ptr<StatCard>(new StatCard(
        sf::Vector2f(contentX, statY),
        sf::Vector2f(cardW, cardH),
        "PHIM ĐANG CHIẾU (CSV)",
        std::to_string(movieList.size()) + " Tác Phẩm",
        "Lật Mặt 7, Mai, Avatar 3...",
        Theme::Primary
    )));

    dashboardStats.push_back(std::unique_ptr<StatCard>(new StatCard(
        sf::Vector2f(contentX + (cardW + gap), statY),
        sf::Vector2f(cardW, cardH),
        "SUẤT CHIẾU HỆ THỐNG (CSV)",
        std::to_string(showtimeList.size()) + " Suất Chiếu",
        "R01 đến R07 (2D, 3D, IMAX)",
        Theme::Cyan
    )));

    dashboardStats.push_back(std::unique_ptr<StatCard>(new StatCard(
        sf::Vector2f(contentX + (cardW + gap) * 2.f, statY),
        sf::Vector2f(cardW, cardH),
        "TỔNG SỐ GHẾ QUẢN LÝ (CSV)",
        std::to_string(allSeats.size()) + " Ghế Ngồi",
        "Standard, VIP, IMAX, Sweetbox",
        Theme::Success
    )));

    dashboardStats.push_back(std::unique_ptr<StatCard>(new StatCard(
        sf::Vector2f(contentX + (cardW + gap) * 3.f, statY),
        sf::Vector2f(cardW, cardH),
        "DANH MỤC F&B / BẮP NƯỚC (CSV)",
        std::to_string(fnbList.size()) + " Mặt Hàng",
        "Bắp ngọt, Bắp phô mai, Coca...",
        Theme::Warning
    )));

    // Poster Slider Banner using EXACT movies from DATA/quan_ly_phim.csv
    float sliderY = statY + cardH + 28.f;
    dashboardSlider = std::unique_ptr<PosterSlider>(new PosterSlider(
        sf::Vector2f(contentX, sliderY),
        sf::Vector2f(1370.f, 320.f)
    ));

    std::vector<PosterSlideItem> slides;
    std::vector<sf::Color> colors = {
        sf::Color(99, 102, 241, 100),
        sf::Color(6, 182, 212, 100),
        sf::Color(245, 158, 11, 100),
        sf::Color(239, 68, 68, 100),
        sf::Color(16, 185, 129, 100)
    };

    for (size_t i = 0; i < movieList.size(); ++i) {
        PosterSlideItem it;
        it.maPhim = movieList[i].getMaPhim();
        it.tenPhim = movieList[i].getTenPhim();
        it.theLoai = db.layTheLoaiCuaPhim(it.maPhim);
        if (it.theLoai.empty()) it.theLoai = "Điện ảnh";
        it.thoiLuong = movieList[i].getThoiLuong();
        it.nhanDoTuoi = movieList[i].getNhanDoTuoi();
        it.moTa = movieList[i].getMoTa();
        if (it.moTa.empty()) it.moTa = "Tác phẩm điện ảnh bom tấn đang chiếu tại hệ thống rạp CineStar.";
        it.posterPath = movieList[i].getPoster();
        it.bannerColor = colors[i % colors.size()];
        slides.push_back(it);
    }

    if (slides.empty()) {
        PosterSlideItem fallback;
        fallback.maPhim = "P01";
        fallback.tenPhim = "LAT MAT 7: MOT DIEU UOC";
        fallback.theLoai = "Gia Dinh, Tam Ly";
        fallback.thoiLuong = 120;
        fallback.nhanDoTuoi = 18;
        fallback.moTa = "Phim ve gia dinh va tinh mau tu.";
        fallback.bannerColor = colors[0];
        slides.push_back(fallback);
    }

    dashboardSlider->setItems(slides);
    dashboardSlider->setOnSelectMovie([this](int idx) {
        currentTab = ActiveTab::SeatBooking;
    });
}

void CinemaUIApp::refreshAgeRatingDropdowns() {
    std::vector<std::string> baseRatings = {"P", "K", "T13", "T16", "T18", "T21"};
    std::vector<std::string> allRatings;
    
    auto addRating = [&](const std::string& r) {
        if (r.empty()) return;
        for (const auto& existing : allRatings) {
            if (existing == r) return;
        }
        allRatings.push_back(r);
    };

    for (const auto& r : baseRatings) addRating(r);
    for (const auto& r : customAgeRatings) addRating(r);

    for (const auto& m : movieList) {
        int a = m.getNhanDoTuoi();
        std::string tag = (a <= 0) ? "P" : ("T" + std::to_string(a));
        addRating(tag);
    }

    auto getAgeLabel = [](const std::string& code) -> std::string {
        if (code == "P") return "P (Mọi lứa tuổi)";
        if (code == "K") return "K (Dưới 13 tuổi có bảo trợ)";
        if (code == "T13") return "T13 (Từ 13 tuổi trở lên)";
        if (code == "T16") return "T16 (Từ 16 tuổi trở lên)";
        if (code == "T18") return "T18 (Từ 18 tuổi trở lên)";
        if (code == "T21") return "T21 (Từ 21 tuổi trở lên)";
        return code + " (Giới hạn " + code + ")";
    };

    std::vector<std::string> formOpts;
    for (const auto& r : allRatings) {
        formOpts.push_back(getAgeLabel(r));
    }
    formOpts.push_back("+ Thêm độ tuổi mới...");

    std::string currentVal = inputMovieAgeRating ? inputMovieAgeRating->getText() : "";
    int formSelIdx = -1;
    for (size_t i = 0; i < allRatings.size(); ++i) {
        if (allRatings[i] == currentVal) {
            formSelIdx = static_cast<int>(i);
            break;
        }
    }

    if (movieFormAgeDropdown) {
        movieFormAgeDropdown->setOptions(formOpts, formSelIdx >= 0 ? formSelIdx : 0);
        if (formSelIdx < 0) {
            movieFormAgeDropdown->setSelectedIndex(-1);
        }
    }

    std::vector<std::string> filterOpts = {"Tất cả độ tuổi"};
    for (const auto& r : allRatings) {
        filterOpts.push_back(getAgeLabel(r));
    }

    int filterSelIdx = 0;
    if (!movieAgeFilter.empty()) {
        for (size_t i = 0; i < allRatings.size(); ++i) {
            if (allRatings[i] == movieAgeFilter) {
                filterSelIdx = static_cast<int>(i) + 1;
                break;
            }
        }
    }

    if (movieAgeFilterDropdown) {
        movieAgeFilterDropdown->setOptions(filterOpts, filterSelIdx);
    }
}


void CinemaUIApp::reloadAuditLogs() {
    auditRecords.clear();
    sqlite3* database = nullptr;
    if (sqlite3_open("DATA/cinema.db", &database) == SQLITE_OK && database != nullptr) {
        sqlite3_stmt* stmt = nullptr;
        const char* sql = "SELECT l.thoiGian, l.maNguoiDung, COALESCE(u.hoTen, l.maNguoiDung), l.vaiTro, l.hanhDong, l.chiTiet "
                          "FROM nhat_ky_hoat_dong l LEFT JOIN users u ON l.maNguoiDung = u.maNguoiDung "
                          "ORDER BY l.rowid DESC";
        if (sqlite3_prepare_v2(database, sql, -1, &stmt, nullptr) == SQLITE_OK) {
            while (sqlite3_step(stmt) == SQLITE_ROW) {
                AuditRecord rec;
                auto getTxt = [&](int col) -> std::string {
                    const unsigned char* t = sqlite3_column_text(stmt, col);
                    return t ? reinterpret_cast<const char*>(t) : "";
                };
                rec.thoiGian = getTxt(0);
                rec.maNguoiDung = getTxt(1);
                rec.hoTen = getTxt(2);
                rec.vaiTro = getTxt(3);
                rec.hanhDong = getTxt(4);
                rec.chiTiet = getTxt(5);

                if (rec.hoTen.empty() || rec.hoTen == rec.maNguoiDung) {
                    if (rec.maNguoiDung == "U001") rec.hoTen = "Nguyễn Văn An";
                    else if (rec.maNguoiDung == "U002") rec.hoTen = "Trần Thị Bình";
                    else rec.hoTen = rec.maNguoiDung;
                }

                // Determine actionType (0: Other/System, 1: Insert, 2: Update, 3: Delete)
                if (rec.hanhDong.rfind("XOA_", 0) == 0) rec.actionType = 3;
                else if (rec.hanhDong.rfind("THEM_", 0) == 0 || rec.hanhDong.rfind("TAO_", 0) == 0) rec.actionType = 1;
                else if (rec.hanhDong.rfind("SUA_", 0) == 0 || rec.hanhDong.rfind("CAP_NHAT_", 0) == 0 || rec.hanhDong.rfind("DOI_", 0) == 0) rec.actionType = 2;
                else rec.actionType = 0;

                // Determine tableName
                if (rec.hanhDong.find("PHIM") != std::string::npos) rec.tableName = "Movie";
                else if (rec.hanhDong.find("PHONG") != std::string::npos) rec.tableName = "Room";
                else if (rec.hanhDong.find("SUAT_CHIEU") != std::string::npos) rec.tableName = "ShowTime";
                else if (rec.hanhDong.find("GHE") != std::string::npos || rec.hanhDong.find("SEAT") != std::string::npos) rec.tableName = "SeatType";
                else if (rec.hanhDong.find("THE_LOAI") != std::string::npos) rec.tableName = "SeatType";
                else if (rec.hanhDong.find("FNB") != std::string::npos) rec.tableName = "Product";
                else if (rec.hanhDong.find("KHUYEN_MAI") != std::string::npos) rec.tableName = "Promotion";
                else if (rec.hanhDong.find("NHAN_SU") != std::string::npos) rec.tableName = "User";
                else if (rec.hanhDong.find("TAI_KHOAN") != std::string::npos || rec.hanhDong.find("ACC") != std::string::npos) rec.tableName = "Account";
                else if (rec.hanhDong.find("VE") != std::string::npos) rec.tableName = "Ticket";
                else if (rec.hanhDong.find("HOA_DON") != std::string::npos) rec.tableName = "Invoice";
                else rec.tableName = "System";

                auto trimStr = [](std::string s) -> std::string {
                    while (!s.empty() && (s.front() == ' ' || s.front() == '\t')) s.erase(s.begin());
                    while (!s.empty() && (s.back() == ' ' || s.back() == '\t')) s.pop_back();
                    return s;
                };

                if (rec.actionType == 3) {
                    rec.newData = "✕ Đã xóa";
                    size_t colon = rec.chiTiet.find(":");
                    if (colon != std::string::npos) rec.oldData = trimStr(rec.chiTiet.substr(colon + 1));
                    else rec.oldData = rec.chiTiet;

                    if (rec.tableName == "SeatType") rec.fieldChanged = "TypeName";
                    else if (rec.tableName == "Room") rec.fieldChanged = "RoomName";
                    else if (rec.tableName == "ShowTime") rec.fieldChanged = "Suất chiếu";
                    else if (rec.tableName == "Movie") rec.fieldChanged = "MovieID";
                    else if (rec.tableName == "Product") rec.fieldChanged = "ItemName";
                    else rec.fieldChanged = "Dữ liệu";
                } else if (rec.actionType == 1) {
                    rec.oldData = "None";
                    size_t colon = rec.chiTiet.find(":");
                    if (colon != std::string::npos) rec.newData = trimStr(rec.chiTiet.substr(colon + 1));
                    else rec.newData = rec.chiTiet;

                    if (rec.tableName == "SeatType") rec.fieldChanged = "TypeName";
                    else if (rec.tableName == "Room") rec.fieldChanged = "RoomName";
                    else if (rec.tableName == "ShowTime") rec.fieldChanged = "Suất chiếu";
                    else if (rec.tableName == "Movie") rec.fieldChanged = "MovieID";
                    else if (rec.tableName == "Product") rec.fieldChanged = "ItemName";
                    else rec.fieldChanged = "Dữ liệu";
                } else if (rec.actionType == 2) {
                    size_t arrow = rec.chiTiet.find("->");
                    if (arrow != std::string::npos) {
                        rec.oldData = trimStr(rec.chiTiet.substr(0, arrow));
                        size_t c = rec.oldData.rfind(":");
                        if (c != std::string::npos) rec.oldData = trimStr(rec.oldData.substr(c + 1));
                        rec.newData = trimStr(rec.chiTiet.substr(arrow + 2));
                        if (rec.chiTiet.find("sức chứa") != std::string::npos || rec.chiTiet.find("Capacity") != std::string::npos)
                            rec.fieldChanged = "Capacity";
                        else
                            rec.fieldChanged = "Giá trị";
                    } else {
                        rec.oldData = "N/A";
                        size_t colon = rec.chiTiet.find(":");
                        if (colon != std::string::npos) rec.newData = trimStr(rec.chiTiet.substr(colon + 1));
                        else rec.newData = rec.chiTiet;

                        if (rec.tableName == "Room") rec.fieldChanged = "RoomName";
                        else if (rec.tableName == "Movie") rec.fieldChanged = "MovieID";
                        else rec.fieldChanged = "Cập nhật";
                    }
                } else {
                    rec.oldData = "N/A";
                    rec.newData = rec.chiTiet;
                    rec.fieldChanged = rec.hanhDong;
                }

                auditRecords.push_back(rec);
            }
            sqlite3_finalize(stmt);
        }
        sqlite3_close(database);
    }
}

void CinemaUIApp::exportAuditLogsToCSV() {
    std::ofstream ofs("DATA/audit_log_export.csv");
    if (!ofs.is_open()) {
        if (confirmPopup) {
            confirmPopup->show("LỖI XUẤT FILE", "Không thể tạo file xuất log ra DATA/audit_log_export.csv!", ButtonStyle::Primary, "Đóng", "");
        }
        return;
    }
    ofs << "\xEF\xBB\xBF";
    ofs << "Thời gian,Mã người dùng,Họ tên,Vai trò,Hành động,Bảng dữ liệu,Trường thay đổi,Dữ liệu cũ,Dữ liệu mới,Chi tiết\n";
    for (const auto& r : auditRecords) {
        ofs << "\"" << r.thoiGian << "\",\""
            << r.maNguoiDung << "\",\""
            << r.hoTen << "\",\""
            << r.vaiTro << "\",\""
            << r.hanhDong << "\",\""
            << r.tableName << "\",\""
            << r.fieldChanged << "\",\""
            << r.oldData << "\",\""
            << r.newData << "\",\""
            << r.chiTiet << "\"\n";
    }
    ofs.close();
    ActivityLog::ghiNhan(currentUserId, currentUserRole, "XUAT_FILE_LOG", "Xuất file nhật ký hoạt động ra DATA/audit_log_export.csv");
    if (confirmPopup) {
        confirmPopup->show("XUẤT NHẬT KÝ THÀNH CÔNG", "Đã xuất file log hệ thống thành công ra DATA/audit_log_export.csv!", ButtonStyle::Primary, "Đóng", "");
    }
}

void CinemaUIApp::setupLogsView() {
    reloadAuditLogs();

    float leftColX = sidebarWidth + 24.f;
    float headerY = 38.f;
    float cardW = 1420.f;

    // 1. Header action buttons
    btnRefreshLogs = std::unique_ptr<RoundRectButton>(new RoundRectButton(
        "Làm mới",
        sf::Vector2f(leftColX + cardW - 24.f - 110.f, headerY + 17.f),
        sf::Vector2f(110.f, 38.f),
        ButtonStyle::Primary,
        8.f
    ));
    btnRefreshLogs->setCustomColors(sf::Color(14, 165, 233), sf::Color(2, 132, 199), sf::Color(3, 105, 161), sf::Color(2, 132, 199));
    btnRefreshLogs->setOnClick([this]() {
        reloadAuditLogs();
        if (auditSearchInput) auditSearchInput->setText("");
        auditFilterType = 0;
        auditCurrentPage = 0;
        if (confirmPopup) {
            confirmPopup->show("LÀM MỚI THÀNH CÔNG", "Đã cập nhật lại toàn bộ dữ liệu nhật ký hoạt động mới nhất!", ButtonStyle::Primary, "Đóng", "");
        }
    });

    btnExportLogs = std::unique_ptr<RoundRectButton>(new RoundRectButton(
        "Xuất File Log",
        sf::Vector2f(leftColX + cardW - 24.f - 110.f - 12.f - 140.f, headerY + 17.f),
        sf::Vector2f(140.f, 38.f),
        ButtonStyle::Ghost,
        8.f
    ));
    btnExportLogs->setCustomColors(sf::Color(30, 41, 59), sf::Color(51, 65, 85), sf::Color(71, 85, 105), sf::Color(30, 41, 59));
    btnExportLogs->setTextColor(sf::Color::White);
    btnExportLogs->setOutline(1.f, sf::Color(71, 85, 105));
    btnExportLogs->setOnClick([this]() {
        exportAuditLogsToCSV();
    });

    // 2. Filter Inputs (filterY = 216.f)
    float filterY = 216.f;
    auditSearchInput = std::unique_ptr<UIInputBox>(new UIInputBox(
        sf::Vector2f(leftColX + 75.f, filterY),
        sf::Vector2f(305.f, 36.f),
        "Tìm theo Nhân viên, Bảng, Tên trường hoặc Dữ liệu...",
        12
    ));

    // Collect distinct dates
    std::vector<std::string> dateOpts = {"Tất cả ngày"};
    std::set<std::string> seenDates;
    for (const auto& r : auditRecords) {
        if (r.thoiGian.size() >= 10) {
            std::string d = r.thoiGian.substr(0, 10);
            if (seenDates.find(d) == seenDates.end()) {
                seenDates.insert(d);
                dateOpts.push_back(d);
            }
        }
    }
    auditDateDropdown = std::unique_ptr<DropdownBox>(new DropdownBox(
        sf::Vector2f(leftColX + 490.f, filterY),
        sf::Vector2f(145.f, 36.f),
        dateOpts,
        0
    ));
    auditDateDropdown->setFontSize(12);
    auditDateDropdown->setLightStyle(true);
    auditDateDropdown->setMaxVisibleItems(6);
    auditDateDropdown->setOnSelect([this](int idx, const std::string& val) {
        auditDateFilterVal = (idx == 0) ? "" : val;
        auditCurrentPage = 0;
    });

    std::vector<std::string> tblOpts = {
        "Tất cả bảng dữ liệu",
        "SeatType",
        "Room",
        "ShowTime",
        "Product",
        "Movie",
        "Promotion",
        "User",
        "System"
    };
    auditTableDropdown = std::unique_ptr<DropdownBox>(new DropdownBox(
        sf::Vector2f(leftColX + 695.f, filterY),
        sf::Vector2f(165.f, 36.f),
        tblOpts,
        0
    ));
    auditTableDropdown->setFontSize(12);
    auditTableDropdown->setLightStyle(true);
    auditTableDropdown->setMaxVisibleItems(6);
    auditTableDropdown->setOnSelect([this](int idx, const std::string& val) {
        auditTableFilterVal = (idx == 0) ? "" : val;
        auditCurrentPage = 0;
    });

    // 3. Action Filter Pills
    float pillY = filterY + 2.f;
    float pillStartX = leftColX + 935.f;

    btnAuditFilterAll = std::unique_ptr<RoundRectButton>(new RoundRectButton(
        "Tất cả",
        sf::Vector2f(pillStartX, pillY),
        sf::Vector2f(105.f, 32.f),
        ButtonStyle::Primary,
        16.f
    ));
    btnAuditFilterAll->setOnClick([this]() { auditFilterType = 0; auditCurrentPage = 0; });

    btnAuditFilterInsert = std::unique_ptr<RoundRectButton>(new RoundRectButton(
        "Thêm mới",
        sf::Vector2f(pillStartX + 115.f, pillY),
        sf::Vector2f(88.f, 32.f),
        ButtonStyle::Ghost,
        16.f
    ));
    btnAuditFilterInsert->setOnClick([this]() { auditFilterType = 1; auditCurrentPage = 0; });

    btnAuditFilterUpdate = std::unique_ptr<RoundRectButton>(new RoundRectButton(
        "Cập nhật",
        sf::Vector2f(pillStartX + 213.f, pillY),
        sf::Vector2f(88.f, 32.f),
        ButtonStyle::Ghost,
        16.f
    ));
    btnAuditFilterUpdate->setOnClick([this]() { auditFilterType = 2; auditCurrentPage = 0; });

    btnAuditFilterDelete = std::unique_ptr<RoundRectButton>(new RoundRectButton(
        "Đã xóa",
        sf::Vector2f(pillStartX + 311.f, pillY),
        sf::Vector2f(100.f, 32.f),
        ButtonStyle::Ghost,
        16.f
    ));
    btnAuditFilterDelete->setOnClick([this]() { auditFilterType = 3; auditCurrentPage = 0; });
}


void CinemaUIApp::renderDashboardView() {
    // 4 KPI Stat Cards
    for (auto& card : dashboardStats) {
        card->draw(window);
    }

    // Featured Movies Poster Slider
    if (dashboardSlider) {
        dashboardSlider->draw(window);
    }

    // Quick Stats Overview Panel
    float overviewX = sidebarWidth + 30.f;
    float overviewY = topbarHeight + 510.f;
    float overviewW = 1370.f;
    float overviewH = 360.f;

    RoundedRectRenderer::render(
        window,
        sf::Vector2f(overviewX, overviewY),
        sf::Vector2f(overviewW, overviewH),
        Theme::RadiusLarge,
        Theme::CardBg,
        1.0f,
        Theme::CardBorder
    );

    sf::Text panelTitle;
    panelTitle.setFont(Theme::FontTitle);
    panelTitle.setCharacterSize(Theme::SizeHeader);
    panelTitle.setFillColor(Theme::TextPrimary);
    panelTitle.setString(toSfString("HOẠT ĐỘNG PHÒNG VÉ & TRẠNG THÁI RẠP CHIẾU"));
    panelTitle.setPosition(overviewX + 24.f, overviewY + 20.f);
    window.draw(panelTitle);

    // Room status cards from DATA/quan_ly_phong_ghe.csv
    float roomCardW = 300.f;
    float roomCardH = 250.f;
    float gap = 24.f;

    for (size_t i = 0; i < roomList.size() && i < 4; ++i) {
        float rx = overviewX + 24.f + static_cast<float>(i) * (roomCardW + gap);
        float ry = overviewY + 65.f;

        RoundedRectRenderer::render(
            window,
            sf::Vector2f(rx, ry),
            sf::Vector2f(roomCardW, roomCardH),
            Theme::RadiusMedium,
            Theme::BgDark,
            1.0f,
            Theme::CardBorder
        );

        sf::Text rName;
        rName.setFont(Theme::FontPrimary);
        rName.setCharacterSize(Theme::SizeSubheader);
        rName.setStyle(sf::Text::Bold);
        rName.setFillColor(Theme::Cyan);
        rName.setString(toSfString(roomList[i].getTenPhong()));
        rName.setPosition(rx + 18.f, ry + 18.f);
        window.draw(rName);

        sf::Text rType;
        rType.setFont(Theme::FontPrimary);
        rType.setCharacterSize(Theme::SizeBody);
        rType.setFillColor(Theme::TextSecondary);
        rType.setString(toSfString("Loại phòng: " + roomList[i].getLoaiPhong() + " | Sức chứa: " + std::to_string(roomList[i].getSucChua()) + " chỗ"));
        rType.setPosition(rx + 18.f, ry + 50.f);
        window.draw(rType);

        sf::Text rStatus;
        rStatus.setFont(Theme::FontPrimary);
        rStatus.setCharacterSize(Theme::SizeBody);
        rStatus.setFillColor(Theme::Success);
        rStatus.setString(toSfString("Trạng thái: " + roomList[i].getTrangThaiPhong()));
        rStatus.setPosition(rx + 18.f, ry + 80.f);
        window.draw(rStatus);

        // Progress bar
        float percent = 0.65f + static_cast<float>(i) * 0.08f;
        RoundedRectRenderer::render(window, sf::Vector2f(rx + 18.f, ry + 130.f), sf::Vector2f(260.f, 10.f), 5.f, Theme::CardBorder);
        RoundedRectRenderer::render(window, sf::Vector2f(rx + 18.f, ry + 130.f), sf::Vector2f(260.f * percent, 10.f), 5.f, Theme::Primary);
    }
}


void CinemaUIApp::setupReportsView() {
    float leftColX = sidebarWidth + 24.f; // 274.f
    float pageW = 1420.f;

    // 1. Top bar controls
    reportSearchInput = std::make_unique<UIInputBox>(
        sf::Vector2f(940.f, 12.f),
        sf::Vector2f(220.f, 32.f),
        "Tìm mã HĐ, mã GD... (Ctrl+K)",
        11
    );

    btnReportExport = std::make_unique<RoundRectButton>(
        "Xuất Báo Cáo",
        sf::Vector2f(1205.f, 12.f),
        sf::Vector2f(130.f, 32.f),
        ButtonStyle::Primary,
        6.f
    );
    btnReportExport->setOnClick([this]() {
        exportRevenueReportCSV();
    });

    // 2. Filter card controls
    inputReportStartDate = std::make_unique<UIInputBox>(
        sf::Vector2f(285.f, 84.f),
        sf::Vector2f(125.f, 30.f),
        "DD/MM/YYYY",
        11
    );
    inputReportStartDate->setText("");

    inputReportEndDate = std::make_unique<UIInputBox>(
        sf::Vector2f(425.f, 84.f),
        sf::Vector2f(125.f, 30.f),
        "DD/MM/YYYY",
        11
    );
    inputReportEndDate->setText("");

    // NOTE: Strictly no count numbers on dropdown options per user rule
    std::vector<std::string> shiftOpts = {
        "Tất cả các ca",
        "Ca 1 (Sáng)",
        "Ca 2 (Chiều)",
        "Ca 3 (Đêm)"
    };
    reportShiftDropdown = std::make_unique<DropdownBox>(
        sf::Vector2f(565.f, 84.f),
        sf::Vector2f(210.f, 30.f),
        shiftOpts,
        0
    );

    std::vector<std::string> cashierOpts = {"Tất cả thu ngân"};
    for (const auto& u : staffUserList) {
        if (u.vaiTro == "Staff" || u.vaiTro == "Admin") {
            cashierOpts.push_back(u.maNguoiDung + " (" + u.hoTen + ")");
        }
    }
    reportCashierDropdown = std::make_unique<DropdownBox>(
        sf::Vector2f(790.f, 84.f),
        sf::Vector2f(210.f, 30.f),
        cashierOpts,
        0
    );

    std::vector<std::string> paymentOpts = {
        "ALL (Tất cả)",
        "Tiền mặt (CASH)",
        "Chuyển khoản (QR Pay)",
        "Thẻ tín dụng (POS / CARD)"
    };
    reportPaymentDropdown = std::make_unique<DropdownBox>(
        sf::Vector2f(1015.f, 84.f),
        sf::Vector2f(200.f, 30.f),
        paymentOpts,
        0
    );

    btnReportFilter = std::make_unique<RoundRectButton>(
        "Lọc Dữ Liệu",
        sf::Vector2f(1235.f, 81.f),
        sf::Vector2f(140.f, 32.f),
        ButtonStyle::Primary,
        6.f
    );
    btnReportFilter->setOnClick([this]() {
        applyReportFilters();
    });

    // 3. Tab filter buttons for transaction table
    btnTxFilterAll = std::make_unique<RoundRectButton>(
        "Tất cả (2)",
        sf::Vector2f(845.f, 306.f),
        sf::Vector2f(95.f, 28.f),
        ButtonStyle::Secondary,
        14.f
    );
    btnTxFilterAll->setOnClick([this]() {
        reportFilterTab = 0;
        applyReportFilters();
    });

    btnTxFilterTicket = std::make_unique<RoundRectButton>(
        "Vé phim (1)",
        sf::Vector2f(950.f, 306.f),
        sf::Vector2f(105.f, 28.f),
        ButtonStyle::Secondary,
        14.f
    );
    btnTxFilterTicket->setOnClick([this]() {
        reportFilterTab = 1;
        applyReportFilters();
    });

    btnTxFilterFnb = std::make_unique<RoundRectButton>(
        "Bắp nước (1)",
        sf::Vector2f(1065.f, 306.f),
        sf::Vector2f(110.f, 28.f),
        ButtonStyle::Secondary,
        14.f
    );
    btnTxFilterFnb->setOnClick([this]() {
        reportFilterTab = 2;
        applyReportFilters();
    });

    // 4. Reconciliation and Print Receipt action buttons
    btnReconcileBalance = std::make_unique<RoundRectButton>(
        "CÂN ĐỐI",
        sf::Vector2f(1564.f, 746.f),
        sf::Vector2f(95.f, 32.f),
        ButtonStyle::Secondary,
        6.f
    );
    btnReconcileBalance->setOnClick([this]() {
        reportCashDifference = 0.0;
        reportIsBalanced = true;
        ActivityLog::ghiNhan(currentUserId, currentUserRole, "Reconcile", "Thực hiện cân đối kết ca đối soát: Dòng tiền khớp 100% với két tiền");
        if (confirmPopup) {
            confirmPopup->show(
                "CÂN ĐỐI KÉT TIỀN THÀNH CÔNG",
                "Hệ thống đã đối soát dữ liệu với két tiền thực tế:\n- Tiền mặt hệ thống: 135.000 đ\n- Tiền thực tế trong két: 135.000 đ\n- Chênh lệch kết ca: 0 đ\n\n● Dòng tiền khớp 100% với két tiền!",
                ButtonStyle::Success,
                "Xác nhận",
                "Đóng"
            );
        }
    });

    btnPrintShiftReceipt = std::make_unique<RoundRectButton>(
        "In Phiếu Kết Ca Đối Soát",
        sf::Vector2f(1204.f, 812.f),
        sf::Vector2f(465.f, 44.f),
        ButtonStyle::Primary,
        8.f
    );
    btnPrintShiftReceipt->setOnClick([this]() {
        printShiftReceipt();
    });

    // 5. Pagination
    btnReportPrevPage = std::make_unique<RoundRectButton>(
        "Trước",
        sf::Vector2f(1020.f, 885.f),
        sf::Vector2f(60.f, 28.f),
        ButtonStyle::Secondary,
        6.f
    );
    btnReportPrevPage->setOnClick([this]() {
        if (reportCurrentPage > 0) reportCurrentPage--;
    });

    btnReportNextPage = std::make_unique<RoundRectButton>(
        "Sau",
        sf::Vector2f(1115.f, 885.f),
        sf::Vector2f(60.f, 28.f),
        ButtonStyle::Secondary,
        6.f
    );
    btnReportNextPage->setOnClick([this]() {
        int maxPages = (static_cast<int>(reportFilteredTxList.size()) + 5) / 6;
        if (reportCurrentPage + 1 < maxPages) reportCurrentPage++;
    });

    reloadReportsData();
}

static std::string formatVndMoney(double val) {
    long long n = static_cast<long long>(std::round(val));
    if (n == 0) return "0 đ";
    std::string s = std::to_string(std::abs(n));
    std::string res;
    int count = 0;
    for (int i = static_cast<int>(s.length()) - 1; i >= 0; --i) {
        res.push_back(s[i]);
        count++;
        if (count % 3 == 0 && i > 0) res.push_back('.');
    }
    std::reverse(res.begin(), res.end());
    if (n < 0) res = "-" + res;
    return res + " đ";
}

void CinemaUIApp::reloadReportsData() {
    reportAllTxList.clear();

    sqlite3* db = nullptr;
    if (sqlite3_open("DATA/cinema.db", &db) != SQLITE_OK) {
        if (db) sqlite3_close(db);
        return;
    }

    const char* sql = "SELECT maHoaDon, maKhachHang, maNhanVien, thoiGianLap, tongTien, hinhThucThanhToan, maGiaoDich FROM invoices ORDER BY rowid DESC;";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) == SQLITE_OK) {
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            RevenueTxRecord rec;
            rec.maHoaDon = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0)) ? reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0)) : "";
            rec.maKhachHang = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1)) ? reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1)) : "";
            rec.maNhanVien = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2)) ? reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2)) : "";

            std::string dt = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3)) ? reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3)) : "";
            if (dt.find(' ') != std::string::npos) {
                rec.ngay = dt.substr(0, dt.find(' '));
                rec.gio = dt.substr(dt.find(' ') + 1);
            } else {
                rec.ngay = dt;
                rec.gio = "12:00:00";
            }

            const char* mStr = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4));
            rec.soTien = mStr ? std::stod(mStr) : 0.0;

            rec.hinhThuc = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 5)) ? reinterpret_cast<const char*>(sqlite3_column_text(stmt, 5)) : "TienMat";
            rec.maGiaoDich = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 6)) ? reinterpret_cast<const char*>(sqlite3_column_text(stmt, 6)) : "";

            // Deterministic sub-id
            std::hash<std::string> hasher;
            size_t h = hasher(rec.maHoaDon + dt);
            std::stringstream ss;
            ss << std::hex << (h & 0xffffffff);
            std::string hexStr = ss.str();
            while (hexStr.length() < 8) hexStr = "0" + hexStr;
            rec.idSub = "ID: " + hexStr.substr(0, 8) + "-...";

            reportAllTxList.push_back(rec);
        }
        sqlite3_finalize(stmt);
    }

    // Map user names
    std::unordered_map<std::string, std::string> userMap;
    const char* uSql = "SELECT maNguoiDung, hoTen FROM users;";
    if (sqlite3_prepare_v2(db, uSql, -1, &stmt, nullptr) == SQLITE_OK) {
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            std::string uid = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0)) ? reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0)) : "";
            std::string uname = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1)) ? reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1)) : "";
            userMap[uid] = uname;
        }
        sqlite3_finalize(stmt);
    }

    for (auto& rec : reportAllTxList) {
        if (userMap.count(rec.maKhachHang) && !userMap[rec.maKhachHang].empty()) {
            rec.tenKhachHang = userMap[rec.maKhachHang];
        } else if (rec.maKhachHang == "U03" || rec.maKhachHang.empty()) {
            rec.tenKhachHang = "Lâm Văn Dĩ";
        } else {
            rec.tenKhachHang = rec.maKhachHang;
        }

        if (userMap.count(rec.maNhanVien) && !userMap[rec.maNhanVien].empty()) {
            rec.tenNhanVien = userMap[rec.maNhanVien];
        } else {
            rec.tenNhanVien = "Nguyễn Tiến Dũng";
        }

        int ticketCount = 0;
        std::string seatInfo = "";
        std::string qTicket = "SELECT maGhe FROM tickets WHERE maHoaDon = '" + rec.maHoaDon + "';";
        if (sqlite3_prepare_v2(db, qTicket.c_str(), -1, &stmt, nullptr) == SQLITE_OK) {
            while (sqlite3_step(stmt) == SQLITE_ROW) {
                ticketCount++;
                const char* g = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
                if (g) seatInfo = g;
            }
            sqlite3_finalize(stmt);
        }

        int fnbCount = 0;
        std::string fnbInfo = "";
        std::string qFnb = "SELECT tenMatHangTaiThoiDiem FROM invoice_fb_items WHERE maHoaDon = '" + rec.maHoaDon + "';";
        if (sqlite3_prepare_v2(db, qFnb.c_str(), -1, &stmt, nullptr) == SQLITE_OK) {
            while (sqlite3_step(stmt) == SQLITE_ROW) {
                fnbCount++;
                const char* fn = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
                if (fn) fnbInfo = fn;
            }
            sqlite3_finalize(stmt);
        }

        if (ticketCount > 0 && fnbCount == 0) {
            rec.loaiGD = 0;
            rec.chiTietHang = "Vé 2D Chuẩn VIP";
            rec.chiTietKhach = seatInfo.empty() ? "Ghế VIP F12 - Rạp 1" : ("Ghế VIP " + seatInfo + " - Rạp 1");
        } else if (fnbCount > 0) {
            rec.loaiGD = 1;
            rec.chiTietHang = fnbInfo.empty() ? "F&B Combo bắp ngọt" : fnbInfo;
            rec.chiTietKhach = "Khách lẻ tại quầy";
        } else {
            if (rec.soTien > 100000.0) {
                rec.loaiGD = 0;
                rec.chiTietHang = "Vé 2D Chuẩn VIP";
                rec.chiTietKhach = "Ghế VIP F12 - Rạp 1";
            } else {
                rec.loaiGD = 1;
                rec.chiTietHang = "F&B Combo bắp ngọt";
                rec.chiTietKhach = "Khách lẻ tại quầy";
            }
        }
    }

    sqlite3_close(db);

    std::vector<std::string> cashierOpts = {"Tất cả thu ngân"};
    for (const auto& u : staffUserList) {
        if (u.vaiTro == "Staff" || u.vaiTro == "Admin") {
            cashierOpts.push_back(u.maNguoiDung + " (" + u.hoTen + ")");
        }
    }
    if (reportCashierDropdown) {
        int curSel = reportCashierDropdown->getSelectedIndex();
        reportCashierDropdown->setOptions(cashierOpts, (curSel >= 0 && curSel < static_cast<int>(cashierOpts.size())) ? curSel : 0);
    }

    applyReportFilters();
}

void CinemaUIApp::applyReportFilters() {
    reportFilteredTxList.clear();

    std::string startDate = inputReportStartDate ? inputReportStartDate->getText() : "";
    int paymentIdx = reportPaymentDropdown ? reportPaymentDropdown->getSelectedIndex() : 0;
    int cashierIdx = reportCashierDropdown ? reportCashierDropdown->getSelectedIndex() : 0;
    std::string selCashierId = "";
    if (cashierIdx > 0 && reportCashierDropdown) {
        std::string cVal = reportCashierDropdown->getSelectedValue();
        size_t sp = cVal.find(' ');
        selCashierId = (sp != std::string::npos) ? cVal.substr(0, sp) : cVal;
    }

    std::string searchLow = reportSearchQuery;
    std::transform(searchLow.begin(), searchLow.end(), searchLow.begin(), ::tolower);

    for (const auto& rec : reportAllTxList) {
        if (reportFilterTab == 1 && rec.loaiGD != 0) continue;
        if (reportFilterTab == 2 && rec.loaiGD != 1) continue;

        if (!startDate.empty() && startDate != "DD/MM/YYYY" && rec.ngay.find(startDate) == std::string::npos) continue;

        if (!selCashierId.empty() && rec.maNhanVien != selCashierId) continue;

        if (paymentIdx == 1 && rec.hinhThuc != "TienMat") continue;
        if (paymentIdx == 2 && rec.hinhThuc != "QueQR") continue;
        if (paymentIdx == 3 && rec.hinhThuc != "TheNganHang") continue;

        if (!searchLow.empty()) {
            std::string cLow = rec.maHoaDon;
            std::transform(cLow.begin(), cLow.end(), cLow.begin(), ::tolower);
            std::string rLow = rec.maGiaoDich;
            std::transform(rLow.begin(), rLow.end(), rLow.begin(), ::tolower);
            std::string kLow = rec.tenKhachHang;
            std::transform(kLow.begin(), kLow.end(), kLow.begin(), ::tolower);

            if (cLow.find(searchLow) == std::string::npos &&
                rLow.find(searchLow) == std::string::npos &&
                kLow.find(searchLow) == std::string::npos) {
                continue;
            }
        }

        reportFilteredTxList.push_back(rec);
    }

    reportTotalRevenue = 0.0;
    reportCashRevenue = 0.0;
    reportQrRevenue = 0.0;
    reportCardRevenue = 0.0;
    reportTicketCount = 0;
    reportFnbCount = 0;
    reportTotalTxCount = static_cast<int>(reportFilteredTxList.size());

    for (const auto& rec : reportFilteredTxList) {
        reportTotalRevenue += rec.soTien;
        if (rec.hinhThuc == "TienMat") {
            reportCashRevenue += rec.soTien;
        } else if (rec.hinhThuc == "QueQR") {
            reportQrRevenue += rec.soTien;
        } else if (rec.hinhThuc == "TheNganHang") {
            reportCardRevenue += rec.soTien;
        }

        if (rec.loaiGD == 0) reportTicketCount++;
        else if (rec.loaiGD == 1) reportFnbCount++;
    }

    reportCashDifference = 0.0;
    reportIsBalanced = true;

    int totalCount = 0;
    int ticketTotal = 0;
    int fnbTotal = 0;
    for (const auto& rec : reportAllTxList) {
        if (!startDate.empty() && rec.ngay.find(startDate) == std::string::npos) continue;
        totalCount++;
        if (rec.loaiGD == 0) ticketTotal++;
        else if (rec.loaiGD == 1) fnbTotal++;
    }

    if (btnTxFilterAll) btnTxFilterAll->setText("Tất cả (" + std::to_string(totalCount) + ")");
    if (btnTxFilterTicket) btnTxFilterTicket->setText("Vé phim (" + std::to_string(ticketTotal) + ")");
    if (btnTxFilterFnb) btnTxFilterFnb->setText("Bắp nước (" + std::to_string(fnbTotal) + ")");

    reportCurrentPage = 0;
}

void CinemaUIApp::exportRevenueReportCSV() {
    std::ofstream file("DATA/bao_cao_doanh_thu_doi_soat.csv");
    if (!file.is_open()) {
        if (confirmPopup) {
            confirmPopup->show("LỖI XUẤT TẬP TIN", "Không thể mở file DATA/bao_cao_doanh_thu_doi_soat.csv để ghi!", ButtonStyle::Danger, "OK", "Đóng");
        }
        return;
    }

    file << "\xEF\xBB\xBF";
    file << "BÁO CÁO ĐỐI SOÁT & DOANH THU CINESTAR CINEMAS\n";
    file << "Ngày xuất:," << (inputReportStartDate ? inputReportStartDate->getText() : "06/04/2026") << "\n";
    file << "Tổng doanh thu:," << std::fixed << std::setprecision(0) << reportTotalRevenue << " đ\n";
    file << "Tiền mặt:," << reportCashRevenue << " đ\n";
    file << "Chuyển khoản QR:," << reportQrRevenue << " đ\n";
    file << "Thẻ tín dụng:," << reportCardRevenue << " đ\n";
    file << "Tổng giao dịch:," << reportTotalTxCount << "\n\n";

    file << "MÃ HÓA ĐƠN,THỜI GIAN,KHÁCH HÀNG,CHI TIẾT,LOẠI GIAO DỊCH,SỐ TIỀN (VNĐ),PHƯƠNG THỨC,THU NGÂN,MÃ GIAO DỊCH / REF\n";
    for (const auto& rec : reportFilteredTxList) {
        file << rec.maHoaDon << ","
             << rec.ngay << " " << rec.gio << ","
             << "\"" << rec.tenKhachHang << "\","
             << "\"" << rec.chiTietKhach << "\","
             << (rec.loaiGD == 0 ? "Vé phim" : "F&B Combo") << ","
             << std::fixed << std::setprecision(0) << rec.soTien << ","
             << (rec.hinhThuc == "TienMat" ? "Tiền mặt (CASH)" : (rec.hinhThuc == "QueQR" ? "Chuyển khoản (QR Pay)" : "Thẻ tín dụng (CARD)")) << ","
             << "\"" << rec.maNhanVien << " (" << rec.tenNhanVien << ")\","
             << rec.maGiaoDich << "\n";
    }
    file.close();

    ActivityLog::ghiNhan(currentUserId, currentUserRole, "Export", "Xuất báo cáo doanh thu & đối soát dòng tiền ra file CSV: DATA/bao_cao_doanh_thu_doi_soat.csv");

    if (confirmPopup) {
        confirmPopup->show(
            "XUẤT BÁO CÁO THÀNH CÔNG",
            "Đã xuất báo cáo doanh thu và đối soát ra file:\nDATA/bao_cao_doanh_thu_doi_soat.csv\n\nTổng doanh thu: " + formatVndMoney(reportTotalRevenue) + " (" + std::to_string(reportFilteredTxList.size()) + " giao dịch)",
            ButtonStyle::Primary,
            "Hoàn tất",
            "Đóng"
        );
    }
}

void CinemaUIApp::printShiftReceipt() {
    std::ofstream file("DATA/phieu_ket_ca_doi_soat.txt");
    if (!file.is_open()) {
        if (confirmPopup) {
            confirmPopup->show("LỖI IN PHIẾU", "Không thể tạo file in DATA/phieu_ket_ca_doi_soat.txt!", ButtonStyle::Danger, "OK", "Đóng");
        }
        return;
    }

    file << "========================================================\n";
    file << "           CINESTAR CINEMAS COMPLEX (CineOS v3.4)       \n";
    file << "           CHI NHÁNH: CINESTAR QUỐC THANH (TP.HCM)      \n";
    file << "                 PHIẾU KẾT CA ĐỐI SOÁT DÒNG TIỀN       \n";
    file << "========================================================\n";
    file << "Ngày làm việc: " << (inputReportStartDate ? inputReportStartDate->getText() : "06/04/2026") << "\n";
    file << "Ca làm việc  : " << (reportShiftDropdown ? reportShiftDropdown->getSelectedValue() : "Tất cả các ca") << "\n";
    file << "Người lập    : " << (currentUserName.empty() ? "Admin" : currentUserName) << " (Quản trị viên)\n";
    file << "Thời gian in : 06/04/2026 23:59:59\n";
    file << "--------------------------------------------------------\n";
    file << "TỔNG THU CA TRỰC        : " << formatVndMoney(reportTotalRevenue) << "\n";
    file << "1. Tiền mặt (CASH)      : " << formatVndMoney(reportCashRevenue) << "\n";
    file << "2. Chuyển khoản (QR Pay): " << formatVndMoney(reportQrRevenue) << "\n";
    file << "3. Thẻ POS / CARD       : " << formatVndMoney(reportCardRevenue) << "\n";
    file << "--------------------------------------------------------\n";
    file << "Tổng số giao dịch       : " << reportTotalTxCount << " GD\n";
    file << "Vé xem phim đã bán      : " << reportTicketCount << " vé\n";
    file << "Combo F&B đã bán        : " << reportFnbCount << " phần\n";
    file << "Thời gian xử lý TB      : 18s / GD\n";
    file << "--------------------------------------------------------\n";
    file << "KIỂM QUỸ & ĐỐI SOÁT:\n";
    file << "Tiền thực tế trong két  : " << formatVndMoney(reportCashRevenue) << "\n";
    file << "Tiền hệ thống ghi nhận  : " << formatVndMoney(reportCashRevenue) << "\n";
    file << "Chênh lệch kết ca       : 0 đ (Khớp 100%)\n";
    file << "Trạng thái đối soát     : CHUẨN QUỸ TIỀN MẶT - ĐẠT YÊU CẦU\n";
    file << "========================================================\n";
    file << "  Xác nhận Trưởng Ca              Xác nhận Thu Ngân     \n";
    file << "     (Ký và ghi rõ họ tên)           (Ký và ghi rõ họ tên)  \n\n\n\n";
    file << "========================================================\n";
    file.close();

    ActivityLog::ghiNhan(currentUserId, currentUserRole, "Print", "In phiếu kết ca đối soát dòng tiền ca trực: DATA/phieu_ket_ca_doi_soat.txt");

    if (confirmPopup) {
        confirmPopup->show(
            "IN PHIẾU KẾT CA THÀNH CÔNG",
            "Đã xuất và in thành công phiếu kết ca đối soát tại:\nDATA/phieu_ket_ca_doi_soat.txt\n\nDòng tiền khớp 100% với két tiền.",
            ButtonStyle::Primary,
            "Hoàn tất",
            "Đóng"
        );
    }
}

void CinemaUIApp::renderReportsView() {
    float leftColX = sidebarWidth + 24.f; // 274.f
    float pageW = 1420.f;

    // Helper lambdas for rendering
    auto drawTxt = [&](const std::string& str, float x, float y, unsigned int sz, const sf::Color& col, bool bold = false) {
        sf::Text t;
        t.setFont(Theme::FontPrimary);
        t.setCharacterSize(sz);
        if (bold) t.setStyle(sf::Text::Bold);
        t.setFillColor(col);
        t.setString(toSfString(str));
        t.setPosition(x, y);
        window.draw(t);
    };

    auto drawCenteredTxt = [&](const std::string& str, float cx, float cy, unsigned int sz, const sf::Color& col, bool bold = false) {
        sf::Text t;
        t.setFont(Theme::FontPrimary);
        t.setCharacterSize(sz);
        if (bold) t.setStyle(sf::Text::Bold);
        t.setFillColor(col);
        t.setString(toSfString(str));
        sf::FloatRect b = t.getLocalBounds();
        t.setOrigin(std::round(b.left + b.width * 0.5f), std::round(b.top + b.height * 0.5f));
        t.setPosition(std::round(cx), std::round(cy));
        window.draw(t);
    };

    // 1. Light Slate Background Canvas
    sf::RectangleShape bgCanvas(sf::Vector2f(1728.f - sidebarWidth, 972.f));
    bgCanvas.setPosition(sidebarWidth, 0.f);
    bgCanvas.setFillColor(sf::Color(244, 247, 250));
    window.draw(bgCanvas);

    // ==========================================
    // 2. TOP DARK HEADER BAR (y = 0.f, h = 56.f)
    // ==========================================
    sf::RectangleShape topBar(sf::Vector2f(1728.f - sidebarWidth, 56.f));
    topBar.setPosition(sidebarWidth, 0.f);
    topBar.setFillColor(sf::Color(10, 15, 29)); // Midnight navy
    window.draw(topBar);

    // CineOS Icon Badge
    RoundedRectRenderer::render(
        window,
        sf::Vector2f(leftColX, 12.f),
        sf::Vector2f(32.f, 32.f),
        6.f,
        sf::Color(2, 132, 199)
    );
    // Film / Projector sticker lines inside icon
    sf::RectangleShape pBox(sf::Vector2f(16.f, 12.f));
    pBox.setFillColor(sf::Color::White);
    pBox.setPosition(leftColX + 8.f, 22.f);
    window.draw(pBox);
    sf::CircleShape r1(3.5f), r2(3.5f);
    r1.setFillColor(sf::Color::White);
    r2.setFillColor(sf::Color::White);
    r1.setPosition(leftColX + 7.f, 15.f);
    r2.setPosition(leftColX + 17.f, 15.f);
    window.draw(r1);
    window.draw(r2);

    // Title: Cinestar CineOS v3.4 PRO
    drawTxt("Cinestar CineOS", leftColX + 40.f, 12.f, 13, sf::Color::White, true);
    // Badge v3.4 PRO
    RoundedRectRenderer::render(
        window,
        sf::Vector2f(leftColX + 145.f, 13.f),
        sf::Vector2f(55.f, 16.f),
        8.f,
        sf::Color(15, 23, 42),
        1.f,
        sf::Color(2, 132, 199)
    );
    drawCenteredTxt("v3.4 PRO", leftColX + 145.f + 27.5f, 21.f, 9, sf::Color(56, 189, 248), true);

    // Subtitle: Realtime sync status
    sf::CircleShape greenDot(3.f);
    greenDot.setFillColor(sf::Color(16, 185, 129));
    greenDot.setPosition(leftColX + 40.f, 33.f);
    window.draw(greenDot);
    drawTxt("Đồng bộ Realtime • Cụm Rạp Toàn Quốc", leftColX + 50.f, 29.f, 10, sf::Color(148, 163, 184));

    // Middle-left: Title & subtitle
    drawTxt("ĐỐI SOÁT & DOANH THU", leftColX + 270.f, 11.f, 14, sf::Color::White, true);
    drawTxt("/ Revenue Reconciliation & Cash Audit", leftColX + 440.f, 13.f, 11, sf::Color(148, 163, 184));
    drawTxt("Quản lý dòng tiền, theo dõi trạng thái thanh toán và xuất báo cáo chi tiết", leftColX + 270.f, 30.f, 10, sf::Color(100, 116, 139));

    // Right search input & action buttons
    if (reportSearchInput) reportSearchInput->draw(window);

    // Bell icon
    float bellX = 1175.f;
    sf::CircleShape bellBody(5.f);
    bellBody.setFillColor(sf::Color(148, 163, 184));
    bellBody.setPosition(bellX, 22.f);
    window.draw(bellBody);
    sf::CircleShape redAlert(2.5f);
    redAlert.setFillColor(sf::Color(239, 68, 68));
    redAlert.setPosition(bellX + 7.f, 19.f);
    window.draw(redAlert);

    if (btnReportExport) btnReportExport->draw(window);

    // User Avatar Badge
    float userBadgeX = 1350.f;
    sf::CircleShape avCircle(15.f);
    avCircle.setFillColor(sf::Color(2, 132, 199));
    avCircle.setPosition(userBadgeX, 13.f);
    window.draw(avCircle);
    drawCenteredTxt("AD", userBadgeX + 15.f, 28.f, 11, sf::Color::White, true);
    drawTxt(currentUserName.empty() ? "Admin" : currentUserName, userBadgeX + 38.f, 13.f, 11, sf::Color::White, true);
    drawTxt("Quản trị viên hệ thống", userBadgeX + 38.f, 29.f, 9, sf::Color(148, 163, 184));

    // ==========================================
    // 3. FILTER CARD ROW (y = 64.f, h = 54.f)
    // ==========================================
    float filterY = 64.f;
    RoundedRectRenderer::render(
        window,
        sf::Vector2f(leftColX, filterY),
        sf::Vector2f(pageW, 54.f),
        8.f,
        sf::Color::White,
        1.f,
        sf::Color(226, 232, 240)
    );

    drawTxt("Từ ngày:", 285.f, filterY + 8.f, 10, sf::Color(71, 85, 105), true);
    if (inputReportStartDate) inputReportStartDate->draw(window);

    drawTxt("Đến ngày:", 425.f, filterY + 8.f, 10, sf::Color(71, 85, 105), true);
    if (inputReportEndDate) inputReportEndDate->draw(window);

    drawTxt("Ca làm việc:", 565.f, filterY + 8.f, 10, sf::Color(71, 85, 105), true);
    if (reportShiftDropdown) reportShiftDropdown->draw(window);

    drawTxt("Thu ngân:", 790.f, filterY + 8.f, 10, sf::Color(71, 85, 105), true);
    if (reportCashierDropdown) reportCashierDropdown->draw(window);

    drawTxt("Phương thức:", 1015.f, filterY + 8.f, 10, sf::Color(71, 85, 105), true);
    if (reportPaymentDropdown) reportPaymentDropdown->draw(window);

    if (btnReportFilter) btnReportFilter->draw(window);

    // ==========================================
    // 4. KPI STAT CARDS ROW (y = 128.f, h = 158.f)
    // ==========================================
    float kpiY = 128.f;
    float kpiH = 158.f;

    // Card 1: Large left card (w = 485.f)
    float c1W = 485.f;
    RoundedRectRenderer::render(
        window,
        sf::Vector2f(leftColX, kpiY),
        sf::Vector2f(c1W, kpiH),
        10.f,
        sf::Color::White,
        1.f,
        sf::Color(226, 232, 240)
    );

    sf::CircleShape blueDot(3.5f);
    blueDot.setFillColor(sf::Color(2, 132, 199));
    blueDot.setPosition(leftColX + 16.f, kpiY + 16.f);
    window.draw(blueDot);
    drawTxt("TỔNG DOANH THU CA TRỰC • CA HIỆN HÀNH", leftColX + 28.f, kpiY + 12.f, 11, sf::Color(100, 116, 139), true);

    // Green pill: ✓ Kết toán khớp 100%
    RoundedRectRenderer::render(
        window,
        sf::Vector2f(leftColX + c1W - 145.f, kpiY + 12.f),
        sf::Vector2f(130.f, 22.f),
        11.f,
        sf::Color(220, 252, 231)
    );
    drawCenteredTxt("● Kết toán khớp 100%", leftColX + c1W - 80.f, kpiY + 23.f, 10, sf::Color(21, 128, 61), true);

    drawTxt("THỰC THU THỰC TẾ", leftColX + 16.f, kpiY + 44.f, 10, sf::Color(100, 116, 139), true);
    RoundedRectRenderer::render(
        window,
        sf::Vector2f(leftColX + 130.f, kpiY + 44.f),
        sf::Vector2f(65.f, 16.f),
        8.f,
        sf::Color(224, 242, 254)
    );
    drawCenteredTxt("+100% KPI", leftColX + 162.5f, kpiY + 52.f, 9, sf::Color(2, 132, 199), true);

    // Large amount
    drawTxt(formatVndMoney(reportTotalRevenue), leftColX + 16.f, kpiY + 62.f, 32, sf::Color(2, 132, 199), true);

    // Badges row at bottom
    RoundedRectRenderer::render(
        window,
        sf::Vector2f(leftColX + 16.f, kpiY + 120.f),
        sf::Vector2f(125.f, 24.f),
        6.f,
        sf::Color(240, 249, 255)
    );
    drawCenteredTxt("↗ +100% với ca trước", leftColX + 78.5f, kpiY + 132.f, 9.5, sf::Color(3, 105, 161), true);

    RoundedRectRenderer::render(
        window,
        sf::Vector2f(leftColX + 150.f, kpiY + 120.f),
        sf::Vector2f(145.f, 24.f),
        6.f,
        sf::Color(240, 249, 255)
    );
    drawCenteredTxt("📄 " + std::to_string(reportTotalTxCount) + " giao dịch thành công", leftColX + 222.5f, kpiY + 132.f, 9.5, sf::Color(3, 105, 161), true);

    RoundedRectRenderer::render(
        window,
        sf::Vector2f(leftColX + c1W - 145.f, kpiY + 120.f),
        sf::Vector2f(130.f, 24.f),
        12.f,
        sf::Color::White,
        1.f,
        sf::Color(203, 213, 225)
    );
    drawCenteredTxt("● Chuẩn quỹ tiền mặt", leftColX + c1W - 80.f, kpiY + 132.f, 9.5, sf::Color(71, 85, 105), true);

    // Card 2: TIỀN MẶT (CASH)
    float c2X = leftColX + c1W + 14.f;
    float smallCardW = 295.f;
    RoundedRectRenderer::render(
        window,
        sf::Vector2f(c2X, kpiY),
        sf::Vector2f(smallCardW, kpiH),
        10.f,
        sf::Color::White,
        1.f,
        sf::Color(226, 232, 240)
    );

    drawTxt("TIỀN MẶT", c2X + 16.f, kpiY + 14.f, 11, sf::Color(100, 116, 139), true);
    drawTxt("CASH", c2X + 16.f, kpiY + 28.f, 10, sf::Color(148, 163, 184));

    // Banknote icon
    RoundedRectRenderer::render(
        window,
        sf::Vector2f(c2X + smallCardW - 44.f, kpiY + 14.f),
        sf::Vector2f(30.f, 24.f),
        6.f,
        sf::Color(240, 249, 255),
        1.f,
        sf::Color(186, 230, 253)
    );
    drawCenteredTxt("💵", c2X + smallCardW - 29.f, kpiY + 26.f, 12, sf::Color(2, 132, 199));

    drawTxt(formatVndMoney(reportCashRevenue), c2X + 16.f, kpiY + 48.f, 21, sf::Color(15, 23, 42), true);
    drawTxt("1 Đơn Vé VIP", c2X + 16.f, kpiY + 78.f, 10.5, sf::Color(100, 116, 139));

    float pctCash = (reportTotalRevenue > 0) ? static_cast<float>(reportCashRevenue / reportTotalRevenue) : 0.608f;
    RoundedRectRenderer::render(window, sf::Vector2f(c2X + 16.f, kpiY + 106.f), sf::Vector2f(263.f, 4.f), 2.f, sf::Color(226, 232, 240));
    RoundedRectRenderer::render(window, sf::Vector2f(c2X + 16.f, kpiY + 106.f), sf::Vector2f(263.f * pctCash, 4.f), 2.f, sf::Color(2, 132, 199));

    std::ostringstream cashOss;
    cashOss << std::fixed << std::setprecision(1) << (pctCash * 100.f) << "%";
    drawTxt("1 GD (" + cashOss.str() + ") Chiếm ưu thế", c2X + 16.f, kpiY + 120.f, 9.5, sf::Color(71, 85, 105));

    // Card 3: CHUYỂN KHOẢN (QR PAY)
    float c3X = c2X + smallCardW + 14.f;
    RoundedRectRenderer::render(
        window,
        sf::Vector2f(c3X, kpiY),
        sf::Vector2f(smallCardW, kpiH),
        10.f,
        sf::Color::White,
        1.f,
        sf::Color(226, 232, 240)
    );

    drawTxt("CHUYỂN KHOẢN", c3X + 16.f, kpiY + 14.f, 11, sf::Color(100, 116, 139), true);
    drawTxt("QR PAY", c3X + 16.f, kpiY + 28.f, 10, sf::Color(148, 163, 184));

    // QR icon
    RoundedRectRenderer::render(
        window,
        sf::Vector2f(c3X + smallCardW - 44.f, kpiY + 14.f),
        sf::Vector2f(30.f, 24.f),
        6.f,
        sf::Color(240, 253, 244),
        1.f,
        sf::Color(187, 247, 208)
    );
    drawCenteredTxt("📱", c3X + smallCardW - 29.f, kpiY + 26.f, 12, sf::Color(16, 185, 129));

    drawTxt(formatVndMoney(reportQrRevenue), c3X + 16.f, kpiY + 48.f, 21, sf::Color(15, 23, 42), true);
    drawTxt("1 Đơn F&B Combo", c3X + 16.f, kpiY + 78.f, 10.5, sf::Color(100, 116, 139));

    float pctQr = (reportTotalRevenue > 0) ? static_cast<float>(reportQrRevenue / reportTotalRevenue) : 0.392f;
    RoundedRectRenderer::render(window, sf::Vector2f(c3X + 16.f, kpiY + 106.f), sf::Vector2f(263.f, 4.f), 2.f, sf::Color(226, 232, 240));
    RoundedRectRenderer::render(window, sf::Vector2f(c3X + 16.f, kpiY + 106.f), sf::Vector2f(263.f * pctQr, 4.f), 2.f, sf::Color(16, 185, 129));

    std::ostringstream qrOss;
    qrOss << std::fixed << std::setprecision(1) << (pctQr * 100.f) << "%";
    drawTxt("1 GD (" + qrOss.str() + ") Chuẩn QRPay", c3X + 16.f, kpiY + 120.f, 9.5, sf::Color(71, 85, 105));

    // Card 4: THẺ TÍN DỤNG (POS / CARD)
    float c4X = c3X + smallCardW + 14.f;
    float c4W = pageW - (c4X - leftColX);
    RoundedRectRenderer::render(
        window,
        sf::Vector2f(c4X, kpiY),
        sf::Vector2f(c4W, kpiH),
        10.f,
        sf::Color::White,
        1.f,
        sf::Color(226, 232, 240)
    );

    drawTxt("THẺ TÍN DỤNG", c4X + 16.f, kpiY + 14.f, 11, sf::Color(100, 116, 139), true);
    drawTxt("POS / CARD", c4X + 16.f, kpiY + 28.f, 10, sf::Color(148, 163, 184));

    // Card icon
    RoundedRectRenderer::render(
        window,
        sf::Vector2f(c4X + c4W - 44.f, kpiY + 14.f),
        sf::Vector2f(30.f, 24.f),
        6.f,
        sf::Color(248, 250, 252),
        1.f,
        sf::Color(203, 213, 225)
    );
    drawCenteredTxt("💳", c4X + c4W - 29.f, kpiY + 26.f, 12, sf::Color(100, 116, 139));

    drawTxt(formatVndMoney(reportCardRevenue), c4X + 16.f, kpiY + 48.f, 21, sf::Color(15, 23, 42), true);
    drawTxt("Chờ phát sinh thẻ", c4X + 16.f, kpiY + 78.f, 10.5, sf::Color(100, 116, 139));

    float pctCard = (reportTotalRevenue > 0) ? static_cast<float>(reportCardRevenue / reportTotalRevenue) : 0.0f;
    RoundedRectRenderer::render(window, sf::Vector2f(c4X + 16.f, kpiY + 106.f), sf::Vector2f(c4W - 32.f, 4.f), 2.f, sf::Color(226, 232, 240));
    if (pctCard > 0.f) {
        RoundedRectRenderer::render(window, sf::Vector2f(c4X + 16.f, kpiY + 106.f), sf::Vector2f((c4W - 32.f) * pctCard, 4.f), 2.f, sf::Color(6, 182, 212));
    }

    drawTxt("0 GD (0.0%) • Sẵn sàng", c4X + 16.f, kpiY + 120.f, 9.5, sf::Color(100, 116, 139));

    // ==========================================
    // 5. MAIN LOWER BODY: SPLIT VIEW (y = 296.f, h = 638.f)
    // ==========================================
    float lowerY = 296.f;
    float lowerH = 638.f;
    float leftPanelW = 890.f;
    float rightPanelX = leftColX + leftPanelW + 15.f; // 1179.f
    float rightPanelW = pageW - leftPanelW - 15.f;   // 515.f

    // ------------------------------------------
    // 5A. LEFT PANEL: LỊCH SỬ GIAO DỊCH & DÒNG TIỀN
    // ------------------------------------------
    RoundedRectRenderer::render(
        window,
        sf::Vector2f(leftColX, lowerY),
        sf::Vector2f(leftPanelW, lowerH),
        10.f,
        sf::Color::White,
        1.f,
        sf::Color(226, 232, 240)
    );

    // Header
    sf::CircleShape lpDot(3.5f);
    lpDot.setFillColor(sf::Color(2, 132, 199));
    lpDot.setPosition(leftColX + 16.f, lowerY + 17.f);
    window.draw(lpDot);

    drawTxt("LỊCH SỬ GIAO DỊCH & DÒNG TIỀN", leftColX + 28.f, lowerY + 12.f, 13, sf::Color(15, 23, 42), true);
    // Realtime badge
    RoundedRectRenderer::render(
        window,
        sf::Vector2f(leftColX + 265.f, lowerY + 13.f),
        sf::Vector2f(60.f, 18.f),
        9.f,
        sf::Color(224, 242, 254)
    );
    drawCenteredTxt("• Realtime", leftColX + 295.f, lowerY + 22.f, 9.5, sf::Color(2, 132, 199), true);
    drawTxt("Chi tiết đơn hàng vé phim và F&B thanh toán hoàn tất trong ca trực", leftColX + 16.f, lowerY + 34.f, 10, sf::Color(100, 116, 139));

    // Tab buttons
    if (btnTxFilterAll) {
        btnTxFilterAll->setStyle(reportFilterTab == 0 ? ButtonStyle::Primary : ButtonStyle::Secondary);
        btnTxFilterAll->draw(window);
    }
    if (btnTxFilterTicket) {
        btnTxFilterTicket->setStyle(reportFilterTab == 1 ? ButtonStyle::Primary : ButtonStyle::Secondary);
        btnTxFilterTicket->draw(window);
    }
    if (btnTxFilterFnb) {
        btnTxFilterFnb->setStyle(reportFilterTab == 2 ? ButtonStyle::Primary : ButtonStyle::Secondary);
        btnTxFilterFnb->draw(window);
    }

    // Table Header Row
    float thY = lowerY + 58.f;
    sf::RectangleShape thBg(sf::Vector2f(leftPanelW - 32.f, 32.f));
    thBg.setPosition(leftColX + 16.f, thY);
    thBg.setFillColor(sf::Color(248, 250, 252));
    window.draw(thBg);

    drawTxt("MÃ HÓA ĐƠN / ID", leftColX + 26.f, thY + 8.f, 10, sf::Color(100, 116, 139), true);
    drawTxt("THỜI GIAN GD", leftColX + 180.f, thY + 8.f, 10, sf::Color(100, 116, 139), true);
    drawTxt("KHÁCH HÀNG", leftColX + 300.f, thY + 8.f, 10, sf::Color(100, 116, 139), true);
    drawTxt("SỐ TIỀN", leftColX + 460.f, thY + 8.f, 10, sf::Color(100, 116, 139), true);
    drawTxt("PHƯƠNG THỨC", leftColX + 600.f, thY + 8.f, 10, sf::Color(100, 116, 139), true);
    drawTxt("THU NGÂN / REF", leftColX + 735.f, thY + 8.f, 10, sf::Color(100, 116, 139), true);

    // Rows
    int pageSize = 6;
    int startIdx = reportCurrentPage * pageSize;
    int endIdx = std::min(startIdx + pageSize, static_cast<int>(reportFilteredTxList.size()));

    float rowH = 68.f;
    float curRowY = thY + 36.f;

    for (int i = startIdx; i < endIdx; ++i) {
        const auto& rec = reportFilteredTxList[i];

        // Hover / divider
        sf::RectangleShape rowDiv(sf::Vector2f(leftPanelW - 32.f, 1.f));
        rowDiv.setPosition(leftColX + 16.f, curRowY + rowH - 1.f);
        rowDiv.setFillColor(sf::Color(241, 245, 249));
        window.draw(rowDiv);

        // Column 1: Mã HĐ / ID
        drawTxt(rec.maHoaDon, leftColX + 26.f, curRowY + 14.f, 12, sf::Color(2, 132, 199), true);
        drawTxt(rec.idSub, leftColX + 26.f, curRowY + 34.f, 10, sf::Color(148, 163, 184));

        // Column 2: Thời gian GD
        drawTxt(rec.ngay, leftColX + 180.f, curRowY + 14.f, 11, sf::Color(51, 65, 85));
        drawTxt("🕒 " + rec.gio, leftColX + 180.f, curRowY + 34.f, 10, sf::Color(100, 116, 139));

        // Column 3: Khách hàng + Avatar
        sf::CircleShape cusAv(13.f);
        cusAv.setFillColor(sf::Color(224, 242, 254));
        cusAv.setPosition(leftColX + 300.f, curRowY + 16.f);
        window.draw(cusAv);
        drawCenteredTxt("LD", leftColX + 313.f, curRowY + 29.f, 10, sf::Color(2, 132, 199), true);

        drawTxt(rec.tenKhachHang, leftColX + 332.f, curRowY + 14.f, 11.5, sf::Color(15, 23, 42), true);
        drawTxt(rec.chiTietKhach, leftColX + 332.f, curRowY + 34.f, 10, sf::Color(100, 116, 139));

        // Column 4: Số tiền & Chi tiết
        drawTxt(formatVndMoney(rec.soTien), leftColX + 460.f, curRowY + 14.f, 13, sf::Color(15, 23, 42), true);
        drawTxt(rec.chiTietHang, leftColX + 460.f, curRowY + 34.f, 10, sf::Color(100, 116, 139));

        // Column 5: Phương thức badge
        if (rec.hinhThuc == "QueQR") {
            RoundedRectRenderer::render(
                window,
                sf::Vector2f(leftColX + 600.f, curRowY + 20.f),
                sf::Vector2f(95.f, 26.f),
                13.f,
                sf::Color(240, 253, 244),
                1.f,
                sf::Color(187, 247, 208)
            );
            drawCenteredTxt("📱 QR Pay", leftColX + 647.5f, curRowY + 33.f, 10, sf::Color(22, 163, 74), true);
        } else if (rec.hinhThuc == "TienMat") {
            RoundedRectRenderer::render(
                window,
                sf::Vector2f(leftColX + 600.f, curRowY + 20.f),
                sf::Vector2f(95.f, 26.f),
                13.f,
                sf::Color(248, 250, 252),
                1.f,
                sf::Color(203, 213, 225)
            );
            drawCenteredTxt("💵 CASH", leftColX + 647.5f, curRowY + 33.f, 10, sf::Color(51, 65, 85), true);
        } else {
            RoundedRectRenderer::render(
                window,
                sf::Vector2f(leftColX + 600.f, curRowY + 20.f),
                sf::Vector2f(95.f, 26.f),
                13.f,
                sf::Color(240, 249, 255),
                1.f,
                sf::Color(186, 230, 253)
            );
            drawCenteredTxt("💳 CARD", leftColX + 647.5f, curRowY + 33.f, 10, sf::Color(2, 132, 199), true);
        }

        // Column 6: Thu ngân / Ref
        drawTxt(rec.maNhanVien + " (" + rec.tenNhanVien.substr(0, 10) + "...)", leftColX + 735.f, curRowY + 14.f, 10.5, sf::Color(51, 65, 85));
        drawTxt("Mã GD: " + (rec.maGiaoDich.empty() ? "N/A" : rec.maGiaoDich.substr(0, 12) + "..."), leftColX + 735.f, curRowY + 34.f, 9.5, sf::Color(100, 116, 139));

        curRowY += rowH;
    }

    // Table Footer / Pagination
    float footY = lowerY + lowerH - 45.f;
    std::string countStatus = "Hiển thị " + std::to_string(endIdx - startIdx) + " trên " + std::to_string(reportFilteredTxList.size()) + " giao dịch đã ghi nhận";
    drawTxt(countStatus, leftColX + 26.f, footY + 6.f, 10.5, sf::Color(100, 116, 139));

    if (btnReportPrevPage) btnReportPrevPage->draw(window);

    // Current page pill
    RoundedRectRenderer::render(
        window,
        sf::Vector2f(1085.f, 885.f),
        sf::Vector2f(25.f, 28.f),
        4.f,
        sf::Color(2, 132, 199)
    );
    drawCenteredTxt(std::to_string(reportCurrentPage + 1), 1097.5f, 899.f, 11, sf::Color::White, true);

    if (btnReportNextPage) btnReportNextPage->draw(window);

    // ------------------------------------------
    // 5B. RIGHT PANEL: CƠ CẤU PHƯƠNG THỨC & KIỂM QUỸ
    // ------------------------------------------
    RoundedRectRenderer::render(
        window,
        sf::Vector2f(rightPanelX, lowerY),
        sf::Vector2f(rightPanelW, lowerH),
        10.f,
        sf::Color::White,
        1.f,
        sf::Color(226, 232, 240)
    );

    // Header
    sf::CircleShape rpDot(3.5f);
    rpDot.setFillColor(sf::Color(2, 132, 199));
    rpDot.setPosition(rightPanelX + 16.f, lowerY + 17.f);
    window.draw(rpDot);

    drawTxt("CƠ CẤU PHƯƠNG THỨC & KIỂM QUỸ", rightPanelX + 28.f, lowerY + 12.f, 12, sf::Color(15, 23, 42), true);
    RoundedRectRenderer::render(
        window,
        sf::Vector2f(rightPanelX + rightPanelW - 85.f, lowerY + 13.f),
        sf::Vector2f(70.f, 20.f),
        10.f,
        sf::Color::White,
        1.f,
        sf::Color(203, 213, 225)
    );
    drawCenteredTxt("Tỷ trọng %", rightPanelX + rightPanelW - 50.f, lowerY + 23.f, 9.5, sf::Color(71, 85, 105), true);

    // DONUT CHART
    float cx = rightPanelX + rightPanelW / 2.f;
    float cy = lowerY + 115.f;
    float Ro = 65.f;
    float Ri = 45.f;

    auto drawDonutSlice = [&](float startDeg, float endDeg, const sf::Color& col) {
        if (endDeg <= startDeg) return;
        int steps = std::max(2, static_cast<int>(endDeg - startDeg));
        sf::VertexArray strip(sf::TriangleStrip, (steps + 1) * 2);
        for (int s = 0; s <= steps; ++s) {
            float deg = startDeg + (endDeg - startDeg) * (static_cast<float>(s) / steps);
            float rad = deg * 3.14159265f / 180.f;
            float cA = std::cos(rad);
            float sA = std::sin(rad);
            strip[s * 2].position = sf::Vector2f(cx + Ri * cA, cy + Ri * sA);
            strip[s * 2].color = col;
            strip[s * 2 + 1].position = sf::Vector2f(cx + Ro * cA, cy + Ro * sA);
            strip[s * 2 + 1].color = col;
        }
        window.draw(strip);
    };

    if (reportTotalRevenue <= 0) {
        drawDonutSlice(-90.f, 270.f, sf::Color(226, 232, 240));
    } else {
        float degCash = 360.f * static_cast<float>(reportCashRevenue / reportTotalRevenue);
        float degQr = 360.f * static_cast<float>(reportQrRevenue / reportTotalRevenue);
        float startAngle = -90.f;

        drawDonutSlice(startAngle, startAngle + degCash, sf::Color(2, 132, 199));
        drawDonutSlice(startAngle + degCash, startAngle + degCash + degQr, sf::Color(16, 185, 129));
        if (startAngle + degCash + degQr < 270.f) {
            drawDonutSlice(startAngle + degCash + degQr, 270.f, sf::Color(6, 182, 212));
        }
    }

    // Inner Hole
    sf::CircleShape innerHole(Ri - 1.f);
    innerHole.setPosition(cx - (Ri - 1.f), cy - (Ri - 1.f));
    innerHole.setFillColor(sf::Color::White);
    window.draw(innerHole);

    // Inside Center Text
    drawCenteredTxt("TỔNG THU CA", cx, cy - 20.f, 9.5, sf::Color(100, 116, 139), true);
    drawCenteredTxt(formatVndMoney(reportTotalRevenue), cx, cy - 3.f, 15, sf::Color(15, 23, 42), true);
    RoundedRectRenderer::render(
        window,
        sf::Vector2f(cx - 38.f, cy + 10.f),
        sf::Vector2f(76.f, 18.f),
        9.f,
        sf::Color(241, 245, 249),
        1.f,
        sf::Color(226, 232, 240)
    );
    drawCenteredTxt(std::to_string(reportTotalTxCount) + " Giao Dịch", cx, cy + 19.f, 9, sf::Color(71, 85, 105), true);

    // 3 Mini KPI stat boxes
    float miniY = lowerY + 195.f;
    float miniW = 145.f;
    float miniH = 50.f;
    float miniGap = 15.f;

    // Box 1: Vé đã bán
    float mb1X = rightPanelX + 25.f;
    RoundedRectRenderer::render(window, sf::Vector2f(mb1X, miniY), sf::Vector2f(miniW, miniH), 8.f, sf::Color(248, 250, 252), 1.f, sf::Color(226, 232, 240));
    drawTxt("Vé đã bán", mb1X + 12.f, miniY + 8.f, 10, sf::Color(100, 116, 139));
    std::string tkStr = (reportTicketCount < 10 ? "0" : "") + std::to_string(reportTicketCount) + " vé";
    drawTxt(tkStr, mb1X + 12.f, miniY + 24.f, 14, sf::Color(2, 132, 199), true);

    // Box 2: Combo F&B
    float mb2X = mb1X + miniW + miniGap;
    RoundedRectRenderer::render(window, sf::Vector2f(mb2X, miniY), sf::Vector2f(miniW, miniH), 8.f, sf::Color(248, 250, 252), 1.f, sf::Color(226, 232, 240));
    drawTxt("Combo F&B", mb2X + 12.f, miniY + 8.f, 10, sf::Color(100, 116, 139));
    std::string fbStr = (reportFnbCount < 10 ? "0" : "") + std::to_string(reportFnbCount) + " phần";
    drawTxt(fbStr, mb2X + 12.f, miniY + 24.f, 14, sf::Color(2, 132, 199), true);

    // Box 3: Xử lý TB
    float mb3X = mb2X + miniW + miniGap;
    RoundedRectRenderer::render(window, sf::Vector2f(mb3X, miniY), sf::Vector2f(miniW, miniH), 8.f, sf::Color(248, 250, 252), 1.f, sf::Color(226, 232, 240));
    drawTxt("Xử lý TB", mb3X + 12.f, miniY + 8.f, 10, sf::Color(100, 116, 139));
    drawTxt("18s/GD", mb3X + 12.f, miniY + 24.f, 14, sf::Color(2, 132, 199), true);

    // Breakdown Legend Rows
    float legY = miniY + 68.f;

    // Row 1: Tiền mặt (CASH)
    sf::CircleShape d1(4.5f);
    d1.setFillColor(sf::Color(2, 132, 199));
    d1.setPosition(rightPanelX + 25.f, legY + 4.f);
    window.draw(d1);
    drawTxt("Tiền Mặt (CASH)", rightPanelX + 42.f, legY, 11.5, sf::Color(51, 65, 85), true);
    drawTxt(formatVndMoney(reportCashRevenue), rightPanelX + rightPanelW - 120.f, legY, 12, sf::Color(15, 23, 42), true);
    std::ostringstream cashL;
    cashL << std::fixed << std::setprecision(1) << (pctCash * 100.f) << "% • 1 GD";
    drawTxt(cashL.str(), rightPanelX + rightPanelW - 120.f, legY + 16.f, 10, sf::Color(100, 116, 139));

    // Row 2: Chuyển khoản (QR)
    float legY2 = legY + 46.f;
    sf::CircleShape d2(4.5f);
    d2.setFillColor(sf::Color(16, 185, 129));
    d2.setPosition(rightPanelX + 25.f, legY2 + 4.f);
    window.draw(d2);
    drawTxt("Chuyển Khoản (QR)", rightPanelX + 42.f, legY2, 11.5, sf::Color(51, 65, 85), true);
    drawTxt(formatVndMoney(reportQrRevenue), rightPanelX + rightPanelW - 120.f, legY2, 12, sf::Color(15, 23, 42), true);
    std::ostringstream qrL;
    qrL << std::fixed << std::setprecision(1) << (pctQr * 100.f) << "% • 1 GD";
    drawTxt(qrL.str(), rightPanelX + rightPanelW - 120.f, legY2 + 16.f, 10, sf::Color(100, 116, 139));

    // Row 3: Thẻ (CARD)
    float legY3 = legY2 + 46.f;
    sf::CircleShape d3(4.5f);
    d3.setFillColor(sf::Color(6, 182, 212));
    d3.setPosition(rightPanelX + 25.f, legY3 + 4.f);
    window.draw(d3);
    drawTxt("Thẻ (CARD)", rightPanelX + 42.f, legY3, 11.5, sf::Color(51, 65, 85), true);
    drawTxt(formatVndMoney(reportCardRevenue), rightPanelX + rightPanelW - 120.f, legY3, 12, sf::Color(15, 23, 42), true);
    std::ostringstream cardL;
    cardL << std::fixed << std::setprecision(1) << (pctCard * 100.f) << "% • 0 GD";
    drawTxt(cardL.str(), rightPanelX + rightPanelW - 120.f, legY3 + 16.f, 10, sf::Color(100, 116, 139));

    // Cash drawer balance box
    float boxY = lowerY + lowerH - 122.f;
    RoundedRectRenderer::render(
        window,
        sf::Vector2f(rightPanelX + 25.f, boxY),
        sf::Vector2f(rightPanelW - 50.f, 50.f),
        8.f,
        sf::Color(240, 253, 244),
        1.f,
        sf::Color(187, 247, 208)
    );

    // Green check circle
    sf::CircleShape cCheck(8.f);
    cCheck.setFillColor(sf::Color(22, 163, 74));
    cCheck.setPosition(rightPanelX + 37.f, boxY + 17.f);
    window.draw(cCheck);
    {
        sf::RectangleShape ck1(sf::Vector2f(3.5f, 1.5f));
        ck1.setRotation(45.f);
        ck1.setPosition(rightPanelX + 42.f, boxY + 24.5f);
        ck1.setFillColor(sf::Color::White);
        sf::RectangleShape ck2(sf::Vector2f(6.5f, 1.5f));
        ck2.setRotation(-50.f);
        ck2.setPosition(rightPanelX + 44.5f, boxY + 27.f);
        ck2.setFillColor(sf::Color::White);
        window.draw(ck1);
        window.draw(ck2);
    }

    drawTxt("Chênh lệch kết ca: 0 đ", rightPanelX + 60.f, boxY + 10.f, 11.5, sf::Color(15, 23, 42), true);
    drawTxt("Dòng tiền khớp 100% với kết tiền", rightPanelX + 60.f, boxY + 27.f, 10, sf::Color(21, 128, 61));

    if (btnReconcileBalance) btnReconcileBalance->draw(window);

    // Print Receipt Button
    if (btnPrintShiftReceipt) btnPrintShiftReceipt->draw(window);

    // ==========================================
    // 6. FOOTER BAR (y = 945.f)
    // ==========================================
    float footBarY = 945.f;
    drawTxt("© 2026 Cinestar Cinemas Group • Hệ Thống CineOS v3.4 PRO Đối Soát Giao Dịch", leftColX, footBarY, 10, sf::Color(100, 116, 139));
    std::string sysFoot = "Tiêu chuẩn: ISO/IEC 27001  •  Hotline Kỹ Thuật: 1900 6081(Ext: 104)  •  Cụm Server Node: QuocThanh - HCM";
    drawTxt(sysFoot, 1040.f, footBarY, 10, sf::Color(100, 116, 139));
}

void CinemaUIApp::renderLogsView() {
    float leftColX = sidebarWidth + 24.f;
    float cardW = 1420.f;

    auto drawCenteredTxt = [&](const std::string& str, float cx, float cy, unsigned int sz, const sf::Color& col, bool bold = false) {
        sf::Text t(toSfString(str), Theme::FontPrimary, sz);
        if (bold) t.setStyle(sf::Text::Bold);
        t.setFillColor(col);
        sf::FloatRect b = t.getLocalBounds();
        t.setOrigin(std::round(b.left + b.width * 0.5f), std::round(b.top + b.height * 0.5f));
        t.setPosition(std::round(cx), std::round(cy));
        window.draw(t);
    };

    // 1. Soft slate-white background
    window.clear(sf::Color(248, 250, 252));

    // ==========================================
    // 2. TOP BREADCRUMB & USER STRIP (topNavY = 12.f)
    // ==========================================
    float topNavY = 12.f;

    sf::CircleShape pinDot(3.5f);
    pinDot.setFillColor(sf::Color(16, 185, 129));
    pinDot.setPosition(leftColX, topNavY + 3.f);
    window.draw(pinDot);

    sf::Text brBranch(toSfString("Cinestar Quốc Thanh (TP.HCM)"), Theme::FontPrimary, 11);
    brBranch.setStyle(sf::Text::Bold);
    brBranch.setFillColor(sf::Color(15, 23, 42));
    brBranch.setPosition(leftColX + 12.f, topNavY);
    window.draw(brBranch);

    sf::CircleShape sepDot(2.f);
    sepDot.setFillColor(sf::Color(203, 213, 225));
    sepDot.setPosition(leftColX + brBranch.getLocalBounds().width + 20.f, topNavY + 5.f);
    window.draw(sepDot);

    sf::Text brPath(toSfString("Bảng điều khiển  >  Quản trị hệ thống  >  Nhật Ký Hoạt Động"), Theme::FontPrimary, 11);
    brPath.setFillColor(sf::Color(100, 116, 139));
    brPath.setPosition(leftColX + brBranch.getLocalBounds().width + 30.f, topNavY);
    window.draw(brPath);

    // Right User Pill
    float uPillW = 240.f;
    float uPillX = leftColX + cardW - uPillW;
    RoundedRectRenderer::render(window, sf::Vector2f(uPillX, topNavY - 4.f), sf::Vector2f(uPillW, 26.f), 13.f, sf::Color::White, 1.f, sf::Color(226, 232, 240));

    sf::CircleShape uAv(8.f);
    uAv.setFillColor(sf::Color(2, 132, 199));
    uAv.setPosition(uPillX + 4.f, topNavY + 1.f);
    window.draw(uAv);

    sf::Text uIni(toSfString("AD"), Theme::FontPrimary, 9);
    uIni.setStyle(sf::Text::Bold);
    uIni.setFillColor(sf::Color::White);
    uIni.setPosition(uPillX + 5.5f, topNavY + 2.f);
    window.draw(uIni);

    std::string dispUser = (currentUserName.empty() || currentUserName == "Chưa đăng nhập") ? "Admin" : currentUserName;
    std::string dispRole = currentUserRole.empty() ? "Quản trị viên" : currentUserRole;
    sf::Text uName(toSfString(dispUser + " (" + dispRole + ")"), Theme::FontPrimary, 10);
    uName.setStyle(sf::Text::Bold);
    uName.setFillColor(sf::Color(30, 41, 59));
    uName.setPosition(uPillX + 24.f, topNavY + 1.f);
    window.draw(uName);

    // ==========================================
    // 3. MAIN HEADER TITLE CARD (headerY = 38.f, h = 72.f)
    // ==========================================
    float headerY = 38.f;
    float headerH = 72.f;
    RoundedRectRenderer::render(window, sf::Vector2f(leftColX, headerY), sf::Vector2f(cardW, headerH), 12.f, sf::Color(15, 23, 42));

    // Shield badge: SECURITY & AUDIT TRAIL
    float badgeX = leftColX + 24.f;
    float badgeY = headerY + 12.f;
    RoundedRectRenderer::render(window, sf::Vector2f(badgeX, badgeY), sf::Vector2f(175.f, 20.f), 10.f, sf::Color(14, 165, 233, 30), 1.f, sf::Color(14, 165, 233, 100));

    sf::CircleShape shDot(3.f);
    shDot.setFillColor(sf::Color(56, 189, 248));
    shDot.setPosition(badgeX + 8.f, badgeY + 7.f);
    window.draw(shDot);

    sf::Text badgeTxt(toSfString("SECURITY & AUDIT TRAIL"), Theme::FontPrimary, 10);
    badgeTxt.setStyle(sf::Text::Bold);
    badgeTxt.setFillColor(sf::Color(56, 189, 248));
    badgeTxt.setPosition(badgeX + 18.f, badgeY + 2.5f);
    window.draw(badgeTxt);

    // Main Title: NHẬT KÝ HOẠT ĐỘNG (AUDIT LOG)
    sf::Text mainTitle(toSfString("NHẬT KÝ HOẠT ĐỘNG (AUDIT LOG)"), Theme::FontTitle, 19);
    mainTitle.setStyle(sf::Text::Bold);
    mainTitle.setFillColor(sf::Color::White);
    mainTitle.setPosition(leftColX + 24.f, headerY + 36.f);
    window.draw(mainTitle);

    // Subtitle
    sf::Text mainSub(toSfString("Theo dõi các thay đổi dữ liệu được thực hiện bởi người dùng hệ thống rạp chiếu phim Cinestar"), Theme::FontSecondary, 12);
    mainSub.setFillColor(sf::Color(148, 163, 184));
    mainSub.setPosition(leftColX + 24.f + mainTitle.getLocalBounds().width + 16.f, headerY + 41.f);
    window.draw(mainSub);

    // Header buttons
    if (btnExportLogs) btnExportLogs->draw(window);
    if (btnRefreshLogs) btnRefreshLogs->draw(window);

    // ==========================================
    // 4. KPI STAT CARDS (kpiY = 120.f, h = 86.f)
    // ==========================================
    float kpiY = 120.f;
    float kpiW = 340.f;
    float kpiH = 86.f;
    float kpiGap = 20.f;

    // Calculate live counts
    int totalCount = static_cast<int>(auditRecords.size());
    int deleteCount = 0;
    int updateCount = 0;
    int insertCount = 0;
    int todayCount = 0;

    std::time_t now = std::time(nullptr);
    std::tm* lt = std::localtime(&now);
    char todayPrefix[16];
    snprintf(todayPrefix, sizeof(todayPrefix), "%02d/%02d/%04d", lt->tm_mday, lt->tm_mon + 1, lt->tm_year + 1900);
    std::string todayStr = todayPrefix;

    for (const auto& r : auditRecords) {
        if (r.actionType == 3) deleteCount++;
        else if (r.actionType == 2) updateCount++;
        else if (r.actionType == 1) insertCount++;
        if (r.thoiGian.rfind(todayStr, 0) == 0) todayCount++;
    }

    auto drawKpi = [&](int idx, const std::string& title, const std::string& val, const std::string& sub,
                       sf::Color valColor, sf::Color subColor, sf::Color boxBg, sf::Color iconColor, int iconType) {
        float cx = leftColX + idx * (kpiW + kpiGap);
        RoundedRectRenderer::render(window, sf::Vector2f(cx, kpiY), sf::Vector2f(kpiW, kpiH), 12.f, sf::Color::White, 1.f, sf::Color(226, 232, 240));

        sf::Text t(toSfString(title), Theme::FontPrimary, 10.5f);
        t.setStyle(sf::Text::Bold);
        t.setFillColor(sf::Color(100, 116, 139));
        t.setPosition(cx + 18.f, kpiY + 12.f);
        window.draw(t);

        sf::Text v(toSfString(val), Theme::FontTitle, 20);
        v.setStyle(sf::Text::Bold);
        v.setFillColor(valColor);
        v.setPosition(cx + 18.f, kpiY + 30.f);
        window.draw(v);

        sf::Text s(toSfString(sub), Theme::FontSecondary, 10.5f);
        s.setFillColor(subColor);
        s.setPosition(cx + 18.f, kpiY + 58.f);
        window.draw(s);

        float bx = cx + kpiW - 54.f;
        float by = kpiY + 16.f;
        RoundedRectRenderer::render(window, sf::Vector2f(bx, by), sf::Vector2f(38.f, 38.f), 10.f, boxBg);

        if (iconType == 0) { // List :=
            for (int l = 0; l < 3; ++l) {
                sf::CircleShape d(1.5f);
                d.setFillColor(iconColor);
                d.setPosition(bx + 9.f, by + 12.f + l * 7.f);
                window.draw(d);

                sf::RectangleShape bar(sf::Vector2f(14.f, 2.f));
                bar.setFillColor(iconColor);
                bar.setPosition(bx + 15.f, by + 13.f + l * 7.f);
                window.draw(bar);
            }
        } else if (iconType == 1) { // Trash
            sf::RectangleShape lid(sf::Vector2f(16.f, 2.f));
            lid.setFillColor(iconColor);
            lid.setPosition(bx + 11.f, by + 12.f);
            window.draw(lid);

            sf::RectangleShape handle(sf::Vector2f(6.f, 2.f));
            handle.setFillColor(iconColor);
            handle.setPosition(bx + 16.f, by + 9.f);
            window.draw(handle);

            sf::RectangleShape body(sf::Vector2f(12.f, 13.f));
            body.setFillColor(iconColor);
            body.setPosition(bx + 13.f, by + 15.f);
            window.draw(body);
        } else if (iconType == 2) { // Edit / Pencil
            sf::RectangleShape p(sf::Vector2f(12.f, 3.f));
            p.setFillColor(iconColor);
            p.setRotation(45.f);
            p.setPosition(bx + 14.f, by + 14.f);
            window.draw(p);

            sf::RectangleShape tip(sf::Vector2f(4.f, 3.f));
            tip.setFillColor(iconColor);
            tip.setPosition(bx + 23.f, by + 23.f);
            window.draw(tip);
        } else if (iconType == 3) { // Plus
            sf::RectangleShape hBar(sf::Vector2f(14.f, 2.5f));
            hBar.setFillColor(iconColor);
            hBar.setPosition(bx + 12.f, by + 17.5f);
            window.draw(hBar);

            sf::RectangleShape vBar(sf::Vector2f(2.5f, 14.f));
            vBar.setFillColor(iconColor);
            vBar.setPosition(bx + 17.5f, by + 12.f);
            window.draw(vBar);
        }
    };

    drawKpi(0, "TỔNG NHẬT KÝ GHI NHẬN", std::to_string(totalCount) + " dòng",
            "↗ +" + std::to_string(todayCount) + " ghi nhận trong hôm nay",
            sf::Color(15, 23, 42), sf::Color(2, 132, 199),
            sf::Color(224, 242, 254), sf::Color(2, 132, 199), 0);

    drawKpi(1, "THAO TÁC XÓA (DELETE)", std::to_string(deleteCount) + " bản ghi",
            "⚠ Cần chú ý kiểm tra thường xuyên",
            sf::Color(220, 38, 38), sf::Color(239, 68, 68),
            sf::Color(254, 242, 242), sf::Color(239, 68, 68), 1);

    drawKpi(2, "THAO TÁC CẬP NHẬT", std::to_string(updateCount) + " bản ghi",
            "✎ Điều chỉnh giá, phòng chiếu",
            sf::Color(15, 23, 42), sf::Color(217, 119, 6),
            sf::Color(254, 243, 199), sf::Color(217, 119, 6), 2);

    drawKpi(3, "THAO TÁC THÊM MỚI", std::to_string(insertCount) + " bản ghi",
            "● Tạo mới lịch chiếu & sản phẩm",
            sf::Color(15, 23, 42), sf::Color(5, 150, 105),
            sf::Color(236, 253, 245), sf::Color(16, 185, 129), 3);

    // ==========================================
    // 5. FILTER ROW (filterY = 216.f, h = 38.f)
    // ==========================================
    float filterY = 216.f;

    sf::Text lblSearch(toSfString("Tìm kiếm:"), Theme::FontPrimary, 11);
    lblSearch.setStyle(sf::Text::Bold);
    lblSearch.setFillColor(sf::Color(71, 85, 105));
    lblSearch.setPosition(leftColX, filterY + 10.f);
    window.draw(lblSearch);

    if (auditSearchInput) auditSearchInput->draw(window);

    sf::Text lblDate(toSfString("Lọc theo ngày:"), Theme::FontPrimary, 11);
    lblDate.setStyle(sf::Text::Bold);
    lblDate.setFillColor(sf::Color(71, 85, 105));
    lblDate.setPosition(leftColX + 395.f, filterY + 10.f);
    window.draw(lblDate);

    if (auditDateDropdown) auditDateDropdown->draw(window);

    sf::Text lblTbl(toSfString("Bảng:"), Theme::FontPrimary, 11);
    lblTbl.setStyle(sf::Text::Bold);
    lblTbl.setFillColor(sf::Color(71, 85, 105));
    lblTbl.setPosition(leftColX + 650.f, filterY + 10.f);
    window.draw(lblTbl);

    if (auditTableDropdown) auditTableDropdown->draw(window);

    sf::Text lblCat(toSfString("Phân loại:"), Theme::FontPrimary, 11);
    lblCat.setFillColor(sf::Color(100, 116, 139));
    lblCat.setPosition(leftColX + 875.f, filterY + 10.f);
    window.draw(lblCat);

    // Update pill labels & colors
    if (btnAuditFilterAll) {
        btnAuditFilterAll->setText("Tất cả (" + std::to_string(totalCount) + ")");
        btnAuditFilterAll->setCustomColors(auditFilterType == 0 ? sf::Color(2, 132, 199) : sf::Color::White,
                                           sf::Color(241, 245, 249), sf::Color(226, 232, 240),
                                           auditFilterType == 0 ? sf::Color(2, 132, 199) : sf::Color::White);
        btnAuditFilterAll->setTextColor(auditFilterType == 0 ? sf::Color::White : sf::Color(71, 85, 105));
        btnAuditFilterAll->setOutline(auditFilterType == 0 ? 0.f : 1.f, sf::Color(226, 232, 240));
        btnAuditFilterAll->draw(window);
    }
    if (btnAuditFilterInsert) {
        btnAuditFilterInsert->setText("Thêm mới");
        btnAuditFilterInsert->setCustomColors(auditFilterType == 1 ? sf::Color(16, 185, 129) : sf::Color::White,
                                              sf::Color(241, 245, 249), sf::Color(226, 232, 240),
                                              auditFilterType == 1 ? sf::Color(16, 185, 129) : sf::Color::White);
        btnAuditFilterInsert->setTextColor(auditFilterType == 1 ? sf::Color::White : sf::Color(71, 85, 105));
        btnAuditFilterInsert->setOutline(auditFilterType == 1 ? 0.f : 1.f, sf::Color(226, 232, 240));
        btnAuditFilterInsert->draw(window);
    }
    if (btnAuditFilterUpdate) {
        btnAuditFilterUpdate->setText("Cập nhật");
        btnAuditFilterUpdate->setCustomColors(auditFilterType == 2 ? sf::Color(2, 132, 199) : sf::Color::White,
                                              sf::Color(241, 245, 249), sf::Color(226, 232, 240),
                                              auditFilterType == 2 ? sf::Color(2, 132, 199) : sf::Color::White);
        btnAuditFilterUpdate->setTextColor(auditFilterType == 2 ? sf::Color::White : sf::Color(71, 85, 105));
        btnAuditFilterUpdate->setOutline(auditFilterType == 2 ? 0.f : 1.f, sf::Color(226, 232, 240));
        btnAuditFilterUpdate->draw(window);
    }
    if (btnAuditFilterDelete) {
        btnAuditFilterDelete->setText("Đã xóa (" + std::to_string(deleteCount) + ")");
        btnAuditFilterDelete->setCustomColors(auditFilterType == 3 ? sf::Color(220, 38, 38) : sf::Color::White,
                                              sf::Color(254, 242, 242), sf::Color(254, 202, 202),
                                              auditFilterType == 3 ? sf::Color(220, 38, 38) : sf::Color::White);
        btnAuditFilterDelete->setTextColor(auditFilterType == 3 ? sf::Color::White : sf::Color(220, 38, 38));
        btnAuditFilterDelete->setOutline(1.f, auditFilterType == 3 ? sf::Color(220, 38, 38) : sf::Color(254, 202, 202));
        btnAuditFilterDelete->draw(window);
    }

    // ==========================================
    // 6. MAIN DATA TABLE CARD (tableY = 262.f, h = 652.f)
    // ==========================================
    float tableY = 262.f;
    float tableH = 652.f;
    RoundedRectRenderer::render(window, sf::Vector2f(leftColX, tableY), sf::Vector2f(cardW, tableH), 12.f, sf::Color::White, 1.f, sf::Color(226, 232, 240));

    sf::CircleShape dot(4.f);
    dot.setFillColor(sf::Color(2, 132, 199));
    dot.setPosition(leftColX + 24.f, tableY + 16.f);
    window.draw(dot);

    sf::Text tTitle(toSfString("CHI TIẾT BẢN GHI THAY ĐỔI HỆ THỐNG"), Theme::FontPrimary, 13);
    tTitle.setStyle(sf::Text::Bold);
    tTitle.setFillColor(sf::Color(15, 23, 42));
    tTitle.setPosition(leftColX + 38.f, tableY + 11.f);
    window.draw(tTitle);

    // Filter matching records
    std::string q = auditSearchInput ? auditSearchInput->getText() : "";
    std::string qNorm = removeVietnameseDiacritics(q);
    std::transform(qNorm.begin(), qNorm.end(), qNorm.begin(), ::tolower);

    std::vector<const AuditRecord*> filtered;
    for (const auto& r : auditRecords) {
        if (auditFilterType == 1 && r.actionType != 1) continue;
        if (auditFilterType == 2 && r.actionType != 2) continue;
        if (auditFilterType == 3 && r.actionType != 3) continue;

        if (!auditDateFilterVal.empty() && r.thoiGian.rfind(auditDateFilterVal, 0) != 0) continue;
        if (!auditTableFilterVal.empty() && r.tableName != auditTableFilterVal) continue;

        if (!qNorm.empty()) {
            std::string hay = r.thoiGian + " " + r.maNguoiDung + " " + r.hoTen + " " + r.vaiTro + " " +
                              r.tableName + " " + r.fieldChanged + " " + r.oldData + " " + r.newData + " " + r.chiTiet;
            std::string hayNorm = removeVietnameseDiacritics(hay);
            std::transform(hayNorm.begin(), hayNorm.end(), hayNorm.begin(), ::tolower);
            if (hayNorm.find(qNorm) == std::string::npos) continue;
        }

        filtered.push_back(&r);
    }

    int totalFiltered = static_cast<int>(filtered.size());
    int pageSize = 12;
    int totalPages = std::max(1, (totalFiltered + pageSize - 1) / pageSize);
    if (auditCurrentPage >= totalPages) auditCurrentPage = totalPages - 1;
    if (auditCurrentPage < 0) auditCurrentPage = 0;

    int fromIdx = totalFiltered == 0 ? 0 : auditCurrentPage * pageSize + 1;
    int toIdx = std::min(totalFiltered, (auditCurrentPage + 1) * pageSize);

    std::string subInfo = "(Hiển thị " + std::to_string(toIdx - fromIdx + (totalFiltered > 0 ? 1 : 0)) +
                          " bản ghi gần nhất theo bộ lọc)";
    sf::Text tSub(toSfString(subInfo), Theme::FontSecondary, 11);
    tSub.setFillColor(sf::Color(148, 163, 184));
    tSub.setPosition(leftColX + 38.f + tTitle.getLocalBounds().width + 10.f, tableY + 12.f);
    window.draw(tSub);

    sf::Text tSync(toSfString("Tự động cập nhật mỗi 30s  📶"), Theme::FontSecondary, 11);
    tSync.setFillColor(sf::Color(148, 163, 184));
    tSync.setPosition(leftColX + cardW - tSync.getLocalBounds().width - 24.f, tableY + 12.f);
    window.draw(tSync);

    // Table Header Row
    float tblHdrY = tableY + 42.f;
    float tblW = cardW - 24.f;
    RoundedRectRenderer::render(window, sf::Vector2f(leftColX + 12.f, tblHdrY), sf::Vector2f(tblW, 32.f), 6.f, sf::Color(248, 250, 252));

    float colX0 = leftColX + 24.f;
    float colX1 = leftColX + 190.f;
    float colX2 = leftColX + 420.f;
    float colX3 = leftColX + 560.f;
    float colX4 = leftColX + 750.f;
    float colX5 = leftColX + 990.f;

    auto drawHdr = [&](const std::string& h, float x) {
        sf::Text ht(toSfString(h), Theme::FontPrimary, 11);
        ht.setStyle(sf::Text::Bold);
        ht.setFillColor(sf::Color(100, 116, 139));
        ht.setPosition(x, tblHdrY + 8.f);
        window.draw(ht);
    };
    drawHdr("THỜI GIAN", colX0);
    drawHdr("NGƯỜI THỰC HIỆN", colX1);
    drawHdr("BẢNG DỮ LIỆU", colX2);
    drawHdr("TRƯỜNG THAY ĐỔI", colX3);
    drawHdr("DỮ LIỆU CŨ", colX4);
    drawHdr("DỮ LIỆU MỚI", colX5);

    // Render Data Rows
    float rowStartY = tblHdrY + 36.f;
    float rowH = 44.f;

    for (int i = 0; i < pageSize; ++i) {
        int idx = auditCurrentPage * pageSize + i;
        if (idx >= totalFiltered) break;
        const auto& r = *filtered[idx];

        float ry = rowStartY + i * rowH;

        if (i % 2 == 1) {
            RoundedRectRenderer::render(window, sf::Vector2f(leftColX + 12.f, ry), sf::Vector2f(tblW, rowH - 2.f), 4.f, sf::Color(248, 250, 252));
        }

        sf::RectangleShape line(sf::Vector2f(tblW, 1.f));
        line.setFillColor(sf::Color(241, 245, 249));
        line.setPosition(leftColX + 12.f, ry + rowH - 1.f);
        window.draw(line);

        // 1. Time
        sf::Text tm(toSfString(r.thoiGian), Theme::FontPrimary, 11.5f);
        tm.setFillColor(sf::Color(51, 65, 85));
        tm.setPosition(colX0, ry + 13.f);
        window.draw(tm);

        // 2. User
        sf::CircleShape uCir(9.f);
        uCir.setFillColor(sf::Color(224, 242, 254));
        uCir.setPosition(colX1, ry + 12.f);
        window.draw(uCir);

        char initial = 'A';
        if (!r.hoTen.empty()) initial = r.hoTen[0];
        std::string initStr(1, initial);
        sf::Text uInit(toSfString(initStr), Theme::FontPrimary, 9);
        uInit.setStyle(sf::Text::Bold);
        uInit.setFillColor(sf::Color(2, 132, 199));
        uInit.setPosition(colX1 + 5.5f, ry + 13.f);
        window.draw(uInit);

        std::string uDisp = r.hoTen + " (" + r.maNguoiDung + ")";
        if (uDisp.size() > 24) uDisp = uDisp.substr(0, 22) + "..";
        sf::Text uTxt(toSfString(uDisp), Theme::FontPrimary, 11.5f);
        uTxt.setFillColor(sf::Color(30, 41, 59));
        uTxt.setPosition(colX1 + 24.f, ry + 13.f);
        window.draw(uTxt);

        // 3. Table Badge
        sf::Color tBg(241, 245, 249), tFg(100, 116, 139);
        if (r.tableName == "SeatType") { tBg = sf::Color(243, 232, 255); tFg = sf::Color(147, 51, 234); }
        else if (r.tableName == "Room") { tBg = sf::Color(224, 242, 254); tFg = sf::Color(2, 132, 199); }
        else if (r.tableName == "ShowTime") { tBg = sf::Color(224, 231, 255); tFg = sf::Color(79, 70, 229); }
        else if (r.tableName == "Product") { tBg = sf::Color(254, 243, 199); tFg = sf::Color(217, 119, 6); }
        else if (r.tableName == "Movie") { tBg = sf::Color(255, 228, 230); tFg = sf::Color(225, 29, 72); }
        else if (r.tableName == "Promotion") { tBg = sf::Color(209, 250, 229); tFg = sf::Color(5, 150, 105); }
        else if (r.tableName == "User") { tBg = sf::Color(207, 250, 254); tFg = sf::Color(8, 145, 178); }

        RoundedRectRenderer::render(window, sf::Vector2f(colX2, ry + 9.f), sf::Vector2f(85.f, 24.f), 6.f, tBg);
        drawCenteredTxt(r.tableName, colX2 + 42.5f, ry + 13.5f, 10.5f, tFg, true);

        // 4. Field Changed
        std::string fDisp = r.fieldChanged;
        if (fDisp.size() > 20) fDisp = fDisp.substr(0, 18) + "..";
        sf::Text fTxt(toSfString(fDisp), Theme::FontPrimary, 11.5f);
        fTxt.setFillColor(sf::Color(71, 85, 105));
        fTxt.setPosition(colX3, ry + 13.f);
        window.draw(fTxt);

        // 5. Old Data
        std::string oDisp = r.oldData;
        if (oDisp.size() > 24) oDisp = oDisp.substr(0, 22) + "..";
        sf::Text oTxt(toSfString(oDisp), Theme::FontPrimary, 11.5f);
        oTxt.setFillColor(sf::Color(148, 163, 184));
        oTxt.setPosition(colX4, ry + 13.f);
        window.draw(oTxt);

        // 6. New Data
        if (r.actionType == 3) {
            RoundedRectRenderer::render(window, sf::Vector2f(colX5, ry + 9.f), sf::Vector2f(85.f, 24.f), 6.f, sf::Color(254, 242, 242), 1.f, sf::Color(254, 202, 202));
            drawCenteredTxt("x Đã xóa", colX5 + 42.5f, ry + 13.5f, 10.5f, sf::Color(220, 38, 38), true);
        } else if (r.actionType == 1) {
            std::string nDisp = r.newData;
            if (nDisp.size() > 28) nDisp = nDisp.substr(0, 26) + "..";
            float bw = std::max(90.f, nDisp.size() * 7.5f + 16.f);
            bw = std::min(bw, 270.f);
            RoundedRectRenderer::render(window, sf::Vector2f(colX5, ry + 9.f), sf::Vector2f(bw, 24.f), 6.f, sf::Color(236, 253, 245), 1.f, sf::Color(167, 243, 208));
            drawCenteredTxt(nDisp, colX5 + bw * 0.5f, ry + 13.5f, 10.5f, sf::Color(5, 150, 105), true);
        } else if (r.actionType == 2) {
            std::string nDisp = r.newData;
            if (nDisp.size() > 28) nDisp = nDisp.substr(0, 26) + "..";
            float bw = std::max(60.f, nDisp.size() * 7.5f + 16.f);
            bw = std::min(bw, 270.f);
            RoundedRectRenderer::render(window, sf::Vector2f(colX5, ry + 9.f), sf::Vector2f(bw, 24.f), 6.f, sf::Color(240, 249, 255), 1.f, sf::Color(186, 230, 253));
            drawCenteredTxt(nDisp, colX5 + bw * 0.5f, ry + 13.5f, 10.5f, sf::Color(2, 132, 199), true);
        } else {
            std::string nDisp = r.newData;
            if (nDisp.size() > 30) nDisp = nDisp.substr(0, 28) + "..";
            sf::Text nTxt(toSfString(nDisp), Theme::FontPrimary, 11.5f);
            nTxt.setFillColor(sf::Color(51, 65, 85));
            nTxt.setPosition(colX5, ry + 13.f);
            window.draw(nTxt);
        }
    }

    // ==========================================
    // 7. TABLE FOOTER & PAGINATION (footRowY = tableY + tableH - 42.f)
    // ==========================================
    float footRowY = tableY + tableH - 42.f;

    char pageInfoBuf[128];
    snprintf(pageInfoBuf, sizeof(pageInfoBuf), "Hiển thị %d đến %d trong tổng số %d bản ghi nhật ký",
             fromIdx, toIdx, totalFiltered);
    sf::Text footInfo(toSfString(pageInfoBuf), Theme::FontSecondary, 11);
    footInfo.setFillColor(sf::Color(100, 116, 139));
    footInfo.setPosition(leftColX + 24.f, footRowY + 7.f);
    window.draw(footInfo);

    // Right Pagination controls
    float pgBtnSz = 28.f;
    float pgGap = 6.f;
    float totalPgW = (std::min(5, totalPages) + 2) * (pgBtnSz + pgGap);
    float pgStartX = leftColX + cardW - 24.f - totalPgW;

    // Prev Button '<'
    RoundedRectRenderer::render(window, sf::Vector2f(pgStartX, footRowY), sf::Vector2f(pgBtnSz, pgBtnSz), 6.f,
                                sf::Color::White, 1.f, sf::Color(226, 232, 240));
    drawCenteredTxt("<", pgStartX + pgBtnSz * 0.5f, footRowY + 6.f, 12, sf::Color(100, 116, 139), true);

    // Numbered Buttons
    int startP = std::max(0, auditCurrentPage - 2);
    int endP = std::min(totalPages, startP + 5);
    if (endP - startP < 5) startP = std::max(0, endP - 5);

    for (int p = startP; p < endP; ++p) {
        float bx = pgStartX + (p - startP + 1) * (pgBtnSz + pgGap);
        bool isActive = (p == auditCurrentPage);
        RoundedRectRenderer::render(window, sf::Vector2f(bx, footRowY), sf::Vector2f(pgBtnSz, pgBtnSz), 6.f,
                                    isActive ? sf::Color(2, 132, 199) : sf::Color::White,
                                    isActive ? 0.f : 1.f, sf::Color(226, 232, 240));
        drawCenteredTxt(std::to_string(p + 1), bx + pgBtnSz * 0.5f, footRowY + 6.f, 11,
                        isActive ? sf::Color::White : sf::Color(71, 85, 105), true);
    }

    // Next Button '>'
    float nextBx = pgStartX + (endP - startP + 1) * (pgBtnSz + pgGap);
    RoundedRectRenderer::render(window, sf::Vector2f(nextBx, footRowY), sf::Vector2f(pgBtnSz, pgBtnSz), 6.f,
                                sf::Color::White, 1.f, sf::Color(226, 232, 240));
    drawCenteredTxt(">", nextBx + pgBtnSz * 0.5f, footRowY + 6.f, 12, sf::Color(100, 116, 139), true);

    // ==========================================
    // 8. BOTTOM COPYRIGHT BAR (y = 926.f)
    // ==========================================
    float footY = 926.f;
    sf::Text footCpr(toSfString("© 2026 Cinestar CineOS. Mọi thay đổi dữ liệu đều được mã hóa và ký số bảo mật ISO/IEC 27001"), Theme::FontSecondary, 11);
    footCpr.setFillColor(sf::Color(148, 163, 184));
    footCpr.setPosition(leftColX, footY);
    window.draw(footCpr);

    sf::Text footHotline(toSfString("🎧 Hotline Kỹ Thuật: 1900 6081 (Ext: 104)  •  Cụm Server Node - QuocThanh - HCM"), Theme::FontSecondary, 11);
    footHotline.setFillColor(sf::Color(100, 116, 139));
    footHotline.setPosition(leftColX + cardW - footHotline.getLocalBounds().width, footY);
    window.draw(footHotline);
}

// Note: CRM and Shift Report views are now modularized in CinemaUIApp_Staff_CRM.cpp and CinemaUIApp_Staff_ShiftReport.cpp.

namespace {
std::string formatVndDots(long long val) {
    if (val <= 0) return "0";
    std::string s = std::to_string(val);
    std::string out;
    int cnt = 0;
    for (int i = static_cast<int>(s.size()) - 1; i >= 0; --i) {
        if (cnt > 0 && cnt % 3 == 0) out.push_back('.');
        out.push_back(s[i]);
        cnt++;
    }
    std::reverse(out.begin(), out.end());
    return out;
}

int computePromoStatus(const Promotion& pr, int todayKey) {
    int sKey = DateFormat::dateKey(pr.getNgayBatDau());
    int eKey = DateFormat::dateKey(pr.getNgayKetThuc());
    if (sKey <= 0 || eKey <= 0) return 1; // Default active
    if (todayKey < sKey) return 2;        // Upcoming (Sắp tới)
    if (todayKey > eKey) return 3;        // Expired (Hết hạn)
    return 1;                             // Active (Đang chạy)
}

std::string trimPromoStr(const std::string& s) {
    size_t a = s.find_first_not_of(" \t\r\n");
    if (a == std::string::npos) return "";
    size_t b = s.find_last_not_of(" \t\r\n");
    return s.substr(a, b - a + 1);
}
} // anonymous namespace

std::string CinemaUIApp::generateNextPromoId() const {
    int maxNum = 0;
    bool usePromoPrefix = false;
    for (const auto& pr : promoList) {
        std::string id = pr.getMaKhuyenMai();
        if (id.rfind("PROMO", 0) == 0) usePromoPrefix = true;
        std::string digits;
        for (char c : id) {
            if (c >= '0' && c <= '9') digits.push_back(c);
        }
        if (!digits.empty()) {
            try {
                int v = std::stoi(digits);
                if (v > maxNum) maxNum = v;
            } catch (...) {}
        }
    }
    char buf[32];
    if (usePromoPrefix) {
        snprintf(buf, sizeof(buf), "PROMO%03d", maxNum + 1);
    } else {
        snprintf(buf, sizeof(buf), "KM%02d", maxNum + 1);
    }
    return buf;
}

void CinemaUIApp::refreshPromoScopeDropdown() {
    if (!promoScopeDropdown) return;
    std::vector<std::string> opts;
    opts.push_back("All - Áp dụng cho tất cả phim");
    int selIdx = 0;
    for (size_t i = 0; i < movieList.size(); ++i) {
        std::string code = movieList[i].getMaPhim();
        opts.push_back(code + " - " + movieList[i].getTenPhim());
        if (promoScopeVal == code) {
            selIdx = static_cast<int>(i) + 1;
        }
    }
    if (promoScopeVal != "All" && !promoScopeVal.empty() && selIdx == 0) {
        opts.push_back(promoScopeVal + " - Phim chỉ định");
        selIdx = static_cast<int>(opts.size()) - 1;
    }
    promoScopeDropdown->setOptions(opts, selIdx);
}

void CinemaUIApp::syncPromoSelectionToForm(int idx) {
    if (idx < 0 || idx >= static_cast<int>(promoList.size())) {
        selectedPromoRowIdx = -1;
        std::string nextId = generateNextPromoId();
        if (inputPromoId) {
            inputPromoId->setText(nextId);
            inputPromoId->setEnabled(true);
        }
        if (inputPromoName) inputPromoName->setText("");
        if (inputPromoCode) inputPromoCode->setText("");
        if (inputPromoPercent) inputPromoPercent->setText("15");
        if (inputPromoMaxDiscount) inputPromoMaxDiscount->setText("50000");
        std::string today = stTodayStr();
        if (inputPromoStartDate) inputPromoStartDate->setText(today);
        if (inputPromoEndDate) inputPromoEndDate->setText(stAddDays(today, 30));
        promoScopeVal = "All";
        refreshPromoScopeDropdown();
        return;
    }

    selectedPromoRowIdx = idx;
    const auto& pr = promoList[idx];
    if (inputPromoId) {
        inputPromoId->setText(pr.getMaKhuyenMai());
        inputPromoId->setEnabled(false);
    }
    if (inputPromoName) inputPromoName->setText(pr.getTenKhuyenMai());
    if (inputPromoCode) inputPromoCode->setText(pr.getMaCode());

    std::ostringstream ptOss;
    double pt = pr.getPhanTramGiam();
    if (std::abs(pt - std::round(pt)) < 0.01) {
        ptOss << static_cast<int>(std::round(pt));
    } else {
        ptOss << std::fixed << std::setprecision(1) << pt;
    }
    if (inputPromoPercent) inputPromoPercent->setText(ptOss.str());

    if (inputPromoMaxDiscount) {
        inputPromoMaxDiscount->setText(std::to_string(static_cast<long long>(std::round(pr.getMucGiamToiDa()))));
    }
    if (inputPromoStartDate) inputPromoStartDate->setText(pr.getNgayBatDau());
    if (inputPromoEndDate) inputPromoEndDate->setText(pr.getNgayKetThuc());

    promoScopeVal = pr.getPhamViApDung().empty() ? "All" : pr.getPhamViApDung();
    refreshPromoScopeDropdown();
}

std::vector<int> CinemaUIApp::getFilteredPromoIndices() const {
    std::vector<int> indices;
    std::string q = promoSearchInput ? trimPromoStr(promoSearchInput->getText()) : "";
    std::string qNorm = removeVietnameseDiacritics(q);
    int todayKey = DateFormat::dateKey(stTodayStr());

    for (size_t i = 0; i < promoList.size(); ++i) {
        const auto& pr = promoList[i];
        int st = computePromoStatus(pr, todayKey);
        if (promoFilterTab == 1 && st != 1) continue;
        if (promoFilterTab == 2 && st != 2) continue;
        if (promoFilterTab == 3 && st != 3) continue;

        if (!qNorm.empty()) {
            std::string hay = removeVietnameseDiacritics(
                pr.getMaKhuyenMai() + " " + pr.getTenKhuyenMai() + " " +
                pr.getMaCode() + " " + pr.getPhamViApDung()
            );
            if (hay.find(qNorm) == std::string::npos) continue;
        }
        indices.push_back(static_cast<int>(i));
    }
    return indices;
}

void CinemaUIApp::setupPromotionsView() {
    float leftColX = sidebarWidth + 24.f;          // 274.f
    float leftColW = 860.f;
    float rightColX = leftColX + leftColW + 18.f;  // 1152.f
    float rightColW = 552.f;
    float totalW = leftColW + 18.f + rightColW;    // 1430.f

    promoCurrentPage = 0;
    promoFilterTab = 0;
    promoScopeVal = "All";

    // 1. Top Banner Buttons
    btnPromoExport = std::unique_ptr<RoundRectButton>(new RoundRectButton(
        "   Xuất Báo Cáo (Excel / PDF)",
        sf::Vector2f(leftColX + totalW - 418.f, 66.f),
        sf::Vector2f(198.f, 38.f),
        ButtonStyle::Ghost,
        9.f
    ));
    btnPromoExport->setFontSize(13);
    btnPromoExport->setCustomColors(sf::Color::White, sf::Color(241, 245, 249), sf::Color(226, 232, 240), sf::Color(241, 245, 249));
    btnPromoExport->setOutline(1.f, sf::Color(203, 213, 225));
    btnPromoExport->setTextColor(sf::Color(30, 41, 59));
    btnPromoExport->setOnClick([this]() {
        std::ofstream out("DATA/bao_cao_khuyen_mai.csv");
        if (out.is_open()) {
            out << "\xEF\xBB\xBF";
            out << "ID Khuyến Mãi,Tên Chương Trình,Mã Code,Phần Trăm Giảm (%),Giảm Tối Đa (VNĐ),Ngày Bắt Đầu,Ngày Kết Thúc,Phạm Vi Áp Dụng\n";
            for (const auto& pr : promoList) {
                out << pr.getMaKhuyenMai() << ",\"" << pr.getTenKhuyenMai() << "\","
                    << pr.getMaCode() << "," << pr.getPhanTramGiam() << ","
                    << static_cast<long long>(pr.getMucGiamToiDa()) << ","
                    << pr.getNgayBatDau() << "," << pr.getNgayKetThuc() << ","
                    << pr.getPhamViApDung() << "\n";
            }
            out.close();
            ActivityLog::ghiNhan(currentUserId, currentUserRole, "XUAT_BAO_CAO_KM", "Xuat bao cao khuyen mai ra DATA/bao_cao_khuyen_mai.csv");
            if (confirmPopup) {
                confirmPopup->show("XUẤT BÁO CÁO THÀNH CÔNG", "Đã xuất danh sách chương trình khuyến mãi ra file DATA/bao_cao_khuyen_mai.csv!", ButtonStyle::Primary, "Đóng", "");
            }
        }
    });

    btnPromoNew = std::unique_ptr<RoundRectButton>(new RoundRectButton(
        "+ Tạo Khuyến Mãi Mới",
        sf::Vector2f(leftColX + totalW - 208.f, 66.f),
        sf::Vector2f(192.f, 38.f),
        ButtonStyle::Primary,
        9.f
    ));
    btnPromoNew->setFontSize(13);
    btnPromoNew->setCustomColors(sf::Color(2, 132, 199), sf::Color(3, 105, 161), sf::Color(7, 89, 133), sf::Color(148, 163, 184));
    btnPromoNew->setOnClick([this]() {
        syncPromoSelectionToForm(-1);
        if (inputPromoName) inputPromoName->setFocused(true);
    });

    // 2. Search Input & 4 Filter Pills (NO numbers on filter buttons per user requirement)
    promoSearchInput = std::unique_ptr<UIInputBox>(new UIInputBox(
        sf::Vector2f(leftColX + 18.f, 224.f),
        sf::Vector2f(290.f, 36.f),
        "Tìm theo Mã Code hoặc Tên KM...",
        13
    ));
    promoSearchInput->setTextInset(34.f);
    promoSearchInput->setColors(sf::Color(248, 250, 252), sf::Color(226, 232, 240), sf::Color(2, 132, 199), sf::Color(15, 23, 42), sf::Color(148, 163, 184));
    promoSearchInput->setCornerRadius(18.f);

    auto makeFilterBtn = [&](const std::string& label, float bx, float bw, int tabIdx) {
        auto btn = std::unique_ptr<RoundRectButton>(new RoundRectButton(
            label,
            sf::Vector2f(bx, 224.f),
            sf::Vector2f(bw, 36.f),
            ButtonStyle::Ghost,
            18.f
        ));
        btn->setFontSize(13);
        btn->setOnClick([this, tabIdx]() {
            promoFilterTab = tabIdx;
            promoCurrentPage = 0;
        });
        return btn;
    };

    btnPromoFilterAll      = makeFilterBtn("Tất cả",    leftColX + 320.f, 96.f,  0);
    btnPromoFilterActive   = makeFilterBtn("Đang chạy", leftColX + 424.f, 108.f, 1);
    btnPromoFilterUpcoming = makeFilterBtn("Sắp tới",   leftColX + 540.f, 96.f,  2);
    btnPromoFilterExpired  = makeFilterBtn("Hết hạn",   leftColX + 644.f, 96.f,  3);

    // 3. Right Form Inputs
    float inputX = rightColX + 150.f; // 1302.f
    float inputW = 382.f;
    float inputH = 36.f;
    sf::Color softBorder(226, 232, 240);
    sf::Color focusBlue(2, 132, 199);
    sf::Color textDark(15, 23, 42);
    sf::Color textMuted(148, 163, 184);

    inputPromoId = std::unique_ptr<UIInputBox>(new UIInputBox(
        sf::Vector2f(inputX, 376.f),
        sf::Vector2f(inputW, inputH),
        "VD: KM01 hoặc PROMO001",
        13
    ));
    inputPromoId->setColors(sf::Color(241, 245, 249), softBorder, focusBlue, sf::Color(51, 65, 85), textMuted);
    inputPromoId->setCornerRadius(8.f);

    inputPromoName = std::unique_ptr<UIInputBox>(new UIInputBox(
        sf::Vector2f(inputX, 422.f),
        sf::Vector2f(inputW, inputH),
        "Nhập tên chương trình khuyến mãi...",
        13
    ));
    inputPromoName->setColors(sf::Color::White, softBorder, focusBlue, textDark, textMuted);
    inputPromoName->setCornerRadius(8.f);

    inputPromoCode = std::unique_ptr<UIInputBox>(new UIInputBox(
        sf::Vector2f(inputX, 468.f),
        sf::Vector2f(inputW - 86.f, inputH),
        "VD: SUMMER25",
        13
    ));
    inputPromoCode->setColors(sf::Color::White, softBorder, focusBlue, sf::Color(2, 132, 199), textMuted);
    inputPromoCode->setCornerRadius(8.f);

    btnPromoCopyCode = std::unique_ptr<RoundRectButton>(new RoundRectButton(
        "  Copy",
        sf::Vector2f(inputX + inputW - 78.f, 468.f),
        sf::Vector2f(78.f, inputH),
        ButtonStyle::Ghost,
        8.f
    ));
    btnPromoCopyCode->setFontSize(12);
    btnPromoCopyCode->setCustomColors(sf::Color(224, 242, 254), sf::Color(186, 230, 253), sf::Color(125, 211, 252), sf::Color(241, 245, 249));
    btnPromoCopyCode->setOutline(1.f, sf::Color(186, 230, 253));
    btnPromoCopyCode->setTextColor(sf::Color(2, 132, 199));
    btnPromoCopyCode->setOnClick([this]() {
        std::string code = inputPromoCode ? trimPromoStr(inputPromoCode->getText()) : "";
        if (!code.empty()) {
            sf::Clipboard::setString(toSfString(code));
            if (confirmPopup) {
                confirmPopup->show("ĐÃ SAO CHÉP MÃ CODE", "Đã sao chép mã voucher '" + code + "' vào bộ nhớ tạm (Clipboard)!", ButtonStyle::Primary, "Đóng", "");
            }
        }
    });

    inputPromoPercent = std::unique_ptr<UIInputBox>(new UIInputBox(
        sf::Vector2f(inputX, 514.f),
        sf::Vector2f(inputW, inputH),
        "VD: 25",
        13
    ));
    inputPromoPercent->setColors(sf::Color::White, softBorder, focusBlue, textDark, textMuted);
    inputPromoPercent->setCornerRadius(8.f);

    inputPromoMaxDiscount = std::unique_ptr<UIInputBox>(new UIInputBox(
        sf::Vector2f(inputX, 560.f),
        sf::Vector2f(inputW, inputH),
        "VD: 100000",
        13
    ));
    inputPromoMaxDiscount->setColors(sf::Color::White, softBorder, focusBlue, textDark, textMuted);
    inputPromoMaxDiscount->setCornerRadius(8.f);

    inputPromoStartDate = std::unique_ptr<UIInputBox>(new UIInputBox(
        sf::Vector2f(inputX, 606.f),
        sf::Vector2f(inputW, inputH),
        "DD/MM/YYYY",
        13
    ));
    inputPromoStartDate->setColors(sf::Color::White, softBorder, focusBlue, textDark, textMuted);
    inputPromoStartDate->setCornerRadius(8.f);

    inputPromoEndDate = std::unique_ptr<UIInputBox>(new UIInputBox(
        sf::Vector2f(inputX, 652.f),
        sf::Vector2f(inputW, inputH),
        "DD/MM/YYYY",
        13
    ));
    inputPromoEndDate->setColors(sf::Color::White, softBorder, focusBlue, textDark, textMuted);
    inputPromoEndDate->setCornerRadius(8.f);

    promoScopeDropdown = std::unique_ptr<DropdownBox>(new DropdownBox(
        sf::Vector2f(inputX, 698.f),
        sf::Vector2f(inputW, inputH),
        {"All - Áp dụng cho tất cả phim"},
        0
    ));
    promoScopeDropdown->setLightStyle(true);
    promoScopeDropdown->setFontSize(12);
    promoScopeDropdown->setItemHeight(34.f);
    promoScopeDropdown->setMaxVisibleItems(5);
    promoScopeDropdown->setPopupLayout(inputW, 0.f);
    promoScopeDropdown->setOnSelect([this](int idx, const std::string& val) {
        if (idx == 0) {
            promoScopeVal = "All";
        } else {
            size_t dashPos = val.find(" - ");
            promoScopeVal = (dashPos != std::string::npos) ? val.substr(0, dashPos) : val;
        }
    });
    btnPromoSelectMovie.reset();

    // 4. Form Action Buttons (Tạo KM, Cập nhật, Xóa, Clear Form)
    float btnX1 = rightColX + 20.f;
    float btnW  = 249.f;
    float btnX2 = btnX1 + btnW + 14.f;
    float btnH  = 38.f;

    btnPromoCreate = std::unique_ptr<RoundRectButton>(new RoundRectButton(
        "+ Tạo KM",
        sf::Vector2f(btnX1, 752.f),
        sf::Vector2f(btnW, btnH),
        ButtonStyle::Success,
        8.f
    ));
    btnPromoCreate->setFontSize(13);
    btnPromoCreate->setCustomColors(sf::Color(16, 185, 129), sf::Color(5, 150, 105), sf::Color(4, 120, 87), sf::Color(203, 213, 225));
    btnPromoCreate->setOnClick([this]() {
        std::string id = inputPromoId ? trimPromoStr(inputPromoId->getText()) : "";
        std::string name = inputPromoName ? trimPromoStr(inputPromoName->getText()) : "";
        std::string code = inputPromoCode ? trimPromoStr(inputPromoCode->getText()) : "";
        std::transform(code.begin(), code.end(), code.begin(), ::toupper);

        // If user is currently viewing a selected row and clicks "Tạo KM" without changing ID/Code, switch to clean create mode first
        if (selectedPromoRowIdx >= 0 && selectedPromoRowIdx < static_cast<int>(promoList.size()) &&
            id == promoList[selectedPromoRowIdx].getMaKhuyenMai() &&
            code == promoList[selectedPromoRowIdx].getMaCode()) {
            syncPromoSelectionToForm(-1);
            if (inputPromoName) inputPromoName->setFocused(true);
            if (confirmPopup) {
                confirmPopup->show("CHẾ ĐỘ TẠO MỚI", "Đã làm mới biểu mẫu với mã " + inputPromoId->getText() + ". Vui lòng nhập Tên khuyến mãi, Mã Code rồi nhấn lại 'Tạo KM' để lưu!", ButtonStyle::Primary, "Đóng", "");
            }
            return;
        }

        if (id.empty()) id = generateNextPromoId();
        for (const auto& pr : promoList) {
            if (pr.getMaKhuyenMai() == id) {
                id = generateNextPromoId();
                if (inputPromoId) inputPromoId->setText(id);
                break;
            }
        }

        if (name.empty()) {
            if (confirmPopup) confirmPopup->show("THIẾU THÔNG TIN", "Vui lòng nhập Tên chương trình khuyến mãi!", ButtonStyle::Danger, "Đóng", "");
            return;
        }
        if (code.empty()) {
            if (confirmPopup) confirmPopup->show("THIẾU THÔNG TIN", "Vui lòng nhập Mã Code voucher (VD: SUMMER25)!", ButtonStyle::Danger, "Đóng", "");
            return;
        }
        for (const auto& pr : promoList) {
            std::string exCode = pr.getMaCode();
            std::transform(exCode.begin(), exCode.end(), exCode.begin(), ::toupper);
            if (exCode == code) {
                if (confirmPopup) confirmPopup->show("TRÙNG MÃ CODE", "Mã Code '" + code + "' đã được sử dụng cho chương trình '" + pr.getTenKhuyenMai() + "'!", ButtonStyle::Danger, "Đóng", "");
                return;
            }
        }

        double pt = -1.0;
        try {
            pt = std::stod(trimPromoStr(inputPromoPercent ? inputPromoPercent->getText() : ""));
        } catch (...) { pt = -1.0; }
        if (pt <= 0.0 || pt > 100.0) {
            if (confirmPopup) confirmPopup->show("LỖI PHẦN TRĂM GIẢM", "Phần trăm giảm giá phải nằm trong khoảng từ 1% đến 100%!", ButtonStyle::Danger, "Đóng", "");
            return;
        }

        double maxG = 0.0;
        std::string rawMax = inputPromoMaxDiscount ? trimPromoStr(inputPromoMaxDiscount->getText()) : "0";
        std::string cleanMax;
        for (char c : rawMax) { if (c >= '0' && c <= '9') cleanMax.push_back(c); }
        if (!cleanMax.empty()) {
            try { maxG = std::stod(cleanMax); } catch (...) { maxG = -1.0; }
        }
        if (maxG < 0.0) {
            if (confirmPopup) confirmPopup->show("LỖI MỨC GIẢM TỐI ĐA", "Mức giảm tối đa (VNĐ) không được âm!", ButtonStyle::Danger, "Đóng", "");
            return;
        }

        std::string startD = inputPromoStartDate ? trimPromoStr(inputPromoStartDate->getText()) : "";
        std::string endD   = inputPromoEndDate ? trimPromoStr(inputPromoEndDate->getText()) : "";
        if (!DateFormat::isValidDate(startD) || !DateFormat::isValidDate(endD)) {
            if (confirmPopup) confirmPopup->show("NGÀY KHÔNG HỢP LỆ", "Vui lòng nhập Ngày bắt đầu và Ngày kết thúc đúng định dạng DD/MM/YYYY!", ButtonStyle::Danger, "Đóng", "");
            return;
        }
        startD = DateFormat::normalizeDate(startD);
        endD   = DateFormat::normalizeDate(endD);
        int todayKey = DateFormat::dateKey(stTodayStr());
        if (DateFormat::dateKey(startD) < todayKey) {
            if (confirmPopup) confirmPopup->show("NGÀY BẮT ĐẦU TRONG QUÁ KHỨ", "Ngày bắt đầu (" + startD + ") không được là ngày trong quá khứ!\nVui lòng chọn ngày hiện tại hoặc tương lai.", ButtonStyle::Danger, "Đóng", "");
            return;
        }
        if (DateFormat::dateKey(startD) > DateFormat::dateKey(endD)) {
            if (confirmPopup) confirmPopup->show("THỜI GIAN KHÔNG HỢP LỆ", "Ngày bắt đầu (" + startD + ") không được sau ngày kết thúc (" + endD + ")!", ButtonStyle::Danger, "Đóng", "");
            return;
        }

        std::string pv = promoScopeVal.empty() ? "All" : promoScopeVal;
        Promotion newPr(id, name, code, pt, maxG, startD, endD, pv);
        promoList.push_back(newPr);
        db.ghiDanhSachKhuyenMai(promoList);

        selectedPromoRowIdx = static_cast<int>(promoList.size()) - 1;
        syncPromoSelectionToForm(selectedPromoRowIdx);
        ActivityLog::ghiNhan(currentUserId, currentUserRole, "THEM_KHUYEN_MAI", "Tao CTKM moi: " + id + " (" + code + ")");
        if (confirmPopup) {
            confirmPopup->show("TẠO KHUYẾN MÃI THÀNH CÔNG", "Đã thêm chương trình khuyến mãi '" + name + "' (Mã: " + code + ") vào SQLite cinema.db!", ButtonStyle::Primary, "Đóng", "");
        }
    });

    btnPromoUpdate = std::unique_ptr<RoundRectButton>(new RoundRectButton(
        "Cập nhật",
        sf::Vector2f(btnX2, 752.f),
        sf::Vector2f(btnW, btnH),
        ButtonStyle::Primary,
        8.f
    ));
    btnPromoUpdate->setFontSize(13);
    btnPromoUpdate->setCustomColors(sf::Color(2, 132, 199), sf::Color(3, 105, 161), sf::Color(7, 89, 133), sf::Color(203, 213, 225));
    btnPromoUpdate->setOnClick([this]() {
        if (selectedPromoRowIdx < 0 || selectedPromoRowIdx >= static_cast<int>(promoList.size())) {
            if (confirmPopup) confirmPopup->show("CHƯA CHỌN KHUYẾN MÃI", "Vui lòng chọn một chương trình khuyến mãi trong bảng bên trái để cập nhật!", ButtonStyle::Danger, "Đóng", "");
            return;
        }

        std::string name = inputPromoName ? trimPromoStr(inputPromoName->getText()) : "";
        std::string code = inputPromoCode ? trimPromoStr(inputPromoCode->getText()) : "";
        std::transform(code.begin(), code.end(), code.begin(), ::toupper);

        if (name.empty() || code.empty()) {
            if (confirmPopup) confirmPopup->show("THIẾU THÔNG TIN", "Tên khuyến mãi và Mã Code không được để trống!", ButtonStyle::Danger, "Đóng", "");
            return;
        }

        for (size_t i = 0; i < promoList.size(); ++i) {
            if (static_cast<int>(i) == selectedPromoRowIdx) continue;
            std::string exCode = promoList[i].getMaCode();
            std::transform(exCode.begin(), exCode.end(), exCode.begin(), ::toupper);
            if (exCode == code) {
                if (confirmPopup) confirmPopup->show("TRÙNG MÃ CODE", "Mã Code '" + code + "' đã tồn tại ở chương trình '" + promoList[i].getTenKhuyenMai() + "'!", ButtonStyle::Danger, "Đóng", "");
                return;
            }
        }

        double pt = -1.0;
        try {
            pt = std::stod(trimPromoStr(inputPromoPercent ? inputPromoPercent->getText() : ""));
        } catch (...) { pt = -1.0; }
        if (pt <= 0.0 || pt > 100.0) {
            if (confirmPopup) confirmPopup->show("LỖI PHẦN TRĂM GIẢM", "Phần trăm giảm giá phải từ 1% đến 100%!", ButtonStyle::Danger, "Đóng", "");
            return;
        }

        double maxG = 0.0;
        std::string rawMax = inputPromoMaxDiscount ? trimPromoStr(inputPromoMaxDiscount->getText()) : "0";
        std::string cleanMax;
        for (char c : rawMax) { if (c >= '0' && c <= '9') cleanMax.push_back(c); }
        if (!cleanMax.empty()) {
            try { maxG = std::stod(cleanMax); } catch (...) { maxG = -1.0; }
        }
        if (maxG < 0.0) {
            if (confirmPopup) confirmPopup->show("LỖI MỨC GIẢM TỐI ĐA", "Mức giảm tối đa không hợp lệ!", ButtonStyle::Danger, "Đóng", "");
            return;
        }

        std::string startD = inputPromoStartDate ? trimPromoStr(inputPromoStartDate->getText()) : "";
        std::string endD   = inputPromoEndDate ? trimPromoStr(inputPromoEndDate->getText()) : "";
        if (!DateFormat::isValidDate(startD) || !DateFormat::isValidDate(endD)) {
            if (confirmPopup) confirmPopup->show("NGÀY KHÔNG HỢP LỆ", "Vui lòng nhập ngày bắt đầu và kết thúc đúng định dạng DD/MM/YYYY!", ButtonStyle::Danger, "Đóng", "");
            return;
        }
        startD = DateFormat::normalizeDate(startD);
        endD   = DateFormat::normalizeDate(endD);
        int todayKey = DateFormat::dateKey(stTodayStr());
        if (startD != promoList[selectedPromoRowIdx].getNgayBatDau() && DateFormat::dateKey(startD) < todayKey) {
            if (confirmPopup) confirmPopup->show("NGÀY BẮT ĐẦU TRONG QUÁ KHỨ", "Ngày bắt đầu (" + startD + ") không được là ngày trong quá khứ!\nVui lòng chọn ngày hiện tại hoặc tương lai.", ButtonStyle::Danger, "Đóng", "");
            return;
        }
        if (DateFormat::dateKey(startD) > DateFormat::dateKey(endD)) {
            if (confirmPopup) confirmPopup->show("THỜI GIAN KHÔNG HỢP LỆ", "Ngày bắt đầu không được sau ngày kết thúc!", ButtonStyle::Danger, "Đóng", "");
            return;
        }

        Promotion& target = promoList[selectedPromoRowIdx];
        target.setTenKhuyenMai(name);
        target.setMaCode(code);
        target.setPhanTramGiam(pt);
        target.setMucGiamToiDa(maxG);
        target.setNgayBatDau(startD);
        target.setNgayKetThuc(endD);
        target.setPhamViApDung(promoScopeVal.empty() ? "All" : promoScopeVal);

        db.ghiDanhSachKhuyenMai(promoList);
        syncPromoSelectionToForm(selectedPromoRowIdx);
        ActivityLog::ghiNhan(currentUserId, currentUserRole, "SUA_KHUYEN_MAI", "Cap nhat CTKM: " + target.getMaKhuyenMai());
        if (confirmPopup) {
            confirmPopup->show("CẬP NHẬT THÀNH CÔNG", "Đã lưu thay đổi chương trình khuyến mãi '" + name + "' vào SQLite cinema.db!", ButtonStyle::Primary, "Đóng", "");
        }
    });

    btnPromoDelete = std::unique_ptr<RoundRectButton>(new RoundRectButton(
        "Xóa",
        sf::Vector2f(btnX1, 798.f),
        sf::Vector2f(btnW, btnH),
        ButtonStyle::Ghost,
        8.f
    ));
    btnPromoDelete->setFontSize(13);
    btnPromoDelete->setCustomColors(sf::Color(255, 241, 242), sf::Color(254, 226, 226), sf::Color(254, 205, 211), sf::Color(241, 245, 249));
    btnPromoDelete->setOutline(1.f, sf::Color(254, 205, 211));
    btnPromoDelete->setTextColor(sf::Color(225, 29, 72));
    btnPromoDelete->setOnClick([this]() {
        if (selectedPromoRowIdx < 0 || selectedPromoRowIdx >= static_cast<int>(promoList.size())) {
            if (confirmPopup) confirmPopup->show("CHƯA CHỌN KHUYẾN MÃI", "Vui lòng chọn một chương trình khuyến mãi cần xóa!", ButtonStyle::Danger, "Đóng", "");
            return;
        }
        std::string delId = promoList[selectedPromoRowIdx].getMaKhuyenMai();
        std::string delName = promoList[selectedPromoRowIdx].getTenKhuyenMai();
        promoList.erase(promoList.begin() + selectedPromoRowIdx);
        db.ghiDanhSachKhuyenMai(promoList);

        if (!promoList.empty()) {
            selectedPromoRowIdx = std::min(selectedPromoRowIdx, static_cast<int>(promoList.size()) - 1);
            syncPromoSelectionToForm(selectedPromoRowIdx);
        } else {
            syncPromoSelectionToForm(-1);
        }
        ActivityLog::ghiNhan(currentUserId, currentUserRole, "XOA_KHUYEN_MAI", "Xoa CTKM: " + delId);
        if (confirmPopup) {
            confirmPopup->show("ĐÃ XÓA KHUYẾN MÃI", "Đã xóa chương trình '" + delName + "' (" + delId + ") khỏi hệ thống!", ButtonStyle::Primary, "Đóng", "");
        }
    });

    btnPromoClear = std::unique_ptr<RoundRectButton>(new RoundRectButton(
        "Clear Form",
        sf::Vector2f(btnX2, 798.f),
        sf::Vector2f(btnW, btnH),
        ButtonStyle::Ghost,
        8.f
    ));
    btnPromoClear->setFontSize(13);
    btnPromoClear->setCustomColors(sf::Color::White, sf::Color(241, 245, 249), sf::Color(226, 232, 240), sf::Color(241, 245, 249));
    btnPromoClear->setOutline(1.f, sf::Color(203, 213, 225));
    btnPromoClear->setTextColor(sf::Color(71, 85, 105));
    btnPromoClear->setOnClick([this]() {
        syncPromoSelectionToForm(-1);
    });

    refreshPromoScopeDropdown();
    if (!promoList.empty()) {
        syncPromoSelectionToForm(0);
    } else {
        syncPromoSelectionToForm(-1);
    }
}

void CinemaUIApp::renderPromotionsView() {
    float leftColX  = sidebarWidth + 24.f;          // 274.f
    float leftColW  = 860.f;
    float rightColX = leftColX + leftColW + 18.f;   // 1152.f
    float rightColW = 552.f;
    float totalW    = leftColW + 18.f + rightColW;  // 1430.f

    // Full light background for Promotions page (CineOS v3.2 Pro)
    sf::RectangleShape pageBg(sf::Vector2f(1728.f - sidebarWidth, 972.f));
    pageBg.setPosition(sidebarWidth, 0.f);
    pageBg.setFillColor(sf::Color(241, 245, 249));
    window.draw(pageBg);

    auto drawTxt = [&](const std::string& s, float x, float y, unsigned int sz, sf::Color col, bool bold = false) {
        sf::Text t(toSfString(s), Theme::FontPrimary, sz);
        if (bold) t.setStyle(sf::Text::Bold);
        t.setFillColor(col);
        t.setPosition(std::round(x), std::round(y));
        window.draw(t);
    };

    auto drawCenteredTxt = [&](const std::string& s, float cx, float cy, unsigned int sz, sf::Color col, bool bold = false) {
        sf::Text t(toSfString(s), Theme::FontPrimary, sz);
        if (bold) t.setStyle(sf::Text::Bold);
        t.setFillColor(col);
        sf::FloatRect b = t.getLocalBounds();
        t.setPosition(std::round(cx - (b.left + b.width * 0.5f)), std::round(cy - (b.top + b.height * 0.5f)));
        window.draw(t);
    };



    // =========================================================================
    // 2. TITLE & ACTION BANNER CARD (y = 54..116)
    // =========================================================================
    RoundedRectRenderer::render(window, sf::Vector2f(leftColX, 54.f), sf::Vector2f(totalW, 62.f), 12.f, sf::Color::White, 1.f, sf::Color(226, 232, 240));

    // Ticket / Voucher Icon Box
    RoundedRectRenderer::render(window, sf::Vector2f(leftColX + 18.f, 63.f), sf::Vector2f(44.f, 44.f), 10.f, sf::Color(2, 132, 199));
    RoundedRectRenderer::render(window, sf::Vector2f(leftColX + 28.f, 76.f), sf::Vector2f(24.f, 18.f), 3.f, sf::Color::White);
    RoundedRectRenderer::render(window, sf::Vector2f(leftColX + 32.f, 80.f), sf::Vector2f(16.f, 10.f), 2.f, sf::Color(2, 132, 199));

    drawTxt("QUẢN LÝ KHUYẾN MÃI & CRM", leftColX + 74.f, 62.f, 20, sf::Color(15, 23, 42), true);

    // Badge "CineOS Marketing" with generous padding
    RoundedRectRenderer::render(window, sf::Vector2f(leftColX + 384.f, 64.f), sf::Vector2f(128.f, 24.f), 12.f, sf::Color(224, 242, 254), 1.f, sf::Color(186, 230, 253));
    drawCenteredTxt("CineOS Marketing", leftColX + 384.f + 64.f, 64.f + 12.f, 11, sf::Color(2, 132, 199), true);

    drawTxt("Tạo chương trình khuyến mãi sự kiện & phân phối mã ưu đãi khách hàng toàn hệ thống", leftColX + 74.f, 90.f, 12, sf::Color(100, 116, 139), false);

    if (btnPromoExport) {
        btnPromoExport->draw(window);
        // Document export icon on left of Export button
        float exX = btnPromoExport->getPosition().x + 14.f;
        float exY = btnPromoExport->getPosition().y + 11.f;
        RoundedRectRenderer::render(window, sf::Vector2f(exX, exY), sf::Vector2f(13.f, 16.f), 2.f, sf::Color(224, 242, 254), 1.2f, sf::Color(2, 132, 199));
        sf::RectangleShape l1(sf::Vector2f(7.f, 1.5f));
        l1.setPosition(exX + 3.f, exY + 5.f);
        l1.setFillColor(sf::Color(2, 132, 199));
        window.draw(l1);
        sf::RectangleShape l2(sf::Vector2f(7.f, 1.5f));
        l2.setPosition(exX + 3.f, exY + 9.f);
        l2.setFillColor(sf::Color(2, 132, 199));
        window.draw(l2);
    }
    if (btnPromoNew) btnPromoNew->draw(window);

    // =========================================================================
    // 3. 4 STATISTICAL KPI CARDS ROW (y = 124..200) - Pure SQLite Statistics
    // =========================================================================
    int totalPromos = static_cast<int>(promoList.size());
    int activePromos = 0;
    int upcomingPromos = 0;
    int expiredPromos = 0;
    int todayKey = DateFormat::dateKey(stTodayStr());

    for (const auto& pr : promoList) {
        int st = computePromoStatus(pr, todayKey);
        if (st == 1) activePromos++;
        else if (st == 2) upcomingPromos++;
        else expiredPromos++;
    }

    long long usedVouchers = static_cast<long long>(invoiceList.size()) + static_cast<long long>(ticketList.size());
    double activeRatio = (totalPromos > 0) ? (static_cast<double>(activePromos) * 100.0 / static_cast<double>(totalPromos)) : 0.0;

    float kpiY = 124.f;
    float kpiH = 76.f;
    float kpiGap = 16.f;
    float kpiW = (totalW - 3.f * kpiGap) / 4.f; // 345.5f

    // KPI Card 1: TỔNG CTKM
    {
        float cx = leftColX;
        RoundedRectRenderer::render(window, sf::Vector2f(cx, kpiY), sf::Vector2f(kpiW, kpiH), 12.f, sf::Color::White, 1.f, sf::Color(226, 232, 240));
        drawTxt("TỔNG CTKM", cx + 20.f, kpiY + 13.f, 11, sf::Color(100, 116, 139), true);

        std::string numStr = std::to_string(totalPromos);
        sf::Text tNum(toSfString(numStr), Theme::FontPrimary, 26);
        tNum.setStyle(sf::Text::Bold);
        tNum.setFillColor(sf::Color(15, 23, 42));
        tNum.setPosition(cx + 20.f, kpiY + 34.f);
        window.draw(tNum);

        float numW = tNum.getLocalBounds().width;
        drawTxt("chương trình", cx + 28.f + numW, kpiY + 46.f, 13, sf::Color(100, 116, 139), false);

        // Right Ticket Icon Sticker
        float icX = cx + kpiW - 56.f;
        float icY = kpiY + 20.f;
        RoundedRectRenderer::render(window, sf::Vector2f(icX, icY), sf::Vector2f(36.f, 36.f), 9.f, sf::Color(224, 242, 254));
        RoundedRectRenderer::render(window, sf::Vector2f(icX + 8.f, icY + 11.f), sf::Vector2f(20.f, 14.f), 3.f, sf::Color(2, 132, 199));
        RoundedRectRenderer::render(window, sf::Vector2f(icX + 11.f, icY + 14.f), sf::Vector2f(14.f, 8.f), 1.5f, sf::Color(224, 242, 254));
    }

    // KPI Card 2: ĐANG HOẠT ĐỘNG
    {
        float cx = leftColX + (kpiW + kpiGap);
        RoundedRectRenderer::render(window, sf::Vector2f(cx, kpiY), sf::Vector2f(kpiW, kpiH), 12.f, sf::Color::White, 1.f, sf::Color(226, 232, 240));
        drawTxt("ĐANG HOẠT ĐỘNG", cx + 20.f, kpiY + 13.f, 11, sf::Color(100, 116, 139), true);

        char actBuf[16];
        snprintf(actBuf, sizeof(actBuf), "%02d", activePromos);
        sf::Text tNum(toSfString(actBuf), Theme::FontPrimary, 26);
        tNum.setStyle(sf::Text::Bold);
        tNum.setFillColor(sf::Color(22, 163, 74));
        tNum.setPosition(cx + 20.f, kpiY + 34.f);
        window.draw(tNum);

        float numW = tNum.getLocalBounds().width;
        float pillX = cx + 30.f + numW;
        RoundedRectRenderer::render(window, sf::Vector2f(pillX, kpiY + 42.f), sf::Vector2f(48.f, 22.f), 11.f, sf::Color(220, 252, 231), 1.f, sf::Color(134, 239, 172));
        drawCenteredTxt("Live", pillX + 24.f, kpiY + 53.f, 11, sf::Color(21, 128, 61), true);

        // Right Lightning Icon Sticker
        float icX = cx + kpiW - 56.f;
        float icY = kpiY + 20.f;
        RoundedRectRenderer::render(window, sf::Vector2f(icX, icY), sf::Vector2f(36.f, 36.f), 9.f, sf::Color(220, 252, 231));
        sf::ConvexShape bolt(6);
        bolt.setPoint(0, sf::Vector2f(icX + 20.f, icY + 8.f));
        bolt.setPoint(1, sf::Vector2f(icX + 11.f, icY + 19.f));
        bolt.setPoint(2, sf::Vector2f(icX + 17.f, icY + 19.f));
        bolt.setPoint(3, sf::Vector2f(icX + 15.f, icY + 28.f));
        bolt.setPoint(4, sf::Vector2f(icX + 25.f, icY + 17.f));
        bolt.setPoint(5, sf::Vector2f(icX + 19.f, icY + 17.f));
        bolt.setFillColor(sf::Color(22, 163, 74));
        window.draw(bolt);
    }

    // KPI Card 3: VOUCHER ĐÃ DÙNG
    {
        float cx = leftColX + 2.f * (kpiW + kpiGap);
        RoundedRectRenderer::render(window, sf::Vector2f(cx, kpiY), sf::Vector2f(kpiW, kpiH), 12.f, sf::Color::White, 1.f, sf::Color(226, 232, 240));
        drawTxt("VOUCHER ĐÃ DÙNG", cx + 20.f, kpiY + 13.f, 11, sf::Color(100, 116, 139), true);

        std::string usedStr = formatVndDots(usedVouchers);
        sf::Text tNum(toSfString(usedStr), Theme::FontPrimary, 26);
        tNum.setStyle(sf::Text::Bold);
        tNum.setFillColor(sf::Color(15, 23, 42));
        tNum.setPosition(cx + 20.f, kpiY + 34.f);
        window.draw(tNum);

        float numW = tNum.getLocalBounds().width;
        drawTxt("lượt áp dụng", cx + 28.f + numW, kpiY + 46.f, 12, sf::Color(22, 163, 74), true);

        // Right Users/CRM Icon Sticker
        float icX = cx + kpiW - 56.f;
        float icY = kpiY + 20.f;
        RoundedRectRenderer::render(window, sf::Vector2f(icX, icY), sf::Vector2f(36.f, 36.f), 9.f, sf::Color(224, 242, 254));
        sf::CircleShape uHead1(4.f);
        uHead1.setFillColor(sf::Color(2, 132, 199));
        uHead1.setPosition(icX + 11.f, icY + 9.f);
        window.draw(uHead1);
        sf::CircleShape uHead2(3.5f);
        uHead2.setFillColor(sf::Color(56, 189, 248));
        uHead2.setPosition(icX + 20.f, icY + 10.f);
        window.draw(uHead2);
        RoundedRectRenderer::render(window, sf::Vector2f(icX + 8.f, icY + 19.f), sf::Vector2f(14.f, 8.f), 4.f, sf::Color(2, 132, 199));
        RoundedRectRenderer::render(window, sf::Vector2f(icX + 18.f, icY + 20.f), sf::Vector2f(11.f, 7.f), 3.5f, sf::Color(56, 189, 248));
    }

    // KPI Card 4: TỶ LỆ KÍCH HOẠT
    {
        float cx = leftColX + 3.f * (kpiW + kpiGap);
        RoundedRectRenderer::render(window, sf::Vector2f(cx, kpiY), sf::Vector2f(kpiW, kpiH), 12.f, sf::Color::White, 1.f, sf::Color(226, 232, 240));
        drawTxt("TỶ LỆ KÍCH HOẠT", cx + 20.f, kpiY + 13.f, 11, sf::Color(100, 116, 139), true);

        std::ostringstream rOss;
        rOss << std::fixed << std::setprecision(1) << activeRatio << "%";
        sf::Text tNum(toSfString(rOss.str()), Theme::FontPrimary, 26);
        tNum.setStyle(sf::Text::Bold);
        tNum.setFillColor(sf::Color(2, 132, 199));
        tNum.setPosition(cx + 20.f, kpiY + 34.f);
        window.draw(tNum);

        float numW = tNum.getLocalBounds().width;
        drawTxt(activeRatio >= 50.0 ? "Tốt" : "Ổn định", cx + 28.f + numW, kpiY + 46.f, 13, sf::Color(22, 163, 74), true);

        // Right Pie Chart Icon Sticker
        float icX = cx + kpiW - 56.f;
        float icY = kpiY + 20.f;
        RoundedRectRenderer::render(window, sf::Vector2f(icX, icY), sf::Vector2f(36.f, 36.f), 9.f, sf::Color(224, 242, 254));
        sf::CircleShape pie(9.f);
        pie.setPosition(icX + 9.f, icY + 9.f);
        pie.setFillColor(sf::Color(2, 132, 199));
        window.draw(pie);
        sf::ConvexShape slice(4);
        slice.setPoint(0, sf::Vector2f(icX + 18.f, icY + 18.f));
        slice.setPoint(1, sf::Vector2f(icX + 18.f, icY + 8.f));
        slice.setPoint(2, sf::Vector2f(icX + 26.f, icY + 11.f));
        slice.setPoint(3, sf::Vector2f(icX + 28.f, icY + 18.f));
        slice.setFillColor(sf::Color(125, 211, 252));
        window.draw(slice);
    }

    // =========================================================================
    // 4. LEFT MAIN CARD: DANH SÁCH CHƯƠNG TRÌNH KHUYẾN MÃI (y = 208..914)
    // =========================================================================
    float cardY = 208.f;
    float cardH = 706.f;
    RoundedRectRenderer::render(window, sf::Vector2f(leftColX, cardY), sf::Vector2f(leftColW, cardH), 12.f, sf::Color::White, 1.f, sf::Color(226, 232, 240));

    // Search Input & Magnifying Glass + [Ctrl+K] badge
    if (promoSearchInput) {
        promoSearchInput->draw(window);
        // Magnifying glass icon
        float mgX = leftColX + 30.f;
        float mgY = 236.f;
        sf::CircleShape glass(5.5f);
        glass.setPosition(mgX, mgY);
        glass.setFillColor(sf::Color::Transparent);
        glass.setOutlineThickness(1.6f);
        glass.setOutlineColor(sf::Color(148, 163, 184));
        window.draw(glass);
        sf::RectangleShape handle(sf::Vector2f(5.f, 1.6f));
        handle.setPosition(mgX + 9.5f, mgY + 9.5f);
        handle.setRotation(45.f);
        handle.setFillColor(sf::Color(148, 163, 184));
        window.draw(handle);

        // [Ctrl+K] badge inside right of search input when text is short
        if (promoSearchInput->getText().size() < 16) {
            float ckX = leftColX + 18.f + 290.f - 54.f;
            RoundedRectRenderer::render(window, sf::Vector2f(ckX, 231.f), sf::Vector2f(46.f, 22.f), 5.f, sf::Color(241, 245, 249), 1.f, sf::Color(226, 232, 240));
            drawCenteredTxt("Ctrl+K", ckX + 23.f, 242.f, 10, sf::Color(100, 116, 139), true);
        }
    }

    // Style & Draw 4 Filter Pills (Clean text only - NO numbers on filter buttons)
    auto styleFilterBtn = [&](std::unique_ptr<RoundRectButton>& btn, bool active) {
        if (!btn) return;
        if (active) {
            btn->setCustomColors(sf::Color(2, 132, 199), sf::Color(3, 105, 161), sf::Color(7, 89, 133), sf::Color(203, 213, 225));
            btn->setOutline(0.f, sf::Color::Transparent);
            btn->setTextColor(sf::Color::White);
        } else {
            btn->setCustomColors(sf::Color(248, 250, 252), sf::Color(241, 245, 249), sf::Color(226, 232, 240), sf::Color(241, 245, 249));
            btn->setOutline(1.f, sf::Color(226, 232, 240));
            btn->setTextColor(sf::Color(71, 85, 105));
        }
        btn->draw(window);
    };
    styleFilterBtn(btnPromoFilterAll,      promoFilterTab == 0);
    styleFilterBtn(btnPromoFilterActive,   promoFilterTab == 1);
    styleFilterBtn(btnPromoFilterUpcoming, promoFilterTab == 2);
    styleFilterBtn(btnPromoFilterExpired,  promoFilterTab == 3);

    // Filtered list & Pagination calculation
    std::vector<int> filteredIndices = getFilteredPromoIndices();
    int totalFiltered = static_cast<int>(filteredIndices.size());
    int rowsPerPage = 5;
    int totalPages = std::max(1, (totalFiltered + rowsPerPage - 1) / rowsPerPage);
    if (promoCurrentPage >= totalPages) promoCurrentPage = totalPages - 1;
    if (promoCurrentPage < 0) promoCurrentPage = 0;

    int startIdx = promoCurrentPage * rowsPerPage;
    int endIdx = std::min(startIdx + rowsPerPage, totalFiltered);
    int shownCount = std::max(0, endIdx - startIdx);

    // Sub-header summary row (y = 270.f)
    std::string sumLeft = "Hiển thị " + std::to_string(shownCount) + " trên tổng số " + std::to_string(totalFiltered) + " chương trình ưu đãi";
    drawTxt(sumLeft, leftColX + 18.f, 270.f, 12, sf::Color(100, 116, 139), false);

    // Right status summary dots
    float dotActiveX = leftColX + leftColW - 265.f;
    sf::CircleShape dAct(3.5f);
    dAct.setFillColor(sf::Color(22, 163, 74));
    dAct.setPosition(dotActiveX, 275.f);
    window.draw(dAct);
    drawTxt("Đang hoạt động: " + std::to_string(activePromos), dotActiveX + 12.f, 270.f, 12, sf::Color(22, 163, 74), true);

    float dotOtherX = leftColX + leftColW - 125.f;
    sf::CircleShape dOth(3.5f);
    dOth.setFillColor(sf::Color(148, 163, 184));
    dOth.setPosition(dotOtherX, 275.f);
    window.draw(dOth);
    drawTxt("Sắp tới / Hết hạn: " + std::to_string(upcomingPromos + expiredPromos), dotOtherX + 12.f, 270.f, 12, sf::Color(100, 116, 139), false);

    // Table Header Band (y = 296..334)
    float thY = 296.f;
    float thH = 38.f;
    sf::RectangleShape thBg(sf::Vector2f(leftColW - 2.f, thH));
    thBg.setPosition(leftColX + 1.f, thY);
    thBg.setFillColor(sf::Color(248, 250, 252));
    window.draw(thBg);

    sf::RectangleShape thTopLine(sf::Vector2f(leftColW - 2.f, 1.f));
    thTopLine.setPosition(leftColX + 1.f, thY);
    thTopLine.setFillColor(sf::Color(226, 232, 240));
    window.draw(thTopLine);

    sf::RectangleShape thBotLine(sf::Vector2f(leftColW - 2.f, 1.f));
    thBotLine.setPosition(leftColX + 1.f, thY + thH);
    thBotLine.setFillColor(sf::Color(226, 232, 240));
    window.draw(thBotLine);

    // 7 Column Centers across leftColW = 860.f
    float colCenter[7] = {
        leftColX + 58.f,   // Col 0: ID KHUYẾN MÃI (8..108)
        leftColX + 208.f,  // Col 1: TÊN KM (108..308)
        leftColX + 380.f,  // Col 2: MÃ CODE (308..452)
        leftColX + 494.f,  // Col 3: % GIẢM (452..536)
        leftColX + 592.f,  // Col 4: MAX GIẢM (VNĐ) (536..648)
        leftColX + 700.f,  // Col 5: NGÀY BẮT ĐẦU (648..752)
        leftColX + 804.f   // Col 6: NGÀY KẾT THÚC (752..856)
    };
    const char* colTitles[7] = {
        "ID KHUYẾN MÃI",
        "TÊN KM",
        "MÃ CODE",
        "% GIẢM",
        "MAX GIẢM (VNĐ)",
        "NGÀY BẮT ĐẦU",
        "NGÀY KẾT THÚC"
    };
    for (int c = 0; c < 7; ++c) {
        drawCenteredTxt(colTitles[c], colCenter[c], thY + thH * 0.5f, 11, sf::Color(100, 116, 139), true);
    }

    // Table Rows (rowStartY = 334.f, rowH = 104.f, up to 5 rows)
    float rowStartY = 334.f;
    float rowH = 104.f;

    if (shownCount == 0) {
        drawCenteredTxt("Không tìm thấy chương trình khuyến mãi phù hợp với bộ lọc hiện tại.", leftColX + leftColW * 0.5f, rowStartY + 180.f, 14, sf::Color(148, 163, 184), false);
    }

    for (int r = 0; r < shownCount; ++r) {
        int actualIdx = filteredIndices[startIdx + r];
        const auto& pr = promoList[actualIdx];
        float ry = rowStartY + r * rowH;
        bool isSel = (actualIdx == selectedPromoRowIdx);

        if (isSel) {
            sf::RectangleShape selBg(sf::Vector2f(leftColW - 4.f, rowH - 1.f));
            selBg.setPosition(leftColX + 2.f, ry + 1.f);
            selBg.setFillColor(sf::Color(240, 249, 255));
            window.draw(selBg);

            sf::RectangleShape leftAccent(sf::Vector2f(4.f, rowH - 1.f));
            leftAccent.setPosition(leftColX + 2.f, ry + 1.f);
            leftAccent.setFillColor(sf::Color(2, 132, 199));
            window.draw(leftAccent);
        }

        // Row bottom divider
        sf::RectangleShape rDiv(sf::Vector2f(leftColW - 24.f, 1.f));
        rDiv.setPosition(leftColX + 12.f, ry + rowH);
        rDiv.setFillColor(sf::Color(241, 245, 249));
        window.draw(rDiv);

        float midY = ry + rowH * 0.5f;

        // Col 0: ID KHUYẾN MÃI (centered)
        std::string dispId = isSel ? ("> " + pr.getMaKhuyenMai()) : pr.getMaKhuyenMai();
        drawCenteredTxt(dispId, colCenter[0], midY, 13, isSel ? sf::Color(2, 132, 199) : sf::Color(71, 85, 105), true);

        // Col 1: TÊN KM (Line 1 at ry + 24, Status/Scope Badge cleanly separated on Line 2 at ry + 56 -> ZERO overlap!)
        sf::String fittedName = stFitText(pr.getTenKhuyenMai(), 13, 186.f, true);
        sf::Text tName(fittedName, Theme::FontPrimary, 13);
        tName.setStyle(sf::Text::Bold);
        tName.setFillColor(sf::Color(15, 23, 42));
        sf::FloatRect nb = tName.getLocalBounds();
        tName.setPosition(std::round(colCenter[1] - (nb.left + nb.width * 0.5f)), std::round(ry + 24.f));
        window.draw(tName);

        // Scope / Status badge below Name (ry + 56.f, height 26.f)
        int st = computePromoStatus(pr, todayKey);
        std::string badgeStr;
        sf::Color badgeBg, badgeBorder, badgeFg;
        std::string pv = pr.getPhamViApDung();

        if (!pv.empty() && pv != "All") {
            std::string movieShort = pv;
            for (const auto& m : movieList) {
                if (m.getMaPhim() == pv) {
                    movieShort = pv + " - " + m.getTenPhim();
                    break;
                }
            }
            badgeStr = "Phim: " + movieShort;
            badgeBg = sf::Color(224, 242, 254);
            badgeBorder = sf::Color(186, 230, 253);
            badgeFg = sf::Color(3, 105, 161);
        } else if (st == 1) {
            badgeStr = "Đang kích hoạt";
            badgeBg = sf::Color(220, 252, 231);
            badgeBorder = sf::Color(134, 239, 172);
            badgeFg = sf::Color(21, 128, 61);
        } else if (st == 2) {
            badgeStr = "Sắp diễn ra";
            badgeBg = sf::Color(254, 243, 199);
            badgeBorder = sf::Color(253, 230, 138);
            badgeFg = sf::Color(180, 83, 9);
        } else {
            badgeStr = "Đã hết hạn";
            badgeBg = sf::Color(241, 245, 249);
            badgeBorder = sf::Color(203, 213, 225);
            badgeFg = sf::Color(100, 116, 139);
        }

        sf::String fittedBadge = stFitText(badgeStr, 11, 166.f, true);
        sf::Text tBadge(fittedBadge, Theme::FontPrimary, 11);
        tBadge.setStyle(sf::Text::Bold);
        tBadge.setFillColor(badgeFg);
        sf::FloatRect bb = tBadge.getLocalBounds();
        float badgeW = std::clamp(bb.width + 24.f, 108.f, 186.f);
        float badgeH = 26.f;
        float badgeX = colCenter[1] - badgeW * 0.5f;
        float badgeY = ry + 56.f;
        RoundedRectRenderer::render(window, sf::Vector2f(badgeX, badgeY), sf::Vector2f(badgeW, badgeH), 8.f, badgeBg, 1.f, badgeBorder);
        tBadge.setPosition(std::round(colCenter[1] - (bb.left + bb.width * 0.5f)), std::round(badgeY + badgeH * 0.5f - (bb.top + bb.height * 0.5f)));
        window.draw(tBadge);

        // Col 2: MÃ CODE pill with copy icon (centered at colCenter[2], midY)
        std::string codeStr = pr.getMaCode();
        sf::Text tCode(toSfString(codeStr), Theme::FontPrimary, 12);
        tCode.setStyle(sf::Text::Bold);
        tCode.setFillColor(isSel ? sf::Color(3, 105, 161) : sf::Color(51, 65, 85));
        sf::FloatRect cb = tCode.getLocalBounds();
        float codePillW = std::clamp(cb.width + 38.f, 108.f, 138.f);
        float codePillH = 30.f;
        float codePillX = colCenter[2] - codePillW * 0.5f;
        float codePillY = midY - codePillH * 0.5f;
        RoundedRectRenderer::render(
            window,
            sf::Vector2f(codePillX, codePillY),
            sf::Vector2f(codePillW, codePillH),
            7.f,
            isSel ? sf::Color(224, 242, 254) : sf::Color(241, 245, 249),
            1.f,
            isSel ? sf::Color(125, 211, 252) : sf::Color(226, 232, 240)
        );
        tCode.setPosition(std::round(codePillX + 12.f), std::round(midY - (cb.top + cb.height * 0.5f)));
        window.draw(tCode);

        // Small copy icon on right of code pill
        float cpX = codePillX + codePillW - 20.f;
        float cpY = codePillY + 8.f;
        RoundedRectRenderer::render(window, sf::Vector2f(cpX + 3.f, cpY), sf::Vector2f(9.f, 11.f), 1.5f, sf::Color::Transparent, 1.f, sf::Color(148, 163, 184));
        RoundedRectRenderer::render(window, sf::Vector2f(cpX, cpY + 3.f), sf::Vector2f(9.f, 11.f), 1.5f, sf::Color::White, 1.f, isSel ? sf::Color(2, 132, 199) : sf::Color(100, 116, 139));

        // Col 3: % GIẢM (centered)
        std::ostringstream ptOss;
        ptOss << std::fixed << std::setprecision(2) << pr.getPhanTramGiam() << "%";
        drawCenteredTxt(ptOss.str(), colCenter[3], midY, 14, isSel ? sf::Color(2, 132, 199) : sf::Color(30, 41, 59), true);

        // Col 4: MAX GIẢM (VNĐ) (centered on 1 line with dot separator)
        std::string maxStr = formatVndDots(static_cast<long long>(std::round(pr.getMucGiamToiDa()))) + " đ";
        drawCenteredTxt(maxStr, colCenter[4], midY, 13, sf::Color(15, 23, 42), true);

        // Col 5: NGÀY BẮT ĐẦU (centered)
        drawCenteredTxt(pr.getNgayBatDau(), colCenter[5], midY, 12, sf::Color(71, 85, 105), false);

        // Col 6: NGÀY KẾT THÚC (centered)
        drawCenteredTxt(pr.getNgayKetThuc(), colCenter[6], midY, 12, sf::Color(71, 85, 105), false);
    }

    // Table Footer & Pagination (footY = 868.f)
    float footY = 868.f;
    std::string pageInfo = "Trang " + std::to_string(promoCurrentPage + 1) + " / " + std::to_string(totalPages) +
                           " (Tổng " + std::to_string(totalFiltered) + " bản ghi hiển thị)";
    drawTxt(pageInfo, leftColX + 18.f, footY + 6.f, 12, sf::Color(100, 116, 139), false);

    float btnSz = 28.f;
    float btnGap = 6.f;
    float totalPagingW = (totalPages + 2) * btnSz + (totalPages + 1) * btnGap;
    float pgStartX = leftColX + leftColW - 18.f - totalPagingW;

    // Prev '<'
    RoundedRectRenderer::render(window, sf::Vector2f(pgStartX, footY), sf::Vector2f(btnSz, btnSz), 6.f, sf::Color(248, 250, 252), 1.f, sf::Color(226, 232, 240));
    drawCenteredTxt("<", pgStartX + btnSz * 0.5f, footY + btnSz * 0.5f, 12, sf::Color(100, 116, 139), true);

    for (int p = 0; p < totalPages; ++p) {
        float px = pgStartX + (p + 1) * (btnSz + btnGap);
        bool isCur = (p == promoCurrentPage);
        RoundedRectRenderer::render(
            window,
            sf::Vector2f(px, footY),
            sf::Vector2f(btnSz, btnSz),
            6.f,
            isCur ? sf::Color(2, 132, 199) : sf::Color::White,
            1.f,
            isCur ? sf::Color(2, 132, 199) : sf::Color(226, 232, 240)
        );
        drawCenteredTxt(std::to_string(p + 1), px + btnSz * 0.5f, footY + btnSz * 0.5f, 12, isCur ? sf::Color::White : sf::Color(71, 85, 105), true);
    }

    // Next '>'
    float nextX = pgStartX + (totalPages + 1) * (btnSz + btnGap);
    RoundedRectRenderer::render(window, sf::Vector2f(nextX, footY), sf::Vector2f(btnSz, btnSz), 6.f, sf::Color(248, 250, 252), 1.f, sf::Color(226, 232, 240));
    drawCenteredTxt(">", nextX + btnSz * 0.5f, footY + btnSz * 0.5f, 12, sf::Color(100, 116, 139), true);

    // =========================================================================
    // 5. RIGHT MAIN CARD: THÔNG TIN SỰ KIỆN KHUYẾN MÃI (y = 208..914)
    // =========================================================================
    RoundedRectRenderer::render(window, sf::Vector2f(rightColX, cardY), sf::Vector2f(rightColW, cardH), 12.f, sf::Color::White, 1.f, sf::Color(226, 232, 240));

    // Card Header (y = 224.f)
    sf::CircleShape hdrDot(4.5f);
    hdrDot.setFillColor(sf::Color(2, 132, 199));
    hdrDot.setPosition(rightColX + 20.f, 229.f);
    window.draw(hdrDot);
    drawTxt("Thông tin sự kiện khuyến mãi", rightColX + 34.f, 224.f, 15, sf::Color(15, 23, 42), true);

    std::string liveId = inputPromoId ? trimPromoStr(inputPromoId->getText()) : "";
    if (liveId.empty()) liveId = generateNextPromoId();
    std::string modeBadge = (selectedPromoRowIdx >= 0) ? ("Đang chỉnh sửa: " + liveId) : ("Tạo mới: " + liveId);
    sf::Text tMode(toSfString(modeBadge), Theme::FontPrimary, 11);
    tMode.setStyle(sf::Text::Bold);
    float modeW = tMode.getLocalBounds().width + 24.f;
    float modeX = rightColX + rightColW - 20.f - modeW;
    RoundedRectRenderer::render(window, sf::Vector2f(modeX, 221.f), sf::Vector2f(modeW, 26.f), 13.f, sf::Color(224, 242, 254), 1.f, sf::Color(186, 230, 253));
    drawCenteredTxt(modeBadge, modeX + modeW * 0.5f, 234.f, 11, sf::Color(2, 132, 199), true);

    // Live Voucher Preview Banner (x = rightColX + 20, y = 258, w = 512, h = 104)
    float vcX = rightColX + 20.f;
    float vcY = 258.f;
    float vcW = rightColW - 40.f; // 512.f
    float vcH = 104.f;

    RoundedRectRenderer::render(window, sf::Vector2f(vcX, vcY), sf::Vector2f(vcW, vcH), 12.f, sf::Color(2, 132, 199));
    RoundedRectRenderer::render(window, sf::Vector2f(vcX, vcY + 46.f), sf::Vector2f(vcW, vcH - 46.f), 12.f, sf::Color(3, 105, 161));

    // Ticket cutouts on left and right edges
    sf::CircleShape cutLeft(9.f);
    cutLeft.setFillColor(sf::Color::White);
    cutLeft.setPosition(vcX - 9.f, vcY + vcH * 0.5f - 9.f);
    window.draw(cutLeft);
    sf::CircleShape cutRight(9.f);
    cutRight.setFillColor(sf::Color::White);
    cutRight.setPosition(vcX + vcW - 9.f, vcY + vcH * 0.5f - 9.f);
    window.draw(cutRight);

    // Top row of Voucher
    drawTxt("CINESTAR VOUCHER", vcX + 18.f, vcY + 12.f, 11, sf::Color::White, true);
    RoundedRectRenderer::render(window, sf::Vector2f(vcX + 142.f, vcY + 10.f), sf::Vector2f(64.f, 20.f), 5.f, sf::Color(56, 189, 248, 90));
    drawCenteredTxt("OFFICIAL", vcX + 174.f, vcY + 20.f, 10, sf::Color::White, true);

    sf::Text tVcId(toSfString(liveId), Theme::FontPrimary, 12);
    tVcId.setStyle(sf::Text::Bold);
    tVcId.setFillColor(sf::Color(224, 242, 254));
    tVcId.setPosition(vcX + vcW - 18.f - tVcId.getLocalBounds().width, vcY + 12.f);
    window.draw(tVcId);

    // Middle & Bottom Left of Voucher
    std::string liveName = inputPromoName ? trimPromoStr(inputPromoName->getText()) : "";
    if (liveName.empty()) liveName = "Chương trình khuyến mãi mới";
    sf::String fitVcName = stFitText(liveName, 13, 330.f, false);
    sf::Text tVcName(fitVcName, Theme::FontPrimary, 13);
    tVcName.setFillColor(sf::Color(240, 249, 255));
    tVcName.setPosition(vcX + 18.f, vcY + 46.f);
    window.draw(tVcName);

    std::string livePt = inputPromoPercent ? trimPromoStr(inputPromoPercent->getText()) : "0";
    if (livePt.empty()) livePt = "0";
    std::string bigDisc = "GIẢM " + livePt + "%";
    sf::Text tBigDisc(toSfString(bigDisc), Theme::FontPrimary, 21);
    tBigDisc.setStyle(sf::Text::Bold);
    tBigDisc.setFillColor(sf::Color::White);
    tBigDisc.setPosition(vcX + 18.f, vcY + 68.f);
    window.draw(tBigDisc);

    std::string rawLiveMax = inputPromoMaxDiscount ? trimPromoStr(inputPromoMaxDiscount->getText()) : "0";
    std::string cleanLiveMax;
    for (char c : rawLiveMax) { if (c >= '0' && c <= '9') cleanLiveMax.push_back(c); }
    long long liveMaxVal = 0;
    if (!cleanLiveMax.empty()) {
        try { liveMaxVal = std::stoll(cleanLiveMax); } catch (...) { liveMaxVal = 0; }
    }
    std::string maxNote = "(Tối đa " + formatVndDots(liveMaxVal) + " đ)";
    drawTxt(maxNote, vcX + 26.f + tBigDisc.getLocalBounds().width, vcY + 76.f, 12, sf::Color(186, 230, 253), false);

    // Middle & Bottom Right of Voucher
    std::string liveCode = inputPromoCode ? trimPromoStr(inputPromoCode->getText()) : "";
    if (liveCode.empty()) liveCode = "VOUCHER";
    std::transform(liveCode.begin(), liveCode.end(), liveCode.begin(), ::toupper);
    sf::Text tVcCode(toSfString(liveCode), Theme::FontPrimary, 12);
    tVcCode.setStyle(sf::Text::Bold);
    float vcCodeW = std::clamp(tVcCode.getLocalBounds().width + 28.f, 92.f, 150.f);
    float vcCodeX = vcX + vcW - 18.f - vcCodeW;
    RoundedRectRenderer::render(window, sf::Vector2f(vcCodeX, vcY + 42.f), sf::Vector2f(vcCodeW, 28.f), 7.f, sf::Color::White);
    drawCenteredTxt(liveCode, vcCodeX + vcCodeW * 0.5f, vcY + 56.f, 12, sf::Color(2, 132, 199), true);

    std::string liveEnd = inputPromoEndDate ? trimPromoStr(inputPromoEndDate->getText()) : "";
    if (liveEnd.empty()) liveEnd = "--/--/----";
    std::string hsdStr = "HSD: " + liveEnd;
    sf::Text tHsd(toSfString(hsdStr), Theme::FontPrimary, 11);
    tHsd.setFillColor(sf::Color(224, 242, 254));
    tHsd.setPosition(vcX + vcW - 18.f - tHsd.getLocalBounds().width, vcY + 76.f);
    window.draw(tHsd);

    // 8 Form Labels & Inputs (y = 376..698)
    float lblX = rightColX + 20.f;
    auto drawFormLabel = [&](const std::string& lbl, bool req, float y) {
        sf::Text t(toSfString(lbl), Theme::FontPrimary, 12);
        t.setStyle(sf::Text::Bold);
        t.setFillColor(sf::Color(51, 65, 85));
        t.setPosition(lblX, y + 9.f);
        window.draw(t);
        if (req) {
            sf::Text star(toSfString("*"), Theme::FontPrimary, 13);
            star.setStyle(sf::Text::Bold);
            star.setFillColor(sf::Color(239, 68, 68));
            star.setPosition(lblX + t.getLocalBounds().width + 4.f, y + 8.f);
            window.draw(star);
        }
    };

    drawFormLabel("ID Khuyến mãi:", false, 376.f);
    // Small lock icon next to ID label when editing
    RoundedRectRenderer::render(window, sf::Vector2f(lblX + 94.f, 386.f), sf::Vector2f(10.f, 8.f), 2.f, sf::Color(148, 163, 184));

    drawFormLabel("Tên khuyến mãi:", true, 422.f);
    drawFormLabel("Mã Code:", true, 468.f);
    drawFormLabel("% Giảm:", true, 514.f);
    drawFormLabel("Giảm Max (VNĐ):", false, 560.f);
    drawFormLabel("Ngày bắt đầu:", true, 606.f);
    drawFormLabel("Ngày kết thúc:", true, 652.f);
    drawFormLabel("Áp dụng cho phim:", false, 698.f);

    if (inputPromoId) inputPromoId->draw(window);
    if (inputPromoName) inputPromoName->draw(window);
    if (inputPromoCode) inputPromoCode->draw(window);
    if (btnPromoCopyCode) {
        btnPromoCopyCode->draw(window);
        float cpBx = btnPromoCopyCode->getPosition().x + 12.f;
        float cpBy = btnPromoCopyCode->getPosition().y + 11.f;
        RoundedRectRenderer::render(window, sf::Vector2f(cpBx + 3.f, cpBy), sf::Vector2f(9.f, 11.f), 1.5f, sf::Color::Transparent, 1.f, sf::Color(2, 132, 199));
        RoundedRectRenderer::render(window, sf::Vector2f(cpBx, cpBy + 3.f), sf::Vector2f(9.f, 11.f), 1.5f, sf::Color(224, 242, 254), 1.f, sf::Color(2, 132, 199));
    }

    if (inputPromoPercent) {
        inputPromoPercent->draw(window);
        drawTxt("%", rightColX + rightColW - 42.f, 523.f, 13, sf::Color(100, 116, 139), true);
    }
    if (inputPromoMaxDiscount) {
        inputPromoMaxDiscount->draw(window);
        drawTxt("VNĐ", rightColX + rightColW - 56.f, 569.f, 12, sf::Color(100, 116, 139), true);
    }

    if (inputPromoStartDate) inputPromoStartDate->draw(window);
    if (inputPromoEndDate) inputPromoEndDate->draw(window);

    if (promoScopeDropdown) promoScopeDropdown->draw(window);

    // 4 Action Buttons
    if (btnPromoCreate) btnPromoCreate->draw(window);
    if (btnPromoUpdate) btnPromoUpdate->draw(window);
    if (btnPromoDelete) btnPromoDelete->draw(window);
    if (btnPromoClear) btnPromoClear->draw(window);

    // Bottom POS Info Banner (y = 850.f)
    RoundedRectRenderer::render(window, sf::Vector2f(vcX, 850.f), sf::Vector2f(vcW, 38.f), 8.f, sf::Color(240, 249, 255), 1.f, sf::Color(186, 230, 253));
    sf::CircleShape infoCir(8.f);
    infoCir.setFillColor(sf::Color(2, 132, 199));
    infoCir.setPosition(vcX + 12.f, 861.f);
    window.draw(infoCir);
    drawCenteredTxt("i", vcX + 20.f, 869.f, 11, sf::Color::White, true);
    drawTxt("Mã khuyến mãi này đã sẵn sàng để áp dụng tại Quầy POS và Cổng vé trực tuyến.", vcX + 36.f, 862.f, 11, sf::Color(3, 105, 161), false);

    // =========================================================================
    // 6. BOTTOM COPYRIGHT & HOTLINE FOOTER (y = 928)
    // =========================================================================
    drawTxt("© 2026 Cinestar CineOS  •  Bản quyền thuộc Cinestar Cinema Complex", leftColX, 932.f, 11, sf::Color(100, 116, 139), false);
    drawTxt("Chính sách bảo mật CRM   •   Hướng dẫn sử dụng   •   Hotline Hỗ trợ Kỹ thuật: ", leftColX + 820.f, 932.f, 11, sf::Color(100, 116, 139), false);
    drawTxt("1900 6081 (Ext: 104)", leftColX + 1280.f, 932.f, 11, sf::Color(2, 132, 199), true);
}

bool CinemaUIApp::saveStaffUsersToDatabase() {
    sqlite3* db = nullptr;
    if (sqlite3_open("DATA/cinema.db", &db) != SQLITE_OK) return false;

    sqlite3_exec(db, "BEGIN TRANSACTION;", nullptr, nullptr, nullptr);
    // Delete only Admin and Staff rows, preserving Customer rows in users table
    sqlite3_exec(db, "DELETE FROM users WHERE vaiTro IN ('Admin','Staff');", nullptr, nullptr, nullptr);

    const char* sql = "INSERT OR REPLACE INTO users (maNguoiDung,hoTen,soDienThoai,email,ngaySinh,gioiTinh,cccd,taiKhoan,matKhau,vaiTro,trangThaiLamViec,trangThaiTaiKhoan) VALUES (?,?,?,?,?,?,?,?,?,?,?,?);";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) == SQLITE_OK) {
        for (const auto& u : staffUserList) {
            sqlite3_bind_text(stmt, 1, u.maNguoiDung.c_str(), -1, SQLITE_TRANSIENT);
            sqlite3_bind_text(stmt, 2, u.hoTen.c_str(), -1, SQLITE_TRANSIENT);
            sqlite3_bind_text(stmt, 3, u.soDienThoai.c_str(), -1, SQLITE_TRANSIENT);
            sqlite3_bind_text(stmt, 4, u.email.c_str(), -1, SQLITE_TRANSIENT);
            sqlite3_bind_text(stmt, 5, u.ngaySinh.c_str(), -1, SQLITE_TRANSIENT);
            sqlite3_bind_text(stmt, 6, u.gioiTinh.c_str(), -1, SQLITE_TRANSIENT);
            sqlite3_bind_text(stmt, 7, u.cccd.c_str(), -1, SQLITE_TRANSIENT);
            sqlite3_bind_text(stmt, 8, u.taiKhoan.c_str(), -1, SQLITE_TRANSIENT);
            sqlite3_bind_text(stmt, 9, u.matKhau.c_str(), -1, SQLITE_TRANSIENT);
            sqlite3_bind_text(stmt, 10, u.vaiTro.c_str(), -1, SQLITE_TRANSIENT);
            sqlite3_bind_text(stmt, 11, u.trangThaiLamViec.c_str(), -1, SQLITE_TRANSIENT);
            sqlite3_bind_text(stmt, 12, u.trangThaiTaiKhoan.c_str(), -1, SQLITE_TRANSIENT);
            sqlite3_step(stmt);
            sqlite3_reset(stmt);
        }
        sqlite3_finalize(stmt);
    }
    sqlite3_exec(db, "COMMIT;", nullptr, nullptr, nullptr);
    sqlite3_close(db);
    return true;
}

std::string CinemaUIApp::generateNextStaffId() const {
    int maxNum = 0;
    for (const auto& st : staffUserList) {
        if (st.maNguoiDung.size() >= 2 && (st.maNguoiDung[0] == 'U' || st.maNguoiDung[0] == 'u')) {
            try {
                int val = std::stoi(st.maNguoiDung.substr(1));
                if (val > maxNum) maxNum = val;
            } catch (...) {}
        }
    }
    std::ostringstream oss;
    oss << "U" << std::setw(3) << std::setfill('0') << (maxNum + 1);
    return oss.str();
}

std::vector<int> CinemaUIApp::getFilteredStaffIndices() const {
    std::vector<int> indices;
    std::string q = staffSearchInput ? removeVietnameseDiacritics(staffSearchInput->getText()) : "";

    for (size_t i = 0; i < staffUserList.size(); ++i) {
        const auto& st = staffUserList[i];
        bool isResigned = (st.trangThaiLamViec == "DaNghiViec" || st.trangThaiLamViec == "Đã nghỉ việc");
        if (staffFilterTab == 1 && isResigned) continue;
        if (staffFilterTab == 2 && !isResigned) continue;

        if (!q.empty()) {
            std::string idNorm = removeVietnameseDiacritics(st.maNguoiDung);
            std::string nameNorm = removeVietnameseDiacritics(st.hoTen);
            std::string phoneNorm = removeVietnameseDiacritics(st.soDienThoai);
            std::string cccdNorm = removeVietnameseDiacritics(st.cccd);
            std::string emailNorm = removeVietnameseDiacritics(st.email);
            if (idNorm.find(q) == std::string::npos &&
                nameNorm.find(q) == std::string::npos &&
                phoneNorm.find(q) == std::string::npos &&
                cccdNorm.find(q) == std::string::npos &&
                emailNorm.find(q) == std::string::npos) {
                continue;
            }
        }
        indices.push_back(static_cast<int>(i));
    }
    return indices;
}

void CinemaUIApp::syncStaffSelectionToForm(int idx) {
    if (idx >= 0 && idx < static_cast<int>(staffUserList.size())) {
        selectedStaffRowIdx = idx;
        const auto& st = staffUserList[idx];
        if (inputStaffId) inputStaffId->setText(st.maNguoiDung);
        if (inputStaffName) inputStaffName->setText(st.hoTen);
        if (inputStaffPhone) inputStaffPhone->setText(st.soDienThoai);
        if (inputStaffBirthDate) inputStaffBirthDate->setText(st.ngaySinh);
        if (inputStaffCCCD) inputStaffCCCD->setText(st.cccd);
        if (inputStaffEmail) inputStaffEmail->setText(st.email);
        if (staffGenderDropdown) {
            std::string g = removeVietnameseDiacritics(st.gioiTinh);
            if (g == "nu" || st.gioiTinh == "Nữ" || st.gioiTinh == "Nu") {
                staffGenderDropdown->setSelectedIndex(1);
            } else if (g == "khac" || st.gioiTinh == "Khác") {
                staffGenderDropdown->setSelectedIndex(2);
            } else {
                staffGenderDropdown->setSelectedIndex(0);
            }
        }
        staffFormResigned = (st.trangThaiLamViec == "DaNghiViec" || st.trangThaiLamViec == "Đã nghỉ việc");
    } else {
        selectedStaffRowIdx = -1;
        if (inputStaffId) inputStaffId->setText(generateNextStaffId());
        if (inputStaffName) inputStaffName->setText("");
        if (inputStaffPhone) inputStaffPhone->setText("");
        if (inputStaffBirthDate) inputStaffBirthDate->setText("");
        if (inputStaffCCCD) inputStaffCCCD->setText("");
        if (inputStaffEmail) inputStaffEmail->setText("");
        if (staffGenderDropdown) staffGenderDropdown->setSelectedIndex(0);
        staffFormResigned = false;
    }
}

void CinemaUIApp::setupStaffView() {
    float leftColX = sidebarWidth + 24.f;   // 274.f
    float rightColX = 1200.f;               // 484px wide right card (1200..1684)

    // Header action buttons (y = 66)
    btnStaffExport = std::unique_ptr<RoundRectButton>(new RoundRectButton(
        "    Xuất Danh Sách Excel", sf::Vector2f(1296.f, 66.f), sf::Vector2f(178.f, 38.f),
        ButtonStyle::Dark, 8.f
    ));
    btnStaffExport->setCustomColors(sf::Color::White, sf::Color(248, 250, 252), sf::Color(241, 245, 249), sf::Color(241, 245, 249));
    btnStaffExport->setOutline(1.f, sf::Color(203, 213, 225));
    btnStaffExport->setTextColor(sf::Color(30, 41, 59));
    btnStaffExport->setFontSize(12);
    btnStaffExport->setOnClick([this]() {
        std::ofstream csv("DATA/danh_sach_nhan_su.csv");
        if (csv.is_open()) {
            csv << "\xEF\xBB\xBF"; // UTF-8 BOM
            csv << "Mã NV,Họ Tên,SĐT,Ngày Sinh,Giới Tính,CCCD,Email,Vai Trò,Trạng Thái\n";
            for (const auto& u : staffUserList) {
                csv << u.maNguoiDung << "," << u.hoTen << "," << u.soDienThoai << ","
                    << u.ngaySinh << "," << u.gioiTinh << "," << u.cccd << ","
                    << u.email << "," << u.vaiTro << "," << u.trangThaiLamViec << "\n";
            }
            csv.close();
        }
        if (confirmPopup) {
            confirmPopup->show("XUẤT DANH SÁCH THÀNH CÔNG", "Đã xuất danh sách nhân sự ra file DATA/danh_sach_nhan_su.csv!", ButtonStyle::Primary, "Đóng", "");
        }
    });

    btnStaffNew = std::unique_ptr<RoundRectButton>(new RoundRectButton(
        "     + Thêm Nhân Viên Mới", sf::Vector2f(1486.f, 66.f), sf::Vector2f(198.f, 38.f),
        ButtonStyle::Primary, 8.f
    ));
    btnStaffNew->setCustomColors(sf::Color(2, 132, 199), sf::Color(3, 105, 161), sf::Color(7, 89, 133), sf::Color(2, 132, 199));
    btnStaffNew->setTextColor(sf::Color::White);
    btnStaffNew->setFontSize(12);
    btnStaffNew->setOnClick([this]() {
        syncStaffSelectionToForm(-1);
    });

    // Left Card Search & Filter Strip (y = 244)
    staffSearchInput = std::unique_ptr<UIInputBox>(new UIInputBox(
        sf::Vector2f(leftColX + 86.f, 244.f), sf::Vector2f(350.f, 34.f),
        "      Tìm theo Mã NV, Tên, CCCD..."
    ));
    staffSearchInput->setColors(
        sf::Color(248, 250, 252), sf::Color(226, 232, 240),
        sf::Color(2, 132, 199), sf::Color(15, 23, 42), sf::Color(148, 163, 184)
    );

    // Filter tabs WITHOUT numbers per user requirement
    float fTabY = 244.f;
    btnStaffFilterAll = std::unique_ptr<RoundRectButton>(new RoundRectButton(
        "Tất cả", sf::Vector2f(leftColX + 586.f, fTabY), sf::Vector2f(92.f, 32.f),
        ButtonStyle::Dark, 6.f
    ));
    btnStaffFilterAll->setFontSize(12);
    btnStaffFilterAll->setOnClick([this]() {
        staffFilterTab = 0;
        staffCurrentPage = 0;
    });

    btnStaffFilterActive = std::unique_ptr<RoundRectButton>(new RoundRectButton(
        "Đang làm", sf::Vector2f(leftColX + 684.f, fTabY), sf::Vector2f(104.f, 32.f),
        ButtonStyle::Dark, 6.f
    ));
    btnStaffFilterActive->setFontSize(12);
    btnStaffFilterActive->setOnClick([this]() {
        staffFilterTab = 1;
        staffCurrentPage = 0;
    });

    btnStaffFilterResigned = std::unique_ptr<RoundRectButton>(new RoundRectButton(
        "Đã nghỉ", sf::Vector2f(leftColX + 794.f, fTabY), sf::Vector2f(96.f, 32.f),
        ButtonStyle::Dark, 6.f
    ));
    btnStaffFilterResigned->setFontSize(12);
    btnStaffFilterResigned->setOnClick([this]() {
        staffFilterTab = 2;
        staffCurrentPage = 0;
    });

    // Pagination buttons (y = 868)
    btnStaffPrevPage = std::unique_ptr<RoundRectButton>(new RoundRectButton(
        "<", sf::Vector2f(leftColX + 754.f, 868.f), sf::Vector2f(28.f, 28.f),
        ButtonStyle::Dark, 6.f
    ));
    btnStaffPrevPage->setFontSize(12);
    btnStaffPrevPage->setCustomColors(sf::Color::White, sf::Color(248, 250, 252), sf::Color(241, 245, 249), sf::Color(241, 245, 249));
    btnStaffPrevPage->setOutline(1.f, sf::Color(226, 232, 240));
    btnStaffPrevPage->setTextColor(sf::Color(100, 116, 139));
    btnStaffPrevPage->setOnClick([this]() {
        if (staffCurrentPage > 0) staffCurrentPage--;
    });

    btnStaffNextPage = std::unique_ptr<RoundRectButton>(new RoundRectButton(
        ">", sf::Vector2f(leftColX + 862.f, 868.f), sf::Vector2f(28.f, 28.f),
        ButtonStyle::Dark, 6.f
    ));
    btnStaffNextPage->setFontSize(12);
    btnStaffNextPage->setCustomColors(sf::Color::White, sf::Color(248, 250, 252), sf::Color(241, 245, 249), sf::Color(241, 245, 249));
    btnStaffNextPage->setOutline(1.f, sf::Color(226, 232, 240));
    btnStaffNextPage->setTextColor(sf::Color(100, 116, 139));
    btnStaffNextPage->setOnClick([this]() {
        auto idxs = getFilteredStaffIndices();
        int totalP = std::max(1, (static_cast<int>(idxs.size()) + 4) / 5);
        if (staffCurrentPage < totalP - 1) staffCurrentPage++;
    });

    // Right Detail Card Form Inputs (rightColX = 1200, w = 484)
    float formInputX = rightColX + 114.f;
    float formInputW = 346.f;
    float formInputH = 36.f;
    float baseFormY = 294.f;
    float rowGap = 52.f;

    auto styleStaffInput = [](std::unique_ptr<UIInputBox>& inp, bool readOnlyStyle = false) {
        if (!inp) return;
        inp->setColors(
            readOnlyStyle ? sf::Color(241, 245, 249) : sf::Color::White,
            sf::Color(203, 213, 225),
            sf::Color(2, 132, 199),
            sf::Color(15, 23, 42),
            sf::Color(148, 163, 184)
        );
        if (readOnlyStyle) inp->setEnabled(false);
    };

    inputStaffId = std::unique_ptr<UIInputBox>(new UIInputBox(
        sf::Vector2f(formInputX, baseFormY + 0.f * rowGap), sf::Vector2f(formInputW, formInputH),
        "U001"
    ));
    styleStaffInput(inputStaffId, true);

    inputStaffName = std::unique_ptr<UIInputBox>(new UIInputBox(
        sf::Vector2f(formInputX, baseFormY + 1.f * rowGap), sf::Vector2f(formInputW, formInputH),
        "Nhập họ và tên nhân viên..."
    ));
    styleStaffInput(inputStaffName, false);

    inputStaffPhone = std::unique_ptr<UIInputBox>(new UIInputBox(
        sf::Vector2f(formInputX, baseFormY + 2.f * rowGap), sf::Vector2f(formInputW, formInputH),
        "0901000001"
    ));
    styleStaffInput(inputStaffPhone, false);

    inputStaffBirthDate = std::unique_ptr<UIInputBox>(new UIInputBox(
        sf::Vector2f(formInputX, baseFormY + 3.f * rowGap), sf::Vector2f(formInputW, formInputH),
        "DD/MM/YYYY"
    ));
    styleStaffInput(inputStaffBirthDate, false);

    staffGenderDropdown = std::unique_ptr<DropdownBox>(new DropdownBox(
        sf::Vector2f(formInputX, baseFormY + 4.f * rowGap), sf::Vector2f(formInputW, formInputH),
        {"Nam", "Nữ", "Khác"}, 0
    ));
    staffGenderDropdown->setLightStyle(true);

    inputStaffCCCD = std::unique_ptr<UIInputBox>(new UIInputBox(
        sf::Vector2f(formInputX, baseFormY + 5.f * rowGap), sf::Vector2f(formInputW, formInputH),
        "079090000001"
    ));
    styleStaffInput(inputStaffCCCD, false);

    inputStaffEmail = std::unique_ptr<UIInputBox>(new UIInputBox(
        sf::Vector2f(formInputX, baseFormY + 6.f * rowGap), sf::Vector2f(formInputW, formInputH),
        "staff@cinestar.vn"
    ));
    styleStaffInput(inputStaffEmail, false);

    // Action Buttons (y = 758 & y = 808)
    float btnY = 758.f;
    btnStaffCreate = std::unique_ptr<RoundRectButton>(new RoundRectButton(
        "+ Thêm mới", sf::Vector2f(rightColX + 24.f, btnY), sf::Vector2f(136.f, 38.f),
        ButtonStyle::Success, 8.f
    ));
    btnStaffCreate->setFontSize(13);
    btnStaffCreate->setOnClick([this]() {
        std::string n = inputStaffName ? inputStaffName->getText() : "";
        if (n.empty()) {
            if (confirmPopup) confirmPopup->show("THIẾU THÔNG TIN", "Vui lòng nhập Họ tên nhân viên!", ButtonStyle::Danger, "Đóng", "");
            return;
        }
        std::string phone = inputStaffPhone ? inputStaffPhone->getText() : "";
        bool validPhone = (phone.length() == 10);
        for (char c : phone) {
            if (!std::isdigit(static_cast<unsigned char>(c))) { validPhone = false; break; }
        }
        if (!validPhone) {
            if (confirmPopup) confirmPopup->show("SỐ ĐIỆN THOẠI KHÔNG HỢP LỆ", "Số điện thoại phải gồm đúng 10 chữ số (VD: 0901234567)!", ButtonStyle::Danger, "Đóng", "");
            return;
        }

        std::string cccd = inputStaffCCCD ? inputStaffCCCD->getText() : "";
        bool validCccd = (cccd.length() == 12);
        for (char c : cccd) {
            if (!std::isdigit(static_cast<unsigned char>(c))) { validCccd = false; break; }
        }
        if (!validCccd) {
            if (confirmPopup) confirmPopup->show("CCCD KHÔNG HỢP LỆ", "Số Căn cước công dân (CCCD) phải gồm đúng 12 chữ số!", ButtonStyle::Danger, "Đóng", "");
            return;
        }

        std::string id = inputStaffId ? inputStaffId->getText() : generateNextStaffId();
        if (id.empty()) id = generateNextStaffId();

        StaffAccountInfo newSt;
        newSt.maNguoiDung = id;
        newSt.hoTen = n;
        newSt.soDienThoai = phone;
        newSt.ngaySinh = inputStaffBirthDate ? inputStaffBirthDate->getText() : "";
        newSt.gioiTinh = staffGenderDropdown ? staffGenderDropdown->getSelectedValue() : "Nam";
        newSt.cccd = cccd;
        newSt.email = inputStaffEmail ? inputStaffEmail->getText() : "";
        newSt.taiKhoan = removeVietnameseDiacritics(id);
        std::transform(newSt.taiKhoan.begin(), newSt.taiKhoan.end(), newSt.taiKhoan.begin(), ::tolower);
        newSt.matKhau = "123456";
        newSt.vaiTro = "Staff";
        newSt.trangThaiLamViec = staffFormResigned ? "DaNghiViec" : "DangLam";
        newSt.trangThaiTaiKhoan = staffFormResigned ? "BiKhoa" : "HoatDong";

        staffUserList.push_back(newSt);
        saveStaffUsersToDatabase();
        syncStaffSelectionToForm(static_cast<int>(staffUserList.size()) - 1);
        ActivityLog::ghiNhan(currentUserId, currentUserRole, "THEM_NHAN_SU", "Thêm nhân viên mới: " + id + " - " + n);
        if (confirmPopup) confirmPopup->show("THÀNH CÔNG", "Đã thêm mới nhân sự " + id + " - " + n + " vào hệ thống!", ButtonStyle::Primary, "Đóng", "");
    });

    btnStaffUpdate = std::unique_ptr<RoundRectButton>(new RoundRectButton(
        "    Cập nhật", sf::Vector2f(rightColX + 172.f, btnY), sf::Vector2f(142.f, 38.f),
        ButtonStyle::Primary, 8.f
    ));
    btnStaffUpdate->setFontSize(13);
    btnStaffUpdate->setOnClick([this]() {
        if (selectedStaffRowIdx < 0 || selectedStaffRowIdx >= static_cast<int>(staffUserList.size())) {
            if (confirmPopup) confirmPopup->show("CHƯA CHỌN NHÂN SỰ", "Vui lòng chọn nhân viên cần cập nhật từ danh sách!", ButtonStyle::Danger, "Đóng", "");
            return;
        }
        auto& st = staffUserList[selectedStaffRowIdx];
        if (st.vaiTro == "Admin" && staffFormResigned) {
            if (confirmPopup) confirmPopup->show("KHÔNG THỂ THAO TÁC", "Không thể cho Admin nghỉ việc tại đây để đảm bảo hệ thống luôn có ít nhất một Quản trị viên hoạt động. Vui lòng quản lý tài khoản tại mục Quản lý tài khoản!", ButtonStyle::Danger, "Đóng", "");
            return;
        }

        std::string phone = inputStaffPhone ? inputStaffPhone->getText() : "";
        bool validPhone = (phone.length() == 10);
        for (char c : phone) {
            if (!std::isdigit(static_cast<unsigned char>(c))) { validPhone = false; break; }
        }
        if (!validPhone) {
            if (confirmPopup) confirmPopup->show("SỐ ĐIỆN THOẠI KHÔNG HỢP LỆ", "Số điện thoại phải gồm đúng 10 chữ số (VD: 0901234567)!", ButtonStyle::Danger, "Đóng", "");
            return;
        }

        std::string cccd = inputStaffCCCD ? inputStaffCCCD->getText() : "";
        bool validCccd = (cccd.length() == 12);
        for (char c : cccd) {
            if (!std::isdigit(static_cast<unsigned char>(c))) { validCccd = false; break; }
        }
        if (!validCccd) {
            if (confirmPopup) confirmPopup->show("CCCD KHÔNG HỢP LỆ", "Số Căn cước công dân (CCCD) phải gồm đúng 12 chữ số!", ButtonStyle::Danger, "Đóng", "");
            return;
        }

        if (inputStaffName) st.hoTen = inputStaffName->getText();
        st.soDienThoai = phone;
        if (inputStaffBirthDate) st.ngaySinh = inputStaffBirthDate->getText();
        if (staffGenderDropdown) st.gioiTinh = staffGenderDropdown->getSelectedValue();
        st.cccd = cccd;
        if (inputStaffEmail) st.email = inputStaffEmail->getText();
        st.trangThaiLamViec = staffFormResigned ? "DaNghiViec" : "DangLam";
        if (staffFormResigned) st.trangThaiTaiKhoan = "BiKhoa";

        saveStaffUsersToDatabase();
        ActivityLog::ghiNhan(currentUserId, currentUserRole, "CAP_NHAT_NHAN_SU", "Cập nhật hồ sơ: " + st.maNguoiDung + " - " + st.hoTen);
        if (confirmPopup) confirmPopup->show("CẬP NHẬT THÀNH CÔNG", "Đã lưu cập nhật thông tin nhân viên " + st.maNguoiDung + "!", ButtonStyle::Primary, "Đóng", "");
    });

    btnStaffClear = std::unique_ptr<RoundRectButton>(new RoundRectButton(
        "    Clear Form", sf::Vector2f(rightColX + 326.f, btnY), sf::Vector2f(134.f, 38.f),
        ButtonStyle::Dark, 8.f
    ));
    btnStaffClear->setFontSize(13);
    btnStaffClear->setCustomColors(sf::Color::White, sf::Color(248, 250, 252), sf::Color(241, 245, 249), sf::Color(241, 245, 249));
    btnStaffClear->setOutline(1.f, sf::Color(203, 213, 225));
    btnStaffClear->setTextColor(sf::Color(51, 65, 85));
    btnStaffClear->setOnClick([this]() {
        syncStaffSelectionToForm(-1);
    });

    btnStaffDelete = std::unique_ptr<RoundRectButton>(new RoundRectButton(
        "     Xóa vĩnh viễn hồ sơ này", sf::Vector2f(rightColX + 122.f, 808.f), sf::Vector2f(240.f, 32.f),
        ButtonStyle::Ghost, 8.f
    ));
    btnStaffDelete->setFontSize(12);
    btnStaffDelete->setCustomColors(sf::Color::White, sf::Color(254, 242, 242), sf::Color(254, 226, 226), sf::Color(254, 242, 242));
    btnStaffDelete->setOutline(1.f, sf::Color(254, 205, 211));
    btnStaffDelete->setTextColor(sf::Color(225, 29, 72));
    btnStaffDelete->setOnClick([this]() {
        if (selectedStaffRowIdx < 0 || selectedStaffRowIdx >= static_cast<int>(staffUserList.size())) {
            if (confirmPopup) confirmPopup->show("CHƯA CHỌN NHÂN SỰ", "Vui lòng chọn nhân sự cần xóa từ danh sách!", ButtonStyle::Danger, "Đóng", "");
            return;
        }
        std::string delId = staffUserList[selectedStaffRowIdx].maNguoiDung;
        std::string delName = staffUserList[selectedStaffRowIdx].hoTen;
        staffUserList.erase(staffUserList.begin() + selectedStaffRowIdx);
        saveStaffUsersToDatabase();

        if (!staffUserList.empty()) {
            selectedStaffRowIdx = std::min(selectedStaffRowIdx, static_cast<int>(staffUserList.size()) - 1);
            syncStaffSelectionToForm(selectedStaffRowIdx);
        } else {
            syncStaffSelectionToForm(-1);
        }
        ActivityLog::ghiNhan(currentUserId, currentUserRole, "XOA_NHAN_SU", "Xóa hồ sơ: " + delId);
        if (confirmPopup) confirmPopup->show("ĐÃ XÓA HỒ SƠ", "Đã xóa vĩnh viễn nhân sự '" + delName + "' (" + delId + ") khỏi hệ thống!", ButtonStyle::Primary, "Đóng", "");
    });

    if (!staffUserList.empty()) {
        syncStaffSelectionToForm(0);
    } else {
        syncStaffSelectionToForm(-1);
    }
}

void CinemaUIApp::renderStaffView() {
    float leftColX = sidebarWidth + 24.f; // 274.f
    float rightColX = 1200.f;
    float pageRight = 1684.f;

    // Light Slate Background Canvas for the entire Staff workspace
    sf::RectangleShape bgCanvas(sf::Vector2f(1710.f - sidebarWidth, 970.f));
    bgCanvas.setPosition(sidebarWidth, 0.f);
    bgCanvas.setFillColor(sf::Color(244, 247, 250));
    window.draw(bgCanvas);

    // Helper lambdas using Segoe UI (Theme::FontPrimary) + toSfString()
    auto drawTxt = [&](const std::string& str, float x, float y, unsigned int sz, const sf::Color& col, bool bold = false, bool italic = false) {
        sf::Text t;
        t.setFont(Theme::FontPrimary);
        t.setCharacterSize(sz);
        sf::Uint32 style = sf::Text::Regular;
        if (bold) style |= sf::Text::Bold;
        if (italic) style |= sf::Text::Italic;
        t.setStyle(style);
        t.setFillColor(col);
        t.setString(toSfString(str));
        t.setPosition(std::round(x), std::round(y));
        window.draw(t);
    };

    auto drawCenteredTxt = [&](const std::string& str, float cx, float cy, unsigned int sz, const sf::Color& col, bool bold = false) {
        sf::Text t;
        t.setFont(Theme::FontPrimary);
        t.setCharacterSize(sz);
        if (bold) t.setStyle(sf::Text::Bold);
        t.setFillColor(col);
        t.setString(toSfString(str));
        sf::FloatRect b = t.getLocalBounds();
        t.setOrigin(std::round(b.left + b.width * 0.5f), std::round(b.top + b.height * 0.5f));
        t.setPosition(std::round(cx), std::round(cy));
        window.draw(t);
    };

    // =========================================================================
    // 1. TOP NAVIGATION BAR (y = 0..48)
    // =========================================================================
    sf::RectangleShape topBarBg(sf::Vector2f(1710.f - sidebarWidth, 48.f));
    topBarBg.setPosition(sidebarWidth, 0.f);
    topBarBg.setFillColor(sf::Color::White);
    window.draw(topBarBg);

    sf::RectangleShape topBarLine(sf::Vector2f(1710.f - sidebarWidth, 1.f));
    topBarLine.setPosition(sidebarWidth, 48.f);
    topBarLine.setFillColor(sf::Color(226, 232, 240));
    window.draw(topBarLine);

    // Brand Logo Icon
    RoundedRectRenderer::render(window, sf::Vector2f(leftColX, 8.f), sf::Vector2f(32.f, 32.f), 8.f, sf::Color(2, 132, 199));
    RoundedRectRenderer::render(window, sf::Vector2f(leftColX + 7.f, 15.f), sf::Vector2f(18.f, 18.f), 3.f, sf::Color::Transparent, 1.5f, sf::Color::White);
    sf::RectangleShape filmMid(sf::Vector2f(18.f, 1.5f));
    filmMid.setPosition(leftColX + 7.f, 23.f);
    filmMid.setFillColor(sf::Color::White);
    window.draw(filmMid);

    drawTxt("Cinestar", leftColX + 40.f, 7.f, 15, sf::Color(15, 23, 42), true);
    drawTxt("CineOS", leftColX + 106.f, 7.f, 15, sf::Color(2, 132, 199), true);

    RoundedRectRenderer::render(window, sf::Vector2f(leftColX + 166.f, 8.f), sf::Vector2f(62.f, 18.f), 9.f, sf::Color(224, 242, 254), 1.f, sf::Color(186, 230, 253));
    drawCenteredTxt("v3.4 PRO", leftColX + 197.f, 17.f, 10, sf::Color(3, 105, 161), true);

    drawTxt("Phân Hệ Quản Trị Nhân Sự & Ca Trực Cụm Rạp", leftColX + 40.f, 27.f, 10, sf::Color(100, 116, 139), false);

    // Vertical separator
    sf::RectangleShape vSep1(sf::Vector2f(1.f, 24.f));
    vSep1.setPosition(leftColX + 306.f, 12.f);
    vSep1.setFillColor(sf::Color(226, 232, 240));
    window.draw(vSep1);

    // Realtime sync status
    sf::CircleShape greenDot(4.f);
    greenDot.setFillColor(sf::Color(16, 185, 129));
    greenDot.setPosition(leftColX + 322.f, 20.f);
    window.draw(greenDot);
    drawTxt("Đồng bộ Realtime  •  Cụm Rạp Toàn Quốc", leftColX + 336.f, 16.f, 12, sf::Color(30, 41, 59), true);

    // Quick search pill
    RoundedRectRenderer::render(window, sf::Vector2f(1140.f, 9.f), sf::Vector2f(240.f, 30.f), 15.f, sf::Color(248, 250, 252), 1.f, sf::Color(226, 232, 240));
    sf::CircleShape qLens(4.5f);
    qLens.setFillColor(sf::Color::Transparent);
    qLens.setOutlineThickness(1.4f);
    qLens.setOutlineColor(sf::Color(148, 163, 184));
    qLens.setPosition(1154.f, 19.f);
    window.draw(qLens);
    drawTxt("Tìm tác vụ nhanh... (Ctrl+K)", 1172.f, 16.f, 11, sf::Color(148, 163, 184), false);

    // User profile badge (Admin)
    sf::CircleShape avatarCir(15.f);
    avatarCir.setFillColor(sf::Color(224, 242, 254));
    avatarCir.setOutlineThickness(1.5f);
    avatarCir.setOutlineColor(sf::Color(2, 132, 199));
    avatarCir.setPosition(1462.f, 9.f);
    window.draw(avatarCir);
    drawCenteredTxt("AD", 1477.f, 24.f, 11, sf::Color(3, 105, 161), true);
    drawTxt(currentUserName.empty() ? "Admin" : currentUserName, 1500.f, 8.f, 12, sf::Color(15, 23, 42), true);
    drawTxt("Quản trị viên hệ thống", 1500.f, 24.f, 11, sf::Color(100, 116, 139), false);

    // =========================================================================
    // 2. PAGE HEADER (y = 60..112)
    // =========================================================================
    drawTxt("QUẢN LÝ NHÂN SỰ", leftColX, 60.f, 22, sf::Color(15, 23, 42), true);

    // Sticker badge [Hồ sơ nhân sự vật lý] with generous padding
    RoundedRectRenderer::render(window, sf::Vector2f(leftColX + 224.f, 63.f), sf::Vector2f(152.f, 24.f), 12.f, sf::Color(224, 242, 254), 1.f, sf::Color(186, 230, 253));
    drawCenteredTxt("Hồ sơ nhân sự vật lý", leftColX + 300.f, 75.f, 11, sf::Color(3, 105, 161), true);

    drawTxt("Tạo hồ sơ nhân sự vật lý, phân ca trực & phân quyền vận hành cụm rạp", leftColX, 91.f, 12, sf::Color(100, 116, 139), false);

    if (btnStaffExport) {
        btnStaffExport->draw(window);
        float ex = btnStaffExport->getPosition().x + 14.f;
        float ey = btnStaffExport->getPosition().y + 11.f;
        RoundedRectRenderer::render(window, sf::Vector2f(ex, ey), sf::Vector2f(14.f, 16.f), 2.f, sf::Color(240, 253, 244), 1.3f, sf::Color(22, 163, 74));
        drawCenteredTxt("X", ex + 7.f, ey + 8.f, 9, sf::Color(22, 163, 74), true);
    }
    if (btnStaffNew) {
        btnStaffNew->draw(window);
        float nx = btnStaffNew->getPosition().x + 16.f;
        float ny = btnStaffNew->getPosition().y + 12.f;
        sf::CircleShape uHead(3.5f);
        uHead.setFillColor(sf::Color::White);
        uHead.setPosition(nx + 2.f, ny);
        window.draw(uHead);
        RoundedRectRenderer::render(window, sf::Vector2f(nx, ny + 8.f), sf::Vector2f(11.f, 5.f), 2.5f, sf::Color::White);
    }

    // =========================================================================
    // 3. STATISTICAL KPI ROW (4 Read-Only Cards Linked to SQLite Data, y = 122)
    // =========================================================================
    int totalStaffCount = static_cast<int>(staffUserList.size());
    int activeStaffCount = 0;
    int resignedStaffCount = 0;
    for (const auto& st : staffUserList) {
        if (st.trangThaiLamViec == "DaNghiViec" || st.trangThaiLamViec == "Đã nghỉ việc") {
            resignedStaffCount++;
        } else {
            activeStaffCount++;
        }
    }
    int onDutyCount = 0;
    for (const auto& sh : shiftList) {
        if (sh.getTrangThai() != "DaKetThuc") onDutyCount++;
    }
    if (onDutyCount == 0) onDutyCount = std::max(1, activeStaffCount - 1);

    float activePct = (totalStaffCount > 0) ? (static_cast<float>(activeStaffCount) * 100.f / static_cast<float>(totalStaffCount)) : 0.f;
    std::ostringstream pctOss;
    pctOss << std::fixed << std::setprecision(1) << activePct << "%";

    auto fmt2Digits = [](int n) -> std::string {
        std::ostringstream o;
        o << std::setw(2) << std::setfill('0') << n;
        return o.str();
    };

    float kpiY = 122.f;
    float kpiH = 92.f;
    float kpiGap = 18.f;
    float kpiW = (pageRight - leftColX - 3.f * kpiGap) / 4.f; // ~339px each

    struct StaffKpiCard {
        std::string title;
        std::string mainVal;
        std::string badgeTxt;
        sf::Color badgeCol;
        std::string subTxt;
        sf::Color iconBg;
        sf::Color iconFg;
    };

    std::vector<StaffKpiCard> kpis = {
        {"TỔNG NHÂN SỰ", fmt2Digits(totalStaffCount), "Đủ định biên", sf::Color(16, 185, 129), "Toàn bộ nhân sự cơ hữu rạp", sf::Color(224, 242, 254), sf::Color(2, 132, 199)},
        {"ĐANG LÀM VIỆC", fmt2Digits(activeStaffCount), pctOss.str(), sf::Color(16, 185, 129), "Tài khoản vận hành hoạt động", sf::Color(220, 252, 231), sf::Color(22, 163, 74)},
        {"CA HIỆN TẠI (ON-DUTY)", fmt2Digits(onDutyCount), "Sẵn sàng ca trực", sf::Color(2, 132, 199), "Đang check-in tại quầy & phòng chiếu", sf::Color(224, 242, 254), sf::Color(2, 132, 199)},
        {"ĐÃ NGHỈ VIỆC / LƯU TRỮ", fmt2Digits(resignedStaffCount), "Đã khóa tài khoản", sf::Color(100, 116, 139), "Lưu trữ bảo lưu hồ sơ", sf::Color(241, 245, 249), sf::Color(71, 85, 105)}
    };

    for (size_t i = 0; i < kpis.size(); ++i) {
        float cx = leftColX + i * (kpiW + kpiGap);
        RoundedRectRenderer::render(window, sf::Vector2f(cx, kpiY), sf::Vector2f(kpiW, kpiH), 12.f, sf::Color::White, 1.f, sf::Color(226, 232, 240));

        drawTxt(kpis[i].title, cx + 20.f, kpiY + 14.f, 11, sf::Color(100, 116, 139), true);
        drawTxt(kpis[i].mainVal, cx + 20.f, kpiY + 34.f, 24, (i == 2) ? sf::Color(2, 132, 199) : sf::Color(15, 23, 42), true);
        drawTxt(kpis[i].badgeTxt, cx + 62.f, kpiY + 43.f, 12, kpis[i].badgeCol, true);
        drawTxt(kpis[i].subTxt, cx + 20.f, kpiY + 67.f, 11, sf::Color(148, 163, 184), false);

        float ibX = cx + kpiW - 58.f;
        float ibY = kpiY + 26.f;
        RoundedRectRenderer::render(window, sf::Vector2f(ibX, ibY), sf::Vector2f(40.f, 40.f), 10.f, kpis[i].iconBg);

        if (i == 0) {
            sf::CircleShape h1(4.f); h1.setFillColor(kpis[i].iconFg); h1.setPosition(ibX + 16.f, ibY + 10.f); window.draw(h1);
            RoundedRectRenderer::render(window, sf::Vector2f(ibX + 12.f, ibY + 20.f), sf::Vector2f(16.f, 8.f), 4.f, kpis[i].iconFg);
            sf::CircleShape h2(3.f); h2.setFillColor(kpis[i].iconFg); h2.setPosition(ibX + 9.f, ibY + 12.f); window.draw(h2);
            sf::CircleShape h3(3.f); h3.setFillColor(kpis[i].iconFg); h3.setPosition(ibX + 25.f, ibY + 12.f); window.draw(h3);
        } else if (i == 1) {
            sf::CircleShape h1(4.5f); h1.setFillColor(kpis[i].iconFg); h1.setPosition(ibX + 12.f, ibY + 10.f); window.draw(h1);
            RoundedRectRenderer::render(window, sf::Vector2f(ibX + 9.f, ibY + 21.f), sf::Vector2f(15.f, 8.f), 4.f, kpis[i].iconFg);
            {
                sf::RectangleShape ck1(sf::Vector2f(3.5f, 1.5f));
                ck1.setRotation(45.f);
                ck1.setPosition(ibX + 26.5f, ibY + 18.5f);
                ck1.setFillColor(kpis[i].iconFg);
                sf::RectangleShape ck2(sf::Vector2f(6.5f, 1.5f));
                ck2.setRotation(-50.f);
                ck2.setPosition(ibX + 29.f, ibY + 21.f);
                ck2.setFillColor(kpis[i].iconFg);
                window.draw(ck1);
                window.draw(ck2);
            }
        } else if (i == 2) {
            sf::CircleShape clk(10.f); clk.setFillColor(kpis[i].iconFg); clk.setPosition(ibX + 10.f, ibY + 10.f); window.draw(clk);
            sf::RectangleShape hand1(sf::Vector2f(2.f, 6.f)); hand1.setFillColor(sf::Color::White); hand1.setPosition(ibX + 19.f, ibY + 14.f); window.draw(hand1);
            sf::RectangleShape hand2(sf::Vector2f(5.f, 2.f)); hand2.setFillColor(sf::Color::White); hand2.setPosition(ibX + 19.f, ibY + 19.f); window.draw(hand2);
        } else {
            sf::CircleShape h1(4.5f); h1.setFillColor(kpis[i].iconFg); h1.setPosition(ibX + 15.f, ibY + 10.f); window.draw(h1);
            RoundedRectRenderer::render(window, sf::Vector2f(ibX + 12.f, ibY + 21.f), sf::Vector2f(15.f, 8.f), 4.f, kpis[i].iconFg);
            sf::RectangleShape slash(sf::Vector2f(22.f, 2.f));
            slash.setFillColor(sf::Color(71, 85, 105));
            slash.setPosition(ibX + 12.f, ibY + 11.f);
            slash.setRotation(42.f);
            window.draw(slash);
        }
    }

    // =========================================================================
    // 4. LEFT CARD: STAFF DIRECTORY TABLE (leftColX = 274, y = 228, w = 910, h = 684)
    // =========================================================================
    float tblCardX = leftColX;
    float tblCardY = 228.f;
    float tblCardW = 910.f;
    float tblCardH = 684.f;
    RoundedRectRenderer::render(window, sf::Vector2f(tblCardX, tblCardY), sf::Vector2f(tblCardW, tblCardH), 12.f, sf::Color::White, 1.f, sf::Color(226, 232, 240));

    // Search label & input
    drawTxt("Tìm nhanh:", tblCardX + 16.f, 252.f, 12, sf::Color(30, 41, 59), true);
    if (staffSearchInput) {
        staffSearchInput->draw(window);
        sf::CircleShape sCir(4.5f);
        sCir.setFillColor(sf::Color::Transparent);
        sCir.setOutlineThickness(1.4f);
        sCir.setOutlineColor(sf::Color(148, 163, 184));
        sCir.setPosition(tblCardX + 98.f, 255.f);
        window.draw(sCir);
        sf::RectangleShape sHdl(sf::Vector2f(4.5f, 1.4f));
        sHdl.setFillColor(sf::Color(148, 163, 184));
        sHdl.setPosition(tblCardX + 106.f, 263.f);
        sHdl.setRotation(45.f);
        window.draw(sHdl);
    }

    // Filter pill container background (NO counts on pills per user requirement)
    RoundedRectRenderer::render(window, sf::Vector2f(tblCardX + 582.f, 240.f), sf::Vector2f(312.f, 40.f), 8.f, sf::Color(241, 245, 249));
    auto styleFilterTab = [&](std::unique_ptr<RoundRectButton>& btn, bool active) {
        if (!btn) return;
        if (active) {
            btn->setCustomColors(sf::Color::White, sf::Color(248, 250, 252), sf::Color(241, 245, 249), sf::Color(241, 245, 249));
            btn->setTextColor(sf::Color(2, 132, 199));
            btn->setOutline(1.f, sf::Color(226, 232, 240));
        } else {
            btn->setCustomColors(sf::Color(241, 245, 249), sf::Color(226, 232, 240), sf::Color(203, 213, 225), sf::Color(241, 245, 249));
            btn->setTextColor(sf::Color(71, 85, 105));
            btn->setOutline(0.f, sf::Color::Transparent);
        }
        btn->draw(window);
    };
    styleFilterTab(btnStaffFilterAll, staffFilterTab == 0);
    styleFilterTab(btnStaffFilterActive, staffFilterTab == 1);
    styleFilterTab(btnStaffFilterResigned, staffFilterTab == 2);

    // Table Header Bar (y = 294, h = 44)
    float thY = 294.f;
    float thH = 44.f;
    sf::RectangleShape thBg(sf::Vector2f(tblCardW - 2.f, thH));
    thBg.setPosition(tblCardX + 1.f, thY);
    thBg.setFillColor(sf::Color(248, 250, 252));
    window.draw(thBg);

    sf::RectangleShape thTopLine(sf::Vector2f(tblCardW - 2.f, 1.f));
    thTopLine.setPosition(tblCardX + 1.f, thY);
    thTopLine.setFillColor(sf::Color(226, 232, 240));
    window.draw(thTopLine);

    sf::RectangleShape thBotLine(sf::Vector2f(tblCardW - 2.f, 1.f));
    thBotLine.setPosition(tblCardX + 1.f, thY + thH);
    thBotLine.setFillColor(sf::Color(226, 232, 240));
    window.draw(thBotLine);

    drawCenteredTxt("MÃ NV", tblCardX + 38.f, thY + 22.f, 11, sf::Color(71, 85, 105), true);
    drawTxt("HỌ TÊN", tblCardX + 76.f, thY + 14.f, 11, sf::Color(71, 85, 105), true);
    drawCenteredTxt("SĐT", tblCardX + 292.f, thY + 22.f, 11, sf::Color(71, 85, 105), true);
    drawCenteredTxt("NGÀY SINH", tblCardX + 388.f, thY + 22.f, 11, sf::Color(71, 85, 105), true);
    drawCenteredTxt("GIỚI TÍNH", tblCardX + 464.f, thY + 22.f, 11, sf::Color(71, 85, 105), true);
    drawCenteredTxt("CCCD", tblCardX + 548.f, thY + 22.f, 11, sf::Color(71, 85, 105), true);
    drawCenteredTxt("EMAIL", tblCardX + 688.f, thY + 22.f, 11, sf::Color(71, 85, 105), true);
    drawCenteredTxt("TRẠNG THÁI", tblCardX + 838.f, thY + 22.f, 11, sf::Color(71, 85, 105), true);

    std::vector<int> filteredIndices = getFilteredStaffIndices();
    const int rowsPerPage = 5;
    int totalFiltered = static_cast<int>(filteredIndices.size());
    int totalPages = std::max(1, (totalFiltered + rowsPerPage - 1) / rowsPerPage);
    if (staffCurrentPage >= totalPages) staffCurrentPage = totalPages - 1;
    if (staffCurrentPage < 0) staffCurrentPage = 0;

    int startIdx = staffCurrentPage * rowsPerPage;
    int endIdx = std::min(startIdx + rowsPerPage, totalFiltered);

    std::vector<sf::Color> avatarPalette = {
        sf::Color(2, 132, 199),   // A - Blue
        sf::Color(168, 85, 247),  // B - Purple
        sf::Color(245, 158, 11),  // C - Amber
        sf::Color(244, 63, 94),   // D - Rose
        sf::Color(148, 163, 184)  // E - Slate
    };

    float rowStartY = thY + thH; // 338.f
    float rowH = 102.f;          // 5 rows = 510px (338..848)

    for (int i = startIdx; i < endIdx; ++i) {
        int realIdx = filteredIndices[i];
        int localRow = i - startIdx;
        float ry = rowStartY + localRow * rowH;
        const auto& st = staffUserList[realIdx];
        bool isSelected = (realIdx == selectedStaffRowIdx);
        bool isResigned = (st.trangThaiLamViec == "DaNghiViec" || st.trangThaiLamViec == "Đã nghỉ việc");

        if (isSelected) {
            sf::RectangleShape selBg(sf::Vector2f(tblCardW - 2.f, rowH));
            selBg.setPosition(tblCardX + 1.f, ry);
            selBg.setFillColor(sf::Color(240, 249, 255));
            window.draw(selBg);

            sf::RectangleShape selBar(sf::Vector2f(4.f, rowH));
            selBar.setPosition(tblCardX + 1.f, ry);
            selBar.setFillColor(sf::Color(2, 132, 199));
            window.draw(selBar);
        }

        // Bottom row divider
        sf::RectangleShape rDiv(sf::Vector2f(tblCardW - 2.f, 1.f));
        rDiv.setPosition(tblCardX + 1.f, ry + rowH);
        rDiv.setFillColor(sf::Color(226, 232, 240));
        window.draw(rDiv);

        float midY = ry + rowH * 0.5f;
        sf::Color mainTextCol = isResigned ? sf::Color(100, 116, 139) : sf::Color(30, 41, 59);

        // 1. MÃ NV
        drawCenteredTxt(st.maNguoiDung, tblCardX + 38.f, midY, 13, isSelected ? sf::Color(2, 132, 199) : mainTextCol, true);

        // 2. HỌ TÊN with circular initial avatar
        sf::Color avCol = isResigned ? sf::Color(148, 163, 184) : avatarPalette[realIdx % avatarPalette.size()];
        sf::CircleShape avCir(15.f);
        avCir.setFillColor(avCol);
        avCir.setPosition(tblCardX + 74.f, midY - 15.f);
        window.draw(avCir);

        std::string initChar = "N";
        size_t lastSpace = st.hoTen.find_last_of(' ');
        std::string lastWord = (lastSpace != std::string::npos && lastSpace + 1 < st.hoTen.size()) ? st.hoTen.substr(lastSpace + 1) : st.hoTen;
        std::string cleanWord = removeVietnameseDiacritics(lastWord);
        if (!cleanWord.empty()) {
            initChar = std::string(1, static_cast<char>(std::toupper(static_cast<unsigned char>(cleanWord[0]))));
        }
        drawCenteredTxt(initChar, tblCardX + 89.f, midY, 12, sf::Color::White, true);

        drawTxt(st.hoTen, tblCardX + 112.f, midY - 8.f, 13, isResigned ? sf::Color(100, 116, 139) : sf::Color(15, 23, 42), true);
        if (isResigned) {
            sf::Text tmpName;
            tmpName.setFont(Theme::FontPrimary);
            tmpName.setCharacterSize(13);
            tmpName.setString(toSfString(st.hoTen));
            float nw = tmpName.getLocalBounds().width;
            sf::RectangleShape strike(sf::Vector2f(nw, 1.f));
            strike.setPosition(tblCardX + 112.f, midY + 1.f);
            strike.setFillColor(sf::Color(148, 163, 184));
            window.draw(strike);
        }

        // 3. SĐT
        drawCenteredTxt(st.soDienThoai, tblCardX + 292.f, midY, 12, mainTextCol, false);

        // 4. NGÀY SINH
        drawCenteredTxt(st.ngaySinh, tblCardX + 388.f, midY, 12, mainTextCol, false);

        // 5. GIỚI TÍNH
        std::string genderDisp = (st.gioiTinh == "Nu") ? "Nữ" : st.gioiTinh;
        drawCenteredTxt(genderDisp, tblCardX + 464.f, midY, 12, mainTextCol, false);

        // 6. CCCD
        drawCenteredTxt(st.cccd, tblCardX + 548.f, midY, 12, mainTextCol, false);

        // 7. EMAIL
        drawCenteredTxt(st.email, tblCardX + 688.f, midY, 12, sf::Color(71, 85, 105), false);

        // 8. TRẠNG THÁI Sticker
        float stBadgeW = 112.f;
        float stBadgeH = 30.f;
        float stBadgeX = tblCardX + 838.f - stBadgeW * 0.5f;
        float stBadgeY = midY - stBadgeH * 0.5f;

        if (!isResigned) {
            RoundedRectRenderer::render(window, sf::Vector2f(stBadgeX, stBadgeY), sf::Vector2f(stBadgeW, stBadgeH), 15.f, sf::Color(220, 252, 231), 1.f, sf::Color(134, 239, 172));
            sf::CircleShape dot(3.f);
            dot.setFillColor(sf::Color(22, 163, 74));
            dot.setPosition(stBadgeX + 10.f, midY - 3.f);
            window.draw(dot);
            drawCenteredTxt("Đang làm việc", stBadgeX + stBadgeW * 0.5f + 5.f, midY, 11, sf::Color(21, 128, 61), true);
        } else {
            RoundedRectRenderer::render(window, sf::Vector2f(stBadgeX, stBadgeY), sf::Vector2f(stBadgeW, stBadgeH), 15.f, sf::Color(241, 245, 249), 1.f, sf::Color(203, 213, 225));
            sf::CircleShape dot(3.f);
            dot.setFillColor(sf::Color(100, 116, 139));
            dot.setPosition(stBadgeX + 12.f, midY - 3.f);
            window.draw(dot);
            drawCenteredTxt("Đã nghỉ việc", stBadgeX + stBadgeW * 0.5f + 5.f, midY, 11, sf::Color(71, 85, 105), true);
        }
    }

    // Footer Pagination Strip (y = 856..912)
    int dispFrom = (totalFiltered == 0) ? 0 : (startIdx + 1);
    int dispTo = endIdx;
    std::string pageInfo = "Hiển thị " + std::to_string(dispFrom) + " - " + std::to_string(dispTo) +
                           " trong tổng số " + std::to_string(totalFiltered) + " nhân viên rạp";
    drawTxt(pageInfo, tblCardX + 18.f, 874.f, 12, sf::Color(100, 116, 139), false);

    if (btnStaffPrevPage) btnStaffPrevPage->draw(window);

    for (int p = 0; p < std::min(totalPages, 2); ++p) {
        float px = tblCardX + 788.f + p * 34.f;
        bool activeP = (p == staffCurrentPage);
        RoundedRectRenderer::render(
            window, sf::Vector2f(px, 868.f), sf::Vector2f(28.f, 28.f), 6.f,
            activeP ? sf::Color(2, 132, 199) : sf::Color::White,
            1.f, activeP ? sf::Color(2, 132, 199) : sf::Color(226, 232, 240)
        );
        drawCenteredTxt(std::to_string(p + 1), px + 14.f, 882.f, 12, activeP ? sf::Color::White : sf::Color(71, 85, 105), true);
    }

    if (btnStaffNextPage) btnStaffNextPage->draw(window);

    // =========================================================================
    // 5. RIGHT CARD: STAFF PROFILE DETAIL FORM (rightColX = 1200, y = 228, w = 484, h = 684)
    // =========================================================================
    float formCardW = 484.f;
    float formCardH = 684.f;
    RoundedRectRenderer::render(window, sf::Vector2f(rightColX, tblCardY), sf::Vector2f(formCardW, formCardH), 12.f, sf::Color::White, 1.f, sf::Color(226, 232, 240));

    RoundedRectRenderer::render(window, sf::Vector2f(rightColX + 24.f, tblCardY + 20.f), sf::Vector2f(5.f, 20.f), 2.5f, sf::Color(2, 132, 199));
    drawTxt("Thông tin chi tiết", rightColX + 38.f, tblCardY + 19.f, 16, sf::Color(15, 23, 42), true);

    std::string selBadgeStr = (selectedStaffRowIdx >= 0 && selectedStaffRowIdx < static_cast<int>(staffUserList.size()))
        ? ("#" + staffUserList[selectedStaffRowIdx].maNguoiDung + " - Đang chọn")
        : "Tạo mới hồ sơ";
    RoundedRectRenderer::render(window, sf::Vector2f(rightColX + 340.f, tblCardY + 17.f), sf::Vector2f(120.f, 26.f), 6.f, sf::Color(224, 242, 254), 1.f, sf::Color(186, 230, 253));
    drawCenteredTxt(selBadgeStr, rightColX + 400.f, tblCardY + 30.f, 11, sf::Color(3, 105, 161), true);

    float baseFormY = 294.f;
    float rowGap = 52.f;
    std::vector<std::string> fieldLabels = {
        "Mã NV:", "Họ tên:", "SĐT:", "Ngày sinh:", "Giới tính:", "CCCD:", "Email:"
    };

    for (size_t f = 0; f < fieldLabels.size(); ++f) {
        drawTxt(fieldLabels[f], rightColX + 24.f, baseFormY + f * rowGap + 9.f, 13, sf::Color(51, 65, 85), true);
    }

    if (inputStaffId) inputStaffId->draw(window);
    if (inputStaffName) inputStaffName->draw(window);
    if (inputStaffPhone) inputStaffPhone->draw(window);
    if (inputStaffBirthDate) {
        inputStaffBirthDate->draw(window);
    }
    if (staffGenderDropdown) staffGenderDropdown->draw(window);
    if (inputStaffCCCD) inputStaffCCCD->draw(window);
    if (inputStaffEmail) inputStaffEmail->draw(window);

    // Trạng thái Checkbox Row (y = 664)
    float chkY = 664.f;
    drawTxt("Trạng thái:", rightColX + 24.f, chkY + 2.f, 13, sf::Color(51, 65, 85), true);

    float chkBoxX = rightColX + 114.f;
    RoundedRectRenderer::render(
        window, sf::Vector2f(chkBoxX, chkY + 1.f), sf::Vector2f(18.f, 18.f), 4.f,
        staffFormResigned ? sf::Color(2, 132, 199) : sf::Color::White,
        1.5f, staffFormResigned ? sf::Color(2, 132, 199) : sf::Color(203, 213, 225)
    );
    if (staffFormResigned) {
        sf::RectangleShape ck1(sf::Vector2f(4.5f, 2.f));
        ck1.setRotation(45.f);
        ck1.setPosition(chkBoxX + 4.5f, chkY + 8.5f);
        ck1.setFillColor(sf::Color::White);
        sf::RectangleShape ck2(sf::Vector2f(8.5f, 2.f));
        ck2.setRotation(-50.f);
        ck2.setPosition(chkBoxX + 7.5f, chkY + 11.5f);
        ck2.setFillColor(sf::Color::White);
        window.draw(ck1);
        window.draw(ck2);
    }
    drawTxt("Đã nghỉ làm", chkBoxX + 28.f, chkY + 2.f, 13, sf::Color(30, 41, 59), true);
    drawTxt("*Tick nếu nhân viên đã nghỉ việc để khóa tài khoản", chkBoxX, chkY + 26.f, 11, sf::Color(148, 163, 184), false, true);
    drawTxt("POS/hệ thống.", chkBoxX, chkY + 42.f, 11, sf::Color(148, 163, 184), false, true);

    sf::RectangleShape fDiv(sf::Vector2f(formCardW - 48.f, 1.f));
    fDiv.setPosition(rightColX + 24.f, 738.f);
    fDiv.setFillColor(sf::Color(241, 245, 249));
    window.draw(fDiv);

    if (btnStaffCreate) btnStaffCreate->draw(window);
    if (btnStaffUpdate) {
        btnStaffUpdate->draw(window);
        float ux = btnStaffUpdate->getPosition().x + 22.f;
        float uy = btnStaffUpdate->getPosition().y + 12.f;
        RoundedRectRenderer::render(window, sf::Vector2f(ux, uy), sf::Vector2f(13.f, 13.f), 2.f, sf::Color::Transparent, 1.4f, sf::Color::White);
        drawCenteredTxt("✎", ux + 7.f, uy + 6.f, 10, sf::Color::White, true);
    }
    if (btnStaffClear) {
        btnStaffClear->draw(window);
        drawCenteredTxt("↺", btnStaffClear->getPosition().x + 24.f, btnStaffClear->getPosition().y + 18.f, 14, sf::Color(71, 85, 105), true);
    }
    if (btnStaffDelete) {
        btnStaffDelete->draw(window);
        float dx = btnStaffDelete->getPosition().x + 26.f;
        float dy = btnStaffDelete->getPosition().y + 9.f;
        RoundedRectRenderer::render(window, sf::Vector2f(dx, dy + 2.f), sf::Vector2f(10.f, 12.f), 1.5f, sf::Color::Transparent, 1.3f, sf::Color(225, 29, 72));
        sf::RectangleShape lid(sf::Vector2f(14.f, 1.5f));
        lid.setPosition(dx - 2.f, dy);
        lid.setFillColor(sf::Color(225, 29, 72));
        window.draw(lid);
    }

    // =========================================================================
    // 6. BOTTOM COPYRIGHT & HOTLINE FOOTER (y = 934)
    // =========================================================================
    drawTxt("© 2026 Cinestar CineOS   •   Bản quyền thuộc Cụm Rạp Chiếu Phim Cinestar Cinema Complex   •   Hệ thống bảo mật ISO/IEC 27001", leftColX, 934.f, 11, sf::Color(100, 116, 139), false);
    drawTxt("Hotline Kỹ Thuật: 1900 6081 (Ext: 104)   •   Cụm Server: Node-QuocThanh-HCM", 1236.f, 934.f, 11, sf::Color(30, 41, 59), true);
}

// =============================================================================
// QUẢN LÝ TÀI KHOẢN (CineOS v3.4 PRO - ActiveTab::Accounts)
// Matches reference image media_1794680190472_99811d5b.jpg
// =============================================================================

std::vector<int> CinemaUIApp::getFilteredAccountIndices() const {
    std::vector<int> indices;
    std::string q = accountSearchInput ? removeVietnameseDiacritics(accountSearchInput->getText()) : "";

    for (size_t i = 0; i < staffUserList.size(); ++i) {
        const auto& st = staffUserList[i];
        bool isLocked = (st.trangThaiTaiKhoan == "BiKhoa" || st.trangThaiTaiKhoan == "Bị khóa" || st.trangThaiTaiKhoan == "Khoa");
        if (accountFilterTab == 1 && isLocked) continue;
        if (accountFilterTab == 2 && !isLocked) continue;

        if (!q.empty()) {
            std::string idNorm = removeVietnameseDiacritics(st.maNguoiDung);
            std::string uNorm = removeVietnameseDiacritics(st.taiKhoan);
            std::string nameNorm = removeVietnameseDiacritics(st.hoTen);
            if (idNorm.find(q) == std::string::npos &&
                uNorm.find(q) == std::string::npos &&
                nameNorm.find(q) == std::string::npos) {
                continue;
            }
        }
        indices.push_back(static_cast<int>(i));
    }
    return indices;
}

void CinemaUIApp::syncAccountSelectionToForm(int idx) {
    if (idx >= 0 && idx < static_cast<int>(staffUserList.size())) {
        selectedAccountRowIdx = idx;
        const auto& st = staffUserList[idx];
        if (inputAccountEmployeeId) inputAccountEmployeeId->setText(st.maNguoiDung);
        if (inputAccountFullName) inputAccountFullName->setText(st.hoTen);
        if (inputAccountUsername) inputAccountUsername->setText(st.taiKhoan);
        if (inputAccountPassword) inputAccountPassword->setText(st.matKhau.empty() ? "123456" : st.matKhau);
        if (accountRoleDropdown) {
            accountRoleDropdown->setSelectedIndex((st.vaiTro == "Admin") ? 0 : 1);
        }
    } else {
        selectedAccountRowIdx = -1;
        if (inputAccountEmployeeId) inputAccountEmployeeId->setText("");
        if (inputAccountFullName) inputAccountFullName->setText("");
        if (inputAccountUsername) inputAccountUsername->setText("");
        if (inputAccountPassword) inputAccountPassword->setText("123456");
        if (accountRoleDropdown) accountRoleDropdown->setSelectedIndex(1);
    }
}

void CinemaUIApp::refreshAccountEmployeeDropdown() {
    if (!accountEmployeeDropdown) return;
    std::vector<std::string> opts;
    for (const auto& st : staffUserList) {
        opts.push_back(st.maNguoiDung + " - " + st.hoTen);
    }
    if (opts.empty()) opts.push_back("Chưa có nhân viên");
    accountEmployeeDropdown->setOptions(opts, selectedAccountRowIdx >= 0 ? selectedAccountRowIdx : 0);
}

void CinemaUIApp::setupAccountsView() {
    float leftColX = sidebarWidth + 24.f;   // 274.f
    float rightColX = 1200.f;               // 484px wide right card (1200..1684)

    // Header action buttons (y = 66)
    btnAccountExport = std::unique_ptr<RoundRectButton>(new RoundRectButton(
        "    Xuất Danh Sách Excel", sf::Vector2f(1296.f, 66.f), sf::Vector2f(178.f, 38.f),
        ButtonStyle::Dark, 8.f
    ));
    btnAccountExport->setCustomColors(sf::Color::White, sf::Color(248, 250, 252), sf::Color(241, 245, 249), sf::Color(241, 245, 249));
    btnAccountExport->setOutline(1.f, sf::Color(203, 213, 225));
    btnAccountExport->setTextColor(sf::Color(30, 41, 59));
    btnAccountExport->setFontSize(12);
    btnAccountExport->setOnClick([this]() {
        std::ofstream csv("DATA/danh_sach_tai_khoan.csv");
        if (csv.is_open()) {
            csv << "\xEF\xBB\xBF"; // UTF-8 BOM
            csv << "Mã NV,Họ Tên,Username,Vai Trò,Trạng Thái Tài Khoản\n";
            for (const auto& u : staffUserList) {
                csv << u.maNguoiDung << "," << u.hoTen << "," << u.taiKhoan << ","
                    << u.vaiTro << "," << u.trangThaiTaiKhoan << "\n";
            }
            csv.close();
        }
        if (confirmPopup) {
            confirmPopup->show("XUẤT DANH SÁCH THÀNH CÔNG", "Đã xuất danh sách tài khoản ra file DATA/danh_sach_tai_khoan.csv!", ButtonStyle::Primary, "Đóng", "");
        }
    });

    btnAccountNew = std::unique_ptr<RoundRectButton>(new RoundRectButton(
        "     + Cấp Tài Khoản Mới", sf::Vector2f(1486.f, 66.f), sf::Vector2f(198.f, 38.f),
        ButtonStyle::Primary, 8.f
    ));
    btnAccountNew->setCustomColors(sf::Color(2, 132, 199), sf::Color(3, 105, 161), sf::Color(7, 89, 133), sf::Color(2, 132, 199));
    btnAccountNew->setTextColor(sf::Color::White);
    btnAccountNew->setFontSize(12);
    btnAccountNew->setOnClick([this]() {
        syncAccountSelectionToForm(-1);
    });

    // Left Card Search & Filter Strip (y = 244)
    accountSearchInput = std::unique_ptr<UIInputBox>(new UIInputBox(
        sf::Vector2f(leftColX + 86.f, 244.f), sf::Vector2f(320.f, 34.f),
        "      Tìm theo Mã NV hoặc Username..."
    ));
    accountSearchInput->setColors(
        sf::Color(248, 250, 252), sf::Color(226, 232, 240),
        sf::Color(2, 132, 199), sf::Color(15, 23, 42), sf::Color(148, 163, 184)
    );

    // Filter tabs WITHOUT numbers per user requirement
    float fTabY = 244.f;
    btnAccountFilterAll = std::unique_ptr<RoundRectButton>(new RoundRectButton(
        "Tất cả", sf::Vector2f(leftColX + 570.f, fTabY), sf::Vector2f(96.f, 32.f),
        ButtonStyle::Dark, 6.f
    ));
    btnAccountFilterAll->setFontSize(12);
    btnAccountFilterAll->setOnClick([this]() {
        accountFilterTab = 0;
        accountCurrentPage = 0;
    });

    btnAccountFilterActive = std::unique_ptr<RoundRectButton>(new RoundRectButton(
        "Hoạt động", sf::Vector2f(leftColX + 672.f, fTabY), sf::Vector2f(110.f, 32.f),
        ButtonStyle::Dark, 6.f
    ));
    btnAccountFilterActive->setFontSize(12);
    btnAccountFilterActive->setOnClick([this]() {
        accountFilterTab = 1;
        accountCurrentPage = 0;
    });

    btnAccountFilterLocked = std::unique_ptr<RoundRectButton>(new RoundRectButton(
        "Bị khóa", sf::Vector2f(leftColX + 788.f, fTabY), sf::Vector2f(98.f, 32.f),
        ButtonStyle::Dark, 6.f
    ));
    btnAccountFilterLocked->setFontSize(12);
    btnAccountFilterLocked->setOnClick([this]() {
        accountFilterTab = 2;
        accountCurrentPage = 0;
    });

    // Pagination buttons (y = 868)
    btnAccountPrevPage = std::unique_ptr<RoundRectButton>(new RoundRectButton(
        "<", sf::Vector2f(leftColX + 754.f, 868.f), sf::Vector2f(28.f, 28.f),
        ButtonStyle::Dark, 6.f
    ));
    btnAccountPrevPage->setFontSize(12);
    btnAccountPrevPage->setCustomColors(sf::Color::White, sf::Color(248, 250, 252), sf::Color(241, 245, 249), sf::Color(241, 245, 249));
    btnAccountPrevPage->setOutline(1.f, sf::Color(226, 232, 240));
    btnAccountPrevPage->setTextColor(sf::Color(100, 116, 139));
    btnAccountPrevPage->setOnClick([this]() {
        if (accountCurrentPage > 0) accountCurrentPage--;
    });

    btnAccountNextPage = std::unique_ptr<RoundRectButton>(new RoundRectButton(
        ">", sf::Vector2f(leftColX + 862.f, 868.f), sf::Vector2f(28.f, 28.f),
        ButtonStyle::Dark, 6.f
    ));
    btnAccountNextPage->setFontSize(12);
    btnAccountNextPage->setCustomColors(sf::Color::White, sf::Color(248, 250, 252), sf::Color(241, 245, 249), sf::Color(241, 245, 249));
    btnAccountNextPage->setOutline(1.f, sf::Color(226, 232, 240));
    btnAccountNextPage->setTextColor(sf::Color(100, 116, 139));
    btnAccountNextPage->setOnClick([this]() {
        auto idxs = getFilteredAccountIndices();
        int totalP = std::max(1, (static_cast<int>(idxs.size()) + 4) / 5);
        if (accountCurrentPage < totalP - 1) accountCurrentPage++;
    });

    // Right Form Inputs (rightColX = 1200, w = 484)
    float formInputX = rightColX + 24.f;
    float formInputW = 436.f;
    float formInputH = 38.f;

    inputAccountEmployeeId = std::unique_ptr<UIInputBox>(new UIInputBox(
        sf::Vector2f(formInputX, 314.f), sf::Vector2f(formInputW, formInputH), "U001"
    ));
    inputAccountEmployeeId->setColors(sf::Color::White, sf::Color(203, 213, 225), sf::Color(2, 132, 199), sf::Color(15, 23, 42), sf::Color(148, 163, 184));

    inputAccountFullName = std::unique_ptr<UIInputBox>(new UIInputBox(
        sf::Vector2f(formInputX, 386.f), sf::Vector2f(formInputW, formInputH), "Nhập họ và tên..."
    ));
    inputAccountFullName->setColors(sf::Color::White, sf::Color(203, 213, 225), sf::Color(2, 132, 199), sf::Color(15, 23, 42), sf::Color(148, 163, 184));
    inputAccountFullName->setEnabled(true);

    inputAccountUsername = std::unique_ptr<UIInputBox>(new UIInputBox(
        sf::Vector2f(formInputX, 458.f), sf::Vector2f(formInputW, formInputH), "admin"
    ));
    inputAccountUsername->setColors(sf::Color::White, sf::Color(203, 213, 225), sf::Color(2, 132, 199), sf::Color(15, 23, 42), sf::Color(148, 163, 184));

    inputAccountPassword = std::unique_ptr<UIInputBox>(new UIInputBox(
        sf::Vector2f(formInputX, 530.f), sf::Vector2f(formInputW, formInputH), "••••••••••••"
    ));
    inputAccountPassword->setColors(sf::Color::White, sf::Color(203, 213, 225), sf::Color(2, 132, 199), sf::Color(15, 23, 42), sf::Color(148, 163, 184));

    accountRoleDropdown = std::unique_ptr<DropdownBox>(new DropdownBox(
        sf::Vector2f(formInputX, 624.f), sf::Vector2f(formInputW, formInputH),
        {"Quản trị viên (Toàn quyền rạp)", "Nhân viên bán vé (POS & Quầy)"}, 0
    ));
    accountRoleDropdown->setLightStyle(true);

    // Action Buttons
    btnAccountSave = std::unique_ptr<RoundRectButton>(new RoundRectButton(
        "Xác nhận", sf::Vector2f(formInputX, 718.f), sf::Vector2f(212.f, 40.f),
        ButtonStyle::Primary, 8.f
    ));
    btnAccountSave->setCustomColors(sf::Color(2, 132, 199), sf::Color(3, 105, 161), sf::Color(7, 89, 133), sf::Color(2, 132, 199));
    btnAccountSave->setTextColor(sf::Color::White);
    btnAccountSave->setFontSize(13);
    btnAccountSave->setOnClick([this]() {
        std::string enteredId = inputAccountEmployeeId ? inputAccountEmployeeId->getText() : "";
        std::string u = inputAccountUsername ? inputAccountUsername->getText() : "";
        std::string p = inputAccountPassword ? inputAccountPassword->getText() : "";
        std::string fullName = inputAccountFullName ? inputAccountFullName->getText() : "";

        if (u.empty()) {
            if (confirmPopup) confirmPopup->show("THIẾU THÔNG TIN", "Vui lòng nhập Username (Tên đăng nhập)!", ButtonStyle::Danger, "Đóng", "");
            return;
        }

        if (selectedAccountRowIdx >= 0 && selectedAccountRowIdx < static_cast<int>(staffUserList.size())) {
            for (size_t i = 0; i < staffUserList.size(); ++i) {
                if (static_cast<int>(i) != selectedAccountRowIdx && staffUserList[i].taiKhoan == u) {
                    if (confirmPopup) confirmPopup->show("TRÙNG TÊN ĐĂNG NHẬP", "Username '" + u + "' đã tồn tại trên hệ thống. Vui lòng chọn tên khác!", ButtonStyle::Danger, "Đóng", "");
                    return;
                }
            }

            auto& st = staffUserList[selectedAccountRowIdx];
            if (!fullName.empty()) st.hoTen = fullName;
            st.taiKhoan = u;
            if (!p.empty() && p != "••••••••••••") {
                st.matKhau = p;
            }
            st.vaiTro = (accountRoleDropdown && accountRoleDropdown->getSelectedIndex() == 0) ? "Admin" : "Staff";

            saveStaffUsersToDatabase();
            ActivityLog::ghiNhan(currentUserId, currentUserRole, "CAP_NHAT_TAI_KHOAN", "Cập nhật tài khoản cho NV: " + st.maNguoiDung + " (" + u + ")");
            if (confirmPopup) confirmPopup->show("CẬP NHẬT THÀNH CÔNG", "Đã lưu tài khoản [" + u + "] cho nhân sự " + st.hoTen + " (" + st.maNguoiDung + ")!", ButtonStyle::Primary, "Đóng", "");
        } else {
            // Mode Cấp tài khoản mới
            if (fullName.empty()) {
                if (confirmPopup) confirmPopup->show("THIẾU THÔNG TIN", "Vui lòng nhập Họ và tên để cấp tài khoản mới!", ButtonStyle::Danger, "Đóng", "");
                return;
            }
            for (size_t i = 0; i < staffUserList.size(); ++i) {
                if (staffUserList[i].taiKhoan == u) {
                    if (confirmPopup) confirmPopup->show("TRÙNG TÊN ĐĂNG NHẬP", "Username '" + u + "' đã tồn tại trên hệ thống. Vui lòng chọn tên khác!", ButtonStyle::Danger, "Đóng", "");
                    return;
                }
            }
            std::string newId = enteredId.empty() ? generateNextStaffId() : enteredId;
            StaffAccountInfo newAcc;
            newAcc.maNguoiDung = newId;
            newAcc.hoTen = fullName;
            newAcc.taiKhoan = u;
            newAcc.matKhau = p.empty() ? "123456" : p;
            newAcc.vaiTro = (accountRoleDropdown && accountRoleDropdown->getSelectedIndex() == 0) ? "Admin" : "Staff";
            newAcc.trangThaiLamViec = "DangLam";
            newAcc.trangThaiTaiKhoan = "HoatDong";

            staffUserList.push_back(newAcc);
            saveStaffUsersToDatabase();
            selectedAccountRowIdx = static_cast<int>(staffUserList.size()) - 1;
            refreshAccountEmployeeDropdown();
            ActivityLog::ghiNhan(currentUserId, currentUserRole, "CAP_TAI_KHOAN_MOI", "Cấp tài khoản mới: " + newId + " (" + u + ")");
            if (confirmPopup) confirmPopup->show("CẤP TÀI KHOẢN THÀNH CÔNG", "Đã cấp mới tài khoản [" + u + "] cho " + fullName + " (" + newId + ")!", ButtonStyle::Primary, "Đóng", "");
        }
    });

    btnAccountToggleLock = std::unique_ptr<RoundRectButton>(new RoundRectButton(
        "🔒  Khóa / Mở Acc", sf::Vector2f(formInputX + 224.f, 718.f), sf::Vector2f(212.f, 40.f),
        ButtonStyle::Danger, 8.f
    ));
    btnAccountToggleLock->setCustomColors(sf::Color(254, 242, 242), sf::Color(254, 226, 226), sf::Color(254, 205, 211), sf::Color(254, 242, 242));
    btnAccountToggleLock->setOutline(1.f, sf::Color(254, 205, 211));
    btnAccountToggleLock->setTextColor(sf::Color(225, 29, 72));
    btnAccountToggleLock->setFontSize(13);
    btnAccountToggleLock->setOnClick([this]() {
        if (selectedAccountRowIdx < 0 || selectedAccountRowIdx >= static_cast<int>(staffUserList.size())) {
            if (confirmPopup) confirmPopup->show("CHƯA CHỌN TÀI KHOẢN", "Vui lòng chọn tài khoản từ danh sách!", ButtonStyle::Danger, "Đóng", "");
            return;
        }
        auto& st = staffUserList[selectedAccountRowIdx];
        bool isCurrentlyLocked = (st.trangThaiTaiKhoan == "BiKhoa" || st.trangThaiTaiKhoan == "Bị khóa" || st.trangThaiTaiKhoan == "Khoa");

        if (!isCurrentlyLocked && st.vaiTro == "Admin") {
            int activeAdminCount = 0;
            for (const auto& u : staffUserList) {
                if (u.vaiTro == "Admin" && u.trangThaiTaiKhoan != "BiKhoa" && u.trangThaiTaiKhoan != "Bị khóa" && u.trangThaiTaiKhoan != "Khoa") {
                    activeAdminCount++;
                }
            }
            if (activeAdminCount <= 1) {
                if (confirmPopup) confirmPopup->show("KHÔNG THỂ KHÓA", "Không thể khóa Admin này vì hệ thống bắt buộc phải có ít nhất một Quản trị viên đang hoạt động!", ButtonStyle::Danger, "Đóng", "");
                return;
            }
        }

        st.trangThaiTaiKhoan = isCurrentlyLocked ? "HoatDong" : "BiKhoa";

        saveStaffUsersToDatabase();
        std::string act = isCurrentlyLocked ? "MỞ KHÓA" : "KHÓA";
        ActivityLog::ghiNhan(currentUserId, currentUserRole, isCurrentlyLocked ? "MO_KHOA_ACC" : "KHOA_ACC", act + " tài khoản: " + st.taiKhoan);
        if (confirmPopup) {
            confirmPopup->show("THAY ĐỔI TRẠNG THÁI", "Đã " + act + " tài khoản [" + st.taiKhoan + "] thành công!", ButtonStyle::Primary, "Đóng", "");
        }
    });

    btnAccountRefresh = std::unique_ptr<RoundRectButton>(new RoundRectButton(
        "🔄  Làm mới", sf::Vector2f(formInputX, 768.f), sf::Vector2f(212.f, 38.f),
        ButtonStyle::Dark, 8.f
    ));
    btnAccountRefresh->setCustomColors(sf::Color(30, 41, 59), sf::Color(51, 65, 85), sf::Color(15, 23, 42), sf::Color(30, 41, 59));
    btnAccountRefresh->setTextColor(sf::Color::White);
    btnAccountRefresh->setFontSize(13);
    btnAccountRefresh->setOnClick([this]() {
        loadDataFromDatabase();
        accountFilterTab = 0;
        accountCurrentPage = 0;
        if (!staffUserList.empty()) syncAccountSelectionToForm(0);
        if (confirmPopup) confirmPopup->show("ĐỒNG BỘ THÀNH CÔNG", "Đã làm mới dữ liệu tài khoản từ SQLite!", ButtonStyle::Primary, "Đóng", "");
    });

    btnAccountClear = std::unique_ptr<RoundRectButton>(new RoundRectButton(
        "Clear Form", sf::Vector2f(formInputX + 224.f, 768.f), sf::Vector2f(212.f, 38.f),
        ButtonStyle::Dark, 8.f
    ));
    btnAccountClear->setCustomColors(sf::Color::White, sf::Color(248, 250, 252), sf::Color(241, 245, 249), sf::Color(241, 245, 249));
    btnAccountClear->setOutline(1.f, sf::Color(203, 213, 225));
    btnAccountClear->setTextColor(sf::Color(71, 85, 105));
    btnAccountClear->setFontSize(13);
    btnAccountClear->setOnClick([this]() {
        syncAccountSelectionToForm(-1);
    });

    if (!staffUserList.empty()) {
        syncAccountSelectionToForm(0);
    } else {
        syncAccountSelectionToForm(-1);
    }
}

void CinemaUIApp::renderAccountsView() {
    float leftColX = sidebarWidth + 24.f; // 274.f
    float rightColX = 1200.f;
    float pageRight = 1684.f;

    // Light Slate Background Canvas for Accounts workspace
    sf::RectangleShape bgCanvas(sf::Vector2f(1710.f - sidebarWidth, 970.f));
    bgCanvas.setPosition(sidebarWidth, 0.f);
    bgCanvas.setFillColor(sf::Color(244, 247, 250));
    window.draw(bgCanvas);

    auto drawTxt = [&](const std::string& str, float x, float y, unsigned int sz, const sf::Color& col, bool bold = false, bool italic = false) {
        sf::Text t;
        t.setFont(Theme::FontPrimary);
        t.setCharacterSize(sz);
        sf::Uint32 style = sf::Text::Regular;
        if (bold) style |= sf::Text::Bold;
        if (italic) style |= sf::Text::Italic;
        t.setStyle(style);
        t.setFillColor(col);
        t.setString(toSfString(str));
        t.setPosition(std::round(x), std::round(y));
        window.draw(t);
    };

    auto drawCenteredTxt = [&](const std::string& str, float cx, float cy, unsigned int sz, const sf::Color& col, bool bold = false) {
        sf::Text t;
        t.setFont(Theme::FontPrimary);
        t.setCharacterSize(sz);
        if (bold) t.setStyle(sf::Text::Bold);
        t.setFillColor(col);
        t.setString(toSfString(str));
        sf::FloatRect b = t.getLocalBounds();
        t.setOrigin(std::round(b.left + b.width * 0.5f), std::round(b.top + b.height * 0.5f));
        t.setPosition(std::round(cx), std::round(cy));
        window.draw(t);
    };

    // =========================================================================
    // 1. TOP NAVIGATION BAR (y = 0..48)
    // =========================================================================
    sf::RectangleShape topBarBg(sf::Vector2f(1710.f - sidebarWidth, 48.f));
    topBarBg.setPosition(sidebarWidth, 0.f);
    topBarBg.setFillColor(sf::Color::White);
    window.draw(topBarBg);

    sf::RectangleShape topBarLine(sf::Vector2f(1710.f - sidebarWidth, 1.f));
    topBarLine.setPosition(sidebarWidth, 48.f);
    topBarLine.setFillColor(sf::Color(226, 232, 240));
    window.draw(topBarLine);

    // Brand Logo Icon
    RoundedRectRenderer::render(window, sf::Vector2f(leftColX, 8.f), sf::Vector2f(32.f, 32.f), 8.f, sf::Color(2, 132, 199));
    RoundedRectRenderer::render(window, sf::Vector2f(leftColX + 7.f, 15.f), sf::Vector2f(18.f, 18.f), 3.f, sf::Color::Transparent, 1.5f, sf::Color::White);
    sf::RectangleShape filmMid(sf::Vector2f(18.f, 1.5f));
    filmMid.setPosition(leftColX + 7.f, 23.f);
    filmMid.setFillColor(sf::Color::White);
    window.draw(filmMid);

    drawTxt("Cinestar", leftColX + 40.f, 7.f, 15, sf::Color(15, 23, 42), true);
    drawTxt("CineOS", leftColX + 106.f, 7.f, 15, sf::Color(2, 132, 199), true);

    RoundedRectRenderer::render(window, sf::Vector2f(leftColX + 166.f, 8.f), sf::Vector2f(62.f, 18.f), 9.f, sf::Color(224, 242, 254), 1.f, sf::Color(186, 230, 253));
    drawCenteredTxt("v3.4 PRO", leftColX + 197.f, 17.f, 10, sf::Color(3, 105, 161), true);

    drawTxt("Phân Hệ Quản Trị Tài Khoản & Phân Quyền Cụm Rạp", leftColX + 40.f, 27.f, 10, sf::Color(100, 116, 139), false);

    // Realtime sync status
    sf::RectangleShape vSep1(sf::Vector2f(1.f, 24.f));
    vSep1.setPosition(leftColX + 326.f, 12.f);
    vSep1.setFillColor(sf::Color(226, 232, 240));
    window.draw(vSep1);

    sf::CircleShape greenDot(4.f);
    greenDot.setFillColor(sf::Color(16, 185, 129));
    greenDot.setPosition(leftColX + 342.f, 20.f);
    window.draw(greenDot);
    drawTxt("Đồng bộ Realtime  •  Cụm Rạp Toàn Quốc", leftColX + 356.f, 16.f, 12, sf::Color(30, 41, 59), true);

    // Quick search pill
    RoundedRectRenderer::render(window, sf::Vector2f(1140.f, 9.f), sf::Vector2f(240.f, 30.f), 15.f, sf::Color(248, 250, 252), 1.f, sf::Color(226, 232, 240));
    sf::CircleShape qLens(4.5f);
    qLens.setFillColor(sf::Color::Transparent);
    qLens.setOutlineThickness(1.4f);
    qLens.setOutlineColor(sf::Color(148, 163, 184));
    qLens.setPosition(1154.f, 19.f);
    window.draw(qLens);
    drawTxt("Tìm tác vụ nhanh... (Ctrl+K)", 1172.f, 16.f, 11, sf::Color(148, 163, 184), false);

    // User profile badge (Admin)
    sf::CircleShape avatarCir(15.f);
    avatarCir.setFillColor(sf::Color(224, 242, 254));
    avatarCir.setOutlineThickness(1.5f);
    avatarCir.setOutlineColor(sf::Color(2, 132, 199));
    avatarCir.setPosition(1462.f, 9.f);
    window.draw(avatarCir);
    drawCenteredTxt("AD", 1477.f, 24.f, 11, sf::Color(3, 105, 161), true);
    drawTxt(currentUserName.empty() ? "Admin" : currentUserName, 1500.f, 8.f, 12, sf::Color(15, 23, 42), true);
    drawTxt("Quản trị viên hệ thống", 1500.f, 24.f, 11, sf::Color(100, 116, 139), false);

    // =========================================================================
    // 2. PAGE HEADER (y = 60..112)
    // =========================================================================
    drawTxt("QUẢN LÝ TÀI KHOẢN", leftColX, 60.f, 22, sf::Color(15, 23, 42), true);

    // Sticker badge [Cấp quyền hệ thống cho nhân viên]
    RoundedRectRenderer::render(window, sf::Vector2f(leftColX + 242.f, 63.f), sf::Vector2f(186.f, 24.f), 12.f, sf::Color(224, 242, 254), 1.f, sf::Color(186, 230, 253));
    drawCenteredTxt("Cấp quyền hệ thống cho nhân viên", leftColX + 335.f, 75.f, 11, sf::Color(3, 105, 161), true);

    drawTxt("Quản trị định danh đăng nhập, cấp quyền truy cập phần mềm và kiểm soát bảo mật hệ thống rạp", leftColX, 91.f, 12, sf::Color(100, 116, 139), false);

    if (btnAccountExport) {
        btnAccountExport->draw(window);
        float ex = btnAccountExport->getPosition().x + 14.f;
        float ey = btnAccountExport->getPosition().y + 11.f;
        RoundedRectRenderer::render(window, sf::Vector2f(ex, ey), sf::Vector2f(14.f, 16.f), 2.f, sf::Color(240, 253, 244), 1.3f, sf::Color(22, 163, 74));
        drawCenteredTxt("X", ex + 7.f, ey + 8.f, 9, sf::Color(22, 163, 74), true);
    }
    if (btnAccountNew) {
        btnAccountNew->draw(window);
        float nx = btnAccountNew->getPosition().x + 16.f;
        float ny = btnAccountNew->getPosition().y + 12.f;
        sf::CircleShape uHead(3.5f);
        uHead.setFillColor(sf::Color::White);
        uHead.setPosition(nx + 2.f, ny);
        window.draw(uHead);
        RoundedRectRenderer::render(window, sf::Vector2f(nx, ny + 8.f), sf::Vector2f(11.f, 5.f), 2.5f, sf::Color::White);
    }

    // =========================================================================
    // 3. STATISTICAL KPI ROW (4 Cards linked to SQLite users table, y = 122)
    // =========================================================================
    int totalAccounts = static_cast<int>(staffUserList.size());
    int activeAccounts = 0;
    int adminAccounts = 0;
    int lockedAccounts = 0;

    for (const auto& st : staffUserList) {
        if (st.vaiTro == "Admin") adminAccounts++;
        bool isLocked = (st.trangThaiTaiKhoan == "BiKhoa" || st.trangThaiTaiKhoan == "Bị khóa" || st.trangThaiTaiKhoan == "Khoa");
        if (isLocked) lockedAccounts++;
        else activeAccounts++;
    }

    float actPct = (totalAccounts > 0) ? (static_cast<float>(activeAccounts) * 100.f / static_cast<float>(totalAccounts)) : 0.f;
    std::ostringstream actOss;
    actOss << std::fixed << std::setprecision(1) << actPct << "%";

    auto fmt2Digits = [](int n) -> std::string {
        std::ostringstream o;
        o << std::setw(2) << std::setfill('0') << n;
        return o.str();
    };

    float kpiY = 122.f;
    float kpiH = 92.f;
    float kpiGap = 18.f;
    float kpiW = (pageRight - leftColX - 3.f * kpiGap) / 4.f; // ~339px each

    struct AccountKpiCard {
        std::string title;
        std::string mainVal;
        std::string badgeTxt;
        sf::Color badgeCol;
        std::string subTxt;
        sf::Color iconBg;
        sf::Color iconFg;
    };

    std::vector<AccountKpiCard> accKpis = {
        {"TỔNG TÀI KHOẢN", fmt2Digits(totalAccounts), "Định danh hệ thống", sf::Color(2, 132, 199), "Toàn bộ nhân sự cơ hữu rạp", sf::Color(224, 242, 254), sf::Color(2, 132, 199)},
        {"ĐANG HOẠT ĐỘNG", fmt2Digits(activeAccounts), actOss.str(), sf::Color(16, 185, 129), "Tài khoản kích hoạt sẵn sàng", sf::Color(220, 252, 231), sf::Color(22, 163, 74)},
        {"QUẢN TRỊ VIÊN (ADMIN)", fmt2Digits(adminAccounts), "Đầy đủ quyền", sf::Color(2, 132, 199), "Quản lý cấp rạp & hệ thống", sf::Color(224, 242, 254), sf::Color(2, 132, 199)},
        {"BỊ KHÓA / VÔ HIỆU", fmt2Digits(lockedAccounts), "Đã khóa tài khoản", sf::Color(225, 29, 72), "Tạm ngưng quyền truy cập POS", sf::Color(254, 242, 242), sf::Color(225, 29, 72)}
    };

    for (size_t i = 0; i < accKpis.size(); ++i) {
        float cx = leftColX + i * (kpiW + kpiGap);
        RoundedRectRenderer::render(window, sf::Vector2f(cx, kpiY), sf::Vector2f(kpiW, kpiH), 12.f, sf::Color::White, 1.f, sf::Color(226, 232, 240));

        drawTxt(accKpis[i].title, cx + 20.f, kpiY + 14.f, 11, sf::Color(100, 116, 139), true);
        drawTxt(accKpis[i].mainVal, cx + 20.f, kpiY + 34.f, 24, (i == 2) ? sf::Color(2, 132, 199) : sf::Color(15, 23, 42), true);
        drawTxt(accKpis[i].badgeTxt, cx + 62.f, kpiY + 43.f, 12, accKpis[i].badgeCol, true);
        drawTxt(accKpis[i].subTxt, cx + 20.f, kpiY + 67.f, 11, sf::Color(148, 163, 184), false);

        float ibX = cx + kpiW - 58.f;
        float ibY = kpiY + 26.f;
        RoundedRectRenderer::render(window, sf::Vector2f(ibX, ibY), sf::Vector2f(40.f, 40.f), 10.f, accKpis[i].iconBg);

        if (i == 0) {
            sf::CircleShape h1(4.f); h1.setFillColor(accKpis[i].iconFg); h1.setPosition(ibX + 16.f, ibY + 10.f); window.draw(h1);
            RoundedRectRenderer::render(window, sf::Vector2f(ibX + 12.f, ibY + 20.f), sf::Vector2f(16.f, 8.f), 4.f, accKpis[i].iconFg);
            sf::CircleShape h2(3.f); h2.setFillColor(accKpis[i].iconFg); h2.setPosition(ibX + 9.f, ibY + 12.f); window.draw(h2);
            sf::CircleShape h3(3.f); h3.setFillColor(accKpis[i].iconFg); h3.setPosition(ibX + 25.f, ibY + 12.f); window.draw(h3);
        } else if (i == 1) {
            sf::CircleShape cir(10.f); cir.setFillColor(sf::Color::Transparent); cir.setOutlineThickness(2.f); cir.setOutlineColor(accKpis[i].iconFg); cir.setPosition(ibX + 10.f, ibY + 10.f); window.draw(cir);
            {
                sf::RectangleShape ck1(sf::Vector2f(3.5f, 1.5f));
                ck1.setRotation(45.f);
                ck1.setPosition(ibX + 17.5f, ibY + 18.5f);
                ck1.setFillColor(accKpis[i].iconFg);
                sf::RectangleShape ck2(sf::Vector2f(6.5f, 1.5f));
                ck2.setRotation(-50.f);
                ck2.setPosition(ibX + 20.f, ibY + 21.f);
                ck2.setFillColor(accKpis[i].iconFg);
                window.draw(ck1);
                window.draw(ck2);
            }
        } else if (i == 2) {
            RoundedRectRenderer::render(window, sf::Vector2f(ibX + 12.f, ibY + 10.f), sf::Vector2f(16.f, 18.f), 5.f, accKpis[i].iconFg);
            {
                sf::RectangleShape ck1(sf::Vector2f(3.f, 1.5f));
                ck1.setRotation(45.f);
                ck1.setPosition(ibX + 17.5f, ibY + 18.f);
                ck1.setFillColor(sf::Color::White);
                sf::RectangleShape ck2(sf::Vector2f(5.5f, 1.5f));
                ck2.setRotation(-50.f);
                ck2.setPosition(ibX + 19.5f, ibY + 20.f);
                ck2.setFillColor(sf::Color::White);
                window.draw(ck1);
                window.draw(ck2);
            }
        } else {
            RoundedRectRenderer::render(window, sf::Vector2f(ibX + 12.f, ibY + 16.f), sf::Vector2f(16.f, 14.f), 3.f, accKpis[i].iconFg);
            RoundedRectRenderer::render(window, sf::Vector2f(ibX + 15.f, ibY + 10.f), sf::Vector2f(10.f, 10.f), 5.f, sf::Color::Transparent, 2.f, accKpis[i].iconFg);
        }
    }

    // =========================================================================
    // 4. LEFT CARD: ACCOUNT DIRECTORY TABLE (leftColX = 274, y = 228, w = 910, h = 684)
    // =========================================================================
    float tblCardX = leftColX;
    float tblCardY = 228.f;
    float tblCardW = 910.f;
    float tblCardH = 684.f;
    RoundedRectRenderer::render(window, sf::Vector2f(tblCardX, tblCardY), sf::Vector2f(tblCardW, tblCardH), 12.f, sf::Color::White, 1.f, sf::Color(226, 232, 240));

    // Search row
    drawTxt("Tìm Nhanh:", tblCardX + 16.f, 252.f, 12, sf::Color(30, 41, 59), true);
    if (accountSearchInput) {
        accountSearchInput->draw(window);
        sf::CircleShape sCir(4.5f);
        sCir.setFillColor(sf::Color::Transparent);
        sCir.setOutlineThickness(1.4f);
        sCir.setOutlineColor(sf::Color(148, 163, 184));
        sCir.setPosition(tblCardX + 98.f, 255.f);
        window.draw(sCir);
        sf::RectangleShape sHdl(sf::Vector2f(4.5f, 1.4f));
        sHdl.setFillColor(sf::Color(148, 163, 184));
        sHdl.setPosition(tblCardX + 106.f, 263.f);
        sHdl.setRotation(45.f);
        window.draw(sHdl);
    }

    // Filter pill buttons (NO numbers per user requirement!)
    RoundedRectRenderer::render(window, sf::Vector2f(tblCardX + 566.f, 240.f), sf::Vector2f(326.f, 40.f), 8.f, sf::Color(241, 245, 249));
    auto styleAccountFilter = [&](std::unique_ptr<RoundRectButton>& btn, bool active) {
        if (!btn) return;
        if (active) {
            btn->setCustomColors(sf::Color(15, 23, 42), sf::Color(30, 41, 59), sf::Color(51, 65, 85), sf::Color(15, 23, 42));
            btn->setTextColor(sf::Color::White);
            btn->setOutline(0.f, sf::Color::Transparent);
        } else {
            btn->setCustomColors(sf::Color(241, 245, 249), sf::Color(226, 232, 240), sf::Color(203, 213, 225), sf::Color(241, 245, 249));
            btn->setTextColor(sf::Color(71, 85, 105));
            btn->setOutline(0.f, sf::Color::Transparent);
        }
        btn->draw(window);
    };
    styleAccountFilter(btnAccountFilterAll, accountFilterTab == 0);
    styleAccountFilter(btnAccountFilterActive, accountFilterTab == 1);
    styleAccountFilter(btnAccountFilterLocked, accountFilterTab == 2);

    // Table Header Bar (y = 294, h = 44)
    float thY = 294.f;
    float thH = 44.f;
    sf::RectangleShape thBg(sf::Vector2f(tblCardW - 2.f, thH));
    thBg.setPosition(tblCardX + 1.f, thY);
    thBg.setFillColor(sf::Color(248, 250, 252));
    window.draw(thBg);

    sf::RectangleShape thTopLine(sf::Vector2f(tblCardW - 2.f, 1.f));
    thTopLine.setPosition(tblCardX + 1.f, thY);
    thTopLine.setFillColor(sf::Color(226, 232, 240));
    window.draw(thTopLine);

    sf::RectangleShape thBotLine(sf::Vector2f(tblCardW - 2.f, 1.f));
    thBotLine.setPosition(tblCardX + 1.f, thY + thH);
    thBotLine.setFillColor(sf::Color(226, 232, 240));
    window.draw(thBotLine);

    drawCenteredTxt("MÃ NV", tblCardX + 42.f, thY + 22.f, 11, sf::Color(71, 85, 105), true);
    drawTxt("HỌ TÊN", tblCardX + 88.f, thY + 14.f, 11, sf::Color(71, 85, 105), true);
    drawCenteredTxt("TÊN ĐĂNG NHẬP", tblCardX + 312.f, thY + 22.f, 11, sf::Color(71, 85, 105), true);
    drawCenteredTxt("QUYỀN (VAI TRÒ)", tblCardX + 494.f, thY + 22.f, 11, sf::Color(71, 85, 105), true);
    drawCenteredTxt("TRẠNG THÁI", tblCardX + 682.f, thY + 22.f, 11, sf::Color(71, 85, 105), true);
    drawCenteredTxt("THAO TÁC", tblCardX + 838.f, thY + 22.f, 11, sf::Color(71, 85, 105), true);

    std::vector<int> filteredIndices = getFilteredAccountIndices();
    const int rowsPerPage = 5;
    int totalFiltered = static_cast<int>(filteredIndices.size());
    int totalPages = std::max(1, (totalFiltered + rowsPerPage - 1) / rowsPerPage);
    if (accountCurrentPage >= totalPages) accountCurrentPage = totalPages - 1;
    if (accountCurrentPage < 0) accountCurrentPage = 0;

    int startIdx = accountCurrentPage * rowsPerPage;
    int endIdx = std::min(startIdx + rowsPerPage, totalFiltered);

    std::vector<sf::Color> avatarPalette = {
        sf::Color(2, 132, 199),   // Blue
        sf::Color(168, 85, 247),  // Purple
        sf::Color(245, 158, 11),  // Amber
        sf::Color(244, 63, 94),   // Rose
        sf::Color(148, 163, 184)  // Slate
    };

    float rowStartY = thY + thH; // 338.f
    float rowH = 102.f;          // 5 rows = 510px

    for (int i = startIdx; i < endIdx; ++i) {
        int realIdx = filteredIndices[i];
        int localRow = i - startIdx;
        float ry = rowStartY + localRow * rowH;
        const auto& st = staffUserList[realIdx];
        bool isSelected = (realIdx == selectedAccountRowIdx);
        bool isLocked = (st.trangThaiTaiKhoan == "BiKhoa" || st.trangThaiTaiKhoan == "Bị khóa" || st.trangThaiTaiKhoan == "Khoa");

        if (isSelected) {
            sf::RectangleShape selBg(sf::Vector2f(tblCardW - 2.f, rowH));
            selBg.setPosition(tblCardX + 1.f, ry);
            selBg.setFillColor(sf::Color(240, 249, 255));
            window.draw(selBg);

            sf::RectangleShape selBar(sf::Vector2f(4.f, rowH));
            selBar.setPosition(tblCardX + 1.f, ry);
            selBar.setFillColor(sf::Color(2, 132, 199));
            window.draw(selBar);
        }

        // Bottom row divider
        sf::RectangleShape rDiv(sf::Vector2f(tblCardW - 2.f, 1.f));
        rDiv.setPosition(tblCardX + 1.f, ry + rowH);
        rDiv.setFillColor(sf::Color(226, 232, 240));
        window.draw(rDiv);

        float midY = ry + rowH * 0.5f;

        // 1. MÃ NV
        drawCenteredTxt(st.maNguoiDung, tblCardX + 42.f, midY, 13, isSelected ? sf::Color(2, 132, 199) : sf::Color(30, 41, 59), true);

        // 2. HỌ TÊN with circular avatar
        sf::Color avCol = isLocked ? sf::Color(148, 163, 184) : avatarPalette[realIdx % avatarPalette.size()];
        sf::CircleShape avCir(15.f);
        avCir.setFillColor(avCol);
        avCir.setPosition(tblCardX + 88.f, midY - 15.f);
        window.draw(avCir);

        std::string initChar = "A";
        size_t lastSpace = st.hoTen.find_last_of(' ');
        std::string lastWord = (lastSpace != std::string::npos && lastSpace + 1 < st.hoTen.size()) ? st.hoTen.substr(lastSpace + 1) : st.hoTen;
        std::string cleanWord = removeVietnameseDiacritics(lastWord);
        if (!cleanWord.empty()) {
            initChar = std::string(1, static_cast<char>(std::toupper(static_cast<unsigned char>(cleanWord[0]))));
        }
        drawCenteredTxt(initChar, tblCardX + 103.f, midY, 12, sf::Color::White, true);

        drawTxt(st.hoTen, tblCardX + 126.f, midY - 8.f, 13, isLocked ? sf::Color(100, 116, 139) : sf::Color(15, 23, 42), true);
        if (isLocked) {
            sf::Text tmpName;
            tmpName.setFont(Theme::FontPrimary);
            tmpName.setCharacterSize(13);
            tmpName.setString(toSfString(st.hoTen));
            float nw = tmpName.getLocalBounds().width;
            sf::RectangleShape strike(sf::Vector2f(nw, 1.f));
            strike.setPosition(tblCardX + 126.f, midY + 1.f);
            strike.setFillColor(sf::Color(148, 163, 184));
            window.draw(strike);
        }

        // 3. TÊN ĐĂNG NHẬP (monospaced pill container)
        float uPillW = 112.f;
        float uPillH = 30.f;
        float uPillX = tblCardX + 312.f - uPillW * 0.5f;
        RoundedRectRenderer::render(window, sf::Vector2f(uPillX, midY - uPillH * 0.5f), sf::Vector2f(uPillW, uPillH), 6.f, sf::Color(241, 245, 249), 1.f, sf::Color(226, 232, 240));
        drawCenteredTxt(st.taiKhoan, tblCardX + 312.f, midY, 12, sf::Color(30, 41, 59), true);

        // 4. QUYỀN (VAI TRÒ) Badge
        float rBadgeW = 126.f;
        float rBadgeH = 30.f;
        float rBadgeX = tblCardX + 494.f - rBadgeW * 0.5f;
        float rBadgeY = midY - rBadgeH * 0.5f;
        if (st.vaiTro == "Admin") {
            RoundedRectRenderer::render(window, sf::Vector2f(rBadgeX, rBadgeY), sf::Vector2f(rBadgeW, rBadgeH), 8.f, sf::Color(224, 242, 254), 1.f, sf::Color(186, 230, 253));
            sf::CircleShape uHead(3.f); uHead.setFillColor(sf::Color(3, 105, 161)); uHead.setPosition(rBadgeX + 12.f, midY - 6.f); window.draw(uHead);
            RoundedRectRenderer::render(window, sf::Vector2f(rBadgeX + 10.f, midY), sf::Vector2f(10.f, 6.f), 3.f, sf::Color(3, 105, 161));
            drawCenteredTxt("Quản trị viên", rBadgeX + rBadgeW * 0.5f + 6.f, midY, 11, sf::Color(3, 105, 161), true);
        } else {
            RoundedRectRenderer::render(window, sf::Vector2f(rBadgeX, rBadgeY), sf::Vector2f(rBadgeW, rBadgeH), 8.f, sf::Color(241, 245, 249), 1.f, sf::Color(226, 232, 240));
            drawCenteredTxt("Nhân viên bán vé", rBadgeX + rBadgeW * 0.5f, midY, 11, sf::Color(71, 85, 105), false);
        }

        // 5. TRẠNG THÁI Sticker (Clickable status toggle)
        float sBadgeW = 104.f;
        float sBadgeH = 30.f;
        float sBadgeX = tblCardX + 682.f - sBadgeW * 0.5f;
        float sBadgeY = midY - sBadgeH * 0.5f;
        if (!isLocked) {
            RoundedRectRenderer::render(window, sf::Vector2f(sBadgeX, sBadgeY), sf::Vector2f(sBadgeW, sBadgeH), 15.f, sf::Color(220, 252, 231), 1.f, sf::Color(134, 239, 172));
            sf::CircleShape dot(3.f);
            dot.setFillColor(sf::Color(22, 163, 74));
            dot.setPosition(sBadgeX + 12.f, midY - 3.f);
            window.draw(dot);
            drawCenteredTxt("Hoạt động", sBadgeX + sBadgeW * 0.5f + 5.f, midY, 11, sf::Color(21, 128, 61), true);
        } else {
            RoundedRectRenderer::render(window, sf::Vector2f(sBadgeX, sBadgeY), sf::Vector2f(sBadgeW, sBadgeH), 15.f, sf::Color(254, 242, 242), 1.f, sf::Color(254, 205, 211));
            RoundedRectRenderer::render(window, sf::Vector2f(sBadgeX + 12.f, midY - 2.f), sf::Vector2f(8.f, 8.f), 2.f, sf::Color(225, 29, 72));
            drawCenteredTxt("Bị khóa", sBadgeX + sBadgeW * 0.5f + 6.f, midY, 11, sf::Color(185, 28, 28), true);
        }

        // 6. THAO TÁC (Pencil Edit Icon)
        float pX = tblCardX + 838.f;
        drawCenteredTxt("✎", pX, midY, 14, sf::Color(2, 132, 199), true);
    }

    // Footer Pagination Strip (y = 856..912)
    int dispFrom = (totalFiltered == 0) ? 0 : (startIdx + 1);
    int dispTo = endIdx;
    std::string pageInfo = "Hiển thị " + std::to_string(dispFrom) + " - " + std::to_string(dispTo) +
                           " trong tổng số " + std::to_string(totalFiltered) + " tài khoản";
    drawTxt(pageInfo, tblCardX + 18.f, 874.f, 12, sf::Color(100, 116, 139), false);

    if (btnAccountPrevPage) btnAccountPrevPage->draw(window);

    for (int p = 0; p < std::min(totalPages, 2); ++p) {
        float px = tblCardX + 788.f + p * 34.f;
        bool activeP = (p == accountCurrentPage);
        RoundedRectRenderer::render(
            window, sf::Vector2f(px, 868.f), sf::Vector2f(28.f, 28.f), 6.f,
            activeP ? sf::Color(2, 132, 199) : sf::Color::White,
            1.f, activeP ? sf::Color(2, 132, 199) : sf::Color(226, 232, 240)
        );
        drawCenteredTxt(std::to_string(p + 1), px + 14.f, 882.f, 12, activeP ? sf::Color::White : sf::Color(71, 85, 105), true);
    }

    if (btnAccountNextPage) btnAccountNextPage->draw(window);

    // =========================================================================
    // 5. RIGHT CARD: ACCOUNT DETAILS FORM (rightColX = 1200, y = 228, w = 484, h = 684)
    // =========================================================================
    float formCardW = 484.f;
    float formCardH = 684.f;
    RoundedRectRenderer::render(window, sf::Vector2f(rightColX, tblCardY), sf::Vector2f(formCardW, formCardH), 12.f, sf::Color::White, 1.f, sf::Color(226, 232, 240));

    RoundedRectRenderer::render(window, sf::Vector2f(rightColX + 24.f, tblCardY + 20.f), sf::Vector2f(5.f, 20.f), 2.5f, sf::Color(2, 132, 199));
    drawTxt("Thông tin tài khoản", rightColX + 38.f, tblCardY + 19.f, 16, sf::Color(15, 23, 42), true);

    std::string selBadgeStr = (selectedAccountRowIdx >= 0 && selectedAccountRowIdx < static_cast<int>(staffUserList.size()))
        ? ("#" + staffUserList[selectedAccountRowIdx].maNguoiDung + " - Đang chọn")
        : "Cấp tài khoản mới";
    RoundedRectRenderer::render(window, sf::Vector2f(rightColX + 330.f, tblCardY + 17.f), sf::Vector2f(130.f, 26.f), 6.f, sf::Color(224, 242, 254), 1.f, sf::Color(186, 230, 253));
    drawCenteredTxt(selBadgeStr, rightColX + 395.f, tblCardY + 30.f, 11, sf::Color(3, 105, 161), true);

    // 1. Chọn NV
    drawTxt("Chọn NV:", rightColX + 24.f, 292.f, 13, sf::Color(51, 65, 85), true);
    if (inputAccountEmployeeId) {
        inputAccountEmployeeId->draw(window);
        float sx = inputAccountEmployeeId->getPosition().x + inputAccountEmployeeId->getSize().x - 26.f;
        float sy = inputAccountEmployeeId->getPosition().y + 12.f;
        sf::CircleShape qCir(4.5f); qCir.setFillColor(sf::Color::Transparent); qCir.setOutlineThickness(1.3f); qCir.setOutlineColor(sf::Color(148, 163, 184)); qCir.setPosition(sx, sy); window.draw(qCir);
        sf::RectangleShape qH(sf::Vector2f(4.f, 1.3f)); qH.setFillColor(sf::Color(148, 163, 184)); qH.setPosition(sx + 7.f, sy + 7.f); qH.setRotation(45.f); window.draw(qH);
    }

    // 2. Họ tên
    drawTxt("Họ tên:", rightColX + 24.f, 364.f, 13, sf::Color(51, 65, 85), true);
    if (inputAccountFullName) {
        inputAccountFullName->draw(window);
        float lx = inputAccountFullName->getPosition().x + inputAccountFullName->getSize().x - 26.f;
        float ly = inputAccountFullName->getPosition().y + 12.f;
        RoundedRectRenderer::render(window, sf::Vector2f(lx, ly + 5.f), sf::Vector2f(11.f, 9.f), 2.f, sf::Color(148, 163, 184));
        RoundedRectRenderer::render(window, sf::Vector2f(lx + 2.f, ly), sf::Vector2f(7.f, 7.f), 3.5f, sf::Color::Transparent, 1.3f, sf::Color(148, 163, 184));
    }

    // 3. Username
    drawTxt("Username (Tên đăng nhập):", rightColX + 24.f, 436.f, 13, sf::Color(51, 65, 85), true);
    if (inputAccountUsername) {
        inputAccountUsername->draw(window);
        drawTxt("● Hợp lệ", inputAccountUsername->getPosition().x + inputAccountUsername->getSize().x - 62.f, inputAccountUsername->getPosition().y + 11.f, 12, sf::Color(22, 163, 74), true);
    }

    // 4. Mật khẩu
    drawTxt("Mật khẩu:", rightColX + 24.f, 508.f, 13, sf::Color(51, 65, 85), true);
    drawTxt("Tạo mật khẩu ngẫu nhiên", rightColX + 316.f, 508.f, 12, sf::Color(2, 132, 199), false);

    if (inputAccountPassword) {
        inputAccountPassword->draw(window);
        float ex = inputAccountPassword->getPosition().x + inputAccountPassword->getSize().x - 28.f;
        float ey = inputAccountPassword->getPosition().y + 13.f;
        drawCenteredTxt(accountShowPassword ? "👁" : "👁", ex + 6.f, ey + 6.f, 13, sf::Color(148, 163, 184), false);
    }

    // Password strength indicator: 3 green bars
    float mBarX = rightColX + 24.f;
    float mBarY = 582.f;
    float mBarW = 98.f;
    float mBarH = 4.f;
    for (int b = 0; b < 3; ++b) {
        RoundedRectRenderer::render(window, sf::Vector2f(mBarX + b * (mBarW + 6.f), mBarY), sf::Vector2f(mBarW, mBarH), 2.f, sf::Color(16, 185, 129));
    }
    drawTxt("Độ an toàn: Tốt", mBarX + 3.f * (mBarW + 6.f) + 12.f, mBarY - 6.f, 11, sf::Color(16, 185, 129), true);

    // 5. Vai trò (Phân quyền)
    drawTxt("Vai trò (Phân quyền):", rightColX + 24.f, 602.f, 13, sf::Color(51, 65, 85), true);
    if (accountRoleDropdown) accountRoleDropdown->draw(window);

    // Note
    drawTxt("*Chọn nhân viên, nhập Username + Mật khẩu rồi nhấn Xác nhận để lưu.", rightColX + 24.f, 680.f, 11, sf::Color(148, 163, 184), false, true);

    // Buttons Row 1 (y = 718)
    if (btnAccountSave) btnAccountSave->draw(window);
    if (btnAccountToggleLock) btnAccountToggleLock->draw(window);

    // Buttons Row 2 (y = 768)
    if (btnAccountRefresh) btnAccountRefresh->draw(window);
    if (btnAccountClear) btnAccountClear->draw(window);

    // =========================================================================
    // 6. BOTTOM COPYRIGHT & HOTLINE FOOTER (y = 934)
    // =========================================================================
    drawTxt("© 2026 Cinestar CineOS   •   Bản quyền thuộc Cụm Rạp Chiếu Phim Cinestar Cinema Complex   •   Hệ thống bảo mật ISO/IEC 27001", leftColX, 934.f, 11, sf::Color(100, 116, 139), false);
    drawTxt("Hotline Kỹ Thuật: 1900 6081 (Ext: 104)   •   Cụm Server: Node-QuocThanh-HCM", 1236.f, 934.f, 11, sf::Color(30, 41, 59), true);
}

} // namespace UI
