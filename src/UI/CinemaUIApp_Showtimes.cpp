#include "../../include/UI/CinemaUIApp.h"
#include "../../include/UI/CinemaUIApp_ShowtimeHelpers.h"
#include "../../include/ActivityLog.h"
#include "../../include/DateFormat.h"
#include <fstream>
#include <algorithm>
#include <ctime>
#include <set>
#include <cmath>

namespace UI {

static std::map<std::string, std::string> s_genreCache;
static std::map<std::string, std::string> s_movieTitleCache;
static std::map<std::string, int> s_movieDurCache;
static std::map<std::string, std::string> s_roomNameCache;
static std::map<std::string, int> s_roomSeatCache;

static bool validateShowtimeInput(
    const std::string& inputDateStr,
    const std::string& inputTimeStr,
    const std::string& roomId,
    int movieDur,
    const std::string& ignoreCode,
    const std::vector<Showtime>& stList,
    const std::vector<Movie>& mList,
    std::string& errMessage,
    std::string& outStartFmt,
    std::string& outEndFmt
) {
    int dDay = 0, dMon = 0, dYear = 0;
    if (!DateFormat::parseDate(inputDateStr, dDay, dMon, dYear)) {
        errMessage = "Định dạng ngày không hợp lệ! Vui lòng nhập định dạng DD/MM/YYYY (ví dụ: 15/11/2026).";
        return false;
    }
    if (dDay < 1 || dDay > 31 || dMon < 1 || dMon > 12 || dYear < 2020 || dYear > 2100) {
        errMessage = "Ngày chiếu không hợp lệ! Vui lòng kiểm tra lại ngày, tháng, năm.";
        return false;
    }

    // 1. Check if date is in the past
    std::time_t now = std::time(nullptr);
    std::tm* lt = std::localtime(&now);
    int curDay = lt->tm_mday;
    int curMon = lt->tm_mon + 1;
    int curYear = lt->tm_year + 1900;
    int curHour = lt->tm_hour;
    int curMin = lt->tm_min;

    int curDateKey = curYear * 10000 + curMon * 100 + curDay;
    int targetDateKey = dYear * 10000 + dMon * 100 + dDay;

    if (targetDateKey < curDateKey) {
        char todayBuf[32];
        snprintf(todayBuf, sizeof(todayBuf), "%02d/%02d/%04d", curDay, curMon, curYear);
        errMessage = "Không thể xếp lịch cho ngày trong quá khứ (" + inputDateStr + ")!\nNgày hiện tại là " + todayBuf + ". Vui lòng chọn từ hôm nay trở đi.";
        return false;
    }

    // 2. Parse start time
    int sMins = 0;
    if (!stParseTimeToMinutes(inputTimeStr, sMins)) {
        errMessage = "Định dạng giờ bắt đầu không hợp lệ! Vui lòng nhập dạng HH:MM (ví dụ: 09:30 AM hoặc 14:00).";
        return false;
    }

    // 3. Check if start time has already passed for today
    int curMins = curHour * 60 + curMin;
    if (targetDateKey == curDateKey && sMins <= curMins) {
        char curTimeBuf[16];
        snprintf(curTimeBuf, sizeof(curTimeBuf), "%02d:%02d", curHour, curMin);
        errMessage = "Giờ bắt đầu (" + stFormatHHMM(sMins) + ") đã qua thời gian hiện tại (" + curTimeBuf + ")!\nKhông thể xếp lịch cho khung giờ đã qua.";
        return false;
    }

    int eMins = sMins + movieDur;
    outStartFmt = stFormatHHMM(sMins);
    outEndFmt = stFormatHHMM(eMins);

    // 4. Check overlapping showtime in same room on same date
    for (const auto& s : stList) {
        if (!ignoreCode.empty() && s.getMaSuatChieu() == ignoreCode) continue;
        if (s.getMaPhong() != roomId) continue;
        if (s.getNgayChieu() != inputDateStr) continue;

        int exStart = 0;
        if (!stParseTimeToMinutes(s.getGioBatDau(), exStart)) continue;
        int exEnd = 0;
        if (!stParseTimeToMinutes(s.getGioKetThuc(), exEnd)) {
            int exDur = 120;
            for (const auto& m : mList) {
                if (m.getMaPhim() == s.getMaPhim()) { exDur = m.getThoiLuong(); break; }
            }
            exEnd = exStart + exDur;
        }

        // Overlap condition: newStart < exEnd && newEnd > exStart
        if (sMins < exEnd && eMins > exStart) {
            std::string pName = s.getMaPhim();
            for (const auto& m : mList) {
                if (m.getMaPhim() == s.getMaPhim()) { pName = m.getTenPhim(); break; }
            }
            errMessage = "Trùng lịch chiếu! Phòng " + roomId + " vào ngày " + inputDateStr +
                         " đã có suất chiếu " + s.getMaSuatChieu() + " (phim '" + pName + "') từ " +
                         s.getGioBatDau() + " đến " + s.getGioKetThuc() + ".\n\nVui lòng chọn khung giờ khác hoặc phòng khác!";
            return false;
        }
    }

    return true;
}

void CinemaUIApp::syncShowtimeSelectionToForm(int idx) {
    if (idx < 0 || idx >= static_cast<int>(showtimeList.size())) return;
    selectedShowtimeRowIdx = idx;
    const Showtime& st = showtimeList[idx];

    for (size_t m = 0; m < movieList.size(); ++m) {
        if (movieList[m].getMaPhim() == st.getMaPhim()) {
            stFormMovieIdx = static_cast<int>(m);
            break;
        }
    }
    for (size_t r = 0; r < roomList.size(); ++r) {
        if (roomList[r].getMaPhong() == st.getMaPhong()) {
            stFormRoomIdx = static_cast<int>(r);
            break;
        }
    }

    if (stMovieDropdown) stMovieDropdown->setSelectedIndex(stFormMovieIdx);
    if (stRoomFormDropdown) stRoomFormDropdown->setSelectedIndex(stFormRoomIdx);
    if (inputStDate) inputStDate->setText(st.getNgayChieu());
    if (inputStTime) {
        std::string startT = st.getGioBatDau();
        int h = 9, m = 0;
        if (sscanf(startT.c_str(), "%d:%d", &h, &m) >= 1) {
            char tb[16];
            snprintf(tb, sizeof(tb), "%02d:%02d %s", (h >= 12 ? (h > 12 ? h - 12 : 12) : (h == 0 ? 12 : h)), m, (h >= 12 ? "PM" : "AM"));
            inputStTime->setText(tb);
            stLastValidMinutes = h * 60 + m;
        } else {
            inputStTime->setText(startT);
        }
    }
}

void CinemaUIApp::refreshShowtimesDropdowns() {
    s_genreCache.clear();
    s_movieTitleCache.clear();
    s_movieDurCache.clear();
    for (const auto& m : movieList) {
        s_genreCache[m.getMaPhim()] = db.layTheLoaiCuaPhim(m.getMaPhim());
        s_movieTitleCache[m.getMaPhim()] = m.getTenPhim();
        s_movieDurCache[m.getMaPhim()] = m.getThoiLuong();
    }
    s_roomNameCache.clear();
    s_roomSeatCache.clear();
    for (const auto& r : roomList) {
        s_roomNameCache[r.getMaPhong()] = r.getTenPhong() + " - " + r.getLoaiPhong();
        s_roomSeatCache[r.getMaPhong()] = r.getSucChua();
    }

    if (stMovieFilterDropdown) {
        int curSel = stMovieFilterDropdown->getSelectedIndex();
        std::vector<std::string> movieFilterOpts = {"Tất cả các phim"};
        for (const auto& m : movieList) {
            movieFilterOpts.push_back(m.getMaPhim() + " - " + m.getTenPhim());
        }
        stMovieFilterDropdown->setOptions(movieFilterOpts, curSel);
    }
    if (stMovieDropdown) {
        std::vector<std::string> movieOpts;
        for (const auto& m : movieList) {
            movieOpts.push_back(m.getTenPhim() + " (" + std::to_string(m.getThoiLuong()) + " phút)");
        }
        if (movieOpts.empty()) movieOpts.push_back("Chưa có phim");
        stMovieDropdown->setOptions(movieOpts, stFormMovieIdx);
    }
    if (stDateDropdown) {
        int curSel = stDateDropdown->getSelectedIndex();
        std::string today = stTodayStr();
        std::vector<std::string> dateOpts = {"Tất cả các ngày", "Hôm nay (" + today + ")"};
        for (const auto& s : showtimeList) {
            std::string d = s.getNgayChieu();
            if (d != today && std::find(dateOpts.begin(), dateOpts.end(), d) == dateOpts.end()) {
                dateOpts.push_back(d);
            }
        }
        stDateDropdown->setOptions(dateOpts, curSel);
    }
    if (stRoomDropdown) {
        int curSel = stRoomDropdown->getSelectedIndex();
        std::vector<std::string> roomOpts = {"Tất cả các phòng"};
        for (const auto& r : roomList) {
            roomOpts.push_back(r.getMaPhong() + " - " + r.getTenPhong());
        }
        stRoomDropdown->setOptions(roomOpts, curSel);
    }
    if (stRoomFormDropdown) {
        int curSel = stRoomFormDropdown->getSelectedIndex();
        std::vector<std::string> roomFormOpts;
        for (const auto& r : roomList) {
            roomFormOpts.push_back(r.getTenPhong() + " - " + r.getLoaiPhong() + " (" + std::to_string(r.getSucChua()) + " ghế)");
        }
        if (roomFormOpts.empty()) roomFormOpts.push_back("Chưa có phòng");
        stRoomFormDropdown->setOptions(roomFormOpts, curSel);
    }
}


void CinemaUIApp::setupShowtimesView() {
    float leftColX = sidebarWidth + 24.f;  // 284.f
    float leftColW = 880.f;
    float rightColX = leftColX + leftColW + 20.f; // 1184.f
    float rightColW = 520.f;

    // Header Action Buttons (inside Title Card at y = 54.f)
    btnStExport = std::unique_ptr<RoundRectButton>(new RoundRectButton(
        "Xuất Báo Cáo (Excel / PDF)",
        sf::Vector2f(rightColX + rightColW - 360.f, 54.f),
        sf::Vector2f(190.f, 38.f),
        ButtonStyle::Ghost,
        10.f
    ));
    btnStExport->setFontSize(13);
    btnStExport->setCustomColors(sf::Color::White, sf::Color(241, 245, 249), sf::Color(226, 232, 240), sf::Color(241, 245, 249));
    btnStExport->setOutline(1.f, sf::Color(226, 232, 240));
    btnStExport->setTextColor(sf::Color(51, 65, 85));
    btnStExport->setOnClick([this]() {
        std::ofstream out("DATA/xuat_bao_cao_suat_chieu.csv");
        if (out.is_open()) {
            out << "MÃ SUẤT,MÃ PHIM,TÊN PHIM,MÃ PHÒNG,TÊN PHÒNG,NGÀY CHIẾU,GIỜ BẮT ĐẦU,GIỜ KẾT THÚC,GIÁ VÉ\n";
            for (const auto& st : showtimeList) {
                std::string pName = st.getMaPhim();
                for (const auto& m : movieList) { if (m.getMaPhim() == st.getMaPhim()) { pName = m.getTenPhim(); break; } }
                std::string rName = st.getMaPhong();
                for (const auto& r : roomList) { if (r.getMaPhong() == st.getMaPhong()) { rName = r.getTenPhong(); break; } }
                out << st.getMaSuatChieu() << "," << st.getMaPhim() << ",\"" << pName << "\"," << st.getMaPhong() << ",\"" << rName << "\","
                    << st.getNgayChieu() << "," << st.getGioBatDau() << "," << st.getGioKetThuc() << "," << static_cast<long long>(st.getGiaVeCoBan()) << "\n";
            }
            out.close();
            ActivityLog::ghiNhan(currentUserId, currentUserRole, "XUAT_BAO_CAO", "Xuat bao cao suat chieu ra CSV");
            if (confirmPopup) {
                confirmPopup->show("XUẤT BÁO CÁO THÀNH CÔNG", "Đã xuất báo cáo lịch chiếu ra tệp DATA/xuat_bao_cao_suat_chieu.csv", ButtonStyle::Primary, "Đóng", "");
            }
        }
    });

    btnStNew = std::unique_ptr<RoundRectButton>(new RoundRectButton(
        "+ Tạo Suất Chiếu Mới",
        sf::Vector2f(rightColX + rightColW - 160.f, 54.f),
        sf::Vector2f(160.f, 38.f),
        ButtonStyle::Primary,
        10.f
    ));
    btnStNew->setFontSize(13);
    btnStNew->setCustomColors(sf::Color(2, 132, 199), sf::Color(3, 105, 161), sf::Color(7, 89, 133), sf::Color(203, 213, 225));
    btnStNew->setOnClick([this]() {
        selectedShowtimeRowIdx = -1;
        if (inputStTime) inputStTime->setText("");
        if (inputStDate) inputStDate->setText(stTodayStr());
        stLastValidMinutes = -1;
    });

    // Sub-Tabs (y = ST_TAB_Y, h = 38.f)
    btnStTabList = std::unique_ptr<RoundRectButton>(new RoundRectButton(
        "Danh Sách Suất Chiếu",
        sf::Vector2f(leftColX, ST_TAB_Y),
        sf::Vector2f(230.f, 38.f),
        ButtonStyle::Primary,
        10.f
    ));
    btnStTabList->setFontSize(13);
    btnStTabList->setOnClick([this]() { showtimeSubTab = 0; showtimeCurrentPage = 0; });

    btnStTabTimeline = std::unique_ptr<RoundRectButton>(new RoundRectButton(
        "Timeline Phòng Chiếu",
        sf::Vector2f(leftColX + 240.f, ST_TAB_Y),
        sf::Vector2f(230.f, 38.f),
        ButtonStyle::Ghost,
        10.f
    ));
    btnStTabTimeline->setFontSize(13);
    btnStTabTimeline->setCustomColors(sf::Color::White, sf::Color(241, 245, 249), sf::Color(226, 232, 240), sf::Color(241, 245, 249));
    btnStTabTimeline->setOutline(1.f, sf::Color(226, 232, 240));
    btnStTabTimeline->setTextColor(sf::Color(71, 85, 105));
    btnStTabTimeline->setOnClick([this]() { showtimeSubTab = 1; showtimeCurrentPage = 0; });

    btnStTabByMovie = std::unique_ptr<RoundRectButton>(new RoundRectButton(
        "Lịch Chiếu Theo Phim",
        sf::Vector2f(leftColX + 480.f, ST_TAB_Y),
        sf::Vector2f(200.f, 38.f),
        ButtonStyle::Ghost,
        10.f
    ));
    btnStTabByMovie->setFontSize(13);
    btnStTabByMovie->setCustomColors(sf::Color::White, sf::Color(241, 245, 249), sf::Color(226, 232, 240), sf::Color(241, 245, 249));
    btnStTabByMovie->setOutline(1.f, sf::Color(226, 232, 240));
    btnStTabByMovie->setTextColor(sf::Color(71, 85, 105));
    btnStTabByMovie->setOnClick([this]() { showtimeSubTab = 2; showtimeCurrentPage = 0; });

    // Filter Bar (y = ST_FILTER_Y, h = 38.f) - Perfectly spanning 880px
    stSearchInput = std::unique_ptr<UIInputBox>(new UIInputBox(
        sf::Vector2f(leftColX, ST_FILTER_Y),
        sf::Vector2f(230.f, 38.f),
        "Tìm phim, phòng, mã ST...",
        12
    ));
    stSearchInput->setTextInset(32.f);
    stSearchInput->setColors(sf::Color::White, sf::Color(226, 232, 240), sf::Color(2, 132, 199), sf::Color(15, 23, 42), sf::Color(148, 163, 184));
    stSearchInput->setCornerRadius(8.f);

    std::vector<std::string> movieFilterOpts = {"Tất cả các phim"};
    for (const auto& m : movieList) {
        movieFilterOpts.push_back(m.getMaPhim() + " - " + m.getTenPhim());
    }
    stMovieFilterDropdown = std::unique_ptr<DropdownBox>(new DropdownBox(
        sf::Vector2f(leftColX + 240.f, ST_FILTER_Y),
        sf::Vector2f(190.f, 38.f),
        movieFilterOpts,
        0
    ));
    stMovieFilterDropdown->setFontSize(12);
    stMovieFilterDropdown->setLightStyle(true);
    stMovieFilterDropdown->setMaxVisibleItems(6);
    stMovieFilterDropdown->setOnSelect([this](int idx, const std::string& val) {
        if (idx == 0) stMovieFilterVal = "";
        else if (idx - 1 < static_cast<int>(movieList.size())) {
            stMovieFilterVal = movieList[idx - 1].getMaPhim();
        }
        showtimeCurrentPage = 0;
    });

    std::string today = stTodayStr();
    std::vector<std::string> dateOpts = {"Tất cả các ngày", "Hôm nay (" + today + ")"};
    std::set<std::string> seenDates;
    for (const auto& st : showtimeList) {
        std::string d = st.getNgayChieu();
        if (!d.empty() && d != today && seenDates.find(d) == seenDates.end()) {
            seenDates.insert(d);
            dateOpts.push_back(d);
        }
    }
    stDateDropdown = std::unique_ptr<DropdownBox>(new DropdownBox(
        sf::Vector2f(leftColX + 440.f, ST_FILTER_Y),
        sf::Vector2f(180.f, 38.f),
        dateOpts,
        0
    ));
    stDateDropdown->setFontSize(12);
    stDateDropdown->setLightStyle(true);
    stDateDropdown->setMaxVisibleItems(5);
    stDateDropdown->setOnSelect([this](int idx, const std::string& val) {
        if (idx == 0) stDateFilterVal = "";
        else if (idx == 1) stDateFilterVal = stTodayStr();
        else stDateFilterVal = val;
        showtimeCurrentPage = 0;
    });

    // btnStTodayQuick removed as requested: "Xoá khung chữ hôm nay bên cạnh lọc ngày"
    btnStTodayQuick.reset();

    std::vector<std::string> roomFilterOpts = {"Tất cả phòng"};
    for (const auto& r : roomList) {
        roomFilterOpts.push_back(r.getMaPhong() + " (" + r.getTenPhong() + ")");
    }
    stRoomDropdown = std::unique_ptr<DropdownBox>(new DropdownBox(
        sf::Vector2f(leftColX + 630.f, ST_FILTER_Y),
        sf::Vector2f(198.f, 38.f),
        roomFilterOpts,
        0
    ));
    stRoomDropdown->setFontSize(12);
    stRoomDropdown->setLightStyle(true);
    stRoomDropdown->setMaxVisibleItems(5);
    stRoomDropdown->setOnSelect([this](int idx, const std::string& val) {
        if (idx == 0) stRoomFilterVal = "";
        else if (idx - 1 < static_cast<int>(roomList.size())) {
            stRoomFilterVal = roomList[idx - 1].getMaPhong();
        }
        showtimeCurrentPage = 0;
    });

    btnStConfig = std::unique_ptr<RoundRectButton>(new RoundRectButton(
        "",
        sf::Vector2f(leftColX + 838.f, ST_FILTER_Y),
        sf::Vector2f(42.f, 38.f),
        ButtonStyle::Ghost,
        8.f
    ));
    btnStConfig->setCustomColors(sf::Color::White, sf::Color(241, 245, 249), sf::Color(226, 232, 240), sf::Color(241, 245, 249));
    btnStConfig->setOutline(1.f, sf::Color(226, 232, 240));
    btnStConfig->setOnClick([this]() {
        if (stSearchInput) stSearchInput->setText("");
        if (stMovieFilterDropdown) stMovieFilterDropdown->setSelectedIndex(0);
        if (stDateDropdown) stDateDropdown->setSelectedIndex(0);
        if (stRoomDropdown) stRoomDropdown->setSelectedIndex(0);
        stMovieFilterVal = "";
        stDateFilterVal = "";
        stRoomFilterVal = "";
        showtimeCurrentPage = 0;
        refreshShowtimesDropdowns();
    });

    // Form Inputs on Right Card
    float formX = rightColX + 20.f;
    float formW = rightColW - 40.f; // 480.f

    // 1. Movie Dropdown
    std::vector<std::string> movieOpts;
    for (const auto& m : movieList) {
        movieOpts.push_back(m.getTenPhim() + " (" + std::to_string(m.getThoiLuong()) + " phút)");
    }
    if (movieOpts.empty()) movieOpts.push_back("Chưa có phim");
    stMovieDropdown = std::unique_ptr<DropdownBox>(new DropdownBox(
        sf::Vector2f(formX, ST_FORM_MOVIE_Y),
        sf::Vector2f(formW, 38.f),
        movieOpts,
        0
    ));
    stMovieDropdown->setFontSize(13);
    stMovieDropdown->setLightStyle(true);
    stMovieDropdown->setMaxVisibleItems(6);
    stMovieDropdown->setOnSelect([this](int idx, const std::string& val) {
        if (idx >= 0 && idx < static_cast<int>(movieList.size())) {
            stFormMovieIdx = idx;
        }
    });

    // 2. Room Dropdown
    std::vector<std::string> roomFormOpts;
    for (const auto& r : roomList) {
        roomFormOpts.push_back(r.getTenPhong() + " - " + r.getLoaiPhong() + " (" + std::to_string(r.getSucChua()) + " ghế)");
    }
    if (roomFormOpts.empty()) roomFormOpts.push_back("Chưa có phòng");
    stRoomFormDropdown = std::unique_ptr<DropdownBox>(new DropdownBox(
        sf::Vector2f(formX, ST_FORM_ROOM_Y),
        sf::Vector2f(formW, 38.f),
        roomFormOpts,
        0
    ));
    stRoomFormDropdown->setFontSize(13);
    stRoomFormDropdown->setLightStyle(true);
    stRoomFormDropdown->setMaxVisibleItems(5);
    stRoomFormDropdown->setOnSelect([this](int idx, const std::string& val) {
        if (idx >= 0 && idx < static_cast<int>(roomList.size())) {
            stFormRoomIdx = idx;
        }
    });

    // 3. Row: Date & Start Time
    float halfFormW = (formW - 14.f) / 2.f; // 233.f
    inputStDate = std::unique_ptr<UIInputBox>(new UIInputBox(
        sf::Vector2f(formX, ST_FORM_DATE_Y),
        sf::Vector2f(halfFormW, 38.f),
        stTodayStr(),
        13
    ));
    inputStDate->setTextInset(14.f);
    inputStDate->setColors(sf::Color(248, 250, 252), sf::Color(203, 213, 225), sf::Color(2, 132, 199), sf::Color(15, 23, 42), sf::Color(148, 163, 184));
    inputStDate->setCornerRadius(8.f);
    inputStDate->setText(stTodayStr());

    inputStTime = std::unique_ptr<UIInputBox>(new UIInputBox(
        sf::Vector2f(formX + halfFormW + 14.f, ST_FORM_DATE_Y),
        sf::Vector2f(halfFormW, 38.f),
        "Nhập giờ (vd: 09:30)...",
        13
    ));
    inputStTime->setTextInset(14.f);
    inputStTime->setColors(sf::Color(248, 250, 252), sf::Color(203, 213, 225), sf::Color(2, 132, 199), sf::Color(15, 23, 42), sf::Color(148, 163, 184));
    inputStTime->setCornerRadius(8.f);
    inputStTime->setText("");

    stCleaningMinutes = 0;
    btnStCleanMinus.reset();
    btnStCleanPlus.reset();
    btnStPreset10.reset();
    btnStPreset15.reset();
    btnStPreset20.reset();

    // 4. Form Action Buttons
    float aBtnW = (formW - 24.f) / 4.f; // ~114.f
    btnStSave = std::unique_ptr<RoundRectButton>(new RoundRectButton(
        "Cập nhật",
        sf::Vector2f(formX, ST_FORM_BTN_ROW_Y),
        sf::Vector2f(aBtnW, 40.f),
        ButtonStyle::Primary,
        8.f
    ));
    btnStSave->setFontSize(13);
    btnStSave->setCustomColors(sf::Color(2, 132, 199), sf::Color(3, 105, 161), sf::Color(7, 89, 133), sf::Color(203, 213, 225));
    btnStSave->setOnClick([this]() {
        if (selectedShowtimeRowIdx < 0 || selectedShowtimeRowIdx >= static_cast<int>(showtimeList.size())) {
            if (confirmPopup) confirmPopup->show("THÔNG BÁO", "Vui lòng chọn một suất chiếu trong bảng để chỉnh sửa!", ButtonStyle::Primary, "Đóng", "");
            return;
        }
        int mIdx = stMovieDropdown ? stMovieDropdown->getSelectedIndex() : stFormMovieIdx;
        if (mIdx < 0 || mIdx >= static_cast<int>(movieList.size())) mIdx = 0;
        std::string mId = !movieList.empty() ? movieList[mIdx].getMaPhim() : "P01";
        int dur = !movieList.empty() ? movieList[mIdx].getThoiLuong() : 120;

        int rIdx = stRoomFormDropdown ? stRoomFormDropdown->getSelectedIndex() : stFormRoomIdx;
        if (rIdx < 0 || rIdx >= static_cast<int>(roomList.size())) rIdx = 0;
        std::string rId = !roomList.empty() ? roomList[rIdx].getMaPhong() : "R01";

        std::string date = inputStDate ? inputStDate->getText() : stTodayStr();
        if (date.empty()) date = stTodayStr();
        std::string startT = inputStTime ? inputStTime->getText() : "";
        if (startT.empty()) {
            if (confirmPopup) confirmPopup->show("THÔNG BÁO", "Vui lòng tự nhập giờ bắt đầu suất chiếu (ví dụ: 09:30 hoặc 14:00)!", ButtonStyle::Danger, "Đóng", "");
            return;
        }

        Showtime& cur = showtimeList[selectedShowtimeRowIdx];
        std::string err;
        std::string sFmt, eFmt;
        if (!validateShowtimeInput(date, startT, rId, dur, cur.getMaSuatChieu(), showtimeList, movieList, err, sFmt, eFmt)) {
            if (confirmPopup) confirmPopup->show("LỖI KHÔNG HỢP LỆ", err, ButtonStyle::Danger, "Đóng", "");
            return;
        }

        cur = Showtime(cur.getMaSuatChieu(), mId, rId, date, sFmt, eFmt, cur.getGiaVeCoBan() > 0 ? cur.getGiaVeCoBan() : 85000.0);
        db.ghiDanhSachSuatChieu(showtimeList);
        ActivityLog::ghiNhan(currentUserId, currentUserRole, "CAP_NHAT_SUAT_CHIEU", "Cap nhat suat: " + cur.getMaSuatChieu());
        refreshShowtimesDropdowns();
        syncShowtimeSelectionToForm(selectedShowtimeRowIdx);
        if (confirmPopup) confirmPopup->show("THÀNH CÔNG", "Đã cập nhật suất chiếu " + cur.getMaSuatChieu() + " thành công!", ButtonStyle::Primary, "Đóng", "");
    });

    btnStCreate = std::unique_ptr<RoundRectButton>(new RoundRectButton(
        "+ Thêm mới",
        sf::Vector2f(formX + aBtnW + 8.f, ST_FORM_BTN_ROW_Y),
        sf::Vector2f(aBtnW, 40.f),
        ButtonStyle::Success,
        8.f
    ));
    btnStCreate->setFontSize(13);
    btnStCreate->setCustomColors(sf::Color(22, 163, 74), sf::Color(21, 128, 61), sf::Color(15, 118, 54), sf::Color(203, 213, 225));
    btnStCreate->setOnClick([this]() {
        int mIdx = stMovieDropdown ? stMovieDropdown->getSelectedIndex() : stFormMovieIdx;
        if (mIdx < 0 || mIdx >= static_cast<int>(movieList.size())) mIdx = 0;
        std::string mId = !movieList.empty() ? movieList[mIdx].getMaPhim() : "P01";
        int dur = !movieList.empty() ? movieList[mIdx].getThoiLuong() : 120;

        int rIdx = stRoomFormDropdown ? stRoomFormDropdown->getSelectedIndex() : stFormRoomIdx;
        if (rIdx < 0 || rIdx >= static_cast<int>(roomList.size())) rIdx = 0;
        std::string rId = !roomList.empty() ? roomList[rIdx].getMaPhong() : "R01";

        std::string date = inputStDate ? inputStDate->getText() : stTodayStr();
        if (date.empty()) date = stTodayStr();
        std::string startT = inputStTime ? inputStTime->getText() : "";
        if (startT.empty()) {
            if (confirmPopup) confirmPopup->show("THÔNG BÁO", "Vui lòng tự nhập giờ bắt đầu suất chiếu (ví dụ: 09:30 hoặc 14:00)!", ButtonStyle::Danger, "Đóng", "");
            return;
        }

        std::string err;
        std::string sFmt, eFmt;
        if (!validateShowtimeInput(date, startT, rId, dur, "", showtimeList, movieList, err, sFmt, eFmt)) {
            if (confirmPopup) confirmPopup->show("LỖI KHÔNG HỢP LỆ", err, ButtonStyle::Danger, "Đóng", "");
            return;
        }

        int nextNum = 1;
        std::string nextCode;
        bool exists = false;
        do {
            char buf[16];
            snprintf(buf, sizeof(buf), "SC%02d", nextNum++);
            nextCode = buf;
            exists = false;
            for (const auto& s : showtimeList) { if (s.getMaSuatChieu() == nextCode) { exists = true; break; } }
        } while (exists);

        Showtime newSt(nextCode, mId, rId, date, sFmt, eFmt, 85000.0);
        showtimeList.push_back(newSt);
        db.ghiDanhSachSuatChieu(showtimeList);
        refreshShowtimesDropdowns();
        selectedShowtimeRowIdx = static_cast<int>(showtimeList.size()) - 1;
        showtimeCurrentPage = selectedShowtimeRowIdx / 4;
        syncShowtimeSelectionToForm(selectedShowtimeRowIdx);
        ActivityLog::ghiNhan(currentUserId, currentUserRole, "THEM_SUAT_CHIEU", "Them suat chieu moi: " + nextCode);
        if (confirmPopup) confirmPopup->show("THÀNH CÔNG", "Đã tạo suất chiếu mới: " + nextCode, ButtonStyle::Primary, "Đóng", "");
    });

    btnStDelete = std::unique_ptr<RoundRectButton>(new RoundRectButton(
        "Xóa suất",
        sf::Vector2f(formX + (aBtnW + 8.f) * 2.f, ST_FORM_BTN_ROW_Y),
        sf::Vector2f(aBtnW, 40.f),
        ButtonStyle::Danger,
        8.f
    ));
    btnStDelete->setFontSize(13);
    btnStDelete->setCustomColors(sf::Color(254, 242, 242), sf::Color(254, 226, 226), sf::Color(252, 165, 165), sf::Color(241, 245, 249));
    btnStDelete->setOutline(1.f, sf::Color(254, 202, 202));
    btnStDelete->setTextColor(sf::Color(220, 38, 38));
    btnStDelete->setOnClick([this]() {
        if (selectedShowtimeRowIdx >= 0 && selectedShowtimeRowIdx < static_cast<int>(showtimeList.size())) {
            std::string code = showtimeList[selectedShowtimeRowIdx].getMaSuatChieu();
            showtimeList.erase(showtimeList.begin() + selectedShowtimeRowIdx);
            db.ghiDanhSachSuatChieu(showtimeList);
            refreshShowtimesDropdowns();
            if (selectedShowtimeRowIdx >= static_cast<int>(showtimeList.size())) {
                selectedShowtimeRowIdx = static_cast<int>(showtimeList.size()) - 1;
            }
            if (selectedShowtimeRowIdx >= 0) {
                syncShowtimeSelectionToForm(selectedShowtimeRowIdx);
            }
            ActivityLog::ghiNhan(currentUserId, currentUserRole, "XOA_SUAT_CHIEU", "Xoa suat chieu: " + code);
            if (confirmPopup) confirmPopup->show("THÀNH CÔNG", "Đã xóa suất chiếu " + code + "!", ButtonStyle::Primary, "Đóng", "");
        } else {
            if (confirmPopup) confirmPopup->show("THÔNG BÁO", "Vui lòng chọn một suất chiếu để xóa!", ButtonStyle::Primary, "Đóng", "");
        }
    });

    btnStReset = std::unique_ptr<RoundRectButton>(new RoundRectButton(
        "Đặt lại",
        sf::Vector2f(formX + (aBtnW + 8.f) * 3.f, ST_FORM_BTN_ROW_Y),
        sf::Vector2f(aBtnW, 40.f),
        ButtonStyle::Ghost,
        8.f
    ));
    btnStReset->setFontSize(13);
    btnStReset->setCustomColors(sf::Color::White, sf::Color(241, 245, 249), sf::Color(226, 232, 240), sf::Color(241, 245, 249));
    btnStReset->setOutline(1.f, sf::Color(226, 232, 240));
    btnStReset->setTextColor(sf::Color(71, 85, 105));
    btnStReset->setOnClick([this]() {
        selectedShowtimeRowIdx = -1;
        stFormMovieIdx = 0;
        stFormRoomIdx = 0;
        if (stMovieDropdown) stMovieDropdown->setSelectedIndex(0);
        if (stRoomFormDropdown) stRoomFormDropdown->setSelectedIndex(0);
        if (inputStTime) inputStTime->setText("");
        if (inputStDate) inputStDate->setText(stTodayStr());
        stLastValidMinutes = -1;
    });

    refreshShowtimesDropdowns();
    selectedShowtimeRowIdx = -1;
    if (inputStTime) inputStTime->setText("");
}

void CinemaUIApp::renderShowtimesView() {
    float contentAreaW = 1728.f - sidebarWidth; // 1468.f
    float leftColX = sidebarWidth + 24.f;       // 284.f
    float leftColW = 880.f;
    float rightColX = leftColX + leftColW + 20.f; // 1184.f
    float rightColW = 520.f;

    // 1. Soft slate-white canvas background (#f8fafc)
    window.clear(sf::Color(248, 250, 252));

    // ==========================================
    // 2. TOP BREADCRUMB & USER STRIP (y = 12.f)
    // ==========================================
    float topNavY = 12.f;

    // Location pin dot + Cinema branch
    sf::CircleShape pinDot(3.5f);
    pinDot.setFillColor(sf::Color(16, 185, 129));
    pinDot.setPosition(leftColX, topNavY + 3.f);
    window.draw(pinDot);

    sf::Text brBranch(toSfString("Cinestar Quốc Thanh (TP.HCM)"), Theme::FontPrimary, 11);
    brBranch.setStyle(sf::Text::Bold);
    brBranch.setFillColor(sf::Color(15, 23, 42));
    brBranch.setPosition(leftColX + 12.f, topNavY);
    window.draw(brBranch);

    // Separator dot
    sf::CircleShape sepDot(2.f);
    sepDot.setFillColor(sf::Color(203, 213, 225));
    sepDot.setPosition(leftColX + brBranch.getLocalBounds().width + 20.f, topNavY + 5.f);
    window.draw(sepDot);

    // Breadcrumb path
    sf::Text brPath(toSfString("Bảng điều khiển  >  Quản lý lịch chiếu  >  Quản Lý Suất Chiếu"), Theme::FontPrimary, 11);
    brPath.setFillColor(sf::Color(100, 116, 139));
    brPath.setPosition(leftColX + brBranch.getLocalBounds().width + 30.f, topNavY);
    window.draw(brPath);

    // Right User Pill
    float uPillW = 240.f;
    float uPillX = rightColX + rightColW - uPillW;
    RoundedRectRenderer::render(window, sf::Vector2f(uPillX, topNavY - 4.f), sf::Vector2f(uPillW, 26.f), 13.f, sf::Color::White, 1.f, sf::Color(226, 232, 240));

    sf::CircleShape uAv(8.f);
    uAv.setFillColor(sf::Color(2, 132, 199));
    uAv.setPosition(uPillX + 4.f, topNavY + 1.f);
    window.draw(uAv);

    sf::Text uIni(toSfString("LT"), Theme::FontPrimary, 9);
    uIni.setStyle(sf::Text::Bold);
    uIni.setFillColor(sf::Color::White);
    uIni.setPosition(uPillX + 6.5f, topNavY + 2.f);
    window.draw(uIni);

    std::string dispUser = currentUserName.empty() ? "Lê Minh Tuấn" : currentUserName;
    std::string dispRole = currentUserRole.empty() ? "Trưởng Ca Vận Hành" : currentUserRole;
    sf::Text uName(toSfString(dispUser + " (" + dispRole + ")"), Theme::FontPrimary, 10);
    uName.setStyle(sf::Text::Bold);
    uName.setFillColor(sf::Color(30, 41, 59));
    uName.setPosition(uPillX + 24.f, topNavY + 1.f);
    window.draw(uName);

    // ==========================================
    // 3. MAIN HEADER TITLE CARD (y = ST_HEADER_Y = 38.f, h = 68.f)
    // ==========================================
    float titleCardY = ST_HEADER_Y;
    float titleCardH = 68.f;
    float totalW = leftColW + 20.f + rightColW; // 1420.f
    RoundedRectRenderer::render(window, sf::Vector2f(leftColX, titleCardY), sf::Vector2f(totalW, titleCardH), 12.f, sf::Color::White, 1.f, sf::Color(226, 232, 240));

    // Left Icon Square (Sky Blue with Clock/Calendar)
    float iconBoxX = leftColX + 16.f;
    float iconBoxY = titleCardY + 12.f;
    RoundedRectRenderer::render(window, sf::Vector2f(iconBoxX, iconBoxY), sf::Vector2f(44.f, 44.f), 10.f, sf::Color(2, 132, 199));

    // Clock icon
    sf::CircleShape cClock(10.f);
    cClock.setPosition(iconBoxX + 12.f, iconBoxY + 12.f);
    cClock.setFillColor(sf::Color::Transparent);
    cClock.setOutlineThickness(2.f);
    cClock.setOutlineColor(sf::Color::White);
    window.draw(cClock);

    sf::RectangleShape cHandH(sf::Vector2f(6.f, 2.f));
    cHandH.setPosition(iconBoxX + 22.f, iconBoxY + 22.f);
    cHandH.setFillColor(sf::Color::White);
    window.draw(cHandH);

    sf::RectangleShape cHandV(sf::Vector2f(2.f, 6.f));
    cHandV.setPosition(iconBoxX + 22.f, iconBoxY + 16.f);
    cHandV.setFillColor(sf::Color::White);
    window.draw(cHandV);

    // Title: QUẢN LÝ SUẤT CHIẾU
    sf::Text mainTitle(toSfString("QUẢN LÝ SUẤT CHIẾU"), Theme::FontTitle, 20);
    mainTitle.setStyle(sf::Text::Bold);
    mainTitle.setFillColor(sf::Color(15, 23, 42));
    mainTitle.setPosition(leftColX + 72.f, titleCardY + 11.f);
    window.draw(mainTitle);

    // Realtime Sync Badge
    float rBadgeX = leftColX + 72.f + mainTitle.getLocalBounds().width + 12.f;
    float rBadgeY = titleCardY + 13.f;
    RoundedRectRenderer::render(window, sf::Vector2f(rBadgeX, rBadgeY), sf::Vector2f(160.f, 22.f), 11.f, sf::Color(236, 253, 245), 1.f, sf::Color(167, 243, 208));

    sf::CircleShape rDot(3.f);
    rDot.setFillColor(sf::Color(16, 185, 129));
    rDot.setPosition(rBadgeX + 8.f, rBadgeY + 8.f);
    window.draw(rDot);

    sf::Text rBadgeTxt(toSfString("Dữ Liệu Đồng Bộ Realtime"), Theme::FontPrimary, 10);
    rBadgeTxt.setStyle(sf::Text::Bold);
    rBadgeTxt.setFillColor(sf::Color(5, 150, 105));
    rBadgeTxt.setPosition(rBadgeX + 18.f, rBadgeY + 4.f);
    window.draw(rBadgeTxt);

    // Subtitle
    sf::Text mainSub(toSfString("Quản trị lịch chiếu phim, phân bổ phòng chiếu và điều phối khung giờ vận hành CineOS"), Theme::FontSecondary, 12);
    mainSub.setFillColor(sf::Color(100, 116, 139));
    mainSub.setPosition(leftColX + 72.f, titleCardY + 37.f);
    window.draw(mainSub);

    // Top action buttons (Excel export & Create new)
    if (btnStExport) btnStExport->draw(window);
    if (btnStNew) btnStNew->draw(window);

    // ==========================================
    // 4. SUB-TABS BAR (y = ST_TAB_Y = 118.f, h = 38.f)
    // ==========================================
    if (btnStTabList) {
        btnStTabList->setCustomColors(showtimeSubTab == 0 ? sf::Color(2, 132, 199) : sf::Color::White,
                                      sf::Color(241, 245, 249), sf::Color(226, 232, 240),
                                      showtimeSubTab == 0 ? sf::Color(2, 132, 199) : sf::Color::White);
        btnStTabList->setTextColor(showtimeSubTab == 0 ? sf::Color::White : sf::Color(71, 85, 105));
        btnStTabList->setOutline(showtimeSubTab == 0 ? 0.f : 1.f, sf::Color(226, 232, 240));
        btnStTabList->draw(window);

        // Badge count inside Tab 1
        char cBuf[16];
        snprintf(cBuf, sizeof(cBuf), "%02zu", showtimeList.size());
        RoundedRectRenderer::render(window, sf::Vector2f(leftColX + 188.f, ST_TAB_Y + 8.f), sf::Vector2f(32.f, 22.f), 11.f,
                                    showtimeSubTab == 0 ? sf::Color(3, 105, 161) : sf::Color(241, 245, 249));
        sf::Text cntTxt(cBuf, Theme::FontPrimary, 11);
        cntTxt.setStyle(sf::Text::Bold);
        cntTxt.setFillColor(showtimeSubTab == 0 ? sf::Color::White : sf::Color(100, 116, 139));
        cntTxt.setPosition(leftColX + 195.f, ST_TAB_Y + 11.f);
        window.draw(cntTxt);
    }

    if (btnStTabTimeline) {
        btnStTabTimeline->setCustomColors(showtimeSubTab == 1 ? sf::Color(2, 132, 199) : sf::Color::White,
                                          sf::Color(241, 245, 249), sf::Color(226, 232, 240),
                                          showtimeSubTab == 1 ? sf::Color(2, 132, 199) : sf::Color::White);
        btnStTabTimeline->setTextColor(showtimeSubTab == 1 ? sf::Color::White : sf::Color(71, 85, 105));
        btnStTabTimeline->setOutline(showtimeSubTab == 1 ? 0.f : 1.f, sf::Color(226, 232, 240));
        btnStTabTimeline->draw(window);

        // Badge inside Tab 2
        char rBuf[16];
        snprintf(rBuf, sizeof(rBuf), "%02zu", roomList.size());
        RoundedRectRenderer::render(window, sf::Vector2f(leftColX + 240.f + 188.f, ST_TAB_Y + 8.f), sf::Vector2f(32.f, 22.f), 11.f,
                                    showtimeSubTab == 1 ? sf::Color(3, 105, 161) : sf::Color(241, 245, 249));
        sf::Text tmTxt(rBuf, Theme::FontPrimary, 11);
        tmTxt.setStyle(sf::Text::Bold);
        tmTxt.setFillColor(showtimeSubTab == 1 ? sf::Color::White : sf::Color(100, 116, 139));
        tmTxt.setPosition(leftColX + 240.f + 195.f, ST_TAB_Y + 11.f);
        window.draw(tmTxt);
    }

    if (btnStTabByMovie) {
        btnStTabByMovie->setCustomColors(showtimeSubTab == 2 ? sf::Color(2, 132, 199) : sf::Color::White,
                                         sf::Color(241, 245, 249), sf::Color(226, 232, 240),
                                         showtimeSubTab == 2 ? sf::Color(2, 132, 199) : sf::Color::White);
        btnStTabByMovie->setTextColor(showtimeSubTab == 2 ? sf::Color::White : sf::Color(71, 85, 105));
        btnStTabByMovie->setOutline(showtimeSubTab == 2 ? 0.f : 1.f, sf::Color(226, 232, 240));
        btnStTabByMovie->draw(window);
    }

    // ==========================================
    // 5. FILTER BAR (y = ST_FILTER_Y = 166.f, h = 38.f)
    // ==========================================
    if (stSearchInput) stSearchInput->draw(window);
    // Mini search lens icon
    sf::CircleShape lens(4.5f);
    lens.setFillColor(sf::Color::Transparent);
    lens.setOutlineThickness(1.5f);
    lens.setOutlineColor(sf::Color(148, 163, 184));
    lens.setPosition(leftColX + 14.f, ST_FILTER_Y + 11.f);
    window.draw(lens);
    sf::RectangleShape handle(sf::Vector2f(4.f, 1.5f));
    handle.rotate(45.f);
    handle.setFillColor(sf::Color(148, 163, 184));
    handle.setPosition(leftColX + 21.f, ST_FILTER_Y + 19.f);
    window.draw(handle);

    if (stMovieFilterDropdown) stMovieFilterDropdown->draw(window);
    if (stDateDropdown) stDateDropdown->draw(window);
    if (stRoomDropdown) stRoomDropdown->draw(window);
    if (btnStConfig) {
        btnStConfig->draw(window);
        // Little gear/filter icon inside config button
        sf::CircleShape gear(6.f);
        gear.setFillColor(sf::Color::Transparent);
        gear.setOutlineThickness(1.8f);
        gear.setOutlineColor(sf::Color(100, 116, 139));
        gear.setPosition(leftColX + 850.f, ST_FILTER_Y + 13.f);
        window.draw(gear);
        sf::CircleShape gearHole(2.f);
        gearHole.setFillColor(sf::Color(100, 116, 139));
        gearHole.setPosition(leftColX + 854.f, ST_FILTER_Y + 17.f);
        window.draw(gearHole);
    }

    // ==========================================
    // 6. MAIN CONTENT CARDS (y = ST_CARD_Y = 214.f, h = ST_CARD_H = 616.f)
    // ==========================================
    float cardY = ST_CARD_Y;
    float cardH = ST_CARD_H;

    // ------------------------------------------
    // LEFT CARD: Danh Sách Suất Chiếu Trực Quan / Timeline / Lịch Theo Phim
    // ------------------------------------------
    RoundedRectRenderer::render(window, sf::Vector2f(leftColX, cardY), sf::Vector2f(leftColW, cardH), 12.f, sf::Color::White, 1.f, sf::Color(226, 232, 240));

    if (showtimeSubTab == 0) {
        // ==========================================
        // SUBTAB 0: DANH SÁCH BẢNG SUẤT CHIẾU TRỰC QUAN
        // ==========================================
        sf::CircleShape lDot(4.f);
        lDot.setFillColor(sf::Color(2, 132, 199));
        lDot.setPosition(leftColX + 20.f, cardY + 18.f);
        window.draw(lDot);

        sf::Text lTitle(toSfString("Danh Sách Suất Chiếu Trực Quan"), Theme::FontPrimary, 15);
        lTitle.setStyle(sf::Text::Bold);
        lTitle.setFillColor(sf::Color(15, 23, 42));
        lTitle.setPosition(leftColX + 34.f, cardY + 12.f);
        window.draw(lTitle);

        char cntTodayBuf[32];
        snprintf(cntTodayBuf, sizeof(cntTodayBuf), "%zu Suất trong hệ thống", showtimeList.size());
        float cTBX = leftColX + 34.f + lTitle.getLocalBounds().width + 10.f;
        RoundedRectRenderer::render(window, sf::Vector2f(cTBX, cardY + 13.f), sf::Vector2f(145.f, 22.f), 11.f, sf::Color(224, 242, 254), 1.f, sf::Color(186, 230, 253));
        sf::Text cTBTxt(toSfString(cntTodayBuf), Theme::FontPrimary, 10);
        cTBTxt.setStyle(sf::Text::Bold);
        cTBTxt.setFillColor(sf::Color(2, 132, 199));
        cTBTxt.setPosition(cTBX + 8.f, cardY + 16.f);
        window.draw(cTBTxt);

        sf::Text lNote(toSfString("Nhấp Sửa hoặc Xóa trên từng hàng"), Theme::FontSecondary, 11);
        lNote.setFillColor(sf::Color(148, 163, 184));
        lNote.setPosition(leftColX + leftColW - lNote.getLocalBounds().width - 20.f, cardY + 14.f);
        window.draw(lNote);

        // Table Column Header Row
        float tblHdrY = cardY + 44.f;
        RoundedRectRenderer::render(window, sf::Vector2f(leftColX + 8.f, tblHdrY), sf::Vector2f(leftColW - 16.f, 28.f), 6.f, sf::Color(248, 250, 252));

        sf::Text th1(toSfString("MÃ ST"), Theme::FontPrimary, 11);
        th1.setStyle(sf::Text::Bold); th1.setFillColor(sf::Color(100, 116, 139));
        th1.setPosition(leftColX + 24.f, tblHdrY + 6.f);
        window.draw(th1);

        sf::Text th2(toSfString("PHIM & ĐỊNH DẠNG"), Theme::FontPrimary, 11);
        th2.setStyle(sf::Text::Bold); th2.setFillColor(sf::Color(100, 116, 139));
        th2.setPosition(leftColX + 96.f, tblHdrY + 6.f);
        window.draw(th2);

        sf::Text th3(toSfString("PHÒNG CHIẾU"), Theme::FontPrimary, 11);
        th3.setStyle(sf::Text::Bold); th3.setFillColor(sf::Color(100, 116, 139));
        th3.setPosition(leftColX + 410.f, tblHdrY + 6.f);
        window.draw(th3);

        sf::Text th4(toSfString("KHUNG GIỜ CHIẾU"), Theme::FontPrimary, 11);
        th4.setStyle(sf::Text::Bold); th4.setFillColor(sf::Color(100, 116, 139));
        th4.setPosition(leftColX + 600.f, tblHdrY + 6.f);
        window.draw(th4);

        sf::Text th5(toSfString("THAO TÁC"), Theme::FontPrimary, 11);
        th5.setStyle(sf::Text::Bold); th5.setFillColor(sf::Color(100, 116, 139));
        th5.setPosition(leftColX + 780.f, tblHdrY + 6.f);
        window.draw(th5);

        // Filter showtimes matching current search & dropdown filters (Case- and diacritic-insensitive!)
        std::string q = stSearchInput ? stSearchInput->getText() : "";
        std::string qClean = removeVietnameseDiacritics(q);

        std::vector<int> visibleIdx;
        for (size_t i = 0; i < showtimeList.size(); ++i) {
            if (!stDateFilterVal.empty() && showtimeList[i].getNgayChieu() != stDateFilterVal) continue;
            if (!stRoomFilterVal.empty() && showtimeList[i].getMaPhong() != stRoomFilterVal) continue;
            if (!stMovieFilterVal.empty() && showtimeList[i].getMaPhim() != stMovieFilterVal) continue;
            if (!qClean.empty()) {
                std::string scClean = removeVietnameseDiacritics(showtimeList[i].getMaSuatChieu());
                std::string spClean = removeVietnameseDiacritics(showtimeList[i].getMaPhong());
                std::string smClean = removeVietnameseDiacritics(showtimeList[i].getMaPhim());
                std::string pName = "";
                auto itP = s_movieTitleCache.find(showtimeList[i].getMaPhim());
                if (itP != s_movieTitleCache.end()) pName = itP->second;
                std::string pNameClean = removeVietnameseDiacritics(pName);

                std::string rName = "";
                auto itR = s_roomNameCache.find(showtimeList[i].getMaPhong());
                if (itR != s_roomNameCache.end()) rName = itR->second;
                std::string rNameClean = removeVietnameseDiacritics(rName);

                if (scClean.find(qClean) == std::string::npos &&
                    spClean.find(qClean) == std::string::npos &&
                    smClean.find(qClean) == std::string::npos &&
                    pNameClean.find(qClean) == std::string::npos &&
                    rNameClean.find(qClean) == std::string::npos) {
                    continue;
                }
            }
            visibleIdx.push_back(static_cast<int>(i));
        }

        int totalVisible = static_cast<int>(visibleIdx.size());
        int totalPages = std::max(1, (totalVisible + 3) / 4);
        if (showtimeCurrentPage >= totalPages) showtimeCurrentPage = totalPages - 1;
        if (showtimeCurrentPage < 0) showtimeCurrentPage = 0;

        int startIdx = showtimeCurrentPage * 4;
        int endIdx = std::min(startIdx + 4, totalVisible);

        float rowStartY = tblHdrY + 34.f;
        float rowH = 118.f; // Fits doubled poster 76x100 comfortably!

        // Supersampled icons for Sửa (Pencil) & Xóa (Trash) like in Phim & Thể Loại
        static sf::Texture stPencilTex, stTrashTex;
        static bool stActionIconsReady = false;
        if (!stActionIconsReady) {
            const int icS = 26;
            sf::Image pImg, tImg;
            pImg.create(icS, icS, sf::Color(0, 0, 0, 0));
            tImg.create(icS, icS, sf::Color(0, 0, 0, 0));

            auto distSeg = [](float px, float py, float ax, float ay, float bx, float by) -> float {
                float vx = bx - ax, vy = by - ay;
                float wx = px - ax, wy = py - ay;
                float c1 = vx * wx + vy * wy;
                if (c1 <= 0.f) return std::sqrt(wx * wx + wy * wy);
                float c2 = vx * vx + vy * vy;
                if (c2 <= c1) return std::sqrt((px - bx) * (px - bx) + (py - by) * (py - by));
                float b = c1 / c2;
                float pbx = ax + b * vx, pby = ay + b * vy;
                return std::sqrt((px - pbx) * (px - pbx) + (py - pby) * (py - pby));
            };

            for (int py = 0; py < icS; ++py) {
                for (int px = 0; px < icS; ++px) {
                    int pHits = 0, tHits = 0;
                    for (int sy = 0; sy < 4; ++sy) {
                        for (int sx = 0; sx < 4; ++sx) {
                            float fx = (px + (sx + 0.5f) * 0.25f) - icS * 0.5f;
                            float fy = (py + (sy + 0.5f) * 0.25f) - icS * 0.5f;

                            float u = (fx - fy) * 0.70710678f;
                            float v = (fx + fy) * 0.70710678f;
                            bool inPencil = false;
                            if (u >= -3.2f && u <= 5.2f && std::fabs(v) <= 2.1f) {
                                bool border = (std::fabs(v) >= 1.0f) || (u >= 4.1f) || (std::fabs(u - 2.4f) <= 0.55f) || (std::fabs(u + 3.0f) <= 0.5f);
                                if (border) inPencil = true;
                            }
                            if (u >= -6.2f && u < -3.2f) {
                                float maxV = ((u + 6.2f) / 3.0f) * 2.1f;
                                if (std::fabs(v) <= maxV && (std::fabs(v) >= maxV - 1.1f || u <= -4.8f)) {
                                    inPencil = true;
                                }
                            }
                            if (inPencil) pHits++;

                            bool inTrash = false;
                            if (distSeg(fx, fy, -5.8f, -4.6f, 5.8f, -4.6f) <= 0.75f) inTrash = true;
                            if (fy >= -7.0f && fy <= -4.6f && std::fabs(fx) <= 2.4f) {
                                if (fy <= -5.8f || std::fabs(fx) >= 1.25f) inTrash = true;
                            }
                            if (distSeg(fx, fy, -4.5f, -4.2f, -3.8f, 6.2f) <= 0.7f) inTrash = true;
                            if (distSeg(fx, fy,  4.5f, -4.2f,  3.8f, 6.2f) <= 0.7f) inTrash = true;
                            if (distSeg(fx, fy, -3.8f,  6.2f,  3.8f, 6.2f) <= 0.7f) inTrash = true;
                            if (distSeg(fx, fy, -1.5f, -1.8f, -1.3f, 3.8f) <= 0.6f) inTrash = true;
                            if (distSeg(fx, fy,  1.5f, -1.8f,  1.3f, 3.8f) <= 0.6f) inTrash = true;

                            if (inTrash) tHits++;
                        }
                    }
                    if (pHits > 0) {
                        pImg.setPixel(px, py, sf::Color(51, 65, 85, static_cast<sf::Uint8>((pHits * 255) / 16)));
                    }
                    if (tHits > 0) {
                        tImg.setPixel(px, py, sf::Color(220, 38, 38, static_cast<sf::Uint8>((tHits * 255) / 16)));
                    }
                }
            }
            stPencilTex.loadFromImage(pImg);
            stPencilTex.setSmooth(true);
            stTrashTex.loadFromImage(tImg);
            stTrashTex.setSmooth(true);
            stActionIconsReady = true;
        }

        for (int r = startIdx; r < endIdx; ++r) {
            int actualIdx = visibleIdx[r];
            const Showtime& item = showtimeList[actualIdx];
            float ry = rowStartY + (r - startIdx) * rowH;
            bool isSel = (actualIdx == selectedShowtimeRowIdx);

            // Row background
            sf::Color rBgColor = isSel ? sf::Color(240, 249, 255) : ((r % 2 == 0) ? sf::Color::White : sf::Color(250, 250, 250));
            RoundedRectRenderer::render(window, sf::Vector2f(leftColX + 8.f, ry), sf::Vector2f(leftColW - 16.f, rowH - 6.f), 8.f, rBgColor,
                                        isSel ? 1.5f : 1.f, isSel ? sf::Color(2, 132, 199) : sf::Color(241, 245, 249));

            if (isSel) {
                sf::RectangleShape selBar(sf::Vector2f(3.5f, rowH - 16.f));
                selBar.setPosition(leftColX + 10.f, ry + 5.f);
                selBar.setFillColor(sf::Color(2, 132, 199));
                window.draw(selBar);
            }

            // Col 1: Mã ST Pill
            RoundedRectRenderer::render(window, sf::Vector2f(leftColX + 16.f, ry + 44.f), sf::Vector2f(56.f, 26.f), 6.f,
                                        isSel ? sf::Color(224, 242, 254) : sf::Color(240, 249, 255),
                                        1.f, isSel ? sf::Color(2, 132, 199) : sf::Color(186, 230, 253));
            sf::Text stCode(item.getMaSuatChieu(), Theme::FontPrimary, 12);
            stCode.setStyle(sf::Text::Bold);
            stCode.setFillColor(sf::Color(2, 132, 199));
            stCode.setPosition(leftColX + 26.f, ry + 49.f);
            window.draw(stCode);

            // Col 2: Phim & Định dạng (Poster DOUBLED to 76x100!)
            float pX = leftColX + 80.f;
            float pY = ry + 9.f;
            RoundedRectRenderer::render(window, sf::Vector2f(pX, pY), sf::Vector2f(76.f, 100.f), 6.f, sf::Color(226, 232, 240), 1.f, sf::Color(203, 213, 225));

            auto pIt = moviePosterTextures.find(item.getMaPhim());
            if (pIt != moviePosterTextures.end() && pIt->second) {
                sf::Sprite spr(*pIt->second);
                sf::Vector2u tsz = pIt->second->getSize();
                if (tsz.x > 0 && tsz.y > 0) {
                    spr.setScale(76.f / static_cast<float>(tsz.x), 100.f / static_cast<float>(tsz.y));
                    spr.setPosition(pX, pY);
                    window.draw(spr);
                }
            }

            // Movie title + Genre info
            std::string movieTitle = item.getMaPhim();
            auto itT = s_movieTitleCache.find(item.getMaPhim());
            if (itT != s_movieTitleCache.end()) movieTitle = itT->second;

            std::string movieGenre = "Tình Cảm";
            auto itG = s_genreCache.find(item.getMaPhim());
            if (itG != s_genreCache.end()) movieGenre = itG->second;

            int movieDur = 120;
            auto itD = s_movieDurCache.find(item.getMaPhim());
            if (itD != s_movieDurCache.end()) movieDur = itD->second;

            sf::Text mTitle(toSfString(movieTitle), Theme::FontPrimary, 13);
            mTitle.setStyle(sf::Text::Bold);
            mTitle.setFillColor(sf::Color(15, 23, 42));
            mTitle.setPosition(pX + 86.f, ry + 26.f);
            window.draw(mTitle);

            // Badge 2D Phẳng
            float tagX = pX + 86.f + mTitle.getLocalBounds().width + 8.f;
            RoundedRectRenderer::render(window, sf::Vector2f(tagX, ry + 26.f), sf::Vector2f(64.f, 18.f), 4.f, sf::Color(253, 242, 248), 1.f, sf::Color(251, 207, 232));
            sf::Text tagTxt(toSfString("2D Phẳng"), Theme::FontPrimary, 9);
            tagTxt.setStyle(sf::Text::Bold);
            tagTxt.setFillColor(sf::Color(219, 39, 119));
            tagTxt.setPosition(tagX + 6.f, ry + 28.f);
            window.draw(tagTxt);

            sf::Text mSub(toSfString(movieGenre + " • " + std::to_string(movieDur) + "p"), Theme::FontPrimary, 11);
            mSub.setFillColor(sf::Color(100, 116, 139));
            mSub.setPosition(pX + 86.f, ry + 54.f);
            window.draw(mSub);

            // Col 3: Phòng chiếu
            std::string roomName = item.getMaPhong();
            auto itR = s_roomNameCache.find(item.getMaPhong());
            if (itR != s_roomNameCache.end()) roomName = itR->second;

            int roomSeats = 100;
            auto itS = s_roomSeatCache.find(item.getMaPhong());
            if (itS != s_roomSeatCache.end()) roomSeats = itS->second;

            float rColX = leftColX + 410.f;
            sf::RectangleShape rIc(sf::Vector2f(10.f, 10.f));
            rIc.setPosition(rColX, ry + 36.f);
            rIc.setFillColor(sf::Color(2, 132, 199));
            window.draw(rIc);

            sf::Text rTxt(toSfString(roomName), Theme::FontPrimary, 12);
            rTxt.setStyle(sf::Text::Bold);
            rTxt.setFillColor(sf::Color(15, 23, 42));
            rTxt.setPosition(rColX + 16.f, ry + 32.f);
            window.draw(rTxt);

            sf::Text rSub(toSfString(std::to_string(roomSeats) + " ghế • Tiêu chuẩn"), Theme::FontSecondary, 10);
            rSub.setFillColor(sf::Color(100, 116, 139));
            rSub.setPosition(rColX + 16.f, ry + 56.f);
            window.draw(rSub);

            // Col 4: Khung giờ chiếu
            float tColX = leftColX + 600.f;
            std::string timeSpan = item.getGioBatDau() + " -> " + item.getGioKetThuc();
            sf::Text tTxt(toSfString(timeSpan), Theme::FontPrimary, 13);
            tTxt.setStyle(sf::Text::Bold);
            tTxt.setFillColor(sf::Color(15, 23, 42));
            tTxt.setPosition(tColX, ry + 34.f);
            window.draw(tTxt);

            sf::Text tDate(toSfString("Ngày: " + item.getNgayChieu()), Theme::FontSecondary, 10);
            tDate.setFillColor(sf::Color(100, 116, 139));
            tDate.setPosition(tColX, ry + 58.f);
            window.draw(tDate);

            // Col 5: Thao tác (Vector Sửa/Pencil & Xóa/Trash like Phim & Thể Loại)
            float aColX = leftColX + 780.f;
            float icY = ry + 46.f;

            // Sửa Button (Pencil)
            RoundedRectRenderer::render(window, sf::Vector2f(aColX, icY), sf::Vector2f(26.f, 26.f), 6.f, sf::Color(241, 245, 249));
            sf::Sprite editSpr(stPencilTex);
            editSpr.setPosition(aColX, icY);
            window.draw(editSpr);

            // Xóa Button (Trash)
            RoundedRectRenderer::render(window, sf::Vector2f(aColX + 32.f, icY), sf::Vector2f(26.f, 26.f), 6.f, sf::Color(254, 242, 242));
            sf::Sprite delSpr(stTrashTex);
            delSpr.setPosition(aColX + 32.f, icY);
            window.draw(delSpr);
        }

        // Pagination Footer inside Left Card
        float footPagingY = cardY + cardH - 36.f;
        char pageInfoBuf[64];
        snprintf(pageInfoBuf, sizeof(pageInfoBuf), "Hiển thị %d - %d trong tổng số %d suất chiếu",
                 totalVisible == 0 ? 0 : (startIdx + 1), endIdx, totalVisible);
        sf::Text pInfo(toSfString(pageInfoBuf), Theme::FontSecondary, 11);
        pInfo.setFillColor(sf::Color(100, 116, 139));
        pInfo.setPosition(leftColX + 20.f, footPagingY + 4.f);
        window.draw(pInfo);

        float btnSz = 28.f;
        float btnGap = 6.f;
        float totalPagingW = (totalPages + 2) * btnSz + (totalPages + 1) * btnGap;
        float pgX = leftColX + (leftColW - totalPagingW) / 2.f;

        // Prev Button '<'
        RoundedRectRenderer::render(window, sf::Vector2f(pgX, footPagingY), sf::Vector2f(btnSz, btnSz), 6.f, sf::Color(248, 250, 252), 1.f, sf::Color(226, 232, 240));
        sf::Text prevTxt("<", Theme::FontPrimary, 12);
        prevTxt.setStyle(sf::Text::Bold);
        prevTxt.setFillColor(sf::Color(100, 116, 139));
        prevTxt.setPosition(pgX + 9.f, footPagingY + 4.f);
        window.draw(prevTxt);

        for (int p = 0; p < totalPages; ++p) {
            float pbX = pgX + (p + 1) * (btnSz + btnGap);
            bool isCurP = (p == showtimeCurrentPage);
            RoundedRectRenderer::render(window, sf::Vector2f(pbX, footPagingY), sf::Vector2f(btnSz, btnSz), 6.f,
                                        isCurP ? sf::Color(2, 132, 199) : sf::Color::White,
                                        1.f, isCurP ? sf::Color(2, 132, 199) : sf::Color(226, 232, 240));
            sf::Text pNum(std::to_string(p + 1), Theme::FontPrimary, 11);
            pNum.setStyle(sf::Text::Bold);
            pNum.setFillColor(isCurP ? sf::Color::White : sf::Color(71, 85, 105));
            pNum.setPosition(pbX + (p >= 9 ? 6.f : 9.f), footPagingY + 4.f);
            window.draw(pNum);
        }

        // Next Button '>'
        float nextBtnX = pgX + (totalPages + 1) * (btnSz + btnGap);
        RoundedRectRenderer::render(window, sf::Vector2f(nextBtnX, footPagingY), sf::Vector2f(btnSz, btnSz), 6.f, sf::Color(248, 250, 252), 1.f, sf::Color(226, 232, 240));
        sf::Text nextTxt(">", Theme::FontPrimary, 12);
        nextTxt.setStyle(sf::Text::Bold);
        nextTxt.setFillColor(sf::Color(100, 116, 139));
        nextTxt.setPosition(nextBtnX + 9.f, footPagingY + 4.f);
        window.draw(nextTxt);
    }
    else if (showtimeSubTab == 1) {
        // ==========================================
        // SUBTAB 1: TIMELINE PHÒNG CHIẾU TRỰC QUAN (ALL 8 ROOMS + DATE NAVIGATOR)
        // ==========================================
        sf::CircleShape lDot(4.f);
        lDot.setFillColor(sf::Color(2, 132, 199));
        lDot.setPosition(leftColX + 20.f, cardY + 16.f);
        window.draw(lDot);

        sf::Text lTitle(toSfString("Timeline Phòng Chiếu Trực Quan"), Theme::FontPrimary, 15);
        lTitle.setStyle(sf::Text::Bold);
        lTitle.setFillColor(sf::Color(15, 23, 42));
        lTitle.setPosition(leftColX + 34.f, cardY + 10.f);
        window.draw(lTitle);

        if (showtimeTimelineDate.empty()) showtimeTimelineDate = stTodayStr();
        if (!stDateFilterVal.empty()) showtimeTimelineDate = stDateFilterVal;
        std::string curDateDisplay = showtimeTimelineDate;

        sf::Text lSub(toSfString("Lịch vận hành trực quan theo phòng • Nhấp ô suất chiếu để xem & sửa"), Theme::FontSecondary, 11);
        lSub.setFillColor(sf::Color(100, 116, 139));
        lSub.setPosition(leftColX + 34.f, cardY + 28.f);
        window.draw(lSub);

        // Date Navigator on top right: [<] [Ngày: DD/MM/YYYY] [>]
        float dtNavStartX = leftColX + leftColW - 190.f;
        // Button '<'
        RoundedRectRenderer::render(window, sf::Vector2f(dtNavStartX, cardY + 12.f), sf::Vector2f(24.f, 24.f), 6.f, sf::Color(240, 249, 255), 1.f, sf::Color(186, 230, 253));
        sf::Text prevDayTxt("<", Theme::FontPrimary, 12);
        prevDayTxt.setStyle(sf::Text::Bold);
        prevDayTxt.setFillColor(sf::Color(2, 132, 199));
        prevDayTxt.setPosition(dtNavStartX + 7.f, cardY + 15.f);
        window.draw(prevDayTxt);

        // Date Display Badge
        RoundedRectRenderer::render(window, sf::Vector2f(dtNavStartX + 28.f, cardY + 12.f), sf::Vector2f(130.f, 24.f), 6.f, sf::Color(240, 249, 255), 1.f, sf::Color(186, 230, 253));
        sf::Text dtBadgeTxt(toSfString("Ngày: " + curDateDisplay), Theme::FontPrimary, 10);
        dtBadgeTxt.setStyle(sf::Text::Bold);
        dtBadgeTxt.setFillColor(sf::Color(2, 132, 199));
        sf::FloatRect dtb = dtBadgeTxt.getLocalBounds();
        dtBadgeTxt.setPosition(std::round(dtNavStartX + 28.f + (130.f - dtb.width) / 2.f - dtb.left), std::round(cardY + 12.f + (24.f - dtb.height) / 2.f - dtb.top));
        window.draw(dtBadgeTxt);

        // Button '>'
        RoundedRectRenderer::render(window, sf::Vector2f(dtNavStartX + 162.f, cardY + 12.f), sf::Vector2f(24.f, 24.f), 6.f, sf::Color(240, 249, 255), 1.f, sf::Color(186, 230, 253));
        sf::Text nextDayTxt(">", Theme::FontPrimary, 12);
        nextDayTxt.setStyle(sf::Text::Bold);
        nextDayTxt.setFillColor(sf::Color(2, 132, 199));
        nextDayTxt.setPosition(dtNavStartX + 169.f, cardY + 15.f);
        window.draw(nextDayTxt);

        // Time Ruler: 08:00 to 24:00 (16 hours = 960 mins)
        float rulerY = cardY + 50.f;
        float tlStartX = leftColX + 116.f;
        float tlW = 744.f;
        RoundedRectRenderer::render(window, sf::Vector2f(tlStartX, rulerY), sf::Vector2f(tlW, 22.f), 6.f, sf::Color(248, 250, 252), 1.f, sf::Color(226, 232, 240));

        for (int h = 8; h <= 24; h += 2) {
            float hx = tlStartX + ((h - 8) / 16.f) * tlW;
            char hBuf[8];
            snprintf(hBuf, sizeof(hBuf), "%02d:00", h % 24);
            sf::Text hTxt(hBuf, Theme::FontPrimary, 10);
            hTxt.setStyle(sf::Text::Bold);
            hTxt.setFillColor(sf::Color(100, 116, 139));
            sf::FloatRect hb = hTxt.getLocalBounds();
            hTxt.setPosition(std::min(hx - hb.width / 2.f, tlStartX + tlW - hb.width - 4.f), rulerY + 4.f);
            window.draw(hTxt);
        }

        // Room Lanes: ALL 8 ROOMS (R01..R08) rendered simultaneously!
        float startLaneY = cardY + 76.f;
        float laneH = 54.f;
        float laneGap = 6.f;
        size_t totalLanes = roomList.size();

        for (size_t r = 0; r < totalLanes; ++r) {
            float ly = startLaneY + r * (laneH + laneGap);
            RoundedRectRenderer::render(window, sf::Vector2f(leftColX + 10.f, ly), sf::Vector2f(leftColW - 20.f, laneH), 6.f, sf::Color::White, 1.f, sf::Color(226, 232, 240));

            // Room Info (Left column)
            RoundedRectRenderer::render(window, sf::Vector2f(leftColX + 16.f, ly + 8.f), sf::Vector2f(42.f, 18.f), 4.f, sf::Color(224, 242, 254), 1.f, sf::Color(186, 230, 253));
            sf::Text rCode(roomList[r].getMaPhong(), Theme::FontPrimary, 10);
            rCode.setStyle(sf::Text::Bold);
            rCode.setFillColor(sf::Color(2, 132, 199));
            rCode.setPosition(leftColX + 22.f, ly + 10.f);
            window.draw(rCode);

            sf::Text rCap(toSfString(std::to_string(roomList[r].getSucChua()) + " ghế"), Theme::FontSecondary, 9);
            rCap.setFillColor(sf::Color(100, 116, 139));
            rCap.setPosition(leftColX + 62.f, ly + 10.f);
            window.draw(rCap);

            sf::Text rName(toSfString(roomList[r].getTenPhong()), Theme::FontPrimary, 11);
            rName.setStyle(sf::Text::Bold);
            rName.setFillColor(sf::Color(15, 23, 42));
            rName.setPosition(leftColX + 16.f, ly + 30.f);
            window.draw(rName);

            // Divider vertical line
            sf::RectangleShape divV(sf::Vector2f(1.f, laneH - 12.f));
            divV.setPosition(tlStartX - 8.f, ly + 6.f);
            divV.setFillColor(sf::Color(226, 232, 240));
            window.draw(divV);

            // Timeline Track for this room
            RoundedRectRenderer::render(window, sf::Vector2f(tlStartX, ly + 4.f), sf::Vector2f(tlW, laneH - 8.f), 6.f, sf::Color(248, 250, 252));

            // Draw hour guideline ticks inside track
            for (int h = 8; h <= 24; h += 2) {
                float hx = tlStartX + ((h - 8) / 16.f) * tlW;
                sf::RectangleShape tick(sf::Vector2f(1.f, laneH - 8.f));
                tick.setPosition(hx, ly + 4.f);
                tick.setFillColor(sf::Color(241, 245, 249));
                window.draw(tick);
            }

            // Draw showtime blocks on this lane
            for (size_t i = 0; i < showtimeList.size(); ++i) {
                const auto& st = showtimeList[i];
                if (st.getMaPhong() != roomList[r].getMaPhong()) continue;
                if (st.getNgayChieu() != curDateDisplay) continue;

                int sMins = 540;
                if (!stParseTimeToMinutes(st.getGioBatDau(), sMins)) continue;
                int dur = 120;
                auto itD = s_movieDurCache.find(st.getMaPhim());
                if (itD != s_movieDurCache.end()) dur = itD->second;

                float bx = tlStartX + ((sMins - 480) / 960.f) * tlW;
                float bw = std::max(68.f, (dur / 960.f) * tlW);
                float by = ly + 4.f;
                float bh = laneH - 8.f; // 46.f

                bool isSel = (static_cast<int>(i) == selectedShowtimeRowIdx);

                // Block background
                sf::Color bFill = isSel ? sf::Color(2, 132, 199) : sf::Color(238, 246, 255);
                sf::Color bBorder = isSel ? sf::Color(3, 105, 161) : sf::Color(186, 230, 253);
                RoundedRectRenderer::render(window, sf::Vector2f(bx, by), sf::Vector2f(bw, bh), 6.f, bFill, isSel ? 2.f : 1.f, bBorder);

                std::string mTitleStr = st.getMaPhim();
                auto itT = s_movieTitleCache.find(st.getMaPhim());
                if (itT != s_movieTitleCache.end()) mTitleStr = itT->second;

                std::string titleCombined = st.getMaSuatChieu() + " • " + mTitleStr;
                sf::Text mTitle(stFitText(titleCombined, 10, bw - 10.f, true), Theme::FontPrimary, 10);
                mTitle.setStyle(sf::Text::Bold);
                mTitle.setFillColor(isSel ? sf::Color::White : sf::Color(15, 23, 42));
                mTitle.setPosition(bx + 6.f, by + 6.f);
                window.draw(mTitle);

                // Time text
                std::string timeText = st.getGioBatDau() + " - " + st.getGioKetThuc();
                sf::Text tTxt(stFitText(timeText, 9, bw - 10.f, false), Theme::FontPrimary, 9);
                tTxt.setFillColor(isSel ? sf::Color(224, 242, 254) : sf::Color(71, 85, 105));
                tTxt.setPosition(bx + 6.f, by + 24.f);
                window.draw(tTxt);
            }
        }

        // Timeline footer note
        float footTlY = cardY + cardH - 32.f;
        sf::Text tlNote(toSfString("Ghi chú: Bản đồ hiển thị từ 08:00 đến 24:00. Bấm < hoặc > để xem ngày khác."), Theme::FontSecondary, 11);
        tlNote.setFillColor(sf::Color(100, 116, 139));
        tlNote.setPosition(leftColX + 20.f, footTlY + 2.f);
        window.draw(tlNote);
    }
    else if (showtimeSubTab == 2) {
        // ==========================================
        // SUBTAB 2: LỊCH CHIẾU PHÂN BỔ THEO ĐẦU PHIM (DOUBLED POSTER 84x120 + CLEAN PILLS)
        // ==========================================
        sf::CircleShape lDot(4.f);
        lDot.setFillColor(sf::Color(2, 132, 199));
        lDot.setPosition(leftColX + 20.f, cardY + 18.f);
        window.draw(lDot);

        sf::Text lTitle(toSfString("Lịch Chiếu Phân Bổ Theo Phim"), Theme::FontPrimary, 15);
        lTitle.setStyle(sf::Text::Bold);
        lTitle.setFillColor(sf::Color(15, 23, 42));
        lTitle.setPosition(leftColX + 34.f, cardY + 12.f);
        window.draw(lTitle);

        char cntMBuf[64];
        snprintf(cntMBuf, sizeof(cntMBuf), "%zu Phim đang chiếu", movieList.size());
        float cMBX = leftColX + 34.f + lTitle.getLocalBounds().width + 10.f;
        RoundedRectRenderer::render(window, sf::Vector2f(cMBX, cardY + 13.f), sf::Vector2f(130.f, 22.f), 11.f, sf::Color(224, 242, 254), 1.f, sf::Color(186, 230, 253));
        sf::Text cMBTxt(toSfString(cntMBuf), Theme::FontPrimary, 10);
        cMBTxt.setStyle(sf::Text::Bold);
        cMBTxt.setFillColor(sf::Color(2, 132, 199));
        cMBTxt.setPosition(cMBX + 8.f, cardY + 16.f);
        window.draw(cMBTxt);

        sf::Text lNote(toSfString("Nhấp khung giờ để xem & sửa"), Theme::FontSecondary, 11);
        lNote.setFillColor(sf::Color(148, 163, 184));
        lNote.setPosition(leftColX + leftColW - lNote.getLocalBounds().width - 20.f, cardY + 14.f);
        window.draw(lNote);

        // Movie cards (3 movies per page for doubled poster display)
        int totalMovies = static_cast<int>(movieList.size());
        int moviesPerPage = 3;
        int totalPages = std::max(1, (totalMovies + moviesPerPage - 1) / moviesPerPage);
        if (showtimeCurrentPage >= totalPages) showtimeCurrentPage = totalPages - 1;
        if (showtimeCurrentPage < 0) showtimeCurrentPage = 0;

        int startM = showtimeCurrentPage * moviesPerPage;
        int endM = std::min(startM + moviesPerPage, totalMovies);

        float startCardY = cardY + 48.f;
        float cardBlockH = 154.f;
        float cardGap = 12.f;

        for (int m = startM; m < endM; ++m) {
            float cy = startCardY + (m - startM) * (cardBlockH + cardGap);
            const auto& movie = movieList[m];

            // Card container
            RoundedRectRenderer::render(window, sf::Vector2f(leftColX + 12.f, cy), sf::Vector2f(leftColW - 24.f, cardBlockH), 8.f, sf::Color::White, 1.f, sf::Color(226, 232, 240));

            // Poster DOUBLED to 84x120!
            float pX = leftColX + 24.f;
            float pY = cy + 17.f;
            RoundedRectRenderer::render(window, sf::Vector2f(pX, pY), sf::Vector2f(84.f, 120.f), 6.f, sf::Color(226, 232, 240), 1.f, sf::Color(203, 213, 225));
            auto pIt = moviePosterTextures.find(movie.getMaPhim());
            if (pIt != moviePosterTextures.end() && pIt->second) {
                sf::Sprite spr(*pIt->second);
                sf::Vector2u tsz = pIt->second->getSize();
                if (tsz.x > 0 && tsz.y > 0) {
                    spr.setScale(84.f / static_cast<float>(tsz.x), 120.f / static_cast<float>(tsz.y));
                    spr.setPosition(pX, pY);
                    window.draw(spr);
                }
            }

            // Movie title
            sf::Text mTitle(toSfString(movie.getTenPhim()), Theme::FontPrimary, 14);
            mTitle.setStyle(sf::Text::Bold);
            mTitle.setFillColor(sf::Color(15, 23, 42));
            mTitle.setPosition(pX + 96.f, cy + 18.f);
            window.draw(mTitle);

            // Genre & duration
            std::string mGenre = "Tình Cảm";
            auto itG = s_genreCache.find(movie.getMaPhim());
            if (itG != s_genreCache.end()) mGenre = itG->second;
            sf::Text mSub(toSfString(mGenre + " • " + std::to_string(movie.getThoiLuong()) + " phút"), Theme::FontSecondary, 11);
            mSub.setFillColor(sf::Color(100, 116, 139));
            mSub.setPosition(pX + 96.f, cy + 42.f);
            window.draw(mSub);

            // Quick "+ Thêm suất" button on top-right of movie card
            float addBtnX = leftColX + leftColW - 146.f;
            float addBtnY = cy + 18.f;
            float addBtnW = 120.f;
            float addBtnH = 28.f;
            RoundedRectRenderer::render(window, sf::Vector2f(addBtnX, addBtnY), sf::Vector2f(addBtnW, addBtnH), 6.f, sf::Color(240, 249, 255), 1.f, sf::Color(186, 230, 253));
            sf::Text addBtnTxt(toSfString("+ Thêm suất"), Theme::FontPrimary, 11);
            addBtnTxt.setStyle(sf::Text::Bold);
            addBtnTxt.setFillColor(sf::Color(2, 132, 199));
            sf::FloatRect abB = addBtnTxt.getLocalBounds();
            addBtnTxt.setPosition(std::round(addBtnX + (addBtnW - abB.width) / 2.f - abB.left), std::round(addBtnY + (addBtnH - abB.height) / 2.f - abB.top));
            window.draw(addBtnTxt);

            // Row of showtime pills (Requirement: "chỉ hiện giờ mỗi phim - phòng chiếu")
            float pillStartX = pX + 96.f;
            float pillY = cy + 86.f;
            float pillW = 136.f;
            float pillH = 42.f;
            float pillGap = 10.f;

            int pIdx = 0;
            int totalStForMovie = 0;
            for (size_t i = 0; i < showtimeList.size(); ++i) {
                const auto& st = showtimeList[i];
                if (st.getMaPhim() != movie.getMaPhim()) continue;
                if (!stDateFilterVal.empty() && st.getNgayChieu() != stDateFilterVal) continue;
                totalStForMovie++;

                if (pIdx < 5) {
                    float px = pillStartX + pIdx * (pillW + pillGap);
                    bool isSel = (static_cast<int>(i) == selectedShowtimeRowIdx);

                    RoundedRectRenderer::render(window, sf::Vector2f(px, pillY), sf::Vector2f(pillW, pillH), 6.f,
                                                isSel ? sf::Color(2, 132, 199) : sf::Color(248, 250, 252),
                                                1.f, isSel ? sf::Color(3, 105, 161) : sf::Color(226, 232, 240));

                    // Line 1: Show hours only (e.g. 09:00 - 11:07)
                    std::string timeRange = st.getGioBatDau() + " - " + st.getGioKetThuc();
                    sf::Text tTxt(timeRange, Theme::FontPrimary, 11);
                    tTxt.setStyle(sf::Text::Bold);
                    tTxt.setFillColor(isSel ? sf::Color::White : sf::Color(15, 23, 42));
                    tTxt.setPosition(px + 8.f, pillY + 5.f);
                    window.draw(tTxt);

                    // Line 2: Room only (e.g. R01 • Phòng 01)
                    std::string rName = st.getMaPhong();
                    auto itR = s_roomNameCache.find(st.getMaPhong());
                    if (itR != s_roomNameCache.end()) rName = itR->second;
                    sf::Text rTxt(stFitText(rName, 10, pillW - 16.f, false), Theme::FontSecondary, 10);
                    rTxt.setFillColor(isSel ? sf::Color(224, 242, 254) : sf::Color(100, 116, 139));
                    rTxt.setPosition(px + 8.f, pillY + 23.f);
                    window.draw(rTxt);

                    pIdx++;
                }
            }

            if (totalStForMovie == 0) {
                sf::Text noSt(toSfString("Chưa có suất chiếu nào được xếp cho phim này"), Theme::FontSecondary, 11);
                noSt.setFillColor(sf::Color(148, 163, 184));
                noSt.setPosition(pillStartX, pillY + 12.f);
                window.draw(noSt);
            }
        }

        // Pagination for SubTab 2
        float footPagingY = cardY + cardH - 36.f;
        char pageInfoBuf[64];
        snprintf(pageInfoBuf, sizeof(pageInfoBuf), "Hiển thị %d - %d trong tổng số %d phim",
                 totalMovies == 0 ? 0 : (startM + 1), endM, totalMovies);
        sf::Text pInfo(toSfString(pageInfoBuf), Theme::FontSecondary, 11);
        pInfo.setFillColor(sf::Color(100, 116, 139));
        pInfo.setPosition(leftColX + 20.f, footPagingY + 4.f);
        window.draw(pInfo);

        float btnSz = 28.f;
        float btnGap = 6.f;
        float totalPagingW = (totalPages + 2) * btnSz + (totalPages + 1) * btnGap;
        float pgX = leftColX + (leftColW - totalPagingW) / 2.f;

        // Prev Button '<'
        RoundedRectRenderer::render(window, sf::Vector2f(pgX, footPagingY), sf::Vector2f(btnSz, btnSz), 6.f, sf::Color(248, 250, 252), 1.f, sf::Color(226, 232, 240));
        sf::Text prevTxt("<", Theme::FontPrimary, 12);
        prevTxt.setStyle(sf::Text::Bold);
        prevTxt.setFillColor(sf::Color(100, 116, 139));
        prevTxt.setPosition(pgX + 9.f, footPagingY + 4.f);
        window.draw(prevTxt);

        for (int p = 0; p < totalPages; ++p) {
            float pbX = pgX + (p + 1) * (btnSz + btnGap);
            bool isCurP = (p == showtimeCurrentPage);
            RoundedRectRenderer::render(window, sf::Vector2f(pbX, footPagingY), sf::Vector2f(btnSz, btnSz), 6.f,
                                        isCurP ? sf::Color(2, 132, 199) : sf::Color::White,
                                        1.f, isCurP ? sf::Color(2, 132, 199) : sf::Color(226, 232, 240));
            sf::Text pNum(std::to_string(p + 1), Theme::FontPrimary, 11);
            pNum.setStyle(sf::Text::Bold);
            pNum.setFillColor(isCurP ? sf::Color::White : sf::Color(71, 85, 105));
            pNum.setPosition(pbX + (p >= 9 ? 6.f : 9.f), footPagingY + 4.f);
            window.draw(pNum);
        }

        // Next Button '>'
        float nextBtnX = pgX + (totalPages + 1) * (btnSz + btnGap);
        RoundedRectRenderer::render(window, sf::Vector2f(nextBtnX, footPagingY), sf::Vector2f(btnSz, btnSz), 6.f, sf::Color(248, 250, 252), 1.f, sf::Color(226, 232, 240));
        sf::Text nextTxt(">", Theme::FontPrimary, 12);
        nextTxt.setStyle(sf::Text::Bold);
        nextTxt.setFillColor(sf::Color(100, 116, 139));
        nextTxt.setPosition(nextBtnX + 9.f, footPagingY + 4.f);
        window.draw(nextTxt);
    }

    // ==========================================
    // RIGHT CARD: Chi Tiết & Xếp Lịch Suất Chiếu
    // ==========================================
    RoundedRectRenderer::render(window, sf::Vector2f(rightColX, cardY), sf::Vector2f(rightColW, cardH), 12.f, sf::Color::White, 1.f, sf::Color(226, 232, 240));

    sf::CircleShape rightCardDot(4.f);
    rightCardDot.setFillColor(sf::Color(2, 132, 199));
    rightCardDot.setPosition(rightColX + 20.f, cardY + 18.f);
    window.draw(rightCardDot);

    sf::Text rTitle;
    rTitle.setFont(Theme::FontPrimary);
    rTitle.setCharacterSize(16);
    rTitle.setStyle(sf::Text::Bold);
    rTitle.setFillColor(sf::Color(15, 23, 42));
    rTitle.setString(toSfString("Chi Tiết & Xếp Lịch Suất Chiếu"));
    rTitle.setPosition(rightColX + 34.f, cardY + 12.f);
    window.draw(rTitle);

    sf::Text rSub;
    rSub.setFont(Theme::FontPrimary);
    rSub.setCharacterSize(11);
    rSub.setFillColor(sf::Color(100, 116, 139));
    rSub.setString(toSfString("Quản trị thời gian chiếu và phòng máy CineOS"));
    rSub.setPosition(rightColX + 34.f, cardY + 34.f);
    window.draw(rSub);

    // Current Selection Pill
    bool isEditMode = (selectedShowtimeRowIdx >= 0 && selectedShowtimeRowIdx < static_cast<int>(showtimeList.size()));
    std::string curSelCode = isEditMode
                             ? ("Đang sửa: " + showtimeList[selectedShowtimeRowIdx].getMaSuatChieu())
                             : "+ Tạo suất mới";
    sf::Color badgeBg = isEditMode ? sf::Color(240, 249, 255) : sf::Color(236, 253, 245);
    sf::Color badgeBorder = isEditMode ? sf::Color(186, 230, 253) : sf::Color(167, 243, 208);
    sf::Color badgeText = isEditMode ? sf::Color(2, 132, 199) : sf::Color(22, 163, 74);

    RoundedRectRenderer::render(window, sf::Vector2f(rightColX + rightColW - 140.f, cardY + 14.f), sf::Vector2f(120.f, 24.f), 12.f, badgeBg, 1.f, badgeBorder);
    sf::Text curSelTxt(toSfString(curSelCode), Theme::FontPrimary, 11);
    curSelTxt.setStyle(sf::Text::Bold);
    curSelTxt.setFillColor(badgeText);
    sf::FloatRect csB = curSelTxt.getLocalBounds();
    curSelTxt.setPosition(std::round(rightColX + rightColW - 140.f + (120.f - csB.width) / 2.f - csB.left), std::round(cardY + 14.f + (24.f - csB.height) / 2.f - csB.top));
    window.draw(curSelTxt);

    // Form Field 1: CHỌN PHIM CHIẾU *
    float formX = rightColX + 20.f;
    float formW = rightColW - 40.f;

    sf::Text lblMovie(toSfString("CHỌN PHIM CHIẾU *"), Theme::FontPrimary, 11);
    lblMovie.setStyle(sf::Text::Bold);
    lblMovie.setFillColor(sf::Color(71, 85, 105));
    lblMovie.setPosition(formX, ST_FORM_MOVIE_LBL_Y);
    window.draw(lblMovie);

    if (stMovieDropdown) stMovieDropdown->draw(window);

    int curDur = (!movieList.empty() && stFormMovieIdx >= 0 && stFormMovieIdx < static_cast<int>(movieList.size())) ? movieList[stFormMovieIdx].getThoiLuong() : 120;
    sf::Text mInfo(toSfString("Thời lượng gốc: " + std::to_string(curDur) + " phút   •   Hỗ trợ: 2D, 3D, IMAX Laser"), Theme::FontPrimary, 11);
    mInfo.setFillColor(sf::Color(100, 116, 139));
    mInfo.setPosition(formX, ST_FORM_MOVIE_INFO_Y);
    window.draw(mInfo);

    // Form Field 2: PHÒNG CHIẾU VẬN HÀNH *
    sf::Text lblRoom(toSfString("PHÒNG CHIẾU VẬN HÀNH *"), Theme::FontPrimary, 11);
    lblRoom.setStyle(sf::Text::Bold);
    lblRoom.setFillColor(sf::Color(71, 85, 105));
    lblRoom.setPosition(formX, ST_FORM_ROOM_LBL_Y);
    window.draw(lblRoom);

    if (stRoomFormDropdown) stRoomFormDropdown->draw(window);

    int rIdx = stRoomFormDropdown ? stRoomFormDropdown->getSelectedIndex() : stFormRoomIdx;
    if (rIdx < 0 || rIdx >= static_cast<int>(roomList.size())) rIdx = 0;
    int curRoomCap = (!roomList.empty()) ? roomList[rIdx].getSucChua() : 100;
    sf::Text rInfo(toSfString("Sức chứa: " + std::to_string(curRoomCap) + " ghế   •   Máy chiếu Laser 4K Dolby Atmos"), Theme::FontPrimary, 11);
    rInfo.setFillColor(sf::Color(100, 116, 139));
    rInfo.setPosition(formX, ST_FORM_ROOM_INFO_Y);
    window.draw(rInfo);

    // Form Field 3: Row: NGÀY CHIẾU * & GIỜ BẮT ĐẦU *
    float halfFormW = (formW - 14.f) / 2.f;
    sf::Text lblDate(toSfString("NGÀY CHIẾU *"), Theme::FontPrimary, 11);
    lblDate.setStyle(sf::Text::Bold);
    lblDate.setFillColor(sf::Color(71, 85, 105));
    lblDate.setPosition(formX, ST_FORM_DATE_LBL_Y);
    window.draw(lblDate);

    sf::Text lblTime(toSfString("GIỜ BẮT ĐẦU *"), Theme::FontPrimary, 11);
    lblTime.setStyle(sf::Text::Bold);
    lblTime.setFillColor(sf::Color(71, 85, 105));
    lblTime.setPosition(formX + halfFormW + 14.f, ST_FORM_DATE_LBL_Y);
    window.draw(lblTime);

    if (inputStDate) inputStDate->draw(window);
    if (inputStTime) inputStTime->draw(window);

    // Form Field 4: Estimated Finish Time Card (NO cleaning time, clean and exact!)
    float estY = ST_FORM_EST_Y;
    RoundedRectRenderer::render(window, sf::Vector2f(formX, estY), sf::Vector2f(formW, ST_FORM_EST_H), 10.f, sf::Color(240, 249, 255), 1.f, sf::Color(186, 230, 253));

    sf::Text estLbl(toSfString("GIỜ KẾT THÚC DỰ KIẾN"), Theme::FontPrimary, 11);
    estLbl.setStyle(sf::Text::Bold);
    estLbl.setFillColor(sf::Color(100, 116, 139));
    estLbl.setPosition(formX + 16.f, estY + 12.f);
    window.draw(estLbl);

    // Calculate finish time based on start time + movie duration
    std::string sTimeStr = inputStTime ? inputStTime->getText() : "09:00";
    int sMins = 540;
    stParseTimeToMinutes(sTimeStr, sMins);
    int totalM = sMins + curDur;
    int eH = (totalM / 60) % 24;
    int eM = totalM % 60;
    std::string periodStr = (eH >= 12 && eH < 18) ? "Chiều" : ((eH >= 18) ? "Tối" : ((eH >= 11) ? "Trưa" : "Sáng"));
    char timeFinBuf[32];
    snprintf(timeFinBuf, sizeof(timeFinBuf), "%02d:%02d:00  %s", eH, eM, periodStr.c_str());

    sf::Text estTime(toSfString(timeFinBuf), Theme::FontPrimary, 18);
    estTime.setStyle(sf::Text::Bold);
    estTime.setFillColor(sf::Color(2, 132, 199));
    estTime.setPosition(formX + 16.f, estY + 30.f);
    window.draw(estTime);

    char calcFormula[64];
    snprintf(calcFormula, sizeof(calcFormula), "Tự tính: %d phút phim = %d phút", curDur, curDur);
    sf::Text estFormula(toSfString(calcFormula), Theme::FontPrimary, 11);
    estFormula.setFillColor(sf::Color(100, 116, 139));
    estFormula.setPosition(formX + 16.f, estY + 58.f);
    window.draw(estFormula);

    // Hourglass icon
    sf::ConvexShape hgTop(3);
    hgTop.setPoint(0, sf::Vector2f(formX + formW - 36.f, estY + 24.f));
    hgTop.setPoint(1, sf::Vector2f(formX + formW - 16.f, estY + 24.f));
    hgTop.setPoint(2, sf::Vector2f(formX + formW - 26.f, estY + 40.f));
    hgTop.setFillColor(sf::Color(2, 132, 199));
    window.draw(hgTop);
    sf::ConvexShape hgBot(3);
    hgBot.setPoint(0, sf::Vector2f(formX + formW - 26.f, estY + 40.f));
    hgBot.setPoint(1, sf::Vector2f(formX + formW - 16.f, estY + 56.f));
    hgBot.setPoint(2, sf::Vector2f(formX + formW - 36.f, estY + 56.f));
    hgBot.setFillColor(sf::Color(2, 132, 199));
    window.draw(hgBot);

    // Form Action Buttons
    if (btnStSave) btnStSave->draw(window);
    if (btnStCreate) btnStCreate->draw(window);
    if (btnStDelete) btnStDelete->draw(window);
    if (btnStReset) btnStReset->draw(window);

    // ==========================================
    // 7. FOOTER: 4 KPI CARDS (ĐỔI NGUYÊN Ô THÀNH MÀU CHUYÊN ĐỀ RIÊNG, KHÔNG DÙNG CHẤM MÀU)
    // ==========================================
    float kpiY = ST_KPI_Y;
    float kpiH = 70.f;
    float kpiGap = 16.f;
    float kpiTotalW = 1420.f;
    float kpiCardW = (kpiTotalW - 3 * kpiGap) / 4.f; // 343.f

    // Card 1: Tổng Suất Chiếu (Soft Sky Blue)
    float c1X = leftColX;
    RoundedRectRenderer::render(window, sf::Vector2f(c1X, kpiY), sf::Vector2f(kpiCardW, kpiH), 12.f,
                                sf::Color(238, 246, 255), 1.5f, sf::Color(186, 230, 253));

    sf::Text c1Lbl(toSfString("TỔNG SUẤT CHIẾU"), Theme::FontSecondary, 10);
    c1Lbl.setStyle(sf::Text::Bold); c1Lbl.setFillColor(sf::Color(2, 132, 199));
    c1Lbl.setPosition(c1X + 20.f, kpiY + 11.f);
    window.draw(c1Lbl);

    char c1ValBuf[64];
    snprintf(c1ValBuf, sizeof(c1ValBuf), "%02zu suất / %zu phòng", showtimeList.size(), roomList.size());
    sf::Text c1Val(toSfString(c1ValBuf), Theme::FontPrimary, 15);
    c1Val.setStyle(sf::Text::Bold); c1Val.setFillColor(sf::Color(12, 74, 110));
    c1Val.setPosition(c1X + 20.f, kpiY + 27.f);
    window.draw(c1Val);

    sf::Text c1Sub(toSfString("100% khớp lịch vận hành CineOS"), Theme::FontSecondary, 10);
    c1Sub.setFillColor(sf::Color(3, 105, 161));
    c1Sub.setPosition(c1X + 20.f, kpiY + 48.f);
    window.draw(c1Sub);

    // Card 2: Phòng Đang Hoạt Động (Soft Emerald Green)
    float c2X = c1X + kpiCardW + kpiGap;
    RoundedRectRenderer::render(window, sf::Vector2f(c2X, kpiY), sf::Vector2f(kpiCardW, kpiH), 12.f,
                                sf::Color(236, 253, 245), 1.5f, sf::Color(167, 243, 208));

    sf::Text c2Lbl(toSfString("PHÒNG ĐANG CHIẾU"), Theme::FontSecondary, 10);
    c2Lbl.setStyle(sf::Text::Bold); c2Lbl.setFillColor(sf::Color(5, 150, 105));
    c2Lbl.setPosition(c2X + 20.f, kpiY + 11.f);
    window.draw(c2Lbl);

    char c2ValBuf[64];
    snprintf(c2ValBuf, sizeof(c2ValBuf), "%zu/%zu phòng hoạt động", roomList.size(), roomList.size());
    sf::Text c2Val(toSfString(c2ValBuf), Theme::FontPrimary, 15);
    c2Val.setStyle(sf::Text::Bold); c2Val.setFillColor(sf::Color(6, 78, 59));
    c2Val.setPosition(c2X + 20.f, kpiY + 27.f);
    window.draw(c2Val);

    sf::Text c2Sub(toSfString("100% sẵn sàng vận hành"), Theme::FontSecondary, 10);
    c2Sub.setFillColor(sf::Color(16, 185, 129));
    c2Sub.setPosition(c2X + 20.f, kpiY + 48.f);
    window.draw(c2Sub);

    // Card 3: Phim Chiếu Nhiều Nhất (Soft Rose / Pink)
    float c3X = c2X + kpiCardW + kpiGap;
    RoundedRectRenderer::render(window, sf::Vector2f(c3X, kpiY), sf::Vector2f(kpiCardW, kpiH), 12.f,
                                sf::Color(253, 242, 248), 1.5f, sf::Color(251, 207, 232));

    sf::Text c3Lbl(toSfString("PHIM CHIẾU NHIỀU NHẤT"), Theme::FontSecondary, 10);
    c3Lbl.setStyle(sf::Text::Bold); c3Lbl.setFillColor(sf::Color(219, 39, 119));
    c3Lbl.setPosition(c3X + 20.f, kpiY + 11.f);
    window.draw(c3Lbl);

    std::map<std::string, int> movieCounts;
    for (const auto& s : showtimeList) movieCounts[s.getMaPhim()]++;
    std::string topMovieId = !movieList.empty() ? movieList[0].getMaPhim() : "";
    int maxMCount = 0;
    for (const auto& pair : movieCounts) {
        if (pair.second > maxMCount) { maxMCount = pair.second; topMovieId = pair.first; }
    }
    std::string topMovieName = "Lật Mặt 7";
    for (const auto& m : movieList) { if (m.getMaPhim() == topMovieId) { topMovieName = m.getTenPhim(); break; } }
    char c3ValBuf[64];
    snprintf(c3ValBuf, sizeof(c3ValBuf), "%s (%d suất)", topMovieName.c_str(), maxMCount);
    sf::Text c3Val(toSfString(c3ValBuf), Theme::FontPrimary, 15);
    c3Val.setStyle(sf::Text::Bold); c3Val.setFillColor(sf::Color(131, 24, 67));
    c3Val.setPosition(c3X + 20.f, kpiY + 27.f);
    window.draw(c3Val);

    sf::Text c3Sub(toSfString("Tỷ trọng suất chiếu cao nhất"), Theme::FontSecondary, 10);
    c3Sub.setFillColor(sf::Color(190, 24, 93));
    c3Sub.setPosition(c3X + 20.f, kpiY + 48.f);
    window.draw(c3Sub);

    // Card 4: Khung Giờ Vàng (Soft Warm Amber)
    float c4X = c3X + kpiCardW + kpiGap;
    RoundedRectRenderer::render(window, sf::Vector2f(c4X, kpiY), sf::Vector2f(kpiCardW, kpiH), 12.f,
                                sf::Color(254, 243, 199), 1.5f, sf::Color(253, 230, 138));

    sf::Text c4Lbl(toSfString("KHUNG GIỜ VÀNG"), Theme::FontSecondary, 10);
    c4Lbl.setStyle(sf::Text::Bold); c4Lbl.setFillColor(sf::Color(217, 119, 6));
    c4Lbl.setPosition(c4X + 20.f, kpiY + 11.f);
    window.draw(c4Lbl);

    sf::Text c4Val(toSfString("18:00 - 21:00"), Theme::FontPrimary, 15);
    c4Val.setStyle(sf::Text::Bold); c4Val.setFillColor(sf::Color(120, 53, 15));
    c4Val.setPosition(c4X + 20.f, kpiY + 27.f);
    window.draw(c4Val);

    sf::Text c4Sub(toSfString("Khung giờ cao điểm rạp"), Theme::FontSecondary, 10);
    c4Sub.setFillColor(sf::Color(180, 83, 9));
    c4Sub.setPosition(c4X + 20.f, kpiY + 48.f);
    window.draw(c4Sub);

    // ==========================================
    // 8. BOTTOM COPYRIGHT BAR (y = ST_FOOTER_Y = 924.f)
    // ==========================================
    float footY = ST_FOOTER_Y;
    sf::Text footCpr(toSfString("© 2025 Cinestar CineOS. Bản quyền thuộc Cinestar Cinema Complex."), Theme::FontSecondary, 11);
    footCpr.setFillColor(sf::Color(148, 163, 184));
    footCpr.setPosition(leftColX, footY);
    window.draw(footCpr);

    char syncInfoBuf[128];
    snprintf(syncInfoBuf, sizeof(syncInfoBuf), "Đồng bộ thực: %zu phòng chiếu (%zu suất)  •  Hotline kỹ thuật: 1900 1234",
             roomList.size(), showtimeList.size());
    sf::Text footSync(toSfString(syncInfoBuf), Theme::FontSecondary, 11);
    footSync.setFillColor(sf::Color(100, 116, 139));
    footSync.setPosition(rightColX + rightColW - footSync.getLocalBounds().width, footY);
    window.draw(footSync);
}
} // namespace UI