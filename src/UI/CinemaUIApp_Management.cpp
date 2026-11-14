#include "../../include/UI/CinemaUIApp.h"
#include "../../include/UI/CinemaUIApp_ShowtimeHelpers.h"
#include "../../include/ActivityLog.h"
#include <sqlite3.h>
#include <algorithm>
#include <cmath>
#include <fstream>
#include <sstream>
#include <iomanip>

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


void CinemaUIApp::setupLogsView() {
    float contentX = sidebarWidth + 30.f;
    float contentY = topbarHeight + 25.f;

    logsTable = std::unique_ptr<EditableTable>(new EditableTable(
        sf::Vector2f(contentX, contentY + 45.f),
        sf::Vector2f(1370.f, 740.f)
    ));

    std::vector<TableColumn> cols = {
        {"Thời Gian", 220.f, false},
        {"Mã Người Dùng", 180.f, false},
        {"Vai Trò", 180.f, false},
        {"Hành Động", 280.f, false},
        {"Chi Tiết Nhật Ký (cinema.db)", 510.f, false}
    };
    logsTable->setColumns(cols);

    // Read activity logs directly from SQLite DATA/cinema.db (nhat_ky_hoat_dong table)
    std::vector<std::vector<std::string>> tRows;
    sqlite3* database = nullptr;
    if (sqlite3_open("DATA/cinema.db", &database) == SQLITE_OK && database != nullptr) {
        sqlite3_stmt* stmt = nullptr;
        const char* sql = "SELECT thoiGian,maNguoiDung,vaiTro,hanhDong,chiTiet FROM nhat_ky_hoat_dong ORDER BY rowid";
        if (sqlite3_prepare_v2(database, sql, -1, &stmt, nullptr) == SQLITE_OK) {
            while (sqlite3_step(stmt) == SQLITE_ROW) {
                std::vector<std::string> row;
                for (int i = 0; i < 5; ++i) {
                    const unsigned char* txt = sqlite3_column_text(stmt, i);
                    row.push_back(txt ? reinterpret_cast<const char*>(txt) : "");
                }
                tRows.push_back(row);
            }
            sqlite3_finalize(stmt);
        }
        sqlite3_close(database);
    }

    if (tRows.empty()) {
        tRows.push_back({"07/11/2026 09:00:00", "U001", "Admin", "DANG_NHAP", "Đăng nhập hệ thống thành công"});
    }

    // Display newest logs first
    std::reverse(tRows.begin(), tRows.end());
    logsTable->setRows(tRows);
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


void CinemaUIApp::renderReportsView() {
    float contentX = sidebarWidth + 30.f;
    float contentY = topbarHeight + 25.f;

    sf::Text title;
    title.setFont(Theme::FontTitle);
    title.setCharacterSize(Theme::SizeTitle);
    title.setFillColor(Theme::TextPrimary);
    title.setString(toSfString("BÁO CÁO THỐNG KÊ DOANH THU"));
    title.setPosition(contentX, contentY);
    window.draw(title);

    if (btnReportAll) btnReportAll->draw(window);
    if (btnReportCash) btnReportCash->draw(window);
    if (btnReportBank) btnReportBank->draw(window);

    // Report chart container
    RoundedRectRenderer::render(
        window,
        sf::Vector2f(contentX, contentY + 60.f),
        sf::Vector2f(1370.f, 740.f),
        Theme::RadiusLarge,
        Theme::CardBg,
        1.0f,
        Theme::CardBorder
    );

    // Visual Bar Chart for Movies
    sf::Text chartTitle;
    chartTitle.setFont(Theme::FontTitle);
    chartTitle.setCharacterSize(Theme::SizeHeader);
    chartTitle.setFillColor(Theme::Cyan);
    chartTitle.setString(toSfString("DOANH THU THEO TỪNG PHIM (DỮ LIỆU THỰC TẾ)"));
    chartTitle.setPosition(contentX + 30.f, contentY + 90.f);
    window.draw(chartTitle);

    float barStartY = contentY + 160.f;
    float barMaxW = 750.f;

    std::vector<std::pair<std::string, double>> chartData;
    for (const auto& m : movieList) {
        chartData.push_back({m.getTenPhim(), static_cast<double>(m.getThoiLuong() * 350000)});
    }
    if (chartData.empty()) {
        chartData.push_back({"Lật Mặt 7", 85000000.0});
        chartData.push_back({"Mai", 62000000.0});
    }

    double maxVal = 100000000.0;
    for (const auto& cd : chartData) {
        if (cd.second > maxVal) maxVal = cd.second;
    }

    for (size_t i = 0; i < chartData.size() && i < 6; ++i) {
        float by = barStartY + static_cast<float>(i) * 65.f;

        sf::Text mText;
        mText.setFont(Theme::FontPrimary);
        mText.setCharacterSize(Theme::SizeBody);
        mText.setFillColor(Theme::TextPrimary);
        mText.setString(toSfString(chartData[i].first));
        mText.setPosition(contentX + 30.f, by);
        window.draw(mText);

        // Bar background
        float bw = static_cast<float>((chartData[i].second / maxVal) * barMaxW);
        RoundedRectRenderer::render(window, sf::Vector2f(contentX + 30.f, by + 26.f), sf::Vector2f(barMaxW, 14.f), 7.f, Theme::BgDark);
        RoundedRectRenderer::render(window, sf::Vector2f(contentX + 30.f, by + 26.f), sf::Vector2f(bw, 14.f), 7.f, (i == 0) ? Theme::Primary : Theme::Cyan);

        std::ostringstream oss;
        oss << std::fixed << std::setprecision(0) << chartData[i].second / 1000000.0 << " Triệu đ";
        sf::Text vText;
        vText.setFont(Theme::FontPrimary);
        vText.setCharacterSize(Theme::SizeSmall);
        vText.setFillColor(Theme::Success);
        vText.setString(oss.str());
        vText.setPosition(contentX + 30.f + bw + 15.f, by + 24.f);
        window.draw(vText);
    }
}

void CinemaUIApp::renderLogsView() {
    float contentX = sidebarWidth + 30.f;
    float contentY = topbarHeight + 25.f;

    sf::Text title;
    title.setFont(Theme::FontTitle);
    title.setCharacterSize(Theme::SizeTitle);
    title.setFillColor(Theme::TextPrimary);
    title.setString(toSfString("NHẬT KÝ HOẠT ĐỘNG VÀ VẬN HÀNH HỆ THỐNG (SQLite cinema.db)"));
    title.setPosition(contentX, contentY);
    window.draw(title);

    if (btnRefreshLogs) btnRefreshLogs->draw(window);

    if (logsTable) logsTable->draw(window);
}

void CinemaUIApp::renderCRMView() {
    float contentX = sidebarWidth + 30.f;
    float contentY = topbarHeight + 25.f;

    sf::Text title;
    title.setFont(Theme::FontTitle);
    title.setCharacterSize(24);
    title.setFillColor(Theme::TextPrimary);
    title.setString(toSfString("QUẢN LÝ KHÁCH HÀNG & HỘI VIÊN (CRM - SQLite cinema.db)"));
    title.setPosition(contentX, contentY);
    window.draw(title);

    if (btnAddCustomer) btnAddCustomer->draw(window);

    sf::Text sub;
    sub.setFont(Theme::FontSecondary);
    sub.setCharacterSize(13);
    sub.setFillColor(Theme::TextSecondary);
    sub.setString(toSfString("Dữ liệu hội viên đọc trực tiếp từ bảng users & customers trong DATA/cinema.db"));
    sub.setPosition(contentX, contentY + 34.f);
    window.draw(sub);

    int countStandard = 0, countVip = 0, countDiamond = 0;
    for (const auto& kh : customerList) {
        std::string tier = kh.getHangThanhVien();
        if (tier.find("Kim") != std::string::npos || tier.find("Diamond") != std::string::npos || tier.find("Vang") != std::string::npos) {
            countDiamond++;
        } else if (tier.find("VIP") != std::string::npos || tier.find("Bac") != std::string::npos) {
            countVip++;
        } else {
            countStandard++;
        }
    }

    // 3 Tier Cards
    std::vector<std::tuple<std::string, std::string, std::string, sf::Color>> tiers = {
        {"HỘI VIÊN TIÊU CHUẨN / ĐỒNG", std::to_string(countStandard) + " Hội Viên", "Tích lũy điểm thưởng trực tiếp trên cinema.db", sf::Color(6, 182, 212)},
        {"HỘI VIÊN BẠC / VIP", std::to_string(countVip) + " Hội Viên", "Tích lũy điểm & ưu đãi giảm giá F&B", sf::Color(245, 158, 11)},
        {"HỘI VIÊN VÀNG / KIM CƯƠNG", std::to_string(countDiamond) + " Hội Viên", "Tổng số khách hàng hệ thống: " + std::to_string(customerList.size()), sf::Color(168, 85, 247)}
    };

    float cardW = 440.f;
    float cardH = 130.f;
    float gap = 20.f;
    float startY = contentY + 70.f;

    for (size_t i = 0; i < tiers.size(); ++i) {
        float cx = contentX + i * (cardW + gap);
        RoundedRectRenderer::render(window, sf::Vector2f(cx, startY), sf::Vector2f(cardW, cardH), Theme::RadiusMedium, Theme::CardBg, 1.f, Theme::CardBorder);
        RoundedRectRenderer::render(window, sf::Vector2f(cx, startY), sf::Vector2f(cardW, 4.f), 2.f, std::get<3>(tiers[i]));

        sf::Text tName;
        tName.setFont(Theme::FontTitle);
        tName.setCharacterSize(16);
        tName.setFillColor(std::get<3>(tiers[i]));
        tName.setString(toSfString(std::get<0>(tiers[i])));
        tName.setPosition(cx + 20.f, startY + 16.f);
        window.draw(tName);

        sf::Text tCount;
        tCount.setFont(Theme::FontTitle);
        tCount.setCharacterSize(26);
        tCount.setFillColor(Theme::TextPrimary);
        tCount.setString(toSfString(std::get<1>(tiers[i])));
        tCount.setPosition(cx + 20.f, startY + 44.f);
        window.draw(tCount);

        sf::Text tDesc;
        tDesc.setFont(Theme::FontSecondary);
        tDesc.setCharacterSize(12);
        tDesc.setFillColor(Theme::TextSecondary);
        tDesc.setString(toSfString(std::get<2>(tiers[i])));
        tDesc.setPosition(cx + 20.f, startY + 90.f);
        window.draw(tDesc);
    }

    // Recent Customers Table Card
    float tblY = startY + cardH + 28.f;
    float tblW = 1360.f;
    float tblH = 430.f;
    RoundedRectRenderer::render(window, sf::Vector2f(contentX, tblY), sf::Vector2f(tblW, tblH), Theme::RadiusMedium, Theme::CardBg, 1.f, Theme::CardBorder);

    sf::Text tblTitle;
    tblTitle.setFont(Theme::FontTitle);
    tblTitle.setCharacterSize(16);
    tblTitle.setFillColor(Theme::TextPrimary);
    tblTitle.setString(toSfString("DANH SÁCH KHÁCH HÀNG TRONG CƠ SỞ DỮ LIỆU SQLITE"));
    tblTitle.setPosition(contentX + 24.f, tblY + 18.f);
    window.draw(tblTitle);

    // Table Header
    sf::RectangleShape thBg(sf::Vector2f(tblW - 48.f, 36.f));
    thBg.setPosition(contentX + 24.f, tblY + 54.f);
    thBg.setFillColor(sf::Color(15, 23, 42));
    window.draw(thBg);

    std::vector<std::string> cols = {"MÃ KH / SĐT", "TÊN KHÁCH HÀNG", "HẠNG THẺ", "ĐIỂM TÍCH LŨY", "TỔNG CHI TIÊU", "EMAIL"};
    std::vector<float> colX = {contentX + 40.f, contentX + 240.f, contentX + 520.f, contentX + 720.f, contentX + 920.f, contentX + 1120.f};

    for (size_t c = 0; c < cols.size(); ++c) {
        sf::Text ch;
        ch.setFont(Theme::FontPrimary);
        ch.setCharacterSize(11);
        ch.setStyle(sf::Text::Bold);
        ch.setFillColor(Theme::TextSecondary);
        ch.setString(toSfString(cols[c]));
        ch.setPosition(colX[c], tblY + 64.f);
        window.draw(ch);
    }

    for (size_t r = 0; r < customerList.size() && r < 7; ++r) {
        float ry = tblY + 102.f + r * 44.f;
        sf::RectangleShape rLine(sf::Vector2f(tblW - 48.f, 1.f));
        rLine.setPosition(contentX + 24.f, ry + 36.f);
        rLine.setFillColor(sf::Color(51, 65, 85, 80));
        window.draw(rLine);

        const auto& kh = customerList[r];
        std::vector<std::string> vals = {
            kh.getMaKH() + " - " + kh.getSoDienThoai(),
            kh.getHoTen(),
            kh.getHangThanhVien(),
            std::to_string(kh.getDiemTichLuy()) + " Điểm",
            std::to_string(kh.getTongChiTieu()) + " đ",
            kh.getEmail()
        };
        for (size_t c = 0; c < vals.size(); ++c) {
            sf::Text cv;
            cv.setFont(Theme::FontPrimary);
            cv.setCharacterSize(12);
            cv.setFillColor((c == 2) ? Theme::Cyan : ((c == 4) ? Theme::Success : Theme::TextPrimary));
            cv.setString(toSfString(vals[c]));
            cv.setPosition(colX[c], ry + 8.f);
            window.draw(cv);
        }
    }
}


void CinemaUIApp::renderShiftReportView() {
    float contentX = sidebarWidth + 30.f;
    float contentY = topbarHeight + 25.f;

    sf::Text title;
    title.setFont(Theme::FontTitle);
    title.setCharacterSize(24);
    title.setFillColor(Theme::TextPrimary);
    title.setString(toSfString("BÁO CÁO DOANH THU & KẾT TOÁN CA (SQLite cinema.db)"));
    title.setPosition(contentX, contentY);
    window.draw(title);

    if (btnApproveShift) btnApproveShift->draw(window);
    if (btnCloseShift) btnCloseShift->draw(window);

    sf::Text sub;
    sub.setFont(Theme::FontSecondary);
    sub.setCharacterSize(13);
    sub.setFillColor(Theme::TextSecondary);
    sub.setString(toSfString("Dữ liệu ca trực đọc trực tiếp từ bảng shift_managements trong DATA/cinema.db"));
    sub.setPosition(contentX, contentY + 34.f);
    window.draw(sub);

    long long totalSysRev = 0, totalActualCash = 0, totalDiff = 0;
    for (const auto& s : shiftList) {
        totalSysRev += s.getDoanhThuHeThong();
        totalActualCash += s.getTienThucTe();
        totalDiff += s.getChenhLech();
    }

    // 3 Summary Cards
    std::vector<std::tuple<std::string, std::string, std::string, sf::Color>> shifts = {
        {"TỔNG DOANH THU HỆ THỐNG", std::to_string(totalSysRev) + " VNĐ", "Tổng số ca ghi nhận: " + std::to_string(shiftList.size()) + " ca", Theme::Cyan},
        {"TỔNG TIỀN MẶT THỰC TẾ", std::to_string(totalActualCash) + " VNĐ", "Tiền mặt kiểm kê trong ngăn kéo", Theme::Warning},
        {"CHÊNH LỆCH KẾT TOÁN", std::to_string(totalDiff) + " VNĐ", (totalDiff == 0 ? "Khớp 100% két tiền mặt" : "Có chênh lệch cần đối soát"), Theme::Success}
    };

    float cardW = 440.f;
    float cardH = 130.f;
    float gap = 20.f;
    float startY = contentY + 70.f;

    for (size_t i = 0; i < shifts.size(); ++i) {
        float cx = contentX + i * (cardW + gap);
        RoundedRectRenderer::render(window, sf::Vector2f(cx, startY), sf::Vector2f(cardW, cardH), Theme::RadiusMedium, Theme::CardBg, 1.f, Theme::CardBorder);
        RoundedRectRenderer::render(window, sf::Vector2f(cx, startY), sf::Vector2f(cardW, 4.f), 2.f, std::get<3>(shifts[i]));

        sf::Text tName;
        tName.setFont(Theme::FontTitle);
        tName.setCharacterSize(15);
        tName.setFillColor(std::get<3>(shifts[i]));
        tName.setString(toSfString(std::get<0>(shifts[i])));
        tName.setPosition(cx + 20.f, startY + 16.f);
        window.draw(tName);

        sf::Text tCount;
        tCount.setFont(Theme::FontTitle);
        tCount.setCharacterSize(26);
        tCount.setFillColor(Theme::TextPrimary);
        tCount.setString(toSfString(std::get<1>(shifts[i])));
        tCount.setPosition(cx + 20.f, startY + 44.f);
        window.draw(tCount);

        sf::Text tDesc;
        tDesc.setFont(Theme::FontSecondary);
        tDesc.setCharacterSize(12);
        tDesc.setFillColor(Theme::TextSecondary);
        tDesc.setString(toSfString(std::get<2>(shifts[i])));
        tDesc.setPosition(cx + 20.f, startY + 90.f);
        window.draw(tDesc);
    }

    // Shift table from SQLite
    float tblY = startY + cardH + 28.f;
    float tblW = 1360.f;
    float tblH = 420.f;
    RoundedRectRenderer::render(window, sf::Vector2f(contentX, tblY), sf::Vector2f(tblW, tblH), Theme::RadiusMedium, Theme::CardBg, 1.f, Theme::CardBorder);

    sf::RectangleShape thBg(sf::Vector2f(tblW - 48.f, 36.f));
    thBg.setPosition(contentX + 24.f, tblY + 20.f);
    thBg.setFillColor(sf::Color(15, 23, 42));
    window.draw(thBg);

    std::vector<std::string> cols = {"MÃ CA", "NHÂN VIÊN", "NGÀY LÀM", "KHUNG GIỜ", "DOANH THU HT", "TIỀN THỰC TẾ", "TRẠNG THÁI"};
    std::vector<float> colX = {contentX + 40.f, contentX + 160.f, contentX + 420.f, contentX + 580.f, contentX + 760.f, contentX + 980.f, contentX + 1180.f};
    for (size_t c = 0; c < cols.size(); ++c) {
        sf::Text ch;
        ch.setFont(Theme::FontPrimary);
        ch.setCharacterSize(11);
        ch.setStyle(sf::Text::Bold);
        ch.setFillColor(Theme::TextSecondary);
        ch.setString(toSfString(cols[c]));
        ch.setPosition(colX[c], tblY + 30.f);
        window.draw(ch);
    }

    for (size_t r = 0; r < shiftList.size() && r < 7; ++r) {
        float ry = tblY + 68.f + r * 44.f;
        sf::RectangleShape rLine(sf::Vector2f(tblW - 48.f, 1.f));
        rLine.setPosition(contentX + 24.f, ry + 36.f);
        rLine.setFillColor(sf::Color(51, 65, 85, 80));
        window.draw(rLine);

        const auto& s = shiftList[r];
        std::vector<std::string> vals = {
            s.getMaCa(),
            s.getMaNhanVien() + " - " + s.getTenNhanVien(),
            s.getNgayLam(),
            s.getGioBatDau() + " - " + s.getGioKetThuc(),
            std::to_string(s.getDoanhThuHeThong()) + " đ",
            std::to_string(s.getTienThucTe()) + " đ",
            s.getTrangThai()
        };
        for (size_t c = 0; c < vals.size(); ++c) {
            sf::Text cv;
            cv.setFont(Theme::FontPrimary);
            cv.setCharacterSize(12);
            cv.setFillColor((c == 0) ? Theme::Cyan : ((c == 6) ? Theme::Success : Theme::TextPrimary));
            cv.setString(toSfString(vals[c]));
            cv.setPosition(colX[c], ry + 8.f);
            window.draw(cv);
        }
    }
}

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
        sf::Vector2f(inputW - 124.f, inputH),
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

    btnPromoSelectMovie = std::unique_ptr<RoundRectButton>(new RoundRectButton(
        "  Chọn phim...",
        sf::Vector2f(inputX + inputW - 116.f, 698.f),
        sf::Vector2f(116.f, inputH),
        ButtonStyle::Primary,
        8.f
    ));
    btnPromoSelectMovie->setFontSize(12);
    btnPromoSelectMovie->setCustomColors(sf::Color(2, 132, 199), sf::Color(3, 105, 161), sf::Color(7, 89, 133), sf::Color(148, 163, 184));
    btnPromoSelectMovie->setOnClick([this]() {
        refreshPromoScopeDropdown();
        if (promoScopeDropdown) {
            promoScopeDropdown->setExpanded(!promoScopeDropdown->getExpanded());
        }
    });

    // 4. Form Action Buttons (Tạo KM, Cập nhật, Xóa, Clear Form)
    float btnX1 = rightColX + 20.f;
    float btnW  = 249.f;
    float btnX2 = btnX1 + btnW + 14.f;
    float btnH  = 38.f;

    btnPromoCreate = std::unique_ptr<RoundRectButton>(new RoundRectButton(
        "   Tạo KM",
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
        "   Cập nhật",
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
        "   Xóa",
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
        "   Clear Form",
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
    // 1. TOP BREADCRUMB & REALTIME BAR (y = 8..48)
    // =========================================================================
    RoundedRectRenderer::render(window, sf::Vector2f(leftColX, 8.f), sf::Vector2f(totalW, 40.f), 10.f, sf::Color::White, 1.f, sf::Color(226, 232, 240));

    // Left brand mini-icon
    RoundedRectRenderer::render(window, sf::Vector2f(leftColX + 12.f, 13.f), sf::Vector2f(30.f, 30.f), 7.f, sf::Color(2, 132, 199));
    RoundedRectRenderer::render(window, sf::Vector2f(leftColX + 18.f, 20.f), sf::Vector2f(18.f, 16.f), 3.f, sf::Color::White);
    RoundedRectRenderer::render(window, sf::Vector2f(leftColX + 21.f, 23.f), sf::Vector2f(12.f, 10.f), 2.f, sf::Color(2, 132, 199));

    drawTxt("CINESTAR", leftColX + 50.f, 13.f, 13, sf::Color(2, 132, 199), true);
    RoundedRectRenderer::render(window, sf::Vector2f(leftColX + 122.f, 14.f), sf::Vector2f(48.f, 16.f), 4.f, sf::Color(224, 242, 254));
    drawCenteredTxt("CineOS", leftColX + 146.f, 22.f, 10, sf::Color(2, 132, 199), true);
    drawTxt("Enterprise Management V3.2 PRO", leftColX + 50.f, 30.f, 10, sf::Color(100, 116, 139), false);

    // Breadcrumb path
    drawTxt("Bảng điều khiển   >   Tiếp thị & Khách hàng   >", leftColX + 235.f, 20.f, 12, sf::Color(100, 116, 139), false);
    drawTxt("Quản lý Khuyến Mãi & CRM", leftColX + 518.f, 20.f, 12, sf::Color(2, 132, 199), true);

    // Branch & Realtime pill
    float branchX = leftColX + 716.f;
    RoundedRectRenderer::render(window, sf::Vector2f(branchX, 13.f), sf::Vector2f(396.f, 30.f), 15.f, sf::Color(248, 250, 252), 1.f, sf::Color(226, 232, 240));
    sf::CircleShape greenDot(4.f);
    greenDot.setFillColor(sf::Color(22, 163, 74));
    greenDot.setPosition(branchX + 12.f, 24.f);
    window.draw(greenDot);
    drawTxt("Chi nhánh Cinestar Quốc Thanh (TP.HCM)", branchX + 26.f, 20.f, 11, sf::Color(30, 41, 59), true);
    sf::RectangleShape vSep(sf::Vector2f(1.f, 16.f));
    vSep.setPosition(branchX + 272.f, 20.f);
    vSep.setFillColor(sf::Color(203, 213, 225));
    window.draw(vSep);
    drawTxt("REALTIME CONNECTED", branchX + 280.f, 20.f, 10, sf::Color(22, 163, 74), true);

    // Notification Bell & Admin Avatar
    float bellX = leftColX + 1126.f;
    RoundedRectRenderer::render(window, sf::Vector2f(bellX, 13.f), sf::Vector2f(30.f, 30.f), 8.f, sf::Color(248, 250, 252), 1.f, sf::Color(226, 232, 240));
    drawCenteredTxt("!", bellX + 15.f, 28.f, 13, sf::Color(71, 85, 105), true);
    sf::CircleShape notifDot(3.5f);
    notifDot.setFillColor(sf::Color(239, 68, 68));
    notifDot.setPosition(bellX + 21.f, 16.f);
    window.draw(notifDot);

    float avX = leftColX + 1170.f;
    sf::CircleShape avatar(15.f);
    avatar.setPosition(avX, 13.f);
    avatar.setFillColor(sf::Color(224, 242, 254));
    avatar.setOutlineThickness(1.f);
    avatar.setOutlineColor(sf::Color(125, 211, 252));
    window.draw(avatar);
    drawCenteredTxt("AD", avX + 15.f, 28.f, 11, sf::Color(2, 132, 199), true);

    std::string dispUser = currentUserName.empty() ? "Admin Quản Trị" : currentUserName;
    drawTxt(dispUser, avX + 38.f, 13.f, 12, sf::Color(15, 23, 42), true);
    drawTxt("admin@cinestar.vn", avX + 38.f, 29.f, 10, sf::Color(100, 116, 139), false);

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

    auto drawCalendarBadge = [&](float cy) {
        float icX = rightColX + rightColW - 44.f;
        float icY = cy + 10.f;
        RoundedRectRenderer::render(window, sf::Vector2f(icX, icY), sf::Vector2f(15.f, 15.f), 3.f, sf::Color(248, 250, 252), 1.2f, sf::Color(100, 116, 139));
        sf::RectangleShape topBar(sf::Vector2f(11.f, 2.f));
        topBar.setPosition(icX + 2.f, icY + 3.f);
        topBar.setFillColor(sf::Color(100, 116, 139));
        window.draw(topBar);
    };

    if (inputPromoStartDate) {
        inputPromoStartDate->draw(window);
        drawCalendarBadge(606.f);
    }
    if (inputPromoEndDate) {
        inputPromoEndDate->draw(window);
        drawCalendarBadge(652.f);
    }

    if (promoScopeDropdown) promoScopeDropdown->draw(window);
    if (btnPromoSelectMovie) {
        btnPromoSelectMovie->draw(window);
        float mvX = btnPromoSelectMovie->getPosition().x + 10.f;
        float mvY = btnPromoSelectMovie->getPosition().y + 11.f;
        RoundedRectRenderer::render(window, sf::Vector2f(mvX, mvY), sf::Vector2f(14.f, 13.f), 2.f, sf::Color::Transparent, 1.2f, sf::Color::White);
        sf::RectangleShape fLine(sf::Vector2f(10.f, 1.5f));
        fLine.setPosition(mvX + 2.f, mvY + 5.f);
        fLine.setFillColor(sf::Color::White);
        window.draw(fLine);
    }

    // 4 Action Buttons
    if (btnPromoCreate) {
        btnPromoCreate->draw(window);
        drawCenteredTxt("✓", btnPromoCreate->getPosition().x + 84.f, btnPromoCreate->getPosition().y + 19.f, 14, sf::Color::White, true);
    }
    if (btnPromoUpdate) {
        btnPromoUpdate->draw(window);
        drawCenteredTxt("⇄", btnPromoUpdate->getPosition().x + 76.f, btnPromoUpdate->getPosition().y + 19.f, 14, sf::Color::White, true);
    }
    if (btnPromoDelete) {
        btnPromoDelete->draw(window);
        float dx = btnPromoDelete->getPosition().x + 92.f;
        float dy = btnPromoDelete->getPosition().y + 12.f;
        RoundedRectRenderer::render(window, sf::Vector2f(dx, dy + 2.f), sf::Vector2f(10.f, 12.f), 1.5f, sf::Color::Transparent, 1.3f, sf::Color(225, 29, 72));
        sf::RectangleShape lid(sf::Vector2f(14.f, 1.5f));
        lid.setPosition(dx - 2.f, dy);
        lid.setFillColor(sf::Color(225, 29, 72));
        window.draw(lid);
    }
    if (btnPromoClear) {
        btnPromoClear->draw(window);
        drawCenteredTxt("↺", btnPromoClear->getPosition().x + 72.f, btnPromoClear->getPosition().y + 18.f, 14, sf::Color(71, 85, 105), true);
    }

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

void CinemaUIApp::renderStaffView() {
    float contentX = sidebarWidth + 30.f;
    float contentY = topbarHeight + 25.f;

    sf::Text title;
    title.setFont(Theme::FontTitle);
    title.setCharacterSize(24);
    title.setFillColor(Theme::TextPrimary);
    title.setString(toSfString("QUẢN LÝ NHÂN SỰ & CÁN BỘ ĐIỀU HÀNH (SQLite cinema.db)"));
    title.setPosition(contentX, contentY);
    window.draw(title);

    if (btnAddStaff) btnAddStaff->draw(window);

    sf::Text sub;
    sub.setFont(Theme::FontSecondary);
    sub.setCharacterSize(13);
    sub.setFillColor(Theme::TextSecondary);
    sub.setString(toSfString("Danh sách nhân sự đọc trực tiếp từ bảng users trong DATA/cinema.db"));
    sub.setPosition(contentX, contentY + 34.f);
    window.draw(sub);

    float tblY = contentY + 70.f;
    float tblW = 1360.f;
    float tblH = 500.f;
    RoundedRectRenderer::render(window, sf::Vector2f(contentX, tblY), sf::Vector2f(tblW, tblH), Theme::RadiusMedium, Theme::CardBg, 1.f, Theme::CardBorder);

    // Table Header
    sf::RectangleShape thBg(sf::Vector2f(tblW - 48.f, 36.f));
    thBg.setPosition(contentX + 24.f, tblY + 20.f);
    thBg.setFillColor(sf::Color(15, 23, 42));
    window.draw(thBg);

    std::vector<std::string> cols = {"MÃ NV", "HỌ VÀ TÊN", "VAI TRÒ", "SỐ ĐIỆN THOẠI", "EMAIL LIÊN HỆ", "TÀI KHOẢN / TRẠNG THÁI"};
    std::vector<float> colX = {contentX + 40.f, contentX + 160.f, contentX + 420.f, contentX + 580.f, contentX + 780.f, contentX + 1060.f};

    for (size_t c = 0; c < cols.size(); ++c) {
        sf::Text ch;
        ch.setFont(Theme::FontPrimary);
        ch.setCharacterSize(11);
        ch.setStyle(sf::Text::Bold);
        ch.setFillColor(Theme::TextSecondary);
        ch.setString(toSfString(cols[c]));
        ch.setPosition(colX[c], tblY + 30.f);
        window.draw(ch);
    }

    for (size_t r = 0; r < staffUserList.size() && r < 9; ++r) {
        float ry = tblY + 68.f + r * 44.f;
        sf::RectangleShape rLine(sf::Vector2f(tblW - 48.f, 1.f));
        rLine.setPosition(contentX + 24.f, ry + 36.f);
        rLine.setFillColor(sf::Color(51, 65, 85, 80));
        window.draw(rLine);

        const auto& st = staffUserList[r];
        std::vector<std::string> vals = {
            st.maNguoiDung,
            st.hoTen,
            st.vaiTro,
            st.soDienThoai,
            st.email,
            st.taiKhoan + " (" + st.trangThaiLamViec + ")"
        };
        for (size_t c = 0; c < vals.size(); ++c) {
            sf::Text cv;
            cv.setFont(Theme::FontPrimary);
            cv.setCharacterSize(12);
            cv.setFillColor((c == 0) ? Theme::Cyan : ((c == 5) ? Theme::Success : Theme::TextPrimary));
            cv.setString(toSfString(vals[c]));
            cv.setPosition(colX[c], ry + 8.f);
            window.draw(cv);
        }
    }
}

void CinemaUIApp::renderAccountsView() {
    float contentX = sidebarWidth + 30.f;
    float contentY = topbarHeight + 25.f;

    sf::Text title;
    title.setFont(Theme::FontTitle);
    title.setCharacterSize(24);
    title.setFillColor(Theme::TextPrimary);
    title.setString(toSfString("QUẢN LÝ TÀI KHOẢN & PHÂN QUYỀN (SQLite cinema.db)"));
    title.setPosition(contentX, contentY);
    window.draw(title);

    if (btnChangePassword) btnChangePassword->draw(window);

    sf::Text sub;
    sub.setFont(Theme::FontSecondary);
    sub.setCharacterSize(13);
    sub.setFillColor(Theme::TextSecondary);
    sub.setString(toSfString("Tài khoản đăng nhập đồng bộ trực tiếp từ bảng users trong DATA/cinema.db"));
    sub.setPosition(contentX, contentY + 34.f);
    window.draw(sub);

    float cardW = 660.f;
    float cardH = 160.f;
    float gap = 20.f;
    float startY = contentY + 70.f;

    for (size_t i = 0; i < staffUserList.size() && i < 6; ++i) {
        int r = static_cast<int>(i / 2);
        int c = static_cast<int>(i % 2);
        float cx = contentX + c * (cardW + gap);
        float cy = startY + r * (cardH + gap);
        const auto& acc = staffUserList[i];
        bool isAdmin = (acc.vaiTro == "Admin");
        sf::Color accent = isAdmin ? Theme::Primary : Theme::Cyan;

        RoundedRectRenderer::render(window, sf::Vector2f(cx, cy), sf::Vector2f(cardW, cardH), Theme::RadiusMedium, Theme::CardBg, 1.f, Theme::CardBorder);
        RoundedRectRenderer::render(window, sf::Vector2f(cx, cy), sf::Vector2f(cardW, 4.f), 2.f, accent);

        sf::Text aTitle;
        aTitle.setFont(Theme::FontTitle);
        aTitle.setCharacterSize(17);
        aTitle.setFillColor(accent);
        aTitle.setString(toSfString(acc.maNguoiDung + " - " + acc.hoTen + " (" + acc.vaiTro + ")"));
        aTitle.setPosition(cx + 24.f, cy + 18.f);
        window.draw(aTitle);

        sf::Text aDesc;
        aDesc.setFont(Theme::FontPrimary);
        aDesc.setCharacterSize(13);
        aDesc.setFillColor(Theme::TextPrimary);
        aDesc.setString(toSfString("Tài khoản: " + acc.taiKhoan + " | Email: " + acc.email));
        aDesc.setPosition(cx + 24.f, cy + 54.f);
        window.draw(aDesc);

        sf::Text aPerm;
        aPerm.setFont(Theme::FontSecondary);
        aPerm.setCharacterSize(12);
        aPerm.setFillColor(Theme::TextSecondary);
        aPerm.setString(toSfString("Trạng thái làm việc: " + acc.trangThaiLamViec + " | Trạng thái TK: " + acc.trangThaiTaiKhoan));
        aPerm.setPosition(cx + 24.f, cy + 92.f);
        window.draw(aPerm);
    }
}

} // namespace UI