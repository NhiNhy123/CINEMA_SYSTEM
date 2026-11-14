#include "../../include/UI/CinemaUIApp.h"
#include "../../include/ActivityLog.h"
#include <fstream>
#include <algorithm>
#include <cmath>
#include <iomanip>
#include <sstream>

namespace UI {
void CinemaUIApp::reloadMoviePosterTextures() {
    moviePosterTextures.clear();
    for (const auto& m : movieList) {
        std::string id = m.getMaPhim();
        std::string dbPath = m.getPoster();
        std::vector<std::string> candidates;
        if (!dbPath.empty()) {
            candidates.push_back(dbPath);
            candidates.push_back("assets/posters/" + dbPath);
            candidates.push_back("assets/" + dbPath);
            const size_t separator = dbPath.find_last_of("/\\");
            const std::string fileName = dbPath.substr(
                separator == std::string::npos ? 0 : separator + 1
            );
            candidates.push_back("assets/posters/" + fileName);
            const size_t extension = fileName.find_last_of('.');
            if (extension != std::string::npos &&
                fileName.substr(extension) == ".jgp") {
                candidates.push_back(
                    "assets/posters/" + fileName.substr(0, extension) + ".jpg"
                );
            }
        }
        candidates.push_back("assets/posters/" + id + ".jpg");
        candidates.push_back("assets/posters/" + id + ".png");
        candidates.push_back("assets/posters/" + id + ".jpeg");
        candidates.push_back("assets/posters/" + id + ".webp");
        candidates.push_back("assets/" + id + ".jpg");
        candidates.push_back("assets/" + id + ".png");

        for (const auto& path : candidates) {
            std::ifstream f(path.c_str(), std::ios::binary);
            if (!f.good()) continue;
            f.close();
            std::shared_ptr<sf::Texture> tex(new sf::Texture());
            if (tex->loadFromFile(path)) {
                tex->setSmooth(true);
                moviePosterTextures[id] = tex;
                break;
            }
        }
    }
}

void CinemaUIApp::syncMovieFormAgeDropdown(const std::string& ageCode) {
    if (!movieFormAgeDropdown) return;
    if (ageCode.empty()) {
        movieFormAgeDropdown->setSelectedIndex(-1);
        return;
    }
    bool found = false;
    for (const auto& r : customAgeRatings) {
        if (r == ageCode) { found = true; break; }
    }
    if (!found && ageCode != "P" && ageCode != "K" && ageCode != "T13" && ageCode != "T16" && ageCode != "T18" && ageCode != "T21") {
        customAgeRatings.push_back(ageCode);
    }
    refreshAgeRatingDropdowns();
}

void CinemaUIApp::addNewAgeRating(const std::string& newAge) {
    if (newAge.empty()) return;
    bool found = false;
    for (const auto& r : customAgeRatings) {
        if (r == newAge) { found = true; break; }
    }
    if (!found) {
        customAgeRatings.push_back(newAge);
    }
    if (inputMovieAgeRating) inputMovieAgeRating->setText(newAge);
    refreshAgeRatingDropdowns();
    syncMovieFormAgeDropdown(newAge);
}

void CinemaUIApp::setupMoviesView() {
    float leftColX = sidebarWidth + 24.f;
    float leftColW = 928.f;
    float rightColX = sidebarWidth + 968.f;
    float rightColW = 486.f;

    // Sub-tab buttons matching reference image
    btnSubTabMovies = std::unique_ptr<RoundRectButton>(new RoundRectButton(
        "",
        sf::Vector2f(leftColX, 180.f),
        sf::Vector2f(205.f, 40.f),
        ButtonStyle::Ghost,
        8.f
    ));
    btnSubTabMovies->setFontSize(16);
    btnSubTabMovies->setOutline(0.f, sf::Color::Transparent);
    btnSubTabMovies->setCustomColors(
        sf::Color::Transparent,
        sf::Color(226, 232, 240, 70),
        sf::Color(203, 213, 225, 110),
        sf::Color::Transparent
    );
    btnSubTabMovies->setOnClick([this]() {
        movieSubTab = 0;
        showMovieSpotlight = false;
        spotlightMovieIdx = -1;
    });

    btnSubTabGenres = std::unique_ptr<RoundRectButton>(new RoundRectButton(
        "",
        sf::Vector2f(leftColX + 215.f, 180.f),
        sf::Vector2f(205.f, 40.f),
        ButtonStyle::Ghost,
        8.f
    ));
    btnSubTabGenres->setFontSize(16);
    btnSubTabGenres->setOutline(0.f, sf::Color::Transparent);
    btnSubTabGenres->setCustomColors(
        sf::Color::Transparent,
        sf::Color(226, 232, 240, 70),
        sf::Color(203, 213, 225, 110),
        sf::Color::Transparent
    );
    btnSubTabGenres->setOnClick([this]() {
        movieSubTab = 1;
        showMovieSpotlight = false;
        spotlightMovieIdx = -1;
    });

    // Quick search input inside Left Card (cardY = 232.f)
    movieSearchInput = std::unique_ptr<UIInputBox>(new UIInputBox(
        sf::Vector2f(leftColX + 108.f, 248.f),
        sf::Vector2f(305.f, 36.f),
        "Tìm theo Mã, Tên, Thể loại...",
        Theme::SizeBody
    ));
    movieSearchInput->setTextInset(38.f);
    movieSearchInput->setCornerRadius(8.f);
    movieSearchInput->setTextInset(35.f);
    movieSearchInput->setColors(
        sf::Color(241, 245, 249),
        sf::Color(226, 232, 240),
        sf::Color(2, 132, 199),
        sf::Color(15, 23, 42),
        sf::Color(148, 163, 184)
    );

    // Age Rating Filter Dropdown (Scrollable Dropdown)
    movieAgeFilterDropdown = std::unique_ptr<DropdownBox>(new DropdownBox(
        sf::Vector2f(leftColX + 416.f, 248.f),
        sf::Vector2f(248.f, 36.f),
        {"Tất cả độ tuổi"},
        0
    ));
    movieAgeFilterDropdown->setFontSize(13);
    movieAgeFilterDropdown->setLightStyle(true);
    movieAgeFilterDropdown->setMaxVisibleItems(6);
    movieAgeFilterDropdown->setPlaceholder("Tất cả độ tuổi");
    movieAgeFilterDropdown->setOnSelect([this](int idx, const std::string& val) {
        if (idx == 0 || val.find("Tất cả") != std::string::npos) {
            movieAgeFilter = "";
        } else {
            std::string code = val;
            size_t sp = val.find(' ');
            if (sp != std::string::npos) code = val.substr(0, sp);
            movieAgeFilter = code;
        }
        movieCurrentPage = 0;
        showMovieSpotlight = false;
        spotlightMovieIdx = -1;
    });

    // "+ Thêm mới phim" top-right button inside Left Card filter bar
    btnQuickAddMovie = std::unique_ptr<RoundRectButton>(new RoundRectButton(
        "+ Thêm mới phim",
        sf::Vector2f(leftColX + 680.f, 248.f),
        sf::Vector2f(230.f, 36.f),
        ButtonStyle::Primary,
        8.f
    ));
    btnQuickAddMovie->setFontSize(14);
    btnQuickAddMovie->setCustomColors(
        sf::Color(3, 105, 161),
        sf::Color(2, 132, 199),
        sf::Color(7, 89, 133),
        sf::Color(203, 213, 225)
    );
    btnQuickAddMovie->setOnClick([this]() {
        selectedMovieRowIdx = -1;
        showMovieSpotlight = false;
        spotlightMovieIdx = -1;
        inputMovieId->setText("");
        inputMovieTitle->setText("");
        inputMovieDuration->setText("");
        inputMovieReleaseDate->setText("");
        inputMovieAgeRating->setText("");
        syncMovieFormAgeDropdown("");
        refreshAgeRatingDropdowns();
        syncMovieFormAgeDropdown("");
        inputMovieGenre->setText("");
        if (inputMoviePoster) inputMoviePoster->setText("");
        inputMovieDesc->setText("");
    });

    // Movie Form Inputs (Right Card: rightColX, cardY = 232.f)
    float inputX = rightColX + 136.f;
    float inputW = 330.f;
    float inputH = 35.f;
    sf::Color softInputBg(241, 245, 249);
    sf::Color softInputBorder(226, 232, 240);
    sf::Color focusBlue(2, 132, 199);

    inputMovieId = std::unique_ptr<UIInputBox>(new UIInputBox(
        sf::Vector2f(inputX, 286.f),
        sf::Vector2f(inputW, inputH),
        "Nhập mã phim (VD: P08)...",
        Theme::SizeBody
    ));
    inputMovieId->setColors(softInputBg, softInputBorder, focusBlue, sf::Color(30, 41, 59), sf::Color(148, 163, 184));
    inputMovieId->setCornerRadius(8.f);

    inputMovieTitle = std::unique_ptr<UIInputBox>(new UIInputBox(
        sf::Vector2f(inputX, 331.f),
        sf::Vector2f(inputW, inputH),
        "Nhập tên phim...",
        Theme::SizeBody
    ));
    inputMovieTitle->setColors(softInputBg, softInputBorder, focusBlue, sf::Color(15, 23, 42), sf::Color(148, 163, 184));
    inputMovieTitle->setCornerRadius(8.f);

    inputMovieDuration = std::unique_ptr<UIInputBox>(new UIInputBox(
        sf::Vector2f(inputX, 376.f),
        sf::Vector2f(inputW, inputH),
        "Số phút (VD: 120)...",
        Theme::SizeBody
    ));
    inputMovieDuration->setRightInset(50.f);
    inputMovieDuration->setColors(softInputBg, softInputBorder, focusBlue, sf::Color(15, 23, 42), sf::Color(148, 163, 184));
    inputMovieDuration->setCornerRadius(8.f);

    inputMovieReleaseDate = std::unique_ptr<UIInputBox>(new UIInputBox(
        sf::Vector2f(inputX, 421.f),
        sf::Vector2f(inputW, inputH),
        "DD/MM/YYYY",
        Theme::SizeBody
    ));
    inputMovieReleaseDate->setColors(softInputBg, softInputBorder, focusBlue, sf::Color(15, 23, 42), sf::Color(148, 163, 184));
    inputMovieReleaseDate->setCornerRadius(8.f);

    inputMovieAgeRating = std::unique_ptr<UIInputBox>(new UIInputBox(
        sf::Vector2f(inputX, 466.f),
        sf::Vector2f(inputW, inputH),
        "P, K, T13, T18",
        Theme::SizeBody
    ));
    inputMovieAgeRating->setColors(softInputBg, softInputBorder, focusBlue, sf::Color(15, 23, 42), sf::Color(148, 163, 184));
    inputMovieAgeRating->setCornerRadius(8.f);

    // Dropdown Age Rating Selector (Right Form)
    movieFormAgeDropdown = std::unique_ptr<DropdownBox>(new DropdownBox(
        sf::Vector2f(inputX, 466.f),
        sf::Vector2f(inputW, inputH),
        {},
        0
    ));
    movieFormAgeDropdown->setFontSize(13);
    movieFormAgeDropdown->setLightStyle(true);
    movieFormAgeDropdown->setMaxVisibleItems(6);
    movieFormAgeDropdown->setPlaceholder("-- Chọn độ tuổi --");
    movieFormAgeDropdown->setOnSelect([this](int idx, const std::string& val) {
        (void)idx;
        if (val.find("+ Thêm độ tuổi mới") != std::string::npos) {
            showAgeInputDialog = true;
            if (inputNewAgeRating) inputNewAgeRating->setText("");
            syncMovieFormAgeDropdown(inputMovieAgeRating ? inputMovieAgeRating->getText() : "");
        } else {
            std::string code = val;
            size_t sp = val.find(' ');
            if (sp != std::string::npos) code = val.substr(0, sp);
            if (inputMovieAgeRating) inputMovieAgeRating->setText(code);
        }
    });

    // Custom Age Rating input modal components
    float modalW = 440.f;
    float modalH = 220.f;
    float modalX = (1728.f - modalW) / 2.f;
    float modalY = (972.f - modalH) / 2.f;

    inputNewAgeRating = std::unique_ptr<UIInputBox>(new UIInputBox(
        sf::Vector2f(modalX + 30.f, modalY + 85.f),
        sf::Vector2f(modalW - 60.f, 40.f),
        "Nhập mã độ tuổi (VD: T14, T16, T21)...",
        Theme::SizeBody
    ));
    inputNewAgeRating->setColors(sf::Color(241, 245, 249), sf::Color(226, 232, 240), focusBlue, sf::Color(15, 23, 42), sf::Color(148, 163, 184));
    inputNewAgeRating->setCornerRadius(8.f);

    float mBtnW = (modalW - 60.f - 16.f) / 2.f;
    btnCancelNewAge = std::unique_ptr<RoundRectButton>(new RoundRectButton(
        "Hủy bỏ",
        sf::Vector2f(modalX + 30.f, modalY + 145.f),
        sf::Vector2f(mBtnW, 40.f),
        ButtonStyle::Ghost,
        8.f
    ));
    btnCancelNewAge->setFontSize(14);
    btnCancelNewAge->setOnClick([this]() {
        showAgeInputDialog = false;
    });

    btnSaveNewAge = std::unique_ptr<RoundRectButton>(new RoundRectButton(
        "Thêm độ tuổi",
        sf::Vector2f(modalX + 30.f + mBtnW + 16.f, modalY + 145.f),
        sf::Vector2f(mBtnW, 40.f),
        ButtonStyle::Primary,
        8.f
    ));
    btnSaveNewAge->setFontSize(14);
    btnSaveNewAge->setOnClick([this]() {
        if (!inputNewAgeRating) return;
        std::string raw = inputNewAgeRating->getText();
        size_t s = raw.find_first_not_of(" \t\r\n");
        if (s == std::string::npos) return;
        std::string clean = raw.substr(s, raw.find_last_not_of(" \t\r\n") - s + 1);
        std::transform(clean.begin(), clean.end(), clean.begin(), ::toupper);
        if (clean.rfind("T", 0) != 0 && clean != "P" && clean != "K" && clean != "C13" && clean != "C16" && clean != "C18") {
            bool allDigits = true;
            for (char c : clean) if (c < '0' || c > '9') allDigits = false;
            if (allDigits) clean = "T" + clean;
        }
        addNewAgeRating(clean);
        showAgeInputDialog = false;
    });

    refreshAgeRatingDropdowns();

    inputMovieGenre = std::unique_ptr<UIInputBox>(new UIInputBox(
        sf::Vector2f(inputX, 511.f),
        sf::Vector2f(inputW - 92.f, inputH),
        "Nhập hoặc bấm Chọn...",
        Theme::SizeBody
    ));
    inputMovieGenre->setColors(softInputBg, softInputBorder, focusBlue, sf::Color(15, 23, 42), sf::Color(148, 163, 184));
    inputMovieGenre->setCornerRadius(8.f);

    btnSelectGenre = std::unique_ptr<RoundRectButton>(new RoundRectButton(
        "Chọn...",
        sf::Vector2f(inputX + inputW - 84.f, 511.f),
        sf::Vector2f(84.f, inputH),
        ButtonStyle::Ghost,
        8.f
    ));
    btnSelectGenre->setFontSize(13);
    btnSelectGenre->setCustomColors(sf::Color(224, 242, 254), sf::Color(186, 230, 253), sf::Color(125, 211, 252), sf::Color(241, 245, 249));
    btnSelectGenre->setOutline(1.f, sf::Color(186, 230, 253));
    btnSelectGenre->setTextColor(sf::Color(3, 105, 161));
    btnSelectGenre->setOnClick([this]() {
        if (!genreList.empty()) {
            selectedGenreChoiceIdx = (selectedGenreChoiceIdx + 1) % genreList.size();
            inputMovieGenre->setText(genreList[selectedGenreChoiceIdx].second);
        }
    });

    // Dedicated Poster Path Input (paired with visual Poster Preview slot)
    inputMoviePoster = std::unique_ptr<UIInputBox>(new UIInputBox(
        sf::Vector2f(inputX + 58.f, 566.f),
        sf::Vector2f(inputW - 58.f, inputH),
        "assets/posters/ten_file.jpg",
        Theme::SizeBody
    ));
    inputMoviePoster->setTextInset(34.f);
    inputMoviePoster->setColors(softInputBg, softInputBorder, focusBlue, sf::Color(15, 23, 42), sf::Color(148, 163, 184));
    inputMoviePoster->setCornerRadius(8.f);

    inputMovieDesc = std::unique_ptr<UIInputBox>(new UIInputBox(
        sf::Vector2f(rightColX + 20.f, 650.f),
        sf::Vector2f(rightColW - 40.f, 78.f),
        "Nhập nội dung tóm tắt phim...",
        Theme::SizeBody
    ));
    inputMovieDesc->setColors(softInputBg, softInputBorder, focusBlue, sf::Color(51, 65, 85), sf::Color(148, 163, 184));
    inputMovieDesc->setCornerRadius(8.f);
    inputMovieDesc->setMultiLine(true);

    // Form Action Buttons matching reference image
    float btnW = (rightColW - 40.f - 12.f) / 2.0f; // 217.f
    float btnRow1Y = 796.f;
    float btnRow2Y = 846.f;

    btnMovieCreate = std::unique_ptr<RoundRectButton>(new RoundRectButton(
        "+ Thêm mới",
        sf::Vector2f(rightColX + 20.f, btnRow1Y),
        sf::Vector2f(btnW, 40.f),
        ButtonStyle::Success,
        8.f
    ));
    btnMovieCreate->setFontSize(14);
    btnMovieCreate->setCustomColors(sf::Color(5, 150, 105), sf::Color(4, 120, 87), sf::Color(6, 95, 70), sf::Color(203, 213, 225));
    btnMovieCreate->setOnClick([this]() {
        std::string id = inputMovieId->getText();
        size_t idStart = id.find_first_not_of(" \t\r\n");
        if (idStart == std::string::npos) {
            confirmPopup->show("Lỗi", "Vui lòng nhập mã phim trước khi thêm mới!", ButtonStyle::Danger, "Đóng", "Hủy");
            return;
        }
        id = id.substr(idStart, id.find_last_not_of(" \t\r\n") - idStart + 1);

        std::string title = inputMovieTitle->getText();
        if (title.find_first_not_of(" \t\r\n") == std::string::npos) {
            confirmPopup->show("Lỗi", "Vui lòng nhập tên phim trước khi thêm mới!", ButtonStyle::Danger, "Đóng", "Hủy");
            return;
        }

        // Check if user clicked [+ Thêm mới] while looking at an already selected movie without changing ID
        if (selectedMovieRowIdx >= 0 && selectedMovieRowIdx < static_cast<int>(movieList.size()) &&
            id == movieList[selectedMovieRowIdx].getMaPhim()) {
            selectedMovieRowIdx = -1;
            showMovieSpotlight = false;
            spotlightMovieIdx = -1;
            inputMovieId->setText("");
            inputMovieTitle->setText("");
            inputMovieDuration->setText("");
            inputMovieReleaseDate->setText("");
            inputMovieAgeRating->setText("");
            syncMovieFormAgeDropdown("");
            refreshAgeRatingDropdowns();
            inputMovieGenre->setText("");
            inputMoviePoster->setText("");
            inputMovieDesc->setText("");
            if (inputMovieId) inputMovieId->setFocused(true);
            confirmPopup->show("Chế độ thêm mới", "Đã dọn sạch biểu mẫu để thêm phim mới. Vui lòng nhập thông tin phim (Mã phim, Tên phim,...) rồi nhấn lại nút '+ Thêm mới' để lưu vào hệ thống!", ButtonStyle::Primary, "Nhập thông tin", "Đóng");
            return;
        }

        for (size_t i = 0; i < movieList.size(); ++i) {
            if (movieList[i].getMaPhim() == id) {
                confirmPopup->show("Lỗi", "Mã phim '" + id + "' đã tồn tại trong hệ thống!", ButtonStyle::Danger, "Đóng", "Hủy");
                return;
            }
        }

        int dur = 0;
        try {
            dur = std::stoi(inputMovieDuration->getText());
        } catch (...) {
            dur = 0;
        }
        if (dur <= 0) {
            confirmPopup->show("Lỗi", "Thời lượng phim phải lớn hơn 0 phút!", ButtonStyle::Danger, "Đóng", "Hủy");
            return;
        }

        std::string rel = inputMovieReleaseDate->getText();
        if (rel.empty()) {
            confirmPopup->show("Lỗi", "Vui lòng nhập ngày khởi chiếu (DD/MM/YYYY)!", ButtonStyle::Danger, "Đóng", "Hủy");
            return;
        }
        if (!DateFormat::isValidDate(rel)) {
            confirmPopup->show("Lỗi", "Ngày khởi chiếu không hợp lệ! Vui lòng nhập định dạng DD/MM/YYYY.", ButtonStyle::Danger, "Đóng", "Hủy");
            return;
        }
        std::string normRel = DateFormat::normalizeDate(rel);
        int todayKey = 0;
        if (DateFormat::todayDateKey(todayKey)) {
            if (DateFormat::dateKey(normRel) < todayKey) {
                confirmPopup->show("Lỗi", "Ngày khởi chiếu không được nằm trong quá khứ!", ButtonStyle::Danger, "Đóng", "Hủy");
                return;
            }
        }

        std::string ageStr = inputMovieAgeRating->getText();
        int age = 0;
        if (ageStr == "P" || ageStr == "p" || ageStr == "K" || ageStr == "k") age = 0;
        else {
            std::string numOnly = "";
            for (char ch : ageStr) {
                if (ch >= '0' && ch <= '9') numOnly += ch;
            }
            try { age = numOnly.empty() ? 0 : std::stoi(numOnly); } catch (...) { age = 0; }
        }

        std::string gName = inputMovieGenre->getText();
        std::string desc = inputMovieDesc->getText();
        std::string posterPath = (inputMoviePoster && !inputMoviePoster->getText().empty())
                                 ? inputMoviePoster->getText()
                                 : ("assets/posters/" + id + ".jpg");

        int day = 1, month = 1, year = 2026;
        DateFormat::parseDate(normRel, day, month, year);
        month += 1;
        if (month > 12) { month = 1; year += 1; }
        std::ostringstream ssEnd;
        ssEnd << std::setfill('0') << std::setw(2) << day << "/"
              << std::setw(2) << month << "/" << std::setw(4) << year;
        std::string endDate = ssEnd.str();

        Movie m(id, title, dur, age, normRel, endDate, desc, posterPath);
        movieList.push_back(m);
        db.ghiDanhSachPhim(movieList);
        if (!gName.empty()) {
            std::stringstream gss(gName);
            std::string gToken;
            bool firstGenre = true;
            while (std::getline(gss, gToken, ',')) {
                size_t s = gToken.find_first_not_of(" \t\r\n");
                size_t e = gToken.find_last_not_of(" \t\r\n");
                if (s != std::string::npos && e != std::string::npos) {
                    std::string cleanG = gToken.substr(s, e - s + 1);
                    if (firstGenre) {
                        db.capNhatTheLoaiChoPhim(id, cleanG);
                        firstGenre = false;
                    } else {
                        db.themTheLoaiChoPhim(id, cleanG);
                    }
                }
            }
            db.docDanhSachTheLoai(genreList);
        }
        db.docDanhSachPhim(movieList);
        reloadMoviePosterTextures();
        ActivityLog::ghiNhan(currentUserId, currentUserRole, "THEM_PHIM", "Thêm phim mới: " + title + " (" + id + ")");
        confirmPopup->show("Thành công", "Đã thêm phim mới!", ButtonStyle::Primary, "Đóng", "Quay lại");

        selectedMovieRowIdx = -1;
        showMovieSpotlight = false;
        spotlightMovieIdx = -1;
        inputMovieId->setText("");
        inputMovieTitle->setText("");
        inputMovieDuration->setText("");
        inputMovieReleaseDate->setText("");
        inputMovieAgeRating->setText("");
        syncMovieFormAgeDropdown("");
        refreshAgeRatingDropdowns();
        refreshShowtimesDropdowns();
        inputMovieGenre->setText("");
        if (inputMoviePoster) inputMoviePoster->setText("");
        inputMovieDesc->setText("");
    });

    btnMovieUpdate = std::unique_ptr<RoundRectButton>(new RoundRectButton(
        "Cập nhật",
        sf::Vector2f(rightColX + 20.f + btnW + 12.f, btnRow1Y),
        sf::Vector2f(btnW, 40.f),
        ButtonStyle::Ghost,
        8.f
    ));
    btnMovieUpdate->setFontSize(14);
    btnMovieUpdate->setCustomColors(sf::Color(224, 242, 254), sf::Color(186, 230, 253), sf::Color(125, 211, 252), sf::Color(226, 232, 240));
    btnMovieUpdate->setOutline(1.f, sf::Color(186, 230, 253));
    btnMovieUpdate->setTextColor(sf::Color(3, 105, 161));
    btnMovieUpdate->setOnClick([this]() {
        if (selectedMovieRowIdx >= 0 && selectedMovieRowIdx < static_cast<int>(movieList.size())) {
            std::string title = inputMovieTitle->getText();
            if (title.find_first_not_of(" \t\r\n") == std::string::npos) {
                confirmPopup->show("Lỗi", "Tên phim không được để trống!", ButtonStyle::Danger, "Đóng", "Hủy");
                return;
            }

            int dur = 0;
            try {
                dur = std::stoi(inputMovieDuration->getText());
            } catch (...) {
                dur = 0;
            }
            if (dur <= 0) {
                confirmPopup->show("Lỗi", "Thời lượng phim phải lớn hơn 0 phút!", ButtonStyle::Danger, "Đóng", "Hủy");
                return;
            }

            std::string rel = inputMovieReleaseDate->getText();
            std::string oldRel = DateFormat::normalizeDate(movieList[selectedMovieRowIdx].getNgayKhoiChieu());
            if (!rel.empty()) {
                if (!DateFormat::isValidDate(rel)) {
                    confirmPopup->show("Lỗi", "Ngày khởi chiếu không hợp lệ! Vui lòng nhập định dạng DD/MM/YYYY.", ButtonStyle::Danger, "Đóng", "Hủy");
                    return;
                }
                std::string normRel = DateFormat::normalizeDate(rel);
                int todayKey = 0;
                if (normRel != oldRel && DateFormat::todayDateKey(todayKey)) {
                    if (DateFormat::dateKey(normRel) < todayKey) {
                        confirmPopup->show("Lỗi", "Ngày khởi chiếu mới không được nằm trong quá khứ!", ButtonStyle::Danger, "Đóng", "Hủy");
                        return;
                    }
                }
                rel = normRel;
            }

            std::string ageStr = inputMovieAgeRating->getText();
            int age = 0;
            if (ageStr == "P" || ageStr == "p" || ageStr == "K" || ageStr == "k") age = 0;
            else {
                std::string numOnly = "";
                for (char ch : ageStr) {
                    if (ch >= '0' && ch <= '9') numOnly += ch;
                }
                try { age = numOnly.empty() ? 0 : std::stoi(numOnly); } catch (...) { age = 0; }
            }

            std::string gName = inputMovieGenre->getText();
            std::string mId = movieList[selectedMovieRowIdx].getMaPhim();
            std::string newDesc = inputMovieDesc->getText();
            std::string newPoster = inputMoviePoster ? inputMoviePoster->getText() : "";
            std::string curGenre = db.layTheLoaiCuaPhim(mId);
            std::string curPoster = movieList[selectedMovieRowIdx].getPoster();
            if (curPoster.empty() || curPoster == "assets/posters/poster_default.jpg") {
                curPoster = "assets/posters/" + mId + ".jpg";
            }

            bool isUnchanged = (title == movieList[selectedMovieRowIdx].getTenPhim()) &&
                               (dur == movieList[selectedMovieRowIdx].getThoiLuong()) &&
                               (age == movieList[selectedMovieRowIdx].getNhanDoTuoi()) &&
                               (rel.empty() || rel == oldRel) &&
                               (gName == curGenre) &&
                               (newDesc == movieList[selectedMovieRowIdx].getMoTa()) &&
                               (newPoster.empty() || newPoster == curPoster);
            if (isUnchanged) {
                confirmPopup->show("Thông báo", "Chưa có thông tin nào thay đổi!", ButtonStyle::Primary, "Đóng", "Quay lại");
                return;
            }

            movieList[selectedMovieRowIdx].setTenPhim(title);
            movieList[selectedMovieRowIdx].setThoiLuong(dur);
            movieList[selectedMovieRowIdx].setNhanDoTuoi(age);
            if (!rel.empty()) {
                movieList[selectedMovieRowIdx].setNgayKhoiChieu(rel);
                int day = 1, month = 1, year = 2026;
                DateFormat::parseDate(rel, day, month, year);
                month += 1;
                if (month > 12) { month = 1; year += 1; }
                std::ostringstream ssEnd;
                ssEnd << std::setfill('0') << std::setw(2) << day << "/"
                      << std::setw(2) << month << "/" << std::setw(4) << year;
                movieList[selectedMovieRowIdx].setNgayKetThuc(ssEnd.str());
            }
            movieList[selectedMovieRowIdx].setMoTa(newDesc);
            if (!newPoster.empty()) {
                movieList[selectedMovieRowIdx].setPoster(newPoster);
            }

            db.ghiDanhSachPhim(movieList);
            if (!gName.empty()) {
                std::stringstream gss(gName);
                std::string gToken;
                bool firstGenre = true;
                while (std::getline(gss, gToken, ',')) {
                    size_t s = gToken.find_first_not_of(" \t\r\n");
                    size_t e = gToken.find_last_not_of(" \t\r\n");
                    if (s != std::string::npos && e != std::string::npos) {
                        std::string cleanG = gToken.substr(s, e - s + 1);
                        if (firstGenre) {
                            db.capNhatTheLoaiChoPhim(mId, cleanG);
                            firstGenre = false;
                        } else {
                            db.themTheLoaiChoPhim(mId, cleanG);
                        }
                    }
                }
                db.docDanhSachTheLoai(genreList);
            }
            db.docDanhSachPhim(movieList);
            reloadMoviePosterTextures();
            refreshAgeRatingDropdowns();
            refreshShowtimesDropdowns();

            ActivityLog::ghiNhan(currentUserId, currentUserRole, "SUA_PHIM", "Cập nhật thông tin phim: " + title + " (" + mId + ")");
            confirmPopup->show("Thành công", "Đã cập nhật thông tin!", ButtonStyle::Primary, "Đóng", "Quay lại");
        } else {
            confirmPopup->show("Thông báo", "Vui lòng chọn 1 phim từ danh sách để cập nhật!", ButtonStyle::Primary, "Đóng", "Hủy");
        }
    });

    btnMovieDelete = std::unique_ptr<RoundRectButton>(new RoundRectButton(
        "Xóa",
        sf::Vector2f(rightColX + 20.f, btnRow2Y),
        sf::Vector2f(btnW, 40.f),
        ButtonStyle::Ghost,
        8.f
    ));
    btnMovieDelete->setFontSize(14);
    btnMovieDelete->setCustomColors(sf::Color(254, 226, 226), sf::Color(254, 202, 202), sf::Color(252, 165, 165), sf::Color(226, 232, 240));
    btnMovieDelete->setOutline(1.f, sf::Color(252, 165, 165));
    btnMovieDelete->setTextColor(sf::Color(220, 38, 38));
    btnMovieDelete->setOnClick([this]() {
        if (selectedMovieRowIdx >= 0 && selectedMovieRowIdx < static_cast<int>(movieList.size())) {
            std::string pName = movieList[selectedMovieRowIdx].getTenPhim();
            confirmPopup->show("XÁC NHẬN XÓA", "Bạn có chắc chắn muốn xóa phim '" + pName + "' khỏi hệ thống?", ButtonStyle::Danger, "Xác nhận xóa", "Hủy");
            confirmPopup->setOnConfirm([this, pName]() {
                movieList.erase(movieList.begin() + selectedMovieRowIdx);
                db.ghiDanhSachPhim(movieList);
                reloadMoviePosterTextures();
                refreshAgeRatingDropdowns();
                refreshShowtimesDropdowns();
                ActivityLog::ghiNhan(currentUserId, currentUserRole, "XOA_PHIM", "Xóa phim: " + pName);
                selectedMovieRowIdx = -1;
                showMovieSpotlight = false;
                spotlightMovieIdx = -1;
            });
        }
    });

    btnMovieClear = std::unique_ptr<RoundRectButton>(new RoundRectButton(
        "Clear Form",
        sf::Vector2f(rightColX + 20.f + btnW + 12.f, btnRow2Y),
        sf::Vector2f(btnW, 40.f),
        ButtonStyle::Ghost,
        8.f
    ));
    btnMovieClear->setFontSize(14);
    btnMovieClear->setCustomColors(sf::Color(241, 245, 249), sf::Color(226, 232, 240), sf::Color(203, 213, 225), sf::Color(226, 232, 240));
    btnMovieClear->setOutline(1.f, sf::Color(203, 213, 225));
    btnMovieClear->setTextColor(sf::Color(51, 65, 85));
    btnMovieClear->setOnClick([this]() {
        selectedMovieRowIdx = -1;
        showMovieSpotlight = false;
        spotlightMovieIdx = -1;
        inputMovieId->setText("");
        inputMovieTitle->setText("");
        inputMovieDuration->setText("");
        inputMovieReleaseDate->setText("");
        inputMovieAgeRating->setText("");
        syncMovieFormAgeDropdown("");
        refreshAgeRatingDropdowns();
        refreshShowtimesDropdowns();
        inputMovieGenre->setText("");
        if (inputMoviePoster) inputMoviePoster->setText("");
        inputMovieDesc->setText("");
    });

    // Populate initial form with 1st movie
    if (!movieList.empty()) {
        selectedMovieRowIdx = 0;
        inputMovieId->setText(movieList[0].getMaPhim());
        inputMovieTitle->setText(movieList[0].getTenPhim());
        inputMovieDuration->setText(std::to_string(movieList[0].getThoiLuong()));
        inputMovieReleaseDate->setText(movieList[0].getNgayKhoiChieu());
        int a0 = movieList[0].getNhanDoTuoi();
        std::string code0 = (a0 <= 0) ? "P" : ("T" + std::to_string(a0));
        inputMovieAgeRating->setText(code0);
        syncMovieFormAgeDropdown(code0);
        inputMovieGenre->setText(db.layTheLoaiCuaPhim(movieList[0].getMaPhim()));
        std::string p0 = movieList[0].getPoster();
        if (p0.empty() || p0 == "assets/posters/poster_default.jpg") {
            p0 = "assets/posters/" + movieList[0].getMaPhim() + ".jpg";
        }
        if (inputMoviePoster) inputMoviePoster->setText(p0);
        inputMovieDesc->setText(movieList[0].getMoTa());
    }

    // Genre Form Setup (Matching Reference UI & Linked to SQLite Backend)
    selectedGenreColorIdx = 0;
    genreSortAsc = true;
    genreColorMap["TL01"] = 0; // Red
    genreColorMap["TL02"] = 1; // Purple
    genreColorMap["TL03"] = 2; // Pink
    genreColorMap["TL04"] = 3; // Amber
    genreColorMap["TL05"] = 4; // Blue
    genreColorMap["TL06"] = 5; // Emerald
    genreColorMap["TL07"] = 6; // Cyan
    genreColorMap["TL08"] = 7; // Orange
    genreColorMap["TL09"] = 8; // Crimson

    genreSearchInput = std::unique_ptr<UIInputBox>(new UIInputBox(
        sf::Vector2f(rightColX - 40.f, 180.f),
        sf::Vector2f(310.f, 40.f),
        "Tìm tên hoặc mã thể loại...",
        13
    ));
    genreSearchInput->setTextInset(38.f);
    genreSearchInput->setColors(sf::Color::White, sf::Color(226, 232, 240), focusBlue, sf::Color(15, 23, 42), sf::Color(148, 163, 184));
    genreSearchInput->setCornerRadius(20.f);
    genreSearchInput->setTextInset(35.f);

    btnExportGenres = std::unique_ptr<RoundRectButton>(new RoundRectButton(
        "    Xuất danh sách",
        sf::Vector2f(rightColX + 282.f, 180.f),
        sf::Vector2f(170.f, 40.f),
        ButtonStyle::Ghost,
        10.f
    ));
    btnExportGenres->setFontSize(13);
    btnExportGenres->setCustomColors(sf::Color::White, sf::Color(241, 245, 249), sf::Color(226, 232, 240), sf::Color(241, 245, 249));
    btnExportGenres->setOutline(1.f, sf::Color(226, 232, 240));
    btnExportGenres->setTextColor(sf::Color(30, 41, 59));
    btnExportGenres->setOnClick([this]() {
        std::ofstream out("DATA/danh_sach_the_loai.csv", std::ios::out | std::ios::trunc);
        if (out.is_open()) {
            out << "MaTheLoai,TenTheLoai\n";
            for (const auto& g : genreList) {
                out << g.first << "," << g.second << "\n";
            }
            out.close();
        }
        ActivityLog::ghiNhan(currentUserId, currentUserRole, "XUAT_THE_LOAI", "Xuất danh sách thể loại phim");
        confirmPopup->show("Thành công", "Đã xuất danh sách thể loại!", ButtonStyle::Primary, "Đóng", "Quay lại");
    });

    btnQuickAddGenre = std::unique_ptr<RoundRectButton>(new RoundRectButton(
        "+ Thêm thể loại",
        sf::Vector2f(leftColX + leftColW - 380.f, 247.f),
        sf::Vector2f(135.f, 28.f),
        ButtonStyle::Success,
        6.f
    ));
    btnQuickAddGenre->setFontSize(12);
    btnQuickAddGenre->setCustomColors(
        sf::Color(5, 150, 105),
        sf::Color(4, 120, 87),
        sf::Color(6, 95, 70),
        sf::Color(203, 213, 225)
    );
    btnQuickAddGenre->setOnClick([this]() {
        selectedGenreRowIdx = -1;
        size_t nextId = 1;
        std::string nextCode;
        bool exists = false;
        do {
            std::ostringstream ss;
            ss << "TL" << std::setw(2) << std::setfill('0') << nextId++;
            nextCode = ss.str();
            exists = false;
            for (const auto& g : genreList) {
                if (g.first == nextCode) { exists = true; break; }
            }
        } while (exists);
        inputGenreId->setText(nextCode);
        inputGenreName->setText("");
        if (inputGenreName) inputGenreName->setFocused(true);
        confirmPopup->show("Chế độ thêm mới", "Đã chuyển sang chế độ thêm thể loại mới (Mã: " + nextCode + "). Vui lòng nhập tên thể loại vào ô 'TÊN THỂ LOẠI' rồi nhấn '+ Thêm thể loại' (hoặc 'Lưu thay đổi')!", ButtonStyle::Primary, "Nhập tên", "Đóng");
    });

    float gInputX = rightColX + 24.f;
    float gInputW = rightColW - 48.f;
    float gInputH = 44.f;
    float gHalfBtnW = (gInputW - 14.f) / 2.f;

    inputGenreId = std::unique_ptr<UIInputBox>(new UIInputBox(
        sf::Vector2f(gInputX, 352.f),
        sf::Vector2f(gInputW, gInputH),
        "TL01",
        14
    ));
    inputGenreId->setRightInset(40.f);
    inputGenreId->setColors(sf::Color(241, 245, 249), sf::Color(226, 232, 240), focusBlue, sf::Color(30, 41, 59), sf::Color(148, 163, 184));
    inputGenreId->setCornerRadius(10.f);

    inputGenreName = std::unique_ptr<UIInputBox>(new UIInputBox(
        sf::Vector2f(gInputX, 442.f),
        sf::Vector2f(gInputW, gInputH),
        "Nhập tên thể loại...",
        14
    ));
    inputGenreName->setRightInset(40.f);
    inputGenreName->setColors(sf::Color::White, sf::Color(203, 213, 225), focusBlue, sf::Color(15, 23, 42), sf::Color(148, 163, 184));
    inputGenreName->setCornerRadius(10.f);

    if (!genreList.empty()) {
        selectedGenreRowIdx = 0;
        inputGenreId->setText(genreList[0].first);
        inputGenreName->setText(genreList[0].second);
    }

    btnGenreCreate = std::unique_ptr<RoundRectButton>(new RoundRectButton(
        "  + Thêm thể loại",
        sf::Vector2f(gInputX, 616.f),
        sf::Vector2f(gHalfBtnW, 44.f),
        ButtonStyle::Success,
        10.f
    ));
    btnGenreCreate->setFontSize(14);
    btnGenreCreate->setCustomColors(sf::Color(5, 150, 105), sf::Color(4, 120, 87), sf::Color(6, 95, 70), sf::Color(203, 213, 225));
    btnGenreCreate->setOnClick([this]() {
        std::string gName = inputGenreName ? inputGenreName->getText() : "";
        size_t s = gName.find_first_not_of(" \t\r\n");
        size_t e = gName.find_last_not_of(" \t\r\n");
        std::string trimmedName = (s == std::string::npos) ? "" : gName.substr(s, e - s + 1);

        // Case 1: If user is viewing an existing genre and clicked [+ Thêm thể loại] without changing name
        if (selectedGenreRowIdx >= 0 && selectedGenreRowIdx < static_cast<int>(genreList.size()) &&
            (trimmedName.empty() || trimmedName == genreList[selectedGenreRowIdx].second)) {
            selectedGenreRowIdx = -1;
            size_t nextId = 1;
            std::string nextCode;
            bool exists = false;
            do {
                std::ostringstream ss;
                ss << "TL" << std::setw(2) << std::setfill('0') << nextId++;
                nextCode = ss.str();
                exists = false;
                for (const auto& g : genreList) {
                    if (g.first == nextCode) { exists = true; break; }
                }
            } while (exists);
            inputGenreId->setText(nextCode);
            inputGenreName->setText("");
            if (inputGenreName) inputGenreName->setFocused(true);
            confirmPopup->show("Chế độ thêm mới", "Đã chuyển sang chế độ thêm thể loại mới (Mã dự kiến: " + nextCode + "). Vui lòng nhập tên thể loại vào ô rồi nhấn lại '+ Thêm thể loại' (hoặc 'Lưu thay đổi') để lưu!", ButtonStyle::Primary, "Nhập tên", "Đóng");
            return;
        }

        // Case 2: User wants to save a new genre
        if (trimmedName.empty()) {
            confirmPopup->show("Lỗi", "Tên thể loại không được để trống! Vui lòng nhập tên thể loại mới.", ButtonStyle::Danger, "Đóng", "Quay lại");
            if (inputGenreName) inputGenreName->setFocused(true);
            return;
        }

        // Duplicate check
        for (const auto& g : genreList) {
            if (g.second == trimmedName) {
                confirmPopup->show("Thông báo", "Thể loại '" + trimmedName + "' đã tồn tại trong hệ thống (Mã: " + g.first + ")!", ButtonStyle::Primary, "Đóng", "Quay lại");
                return;
            }
        }

        if (db.themLoaiTheLoai(trimmedName)) {
            genreList.clear();
            db.docDanhSachTheLoai(genreList);
            if (!genreList.empty()) {
                selectedGenreRowIdx = static_cast<int>(genreList.size()) - 1;
                std::string newId = genreList.back().first;
                genreColorMap[newId] = selectedGenreColorIdx;
                inputGenreId->setText(newId);
                inputGenreName->setText(genreList.back().second);
            }
            refreshShowtimesDropdowns();
            ActivityLog::ghiNhan(currentUserId, currentUserRole, "THEM_THE_LOAI", "Thêm thể loại mới: " + trimmedName);
            confirmPopup->show("Thành công", "Đã thêm thể loại mới '" + trimmedName + "' thành công!", ButtonStyle::Primary, "Đóng", "Quay lại");
        } else {
            confirmPopup->show("Lỗi", "Không thể thêm thể loại mới vào cơ sở dữ liệu!", ButtonStyle::Danger, "Đóng", "Quay lại");
        }
    });

    btnGenreDelete = std::unique_ptr<RoundRectButton>(new RoundRectButton(
        "   Xóa thể loại",
        sf::Vector2f(gInputX + gHalfBtnW + 14.f, 616.f),
        sf::Vector2f(gHalfBtnW, 44.f),
        ButtonStyle::Danger,
        10.f
    ));
    btnGenreDelete->setFontSize(14);
    btnGenreDelete->setCustomColors(sf::Color(225, 29, 72), sf::Color(190, 18, 60), sf::Color(159, 18, 57), sf::Color(203, 213, 225));
    btnGenreDelete->setOnClick([this]() {
        if (selectedGenreRowIdx >= 0 && selectedGenreRowIdx < static_cast<int>(genreList.size())) {
            std::string gId = genreList[selectedGenreRowIdx].first;
            std::string gName = genreList[selectedGenreRowIdx].second;
            confirmPopup->show("Xác nhận xóa", "Bạn có chắc chắn muốn xóa thể loại '" + gName + "'?", ButtonStyle::Danger, "Xóa thể loại", "Quay lại");
            confirmPopup->setOnConfirm([this, gId, gName]() {
                if (db.xoaLoaiTheLoai(gId)) {
                    genreList.clear();
                    db.docDanhSachTheLoai(genreList);
                    if (!genreList.empty()) {
                        selectedGenreRowIdx = 0;
                        inputGenreId->setText(genreList[0].first);
                        inputGenreName->setText(genreList[0].second);
                    } else {
                        selectedGenreRowIdx = -1;
                        inputGenreId->setText("TL01");
                        inputGenreName->setText("");
                    }
                    ActivityLog::ghiNhan(currentUserId, currentUserRole, "XOA_THE_LOAI", "Xóa thể loại: " + gName + " (" + gId + ")");
                }
            });
        } else {
            confirmPopup->show("Thông báo", "Vui lòng chọn 1 thể loại từ danh sách để xóa!", ButtonStyle::Primary, "Đóng", "Quay lại");
        }
    });

    btnGenreUpdate = std::unique_ptr<RoundRectButton>(new RoundRectButton(
        "   Lưu thay đổi",
        sf::Vector2f(gInputX, 672.f),
        sf::Vector2f(gInputW, 44.f),
        ButtonStyle::Primary,
        10.f
    ));
    btnGenreUpdate->setFontSize(14);
    btnGenreUpdate->setCustomColors(sf::Color(2, 132, 199), sf::Color(3, 105, 161), sf::Color(7, 89, 133), sf::Color(203, 213, 225));
    btnGenreUpdate->setOnClick([this]() {
        std::string gName = inputGenreName ? inputGenreName->getText() : "";
        size_t s = gName.find_first_not_of(" \t\r\n");
        size_t e = gName.find_last_not_of(" \t\r\n");
        if (s == std::string::npos) {
            confirmPopup->show("Lỗi", "Tên thể loại không được để trống!", ButtonStyle::Danger, "Đóng", "Quay lại");
            return;
        }
        gName = gName.substr(s, e - s + 1);

        // If in Add New mode, saving changes will add the new genre!
        if (selectedGenreRowIdx < 0 || selectedGenreRowIdx >= static_cast<int>(genreList.size())) {
            if (btnGenreCreate) btnGenreCreate->triggerClick();
            return;
        }

        std::string gId = genreList[selectedGenreRowIdx].first;
        std::string oldName = genreList[selectedGenreRowIdx].second;
        int oldCol = genreColorMap.count(gId) ? genreColorMap[gId] : (selectedGenreRowIdx % 5);
        if (gName == oldName && selectedGenreColorIdx == oldCol) {
            confirmPopup->show("Thông báo", "Chưa có thông tin nào thay đổi!", ButtonStyle::Primary, "Đóng", "Quay lại");
            return;
        }
        if (gName != oldName) {
            if (!db.suaLoaiTheLoai(gId, gName)) {
                confirmPopup->show("Lỗi", "Tên thể loại đã tồn tại hoặc không hợp lệ!", ButtonStyle::Danger, "Đóng", "Quay lại");
                return;
            }
            genreList.clear();
            db.docDanhSachTheLoai(genreList);
        }
        genreColorMap[gId] = selectedGenreColorIdx;
        refreshShowtimesDropdowns();
        ActivityLog::ghiNhan(currentUserId, currentUserRole, "SUA_THE_LOAI", "Cập nhật thể loại: " + gName + " (" + gId + ")");
        confirmPopup->show("Thành công", "Đã cập nhật thông tin thể loại!", ButtonStyle::Primary, "Đóng", "Quay lại");
    });

    btnGenreClear = std::unique_ptr<RoundRectButton>(new RoundRectButton(
        "   Làm mới biểu mẫu",
        sf::Vector2f(gInputX, 728.f),
        sf::Vector2f(gInputW, 40.f),
        ButtonStyle::Ghost,
        10.f
    ));
    btnGenreClear->setFontSize(13);
    btnGenreClear->setCustomColors(sf::Color::White, sf::Color(241, 245, 249), sf::Color(226, 232, 240), sf::Color(241, 245, 249));
    btnGenreClear->setOutline(1.f, sf::Color(226, 232, 240));
    btnGenreClear->setTextColor(sf::Color(51, 65, 85));
    btnGenreClear->setOnClick([this]() {
        selectedGenreRowIdx = -1;
        size_t nextId = 1;
        std::string nextCode;
        bool exists = false;
        do {
            std::ostringstream ss;
            ss << "TL" << std::setw(2) << std::setfill('0') << nextId++;
            nextCode = ss.str();
            exists = false;
            for (const auto& g : genreList) {
                if (g.first == nextCode) { exists = true; break; }
            }
        } while (exists);
        inputGenreId->setText(nextCode);
        inputGenreName->setText("");
        if (inputGenreName) inputGenreName->setFocused(true);
        confirmPopup->show("Chế độ thêm mới", "Đã dọn sạch biểu mẫu để thêm thể loại mới (" + nextCode + "). Vui lòng nhập tên thể loại vào ô 'TÊN THỂ LOẠI' rồi nhấn '+ Thêm thể loại' (hoặc 'Lưu thay đổi')!", ButtonStyle::Primary, "Nhập tên", "Đóng");
    });
}

static std::string formatVND(double amount) {
    long long val = static_cast<long long>(amount);
    if (val < 0) val = 0;
    std::string s = std::to_string(val);
    int n = static_cast<int>(s.length()) - 3;
    while (n > 0) {
        s.insert(n, ".");
        n -= 3;
    }
    return s + " VNĐ";
}


void CinemaUIApp::renderMoviesView() {
    float contentAreaW = 1728.f - sidebarWidth;

    // 1. Soft slate-white canvas background (#f8fafc) matching reference
    sf::RectangleShape mBg(sf::Vector2f(contentAreaW, 972.f));
    mBg.setPosition(sidebarWidth, 0.f);
    mBg.setFillColor(sf::Color(248, 250, 252));
    window.draw(mBg);

    // 2. Top Utility Strip (White bar with hub breadcrumb & server online badge)
    sf::RectangleShape topStrip(sf::Vector2f(contentAreaW, 44.f));
    topStrip.setPosition(sidebarWidth, 0.f);
    topStrip.setFillColor(sf::Color::White);
    window.draw(topStrip);

    sf::RectangleShape topStripLine(sf::Vector2f(contentAreaW, 1.f));
    topStripLine.setPosition(sidebarWidth, 44.f);
    topStripLine.setFillColor(sf::Color(226, 232, 240));
    window.draw(topStripLine);

    sf::Text hubTxt;
    hubTxt.setFont(Theme::FontPrimary);
    hubTxt.setCharacterSize(13);
    hubTxt.setStyle(sf::Text::Bold);
    hubTxt.setFillColor(sf::Color(30, 41, 59));
    hubTxt.setString(toSfString("Cinestar Quốc Thanh (Hub 01)  /  Cụm Rạp Trung Tâm"));
    hubTxt.setPosition(sidebarWidth + 24.f, 13.f);
    window.draw(hubTxt);

    // Server Online pill on top right
    RoundedRectRenderer::render(window, sf::Vector2f(1728.f - 185.f, 9.f), sf::Vector2f(125.f, 26.f), 13.f, sf::Color(240, 253, 244), 1.f, sf::Color(187, 247, 208));
    sf::CircleShape onlineDot(4.f);
    onlineDot.setFillColor(sf::Color(22, 163, 74));
    onlineDot.setPosition(1728.f - 173.f, 18.f);
    window.draw(onlineDot);

    sf::Text onlineTxt;
    onlineTxt.setFont(Theme::FontPrimary);
    onlineTxt.setCharacterSize(12);
    onlineTxt.setStyle(sf::Text::Bold);
    onlineTxt.setFillColor(sf::Color(21, 128, 61));
    onlineTxt.setString(toSfString("Máy chủ: Online"));
    onlineTxt.setPosition(1728.f - 160.f, 14.f);
    window.draw(onlineTxt);

    // User avatar circle on top right
    sf::CircleShape userAv(14.f);
    userAv.setFillColor(sf::Color(3, 105, 161));
    userAv.setPosition(1728.f - 48.f, 8.f);
    window.draw(userAv);

    // 3. Main Page Heading (Breadcrumb removed as requested)
    float leftColX = sidebarWidth + 24.f;
    float leftColW = 928.f;
    float rightColX = sidebarWidth + 968.f;
    float rightColW = 486.f;

    sf::Text hTitle;
    hTitle.setFont(Theme::FontPrimary);
    hTitle.setCharacterSize(24);
    hTitle.setStyle(sf::Text::Bold);
    hTitle.setFillColor(sf::Color(15, 23, 42));
    hTitle.setString(toSfString("QUẢN LÝ PHIM & THỂ LOẠI"));
    hTitle.setPosition(leftColX, 62.f);
    window.draw(hTitle);

    sf::Text hSub;
    hSub.setFont(Theme::FontPrimary);
    hSub.setCharacterSize(13);
    hSub.setFillColor(sf::Color(100, 116, 139));
    hSub.setString(toSfString("Quản lý danh sách phim, thông số kiểm duyệt, thông tin chi tiết và danh mục phân loại chiếu rạp"));
    hSub.setPosition(leftColX, 96.f);
    window.draw(hSub);

    // 4. Four Status Summary Pills
    int todayKey = 0;
    DateFormat::todayDateKey(todayKey);
    int nowShowingCount = 0;
    int comingSoonCount = 0;
    for (const auto& m : movieList) {
        int rKey = DateFormat::dateKey(DateFormat::normalizeDate(m.getNgayKhoiChieu()));
        if (todayKey > 0 && rKey > todayKey) comingSoonCount++;
        else nowShowingCount++;
    }

    auto drawStatPill = [&](float px, float py, float pw, const std::string& label, sf::Color bgCol, sf::Color borderCol, sf::Color txtCol, sf::Color dotCol, bool hasDot) {
        RoundedRectRenderer::render(window, sf::Vector2f(px, py), sf::Vector2f(pw, 30.f), 8.f, bgCol, 1.f, borderCol);
        float textOffsetX = px + 12.f;
        if (hasDot) {
            sf::CircleShape d(4.f);
            d.setFillColor(dotCol);
            d.setPosition(px + 12.f, py + 11.f);
            window.draw(d);
            textOffsetX = px + 24.f;
        }
        sf::Text pt;
        pt.setFont(Theme::FontPrimary);
        pt.setCharacterSize(13);
        pt.setStyle(sf::Text::Bold);
        pt.setFillColor(txtCol);
        pt.setString(toSfString(label));
        pt.setPosition(textOffsetX, py + 6.f);
        window.draw(pt);
    };

    float pillY = 134.f;
    drawStatPill(leftColX, pillY, 135.f, "Tổng số: " + std::to_string(movieList.size()) + " phim", sf::Color(241, 245, 249), sf::Color(226, 232, 240), sf::Color(15, 23, 42), sf::Color(3, 105, 161), true);
    drawStatPill(leftColX + 145.f, pillY, 130.f, "Đang chiếu: " + std::to_string(nowShowingCount), sf::Color(241, 245, 249), sf::Color(226, 232, 240), sf::Color(15, 23, 42), sf::Color(3, 105, 161), true);
    drawStatPill(leftColX + 285.f, pillY, 125.f, "Sắp chiếu: " + std::to_string(comingSoonCount), sf::Color(241, 245, 249), sf::Color(226, 232, 240), sf::Color(15, 23, 42), sf::Color(71, 85, 105), true);
    drawStatPill(leftColX + 420.f, pillY, 145.f, "CMS: Trực tuyến", sf::Color(220, 252, 231), sf::Color(187, 247, 208), sf::Color(21, 128, 61), sf::Color(22, 163, 74), true);

    // 5. Sub-tabs Bar (y = 180.f, height = 40.f, matching reference image)
    float tabW = 205.f;
    float tabH = 40.f;
    float tabY = 180.f;
    float tab1X = leftColX;
    float tab2X = leftColX + 215.f;

    RoundedRectRenderer::render(
        window,
        sf::Vector2f(tab1X, tabY),
        sf::Vector2f(tabW, tabH),
        10.f,
        (movieSubTab == 0) ? sf::Color::White : sf::Color(248, 250, 252),
        1.5f,
        (movieSubTab == 0) ? sf::Color(2, 132, 199) : sf::Color(226, 232, 240)
    );
    RoundedRectRenderer::render(
        window,
        sf::Vector2f(tab2X, tabY),
        sf::Vector2f(tabW + 15.f, tabH),
        10.f,
        (movieSubTab == 1) ? sf::Color::White : sf::Color(248, 250, 252),
        1.5f,
        (movieSubTab == 1) ? sf::Color(2, 132, 199) : sf::Color(226, 232, 240)
    );

    if (btnSubTabMovies) btnSubTabMovies->draw(window);
    if (btnSubTabGenres) {
        btnSubTabGenres->setSize(sf::Vector2f(tabW + 15.f, tabH));
        btnSubTabGenres->draw(window);
    }

    // Tab 1: "Quản lý Phim" + [07] badge centered together
    sf::Text tab1Txt;
    tab1Txt.setFont(Theme::FontPrimary);
    tab1Txt.setCharacterSize(15);
    tab1Txt.setStyle(sf::Text::Bold);
    tab1Txt.setFillColor((movieSubTab == 0) ? sf::Color(2, 132, 199) : sf::Color(100, 116, 139));
    tab1Txt.setString(toSfString("Quản lý Phim"));
    sf::FloatRect t1Bounds = tab1Txt.getLocalBounds();

    float badgeW = 30.f;
    float badgeH = 22.f;
    float groupGap = 8.f;
    float totalGroupW = t1Bounds.width + groupGap + badgeW;
    float groupStartX = tab1X + (tabW - totalGroupW) / 2.f;

    tab1Txt.setPosition(
        std::round(groupStartX - t1Bounds.left),
        std::round(tabY + (tabH - t1Bounds.height) / 2.f - t1Bounds.top)
    );
    window.draw(tab1Txt);

    float badgeX = std::round(groupStartX + t1Bounds.width + groupGap);
    float badgeY = std::round(tabY + (tabH - badgeH) / 2.f);
    RoundedRectRenderer::render(
        window,
        sf::Vector2f(badgeX, badgeY),
        sf::Vector2f(badgeW, badgeH),
        11.f,
        (movieSubTab == 0) ? sf::Color(2, 132, 199) : sf::Color(226, 232, 240)
    );

    sf::Text cntBadge;
    cntBadge.setFont(Theme::FontPrimary);
    cntBadge.setCharacterSize(12);
    cntBadge.setStyle(sf::Text::Bold);
    cntBadge.setFillColor((movieSubTab == 0) ? sf::Color::White : sf::Color(71, 85, 105));
    std::ostringstream cntOss;
    cntOss << std::setw(2) << std::setfill('0') << movieList.size();
    cntBadge.setString(cntOss.str());
    sf::FloatRect cbBounds = cntBadge.getLocalBounds();
    cntBadge.setPosition(
        std::round(badgeX + (badgeW - cbBounds.width) / 2.f - cbBounds.left),
        std::round(badgeY + (badgeH - cbBounds.height) / 2.f - cbBounds.top)
    );
    window.draw(cntBadge);

    // Tab 2: "Quản lý Thể loại" + [09] badge centered together
    sf::Text tab2Txt;
    tab2Txt.setFont(Theme::FontPrimary);
    tab2Txt.setCharacterSize(15);
    tab2Txt.setStyle(sf::Text::Bold);
    tab2Txt.setFillColor((movieSubTab == 1) ? sf::Color(2, 132, 199) : sf::Color(100, 116, 139));
    tab2Txt.setString(toSfString("Quản lý Thể loại"));
    sf::FloatRect t2Bounds = tab2Txt.getLocalBounds();

    float tab2ActualW = tabW + 15.f;
    float totalGroup2W = t2Bounds.width + groupGap + badgeW;
    float group2StartX = tab2X + (tab2ActualW - totalGroup2W) / 2.f;

    tab2Txt.setPosition(
        std::round(group2StartX - t2Bounds.left),
        std::round(tabY + (tabH - t2Bounds.height) / 2.f - t2Bounds.top)
    );
    window.draw(tab2Txt);

    float badge2X = std::round(group2StartX + t2Bounds.width + groupGap);
    float badge2Y = std::round(tabY + (tabH - badgeH) / 2.f);
    RoundedRectRenderer::render(
        window,
        sf::Vector2f(badge2X, badge2Y),
        sf::Vector2f(badgeW, badgeH),
        11.f,
        (movieSubTab == 1) ? sf::Color(2, 132, 199) : sf::Color(226, 232, 240)
    );

    sf::Text cnt2Badge;
    cnt2Badge.setFont(Theme::FontPrimary);
    cnt2Badge.setCharacterSize(12);
    cnt2Badge.setStyle(sf::Text::Bold);
    cnt2Badge.setFillColor((movieSubTab == 1) ? sf::Color::White : sf::Color(71, 85, 105));
    std::ostringstream cnt2Oss;
    cnt2Oss << std::setw(2) << std::setfill('0') << genreList.size();
    cnt2Badge.setString(cnt2Oss.str());
    sf::FloatRect cb2Bounds = cnt2Badge.getLocalBounds();
    cnt2Badge.setPosition(
        std::round(badge2X + (badgeW - cb2Bounds.width) / 2.f - cb2Bounds.left),
        std::round(badge2Y + (badgeH - cb2Bounds.height) / 2.f - cb2Bounds.top)
    );
    window.draw(cnt2Badge);

    // Top-right Search & Export controls on Sub-tab bar when movieSubTab == 1
    if (movieSubTab == 1) {
        if (genreSearchInput) genreSearchInput->draw(window);
        // Magnifying glass icon inside genreSearchInput
        sf::CircleShape gLens(5.5f);
        gLens.setPosition(rightColX - 24.f, 193.f);
        gLens.setFillColor(sf::Color::Transparent);
        gLens.setOutlineThickness(1.6f);
        gLens.setOutlineColor(sf::Color(148, 163, 184));
        window.draw(gLens);
        sf::RectangleShape gHandle(sf::Vector2f(5.f, 1.6f));
        gHandle.setPosition(rightColX - 15.f, 203.f);
        gHandle.setRotation(45.f);
        gHandle.setFillColor(sf::Color(148, 163, 184));
        window.draw(gHandle);

        if (btnExportGenres) btnExportGenres->draw(window);
        // Green spreadsheet/file icon inside btnExportGenres
        RoundedRectRenderer::render(window, sf::Vector2f(rightColX + 298.f, 192.f), sf::Vector2f(13.f, 16.f), 2.f, sf::Color(220, 252, 231), 1.2f, sf::Color(22, 163, 74));
        sf::RectangleShape expLine1(sf::Vector2f(7.f, 1.5f));
        expLine1.setPosition(rightColX + 301.f, 197.f);
        expLine1.setFillColor(sf::Color(22, 163, 74));
        window.draw(expLine1);
        sf::RectangleShape expLine2(sf::Vector2f(7.f, 1.5f));
        expLine2.setPosition(rightColX + 301.f, 201.f);
        expLine2.setFillColor(sf::Color(22, 163, 74));
        window.draw(expLine2);
    }

    // Helper lambda to draw a Poster Thumbnail (either loaded sf::Texture or synchronized photo-frame placeholder)
    auto drawPosterBox = [&](const std::string& movieCode, const std::string& customPath, float bx, float by, float bw, float bh, float radius = 8.f) {
        RoundedRectRenderer::render(
            window,
            sf::Vector2f(bx, by),
            sf::Vector2f(bw, bh),
            radius,
            sf::Color(240, 249, 255),
            1.f,
            sf::Color(186, 230, 253)
        );

        std::shared_ptr<sf::Texture> tex = nullptr;
        auto it = moviePosterTextures.find(movieCode);
        if (it != moviePosterTextures.end() && it->second && it->second->getSize().x > 0) {
            tex = it->second;
        } else if (!customPath.empty()) {
            std::ifstream f(customPath.c_str(), std::ios::binary);
            if (f.good()) {
                f.close();
                std::shared_ptr<sf::Texture> loaded(new sf::Texture());
                if (loaded->loadFromFile(customPath)) {
                    loaded->setSmooth(true);
                    moviePosterTextures[movieCode] = loaded;
                    tex = loaded;
                }
            }
        }

        if (tex && tex->getSize().x > 0 && tex->getSize().y > 0) {
            sf::Sprite sp(*tex);
            float pad = 2.f;
            float sx = (bw - pad * 2.f) / static_cast<float>(tex->getSize().x);
            float sy = (bh - pad * 2.f) / static_cast<float>(tex->getSize().y);
            sp.setScale(sx, sy);
            sp.setPosition(bx + pad, by + pad);
            window.draw(sp);
        } else {
            // Clean Square/Rect Photo Placeholder Icon (synchronized with F&B image frame)
            float scale = std::min(bw / 48.f, bh / 64.f);
            if (scale < 0.65f) scale = 0.65f;
            float icW = std::round(24.f * scale);
            float icH = std::round(18.f * scale);
            float icX = std::round(bx + (bw - icW) / 2.f);
            float icY = std::round(by + (bh - icH) / 2.f);

            RoundedRectRenderer::render(
                window,
                sf::Vector2f(icX, icY),
                sf::Vector2f(icW, icH),
                3.f * scale,
                sf::Color::White,
                1.3f * scale,
                sf::Color(2, 132, 199)
            );
            // Sun dot inside photo frame
            sf::CircleShape sunDot(2.2f * scale);
            sunDot.setFillColor(sf::Color(56, 189, 248));
            sunDot.setPosition(icX + 4.f * scale, icY + 3.5f * scale);
            window.draw(sunDot);

            // Mountain landscape peak inside photo frame
            sf::ConvexShape mtn(4);
            mtn.setPoint(0, sf::Vector2f(icX + 2.f * scale, icY + icH - 2.f * scale));
            mtn.setPoint(1, sf::Vector2f(icX + 10.f * scale, icY + 7.5f * scale));
            mtn.setPoint(2, sf::Vector2f(icX + 16.f * scale, icY + 11.f * scale));
            mtn.setPoint(3, sf::Vector2f(icX + icW - 2.f * scale, icY + icH - 2.f * scale));
            mtn.setFillColor(sf::Color(125, 211, 252));
            window.draw(mtn);
        }
    };

    // 6. LEFT CARD: White Table Card (cardY = 232.f, cardH = 716.f)
    float cardY = 232.f;
    float cardH = 716.f;
    RoundedRectRenderer::render(
        window,
        sf::Vector2f(leftColX, cardY),
        sf::Vector2f(leftColW, cardH),
        12.f,
        sf::Color::White,
        1.f,
        sf::Color(226, 232, 240)
    );

    if (movieSubTab == 0) {
        // Filter Bar Row
        sf::Text lblSearch;
        lblSearch.setFont(Theme::FontPrimary);
        lblSearch.setCharacterSize(14);
        lblSearch.setStyle(sf::Text::Bold);
        lblSearch.setFillColor(sf::Color(30, 41, 59));
        lblSearch.setString(toSfString("Tìm nhanh:"));
        lblSearch.setPosition(leftColX + 20.f, cardY + 24.f);
        window.draw(lblSearch);

        if (movieSearchInput) movieSearchInput->draw(window);

        // Magnifying glass icon inside search box
        sf::CircleShape magGlass(5.5f);
        magGlass.setFillColor(sf::Color::Transparent);
        magGlass.setOutlineThickness(1.6f);
        magGlass.setOutlineColor(sf::Color(100, 116, 139));
        magGlass.setPosition(leftColX + 121.f, 259.f);
        window.draw(magGlass);
        sf::RectangleShape magHandle(sf::Vector2f(5.f, 1.6f));
        magHandle.setPosition(leftColX + 130.f, 269.f);
        magHandle.setRotation(45.f);
        magHandle.setFillColor(sf::Color(100, 116, 139));
        window.draw(magHandle);

        // Age Rating Filter Dropdown (Scrollable Dropdown)
        if (movieAgeFilterDropdown) movieAgeFilterDropdown->draw(window);

        if (btnQuickAddMovie) btnQuickAddMovie->draw(window);

        // Table Header (tblHeaderY = 296.f, height = 42.f)
        float tblHeaderY = cardY + 64.f;
        sf::RectangleShape thDivTop(sf::Vector2f(leftColW - 32.f, 1.f));
        thDivTop.setPosition(leftColX + 16.f, tblHeaderY);
        thDivTop.setFillColor(sf::Color(241, 245, 249));
        window.draw(thDivTop);

        sf::RectangleShape thDivBot(sf::Vector2f(leftColW - 32.f, 1.f));
        thDivBot.setPosition(leftColX + 16.f, tblHeaderY + 40.f);
        thDivBot.setFillColor(sf::Color(226, 232, 240));
        window.draw(thDivBot);

        std::vector<std::string> cols = {"MÃ PHIM", "TÊN PHIM", "THỂ LOẠI", "THỜI LƯỢNG", "NGÀY CHIẾU", "NHÃN TUỔI", "TRẠNG THÁI", "THAO TÁC"};
        std::vector<float> colX = {
            leftColX + 24.f,
            leftColX + 88.f,
            leftColX + 372.f,
            leftColX + 484.f,
            leftColX + 572.f,
            leftColX + 668.f,
            leftColX + 746.f,
            leftColX + 846.f
        };

        for (size_t c = 0; c < cols.size(); ++c) {
            sf::Text ch;
            ch.setFont(Theme::FontPrimary);
            ch.setCharacterSize(12);
            ch.setStyle(sf::Text::Bold);
            ch.setFillColor(sf::Color(71, 85, 105));
            ch.setString(toSfString(cols[c]));
            ch.setPosition(colX[c], tblHeaderY + 12.f);
            window.draw(ch);
        }

        // Table Rows (rowStartY = 338.f, rowH = 104.f, 3 rows per page + Spotlight Showcase)
        float rowStartY = tblHeaderY + 42.f;
        float rowH = 104.f;
        std::string query = movieSearchInput ? movieSearchInput->getText() : "";
        std::transform(query.begin(), query.end(), query.begin(), ::tolower);

        std::vector<int> filteredMovieIndices;
        for (size_t i = 0; i < movieList.size(); ++i) {
            std::string pName = movieList[i].getTenPhim();
            std::string pCode = movieList[i].getMaPhim();
            std::string pGenre = db.layTheLoaiCuaPhim(pCode);
            int pAge = movieList[i].getNhanDoTuoi();

            if (!movieAgeFilter.empty()) {
                if (movieAgeFilter == "P") {
                    if (pAge > 0) continue;
                } else if (movieAgeFilter == "K") {
                    if (pAge > 12) continue;
                } else {
                    int targetAge = 0;
                    for (char c : movieAgeFilter) if (c >= '0' && c <= '9') targetAge = targetAge * 10 + (c - '0');
                    if (targetAge > 0 && pAge != targetAge) continue;
                }
            }

            if (!query.empty()) {
                std::string pNameLower = pName;
                std::string pCodeLower = pCode;
                std::string pGenreLower = pGenre;
                std::transform(pNameLower.begin(), pNameLower.end(), pNameLower.begin(), ::tolower);
                std::transform(pCodeLower.begin(), pCodeLower.end(), pCodeLower.begin(), ::tolower);
                std::transform(pGenreLower.begin(), pGenreLower.end(), pGenreLower.begin(), ::tolower);
                if (pNameLower.find(query) == std::string::npos &&
                    pCodeLower.find(query) == std::string::npos &&
                    pGenreLower.find(query) == std::string::npos) {
                    continue;
                }
            }
            filteredMovieIndices.push_back(static_cast<int>(i));
        }

        int totalFiltered = static_cast<int>(filteredMovieIndices.size());
        int pageSize = showMovieSpotlight ? 2 : 5;
        int totalMoviePages = std::max(1, (totalFiltered + pageSize - 1) / pageSize);
        if (movieCurrentPage >= totalMoviePages) movieCurrentPage = totalMoviePages - 1;
        if (movieCurrentPage < 0) movieCurrentPage = 0;

        int startIndex = movieCurrentPage * pageSize;
        int endIndex = std::min(startIndex + pageSize, totalFiltered);

        if (totalFiltered == 0) {
            // Không tìm thấy phim phù hợp (ví dụ gõ jjj): Xóa lựa chọn và Spotlight
            selectedMovieRowIdx = -1;
            showMovieSpotlight = false;
            spotlightMovieIdx = -1;

            sf::Text emptyHdr;
            emptyHdr.setFont(Theme::FontPrimary);
            emptyHdr.setCharacterSize(18);
            emptyHdr.setStyle(sf::Text::Bold);
            emptyHdr.setFillColor(sf::Color(100, 116, 139));
            emptyHdr.setString(toSfString("Không tìm thấy bộ phim nào phù hợp"));
            sf::FloatRect eb = emptyHdr.getLocalBounds();
            emptyHdr.setPosition(leftColX + (leftColW - eb.width) / 2.f, rowStartY + 110.f);
            window.draw(emptyHdr);

            sf::Text emptySub;
            emptySub.setFont(Theme::FontPrimary);
            emptySub.setCharacterSize(14);
            emptySub.setFillColor(sf::Color(148, 163, 184));
            emptySub.setString(toSfString("Vui lòng thử tìm kiếm với từ khóa khác hoặc xóa bộ lọc"));
            sf::FloatRect esb = emptySub.getLocalBounds();
            emptySub.setPosition(leftColX + (leftColW - esb.width) / 2.f, rowStartY + 144.f);
            window.draw(emptySub);
        } else if (showMovieSpotlight) {
            bool spotInPage = false;
            for (int k = startIndex; k < endIndex; ++k) {
                if (filteredMovieIndices[k] == spotlightMovieIdx) {
                    spotInPage = true;
                    break;
                }
            }
            if (!spotInPage && startIndex < totalFiltered) {
                spotlightMovieIdx = filteredMovieIndices[startIndex];
                selectedMovieRowIdx = spotlightMovieIdx;
            }
        }

        size_t displayCount = 0;
        for (int k = startIndex; k < endIndex; ++k) {
            size_t i = static_cast<size_t>(filteredMovieIndices[k]);
            std::string pName = movieList[i].getTenPhim();
            std::string pCode = movieList[i].getMaPhim();
            std::string pGenre = db.layTheLoaiCuaPhim(pCode);
            if (pGenre.empty()) pGenre = "Tâm Lý, Hành Động";
            int pAge = movieList[i].getNhanDoTuoi();

            float ry = rowStartY + static_cast<float>(displayCount) * rowH;
            bool isSelected = (selectedMovieRowIdx == static_cast<int>(i));

            if (isSelected) {
                RoundedRectRenderer::render(
                    window,
                    sf::Vector2f(leftColX + 12.f, ry + 2.f),
                    sf::Vector2f(leftColW - 24.f, rowH - 4.f),
                    8.f,
                    showMovieSpotlight ? sf::Color(224, 242, 254) : sf::Color(241, 245, 249),
                    showMovieSpotlight ? 1.5f : 0.f,
                    showMovieSpotlight ? sf::Color(14, 165, 233) : sf::Color::Transparent
                );
            }

            // 1. Mã Phim
            sf::Text cCode;
            cCode.setFont(Theme::FontPrimary);
            cCode.setCharacterSize(13);
            cCode.setStyle(sf::Text::Bold);
            cCode.setFillColor(isSelected ? sf::Color(3, 105, 161) : sf::Color(71, 85, 105));
            cCode.setString(toSfString(pCode));
            cCode.setPosition(colX[0], ry + 42.f);
            window.draw(cCode);

            // 2. Poster Slot (66x90 - Gấp đôi kích thước cũ 36x50) + Tên Phim + Mô tả ngắn
            float postW = 66.f;
            float postH = 90.f;
            drawPosterBox(pCode, movieList[i].getPoster(), colX[1], ry + 7.f, postW, postH, 8.f);

            float textLeftX = colX[1] + postW + 14.f;
            float maxMovieTitleW = colX[2] - textLeftX - 10.f;
            std::size_t titleLineCount = 1;
            sf::String wrappedTitle = wrapSfTextString(
                Theme::FontPrimary,
                toSfString(pName),
                14,
                maxMovieTitleW,
                2,
                sf::Text::Bold,
                &titleLineCount
            );

            sf::Text cName;
            cName.setFont(Theme::FontPrimary);
            cName.setCharacterSize(titleLineCount > 1 ? 13 : 15);
            cName.setStyle(sf::Text::Bold);
            cName.setLineSpacing(0.88f);
            cName.setFillColor(isSelected ? sf::Color(3, 105, 161) : sf::Color(15, 23, 42));
            cName.setString(wrappedTitle);
            cName.setPosition(textLeftX, ry + (titleLineCount > 1 ? 18.f : 28.f));
            window.draw(cName);

            sf::Text cStudio;
            cStudio.setFont(Theme::FontPrimary);
            cStudio.setCharacterSize(11);
            cStudio.setFillColor(sf::Color(100, 116, 139));
            std::string subDesc = movieList[i].getMoTa();
            if (subDesc.empty()) subDesc = "CineStar Distribution";
            cStudio.setString(wrapSfTextString(Theme::FontPrimary, toSfString(subDesc), 11, maxMovieTitleW, 1));
            cStudio.setPosition(textLeftX, ry + (titleLineCount > 1 ? 58.f : 56.f));
            window.draw(cStudio);

            if (showMovieSpotlight && static_cast<int>(i) == spotlightMovieIdx) {
                sf::Text cHint;
                cHint.setFont(Theme::FontPrimary);
                cHint.setCharacterSize(11);
                cHint.setStyle(sf::Text::Bold);
                cHint.setFillColor(sf::Color(2, 132, 199));
                cHint.setString(toSfString("▲ Đang xem Spotlight (Bấm để đóng)"));
                cHint.setPosition(textLeftX, ry + (titleLineCount > 1 ? 74.f : 72.f));
                window.draw(cHint);
            }

            // 3. Thể Loại
            float maxGenreColW = colX[3] - colX[2] - 8.f;
            std::size_t genreLineCount = 1;
            sf::String wrappedGenre = wrapSfTextString(
                Theme::FontPrimary,
                toSfString(pGenre),
                12,
                maxGenreColW,
                2,
                sf::Text::Regular,
                &genreLineCount
            );
            sf::Text cGenre;
            cGenre.setFont(Theme::FontPrimary);
            cGenre.setCharacterSize(12);
            cGenre.setLineSpacing(0.90f);
            cGenre.setFillColor(sf::Color(71, 85, 105));
            cGenre.setString(wrappedGenre);
            cGenre.setPosition(colX[2], ry + (genreLineCount > 1 ? 34.f : 42.f));
            window.draw(cGenre);

            // 4. Thời lượng
            sf::Text cDur;
            cDur.setFont(Theme::FontPrimary);
            cDur.setCharacterSize(13);
            cDur.setStyle(sf::Text::Bold);
            cDur.setFillColor(isSelected ? sf::Color(3, 105, 161) : sf::Color(30, 41, 59));
            cDur.setString(toSfString(std::to_string(movieList[i].getThoiLuong()) + " phút"));
            cDur.setPosition(colX[3], ry + 42.f);
            window.draw(cDur);

            // 5. Ngày chiếu
            sf::Text cDate;
            cDate.setFont(Theme::FontPrimary);
            cDate.setCharacterSize(13);
            cDate.setStyle(isSelected ? sf::Text::Bold : sf::Text::Regular);
            cDate.setFillColor(isSelected ? sf::Color(3, 105, 161) : sf::Color(30, 41, 59));
            cDate.setString(toSfString(movieList[i].getNgayKhoiChieu()));
            cDate.setPosition(colX[4], ry + 42.f);
            window.draw(cDate);

            // 6. Nhãn tuổi pill
            std::string ageBadgeStr = (pAge <= 0) ? "P" : ("T" + std::to_string(pAge));
            sf::Color ageBg = (pAge >= 16) ? sf::Color(254, 226, 226) : ((pAge >= 13) ? sf::Color(224, 231, 255) : sf::Color(220, 252, 231));
            sf::Color ageFg = (pAge >= 16) ? sf::Color(220, 38, 38) : ((pAge >= 13) ? sf::Color(67, 56, 202) : sf::Color(21, 128, 61));
            RoundedRectRenderer::render(window, sf::Vector2f(colX[5], ry + 38.f), sf::Vector2f(42.f, 26.f), 6.f, ageBg);

            sf::Text cAge;
            cAge.setFont(Theme::FontPrimary);
            cAge.setCharacterSize(12);
            cAge.setStyle(sf::Text::Bold);
            cAge.setFillColor(ageFg);
            cAge.setString(toSfString(ageBadgeStr));
            sf::FloatRect ab = cAge.getLocalBounds();
            cAge.setPosition(colX[5] + (42.f - ab.width) / 2.f, ry + 43.f);
            window.draw(cAge);

            // 7. Trạng thái pill (Đang chiếu / Sắp chiếu)
            int relKey = DateFormat::dateKey(DateFormat::normalizeDate(movieList[i].getNgayKhoiChieu()));
            bool isComingSoon = (todayKey > 0 && relKey > todayKey);
            sf::Color stBg = isComingSoon ? sf::Color(224, 231, 255) : sf::Color(187, 247, 208);
            sf::Color stFg = isComingSoon ? sf::Color(55, 48, 163) : sf::Color(21, 128, 61);
            RoundedRectRenderer::render(window, sf::Vector2f(colX[6] - 8.f, ry + 36.f), sf::Vector2f(82.f, 30.f), 15.f, stBg);

            sf::Text cSt;
            cSt.setFont(Theme::FontPrimary);
            cSt.setCharacterSize(11);
            cSt.setStyle(sf::Text::Bold);
            cSt.setFillColor(stFg);
            cSt.setString(toSfString(isComingSoon ? "Sắp chiếu" : "Đang chiếu"));
            sf::FloatRect sb = cSt.getLocalBounds();
            cSt.setPosition(colX[6] - 8.f + (82.f - sb.width) / 2.f, ry + 43.f);
            window.draw(cSt);

            // 8. Thao tác icons (16x supersampled Edit Pencil & Delete Trash Can)
            static sf::Texture pencilTex, trashTex;
            static bool actionIconsReady = false;
            if (!actionIconsReady) {
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

                                // Pencil: rotated 45 deg (top-right eraser to bottom-left tip)
                                float u = (fx - fy) * 0.70710678f;
                                float v = (fx + fy) * 0.70710678f;
                                bool inPencil = false;
                                // Shaft outline & eraser collar
                                if (u >= -3.2f && u <= 5.2f && std::fabs(v) <= 2.1f) {
                                    bool border = (std::fabs(v) >= 1.0f) || (u >= 4.1f) || (std::fabs(u - 2.4f) <= 0.55f) || (std::fabs(u + 3.0f) <= 0.5f);
                                    if (border) inPencil = true;
                                }
                                // Pointed nib tapering to (-6.2, 0)
                                if (u >= -6.2f && u < -3.2f) {
                                    float maxV = ((u + 6.2f) / 3.0f) * 2.1f;
                                    if (std::fabs(v) <= maxV && (std::fabs(v) >= maxV - 1.1f || u <= -4.8f)) {
                                        inPencil = true;
                                    }
                                }
                                if (inPencil) pHits++;

                                // Trash can: lid, top handle, tapered bin outline, and 2 vertical inner bars
                                bool inTrash = false;
                                // Lid bar
                                if (distSeg(fx, fy, -5.8f, -4.6f, 5.8f, -4.6f) <= 0.75f) inTrash = true;
                                // Top handle arch
                                if (fy >= -7.0f && fy <= -4.6f && std::fabs(fx) <= 2.4f) {
                                    if (fy <= -5.8f || std::fabs(fx) >= 1.25f) inTrash = true;
                                }
                                // Bin outline (left, right, bottom)
                                if (distSeg(fx, fy, -4.5f, -4.2f, -3.8f, 6.2f) <= 0.7f) inTrash = true;
                                if (distSeg(fx, fy,  4.5f, -4.2f,  3.8f, 6.2f) <= 0.7f) inTrash = true;
                                if (distSeg(fx, fy, -3.8f,  6.2f,  3.8f, 6.2f) <= 0.7f) inTrash = true;
                                // Inner vertical slats
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
                pencilTex.loadFromImage(pImg);
                pencilTex.setSmooth(true);
                trashTex.loadFromImage(tImg);
                trashTex.setSmooth(true);
                actionIconsReady = true;
            }

            RoundedRectRenderer::render(window, sf::Vector2f(colX[7], ry + 38.f), sf::Vector2f(26.f, 26.f), 6.f, sf::Color(241, 245, 249));
            sf::Sprite editSpr(pencilTex);
            editSpr.setPosition(colX[7], ry + 38.f);
            window.draw(editSpr);

            RoundedRectRenderer::render(window, sf::Vector2f(colX[7] + 32.f, ry + 38.f), sf::Vector2f(26.f, 26.f), 6.f, sf::Color(254, 242, 242));
            sf::Sprite delSpr(trashTex);
            delSpr.setPosition(colX[7] + 32.f, ry + 38.f);
            window.draw(delSpr);

            // Row divider
            sf::RectangleShape rBorder(sf::Vector2f(leftColW - 32.f, 1.f));
            rBorder.setPosition(leftColX + 16.f, ry + rowH - 1.f);
            rBorder.setFillColor(sf::Color(241, 245, 249));
            window.draw(rBorder);

            displayCount++;
        }

        // Spotlight Showcase Card: Chỉ hiển thị khi showMovieSpotlight == true và có phim hợp lệ
        float footerY = cardY + cardH - 42.f;
        if (showMovieSpotlight && spotlightMovieIdx >= 0 && spotlightMovieIdx < static_cast<int>(movieList.size()) && displayCount > 0) {
            float spotlightY = rowStartY + static_cast<float>(displayCount) * rowH + 8.f;
            float spotlightH = (footerY - 8.f) - spotlightY;
            if (spotlightH >= 180.f) {
                float spotlightX = leftColX + 16.f;
                float spotlightW = leftColW - 32.f;

                const Movie& selM = movieList[spotlightMovieIdx];
                std::string selCode = selM.getMaPhim();
                std::string selTitle = selM.getTenPhim();
                std::string selGenre = db.layTheLoaiCuaPhim(selCode);
                if (selGenre.empty()) selGenre = "Tâm Lý, Hành Động";
                int selAge = selM.getNhanDoTuoi();
                std::string selAgeStr = (selAge <= 0) ? "P" : ("T" + std::to_string(selAge));
                std::string selPoster = selM.getPoster();
                if (selPoster.empty() || selPoster == "assets/posters/poster_default.jpg") {
                    selPoster = "assets/posters/" + selCode + ".jpg";
                }

                // 1. Nền Spotlight Card
                RoundedRectRenderer::render(
                    window,
                    sf::Vector2f(spotlightX, spotlightY),
                    sf::Vector2f(spotlightW, spotlightH),
                    12.f,
                    sf::Color(248, 250, 252),
                    1.5f,
                    sf::Color(219, 234, 254)
                );

                // 2. Dải tiêu đề trên cùng
                RoundedRectRenderer::render(
                    window,
                    sf::Vector2f(spotlightX + 1.f, spotlightY + 1.f),
                    sf::Vector2f(spotlightW - 2.f, 36.f),
                    11.f,
                    sf::Color(240, 249, 255)
                );

                sf::CircleShape dotIcon(5.f);
                dotIcon.setFillColor(sf::Color(2, 132, 199));
                dotIcon.setPosition(spotlightX + 18.f, spotlightY + 13.f);
                window.draw(dotIcon);

                sf::Text sHdr;
                sHdr.setFont(Theme::FontPrimary);
                sHdr.setCharacterSize(14);
                sHdr.setStyle(sf::Text::Bold);
                sHdr.setFillColor(sf::Color(3, 105, 161));
                sHdr.setString(toSfString("THÔNG TIN CHI TIẾT BỘ PHIM (SPOTLIGHT SHOWCASE)"));
                sHdr.setPosition(spotlightX + 36.f, spotlightY + 8.f);
                window.draw(sHdr);

                // Nút [X Đóng Spotlight] ở góc phải dải tiêu đề
                float closeBtnW = 145.f;
                float closeBtnH = 28.f;
                float closeBtnX = spotlightX + spotlightW - closeBtnW - 10.f;
                float closeBtnY = spotlightY + 4.f;

                RoundedRectRenderer::render(
                    window,
                    sf::Vector2f(closeBtnX, closeBtnY),
                    sf::Vector2f(closeBtnW, closeBtnH),
                    6.f,
                    sf::Color(254, 242, 242),
                    1.f,
                    sf::Color(252, 165, 165)
                );

                sf::Text cClose;
                cClose.setFont(Theme::FontPrimary);
                cClose.setCharacterSize(13);
                cClose.setStyle(sf::Text::Bold);
                cClose.setFillColor(sf::Color(220, 38, 38));
                cClose.setString(toSfString("X Đóng Spotlight"));
                sf::FloatRect clb = cClose.getLocalBounds();
                cClose.setPosition(closeBtnX + (closeBtnW - clb.width) / 2.f, closeBtnY + 5.f);
                window.draw(cClose);

                // Poster lớn chất lượng cao
                float pw = 175.f;
                float ph = std::min(spotlightH - 50.f, 255.f);
                if (ph < 130.f) ph = 130.f;
                float px = spotlightX + 20.f;
                float py = spotlightY + 44.f;

                drawPosterBox(selCode, selPoster, px, py, pw, ph, 8.f);

                // Khối thông tin bên phải Poster (Font chữ bự gấp đôi)
                float infoX = px + pw + 26.f;
                float infoW = (spotlightX + spotlightW - 20.f) - infoX;
                float curY = py;

                // 1. Tên phim kích thước rất lớn (34px Bold)
                std::vector<sf::String> titleLines = UI::UIInputBox::wrapText(toSfString(selTitle), infoW - 24.f, Theme::FontPrimary, 34);
                for (size_t tl = 0; tl < titleLines.size() && tl < 2; ++tl) {
                    sf::Text bTitle;
                    bTitle.setFont(Theme::FontPrimary);
                    bTitle.setCharacterSize(34);
                    bTitle.setStyle(sf::Text::Bold);
                    bTitle.setFillColor(sf::Color(15, 23, 42));
                    bTitle.setString(titleLines[tl]);
                    bTitle.setPosition(infoX, curY);
                    window.draw(bTitle);
                    curY += 40.f;
                }
                curY += 6.f;

                // 2. Hàng Badges thông số kích thước lớn (Font 16px Bold, Pill cao 34px - gấp đôi 11px/24px)
                // 2.1 Mã Phim Pill
                float codePillW = 82.f;
                RoundedRectRenderer::render(window, sf::Vector2f(infoX, curY), sf::Vector2f(codePillW, 34.f), 8.f, sf::Color(224, 242, 254));
                sf::Text bCode;
                bCode.setFont(Theme::FontPrimary);
                bCode.setCharacterSize(16);
                bCode.setStyle(sf::Text::Bold);
                bCode.setFillColor(sf::Color(3, 105, 161));
                bCode.setString(toSfString(selCode));
                sf::FloatRect cb = bCode.getLocalBounds();
                bCode.setPosition(infoX + (codePillW - cb.width) / 2.f, curY + 6.f);
                window.draw(bCode);

                // 2.2 Độ tuổi Pill
                float agePillW = 58.f;
                sf::Color ageBg = (selAge >= 16) ? sf::Color(254, 226, 226) : ((selAge >= 13) ? sf::Color(224, 231, 255) : sf::Color(220, 252, 231));
                sf::Color ageFg = (selAge >= 16) ? sf::Color(220, 38, 38) : ((selAge >= 13) ? sf::Color(67, 56, 202) : sf::Color(21, 128, 61));
                RoundedRectRenderer::render(window, sf::Vector2f(infoX + 92.f, curY), sf::Vector2f(agePillW, 34.f), 8.f, ageBg);
                sf::Text bAge;
                bAge.setFont(Theme::FontPrimary);
                bAge.setCharacterSize(16);
                bAge.setStyle(sf::Text::Bold);
                bAge.setFillColor(ageFg);
                bAge.setString(toSfString(selAgeStr));
                sf::FloatRect ab = bAge.getLocalBounds();
                bAge.setPosition(infoX + 92.f + (agePillW - ab.width) / 2.f, curY + 6.f);
                window.draw(bAge);

                // 2.3 Thể loại Pill
                float genrePillW = 180.f;
                RoundedRectRenderer::render(window, sf::Vector2f(infoX + 160.f, curY), sf::Vector2f(genrePillW, 34.f), 8.f, sf::Color(241, 245, 249));
                sf::Text bGenre;
                bGenre.setFont(Theme::FontPrimary);
                bGenre.setCharacterSize(16);
                bGenre.setStyle(sf::Text::Bold);
                bGenre.setFillColor(sf::Color(71, 85, 105));
                std::string gDisp = selGenre;
                if (gDisp.length() > 20) gDisp = gDisp.substr(0, 18) + "..";
                bGenre.setString(toSfString(gDisp));
                sf::FloatRect gb = bGenre.getLocalBounds();
                bGenre.setPosition(infoX + 160.f + (genrePillW - gb.width) / 2.f, curY + 6.f);
                window.draw(bGenre);

                // 2.4 Trạng thái chiếu Pill
                int relKey = DateFormat::dateKey(DateFormat::normalizeDate(selM.getNgayKhoiChieu()));
                bool isComing = (todayKey > 0 && relKey > todayKey);
                sf::Color stBg = isComing ? sf::Color(224, 231, 255) : sf::Color(187, 247, 208);
                sf::Color stFg = isComing ? sf::Color(55, 48, 163) : sf::Color(21, 128, 61);
                float stPillW = 135.f;
                RoundedRectRenderer::render(window, sf::Vector2f(infoX + 350.f, curY), sf::Vector2f(stPillW, 34.f), 8.f, stBg);
                sf::Text bSt;
                bSt.setFont(Theme::FontPrimary);
                bSt.setCharacterSize(16);
                bSt.setStyle(sf::Text::Bold);
                bSt.setFillColor(stFg);
                bSt.setString(toSfString(isComing ? "Sắp chiếu" : "Đang chiếu"));
                sf::FloatRect sb = bSt.getLocalBounds();
                bSt.setPosition(infoX + 350.f + (stPillW - sb.width) / 2.f, curY + 6.f);
                window.draw(bSt);

                curY += 46.f;

                // 3. Thông số kỹ thuật (Font 18px Bold)
                std::string durStr = std::to_string(selM.getThoiLuong()) + " phút (" + std::to_string(selM.getThoiLuong() / 60) + "h " + std::to_string(selM.getThoiLuong() % 60) + "m)";
                std::string specsLine1 = "Thời lượng: " + durStr + "   |   Khởi chiếu: " + selM.getNgayKhoiChieu();

                sf::Text bSpecs;
                bSpecs.setFont(Theme::FontPrimary);
                bSpecs.setCharacterSize(18);
                bSpecs.setStyle(sf::Text::Bold);
                bSpecs.setFillColor(sf::Color(51, 65, 85));
                bSpecs.setString(toSfString(specsLine1));
                bSpecs.setPosition(infoX, curY);
                window.draw(bSpecs);

                curY += 30.f;

                // 4. Tiêu đề "Tóm tắt nội dung phim:" (Font 18px Bold)
                sf::Text bDescHdr;
                bDescHdr.setFont(Theme::FontPrimary);
                bDescHdr.setCharacterSize(18);
                bDescHdr.setStyle(sf::Text::Bold);
                bDescHdr.setFillColor(sf::Color(100, 116, 139));
                bDescHdr.setString(toSfString("Tóm tắt nội dung phim:"));
                bDescHdr.setPosition(infoX, curY);
                window.draw(bDescHdr);

                curY += 28.f;

                // 5. Nội dung tóm tắt chi tiết (Font 18px Regular)
                std::string fullDesc = selM.getMoTa();
                if (fullDesc.empty()) fullDesc = "Chưa có nội dung tóm tắt cho bộ phim này.";
                std::vector<sf::String> descLines = UI::UIInputBox::wrapText(toSfString(fullDesc), infoW - 24.f, Theme::FontPrimary, 18);
                float lineSpacing = 26.f;
                size_t maxShowLines = static_cast<size_t>(std::max(1, static_cast<int>((spotlightY + spotlightH - curY - 26.f) / lineSpacing)));
                for (size_t l = 0; l < descLines.size() && l < maxShowLines; ++l) {
                    sf::Text dl;
                    dl.setFont(Theme::FontPrimary);
                    dl.setCharacterSize(18);
                    dl.setFillColor(sf::Color(51, 65, 85));
                    dl.setString(descLines[l]);
                    dl.setPosition(infoX, curY + l * lineSpacing);
                    window.draw(dl);
                }

                // 6. Dòng hướng dẫn nhỏ ở đáy Spotlight (Font 13px)
                sf::Text bHint;
                bHint.setFont(Theme::FontPrimary);
                bHint.setCharacterSize(13);
                bHint.setFillColor(sf::Color(148, 163, 184));
                bHint.setString(toSfString(">> Bấm lại vào dòng phim ở trên hoặc nút [X Đóng Spotlight] để quay về danh sách đầy đủ."));
                bHint.setPosition(infoX, spotlightY + spotlightH - 24.f);
                window.draw(bHint);
            }
        }

        // Table Footer (Pagination & Record count)
        sf::Text footInfo;
        footInfo.setFont(Theme::FontPrimary);
        footInfo.setCharacterSize(13);
        footInfo.setFillColor(sf::Color(100, 116, 139));
        if (totalFiltered == 0) {
            footInfo.setString(toSfString("Không tìm thấy phim phù hợp"));
        } else {
            footInfo.setString(toSfString("Hiển thị " + std::to_string(startIndex + 1) + " - " + std::to_string(endIndex) + " trên " + std::to_string(totalFiltered) + " phim"));
        }
        footInfo.setPosition(leftColX + 20.f, footerY + 6.f);
        window.draw(footInfo);

        // Pagination buttons [Trước] [1] [2] ... [Sau]
        float pageBtnEndX = leftColX + leftColW - 20.f;
        float pNextW = 50.f;
        float pNextX = pageBtnEndX - pNextW;
        RoundedRectRenderer::render(window, sf::Vector2f(pNextX, footerY), sf::Vector2f(pNextW, 28.f), 6.f, sf::Color(241, 245, 249));
        sf::Text pNext;
        pNext.setFont(Theme::FontPrimary);
        pNext.setCharacterSize(12);
        pNext.setFillColor((movieCurrentPage < totalMoviePages - 1) ? sf::Color(3, 105, 161) : sf::Color(148, 163, 184));
        pNext.setString(toSfString("Sau"));
        sf::FloatRect nb = pNext.getLocalBounds();
        pNext.setPosition(pNextX + (pNextW - nb.width) / 2.f, footerY + 5.f);
        window.draw(pNext);

        float curNumX = pNextX - 8.f;
        for (int p = std::min(totalMoviePages - 1, 4); p >= 0; --p) {
            float pNumW = 28.f;
            curNumX -= pNumW;
            bool isCurP = (p == movieCurrentPage);
            RoundedRectRenderer::render(window, sf::Vector2f(curNumX, footerY), sf::Vector2f(pNumW, 28.f), 6.f, isCurP ? sf::Color(3, 105, 161) : sf::Color(241, 245, 249));
            sf::Text pNum;
            pNum.setFont(Theme::FontPrimary);
            pNum.setCharacterSize(12);
            if (isCurP) pNum.setStyle(sf::Text::Bold);
            pNum.setFillColor(isCurP ? sf::Color::White : sf::Color(71, 85, 105));
            pNum.setString(std::to_string(p + 1));
            sf::FloatRect nmb = pNum.getLocalBounds();
            pNum.setPosition(curNumX + (pNumW - nmb.width) / 2.f, footerY + 5.f);
            window.draw(pNum);
            curNumX -= 4.f;
        }

        float pPrevW = 54.f;
        float pPrevX = curNumX - pPrevW;
        RoundedRectRenderer::render(window, sf::Vector2f(pPrevX, footerY), sf::Vector2f(pPrevW, 28.f), 6.f, sf::Color(241, 245, 249));
        sf::Text pPrev;
        pPrev.setFont(Theme::FontPrimary);
        pPrev.setCharacterSize(12);
        pPrev.setFillColor((movieCurrentPage > 0) ? sf::Color(3, 105, 161) : sf::Color(148, 163, 184));
        pPrev.setString(toSfString("Trước"));
        sf::FloatRect pb = pPrev.getLocalBounds();
        pPrev.setPosition(pPrevX + (pPrevW - pb.width) / 2.f, footerY + 5.f);
        window.draw(pPrev);
    } else {
        // Genre Table (Matching Reference Design 100%)
        // 1. Card Header: "Danh Sách Thể Loại Phim (Áp dụng trên toàn hệ thống rạp)" + "Sắp xếp theo: [Mã thể loại ↓↑]"
        sf::Text gHdrTitle;
        gHdrTitle.setFont(Theme::FontPrimary);
        gHdrTitle.setCharacterSize(17);
        gHdrTitle.setStyle(sf::Text::Bold);
        gHdrTitle.setFillColor(sf::Color(15, 23, 42));
        gHdrTitle.setString(toSfString("Danh Sách Thể Loại Phim"));
        gHdrTitle.setPosition(leftColX + 24.f, cardY + 18.f);
        window.draw(gHdrTitle);

        sf::FloatRect ghtB = gHdrTitle.getLocalBounds();
        sf::Text gHdrSub;
        gHdrSub.setFont(Theme::FontPrimary);
        gHdrSub.setCharacterSize(13);
        gHdrSub.setFillColor(sf::Color(100, 116, 139));
        gHdrSub.setString(toSfString("(Áp dụng trên toàn hệ thống rạp)"));
        gHdrSub.setPosition(leftColX + 34.f + ghtB.width, cardY + 21.f);
        window.draw(gHdrSub);

        if (btnQuickAddGenre) btnQuickAddGenre->draw(window);

        float sortBtnW = 114.f;
        float sortBtnH = 28.f;
        float sortBtnX = leftColX + leftColW - 138.f;
        float sortBtnY = cardY + 15.f;

        RoundedRectRenderer::render(
            window,
            sf::Vector2f(sortBtnX, sortBtnY),
            sf::Vector2f(sortBtnW, sortBtnH),
            6.f,
            sf::Color(248, 250, 252),
            1.f,
            sf::Color(226, 232, 240)
        );
        sf::Text sortBtnTxt;
        sortBtnTxt.setFont(Theme::FontPrimary);
        sortBtnTxt.setCharacterSize(12);
        sortBtnTxt.setStyle(sf::Text::Bold);
        sortBtnTxt.setFillColor(sf::Color(30, 41, 59));
        sortBtnTxt.setString(toSfString(genreSortAsc ? "Mã thể loại ↓↑" : "Mã thể loại ↑↓"));
        sf::FloatRect sbB = sortBtnTxt.getLocalBounds();
        sortBtnTxt.setPosition(
            std::round(sortBtnX + (sortBtnW - sbB.width) / 2.f - sbB.left),
            std::round(sortBtnY + (sortBtnH - sbB.height) / 2.f - sbB.top)
        );
        window.draw(sortBtnTxt);

        sf::Text sortLbl;
        sortLbl.setFont(Theme::FontPrimary);
        sortLbl.setCharacterSize(13);
        sortLbl.setFillColor(sf::Color(100, 116, 139));
        sortLbl.setString(toSfString("Sắp xếp theo:"));
        sf::FloatRect slB = sortLbl.getLocalBounds();
        float sortLblX = sortBtnX - 10.f - slB.width;
        sortLbl.setPosition(sortLblX, cardY + 20.f);
        window.draw(sortLbl);

        // 2. Table Column Header Strip
        float tblHeaderY = cardY + 58.f;
        sf::RectangleShape thBg(sf::Vector2f(leftColW - 2.f, 40.f));
        thBg.setPosition(leftColX + 1.f, tblHeaderY);
        thBg.setFillColor(sf::Color(248, 250, 252));
        window.draw(thBg);

        sf::RectangleShape thTopLine(sf::Vector2f(leftColW - 2.f, 1.f));
        thTopLine.setPosition(leftColX + 1.f, tblHeaderY);
        thTopLine.setFillColor(sf::Color(241, 245, 249));
        window.draw(thTopLine);

        sf::RectangleShape thBotLine(sf::Vector2f(leftColW - 2.f, 1.f));
        thBotLine.setPosition(leftColX + 1.f, tblHeaderY + 39.f);
        thBotLine.setFillColor(sf::Color(241, 245, 249));
        window.draw(thBotLine);

        std::vector<std::string> cols = {"MÃ THỂ LOẠI", "TÊN THỂ LOẠI"};
        std::vector<float> colX = {leftColX + 36.f, leftColX + 210.f};

        for (size_t c = 0; c < cols.size(); ++c) {
            sf::Text ch;
            ch.setFont(Theme::FontPrimary);
            ch.setCharacterSize(12);
            ch.setStyle(sf::Text::Bold);
            ch.setFillColor(sf::Color(100, 116, 139));
            ch.setString(toSfString(cols[c]));
            ch.setPosition(colX[c], tblHeaderY + 12.f);
            window.draw(ch);
        }

        // Palette of indicator dot colors matching reference screenshot
        const sf::Color dotColors[] = {
            sf::Color(239, 68, 68),   // 0: Red (Hành Động)
            sf::Color(139, 92, 246),  // 1: Purple (Kinh Dị)
            sf::Color(236, 72, 153),  // 2: Pink (Tình Cảm)
            sf::Color(245, 158, 11),  // 3: Amber (Hoạt Hình)
            sf::Color(59, 130, 246),  // 4: Blue (Tâm Lý)
            sf::Color(16, 185, 129),  // 5: Emerald (Gia Đình)
            sf::Color(6, 182, 212),   // 6: Cyan (Viễn Tưởng)
            sf::Color(249, 115, 22),  // 7: Orange (Kịch Tính)
            sf::Color(220, 38, 38)    // 8: Crimson (Gay Cấn)
        };

        std::string q = genreSearchInput ? genreSearchInput->getText() : "";
        std::transform(q.begin(), q.end(), q.begin(), ::tolower);

        std::vector<int> visibleIndices;
        for (size_t i = 0; i < genreList.size(); ++i) {
            if (!q.empty()) {
                std::string cLow = genreList[i].first;
                std::string nLow = genreList[i].second;
                std::transform(cLow.begin(), cLow.end(), cLow.begin(), ::tolower);
                std::transform(nLow.begin(), nLow.end(), nLow.begin(), ::tolower);
                if (cLow.find(q) == std::string::npos && nLow.find(q) == std::string::npos) {
                    continue;
                }
            }
            visibleIndices.push_back(static_cast<int>(i));
        }
        if (!genreSortAsc) {
            std::reverse(visibleIndices.begin(), visibleIndices.end());
        }

        float rowStartY = tblHeaderY + 40.f;
        float rowH = 56.f;
        int shownGenres = 0;
        for (size_t v = 0; v < visibleIndices.size() && v < 10; ++v) {
            int idx = visibleIndices[v];
            float ry = rowStartY + static_cast<float>(v) * rowH;
            bool isSelected = (selectedGenreRowIdx == idx);

            sf::RectangleShape rBg(sf::Vector2f(leftColW - 2.f, rowH - 1.f));
            rBg.setPosition(leftColX + 1.f, ry);
            rBg.setFillColor(isSelected ? sf::Color(240, 249, 255) : sf::Color::White);
            window.draw(rBg);

            if (isSelected) {
                sf::RectangleShape leftAccent(sf::Vector2f(4.f, rowH - 1.f));
                leftAccent.setPosition(leftColX + 1.f, ry);
                leftAccent.setFillColor(sf::Color(2, 132, 199));
                window.draw(leftAccent);
            }

            sf::RectangleShape rowDiv(sf::Vector2f(leftColW - 2.f, 1.f));
            rowDiv.setPosition(leftColX + 1.f, ry + rowH - 1.f);
            rowDiv.setFillColor(sf::Color(241, 245, 249));
            window.draw(rowDiv);

            // Code Pill Badge [TL01]
            sf::Color pillBg = isSelected ? sf::Color(224, 242, 254) : sf::Color(241, 245, 249);
            sf::Color pillFg = isSelected ? sf::Color(2, 132, 199) : sf::Color(71, 85, 105);
            RoundedRectRenderer::render(window, sf::Vector2f(colX[0], ry + 15.f), sf::Vector2f(54.f, 26.f), 6.f, pillBg);

            sf::Text tId;
            tId.setFont(Theme::FontPrimary);
            tId.setCharacterSize(12);
            tId.setStyle(sf::Text::Bold);
            tId.setFillColor(pillFg);
            tId.setString(genreList[idx].first);
            sf::FloatRect idB = tId.getLocalBounds();
            tId.setPosition(
                std::round(colX[0] + (54.f - idB.width) / 2.f - idB.left),
                std::round(ry + 15.f + (26.f - idB.height) / 2.f - idB.top)
            );
            window.draw(tId);

            // Colored Dot + Genre Name
            int cIdx = genreColorMap.count(genreList[idx].first) ? genreColorMap[genreList[idx].first] : idx;
            sf::CircleShape gDot(4.5f);
            gDot.setFillColor(dotColors[cIdx % 9]);
            gDot.setPosition(colX[1], ry + 23.5f);
            window.draw(gDot);

            sf::Text tName;
            tName.setFont(Theme::FontPrimary);
            tName.setCharacterSize(14);
            tName.setStyle(sf::Text::Bold);
            tName.setFillColor(isSelected ? sf::Color(15, 23, 42) : sf::Color(30, 41, 59));
            tName.setString(toSfString(genreList[idx].second));
            tName.setPosition(colX[1] + 18.f, ry + 18.f);
            window.draw(tName);

            shownGenres++;
        }

        // 3. Genre Table Footer ("Hiển thị 9 trên 9 thể loại phim" + [<] [1] [>])
        float footerY = cardY + cardH - 44.f;
        sf::RectangleShape footLine(sf::Vector2f(leftColW - 2.f, 1.f));
        footLine.setPosition(leftColX + 1.f, footerY - 10.f);
        footLine.setFillColor(sf::Color(241, 245, 249));
        window.draw(footLine);

        sf::Text gFootTxt;
        gFootTxt.setFont(Theme::FontPrimary);
        gFootTxt.setCharacterSize(13);
        gFootTxt.setFillColor(sf::Color(100, 116, 139));
        gFootTxt.setString(toSfString("Hiển thị " + std::to_string(shownGenres) + " trên " + std::to_string(genreList.size()) + " thể loại phim"));
        gFootTxt.setPosition(leftColX + 24.f, footerY + 4.f);
        window.draw(gFootTxt);

        // Pagination [<] [1] [>]
        float pgX = leftColX + leftColW - 118.f;
        RoundedRectRenderer::render(window, sf::Vector2f(pgX, footerY), sf::Vector2f(28.f, 28.f), 6.f, sf::Color::White, 1.f, sf::Color(226, 232, 240));
        sf::Text pgPrev;
        pgPrev.setFont(Theme::FontPrimary);
        pgPrev.setCharacterSize(13);
        pgPrev.setStyle(sf::Text::Bold);
        pgPrev.setFillColor(sf::Color(148, 163, 184));
        pgPrev.setString("<");
        pgPrev.setPosition(pgX + 10.f, footerY + 5.f);
        window.draw(pgPrev);

        RoundedRectRenderer::render(window, sf::Vector2f(pgX + 34.f, footerY), sf::Vector2f(28.f, 28.f), 6.f, sf::Color(2, 132, 199));
        sf::Text pgOne;
        pgOne.setFont(Theme::FontPrimary);
        pgOne.setCharacterSize(13);
        pgOne.setStyle(sf::Text::Bold);
        pgOne.setFillColor(sf::Color::White);
        pgOne.setString("1");
        pgOne.setPosition(pgX + 44.f, footerY + 5.f);
        window.draw(pgOne);

        RoundedRectRenderer::render(window, sf::Vector2f(pgX + 68.f, footerY), sf::Vector2f(28.f, 28.f), 6.f, sf::Color::White, 1.f, sf::Color(226, 232, 240));
        sf::Text pgNext;
        pgNext.setFont(Theme::FontPrimary);
        pgNext.setCharacterSize(13);
        pgNext.setStyle(sf::Text::Bold);
        pgNext.setFillColor(sf::Color(148, 163, 184));
        pgNext.setString(">");
        pgNext.setPosition(pgX + 78.f, footerY + 5.f);
        window.draw(pgNext);
    }

    // 7. RIGHT CARD: Form Card (White, cardY = 232.f, cardH = 716.f)
    RoundedRectRenderer::render(
        window,
        sf::Vector2f(rightColX, cardY),
        sf::Vector2f(rightColW, cardH),
        12.f,
        sf::Color::White,
        1.f,
        sf::Color(226, 232, 240)
    );

    if (movieSubTab == 0) {
        // Title "Thông tin Phim" + Status Badge "● Đang sửa: P01"
        sf::Text formTitle;
        formTitle.setFont(Theme::FontPrimary);
        formTitle.setCharacterSize(18);
        formTitle.setStyle(sf::Text::Bold);
        formTitle.setFillColor(sf::Color(15, 23, 42));
        formTitle.setString(toSfString("Thông tin Phim"));
        formTitle.setPosition(rightColX + 20.f, cardY + 18.f);
        window.draw(formTitle);

        std::string curIdBadge;
        sf::Color badgeBg(224, 242, 254);
        sf::Color dotColor(3, 105, 161);
        sf::Color badgeTextColor(3, 105, 161);

        if (selectedMovieRowIdx >= 0 && selectedMovieRowIdx < static_cast<int>(movieList.size())) {
            curIdBadge = "Đang sửa: " + movieList[selectedMovieRowIdx].getMaPhim();
        } else if (movieSearchInput && !movieSearchInput->getText().empty()) {
            curIdBadge = "Không có kết quả";
            badgeBg = sf::Color(241, 245, 249);
            dotColor = sf::Color(148, 163, 184);
            badgeTextColor = sf::Color(100, 116, 139);
        } else {
            curIdBadge = "Tạo phim mới";
        }

        RoundedRectRenderer::render(window, sf::Vector2f(rightColX + rightColW - 148.f, cardY + 16.f), sf::Vector2f(128.f, 26.f), 13.f, badgeBg);
        sf::CircleShape editDot(3.5f);
        editDot.setFillColor(dotColor);
        editDot.setPosition(rightColX + rightColW - 138.f, cardY + 25.f);
        window.draw(editDot);

        sf::Text editBadgeTxt;
        editBadgeTxt.setFont(Theme::FontPrimary);
        editBadgeTxt.setCharacterSize(12);
        editBadgeTxt.setStyle(sf::Text::Bold);
        editBadgeTxt.setFillColor(badgeTextColor);
        editBadgeTxt.setString(toSfString(curIdBadge));
        editBadgeTxt.setPosition(rightColX + rightColW - 126.f, cardY + 21.f);
        window.draw(editBadgeTxt);

        // Labels (Bold 13px Segoe UI)
        std::vector<std::pair<std::string, float>> formLabels = {
            {"Mã Phim: *", 294.f},
            {"Tên Phim: *", 339.f},
            {"Thời lượng:", 384.f},
            {"Ngày chiếu:", 429.f},
            {"Nhãn tuổi (Độ):", 474.f},
            {"Thể loại:", 519.f},
            {"Poster Phim:", 566.f},
            {"Mô tả:", 626.f}
        };

        for (const auto& fl : formLabels) {
            sf::Text lText;
            lText.setFont(Theme::FontPrimary);
            lText.setCharacterSize(13);
            lText.setStyle(sf::Text::Bold);
            lText.setFillColor(sf::Color(51, 65, 85));
            lText.setString(toSfString(fl.first));
            lText.setPosition(rightColX + 20.f, fl.second);
            window.draw(lText);
        }

        // Draw Form Inputs
        float inputX = rightColX + 136.f;
        if (inputMovieId) inputMovieId->draw(window);
        if (inputMovieTitle) inputMovieTitle->draw(window);
        if (inputMovieDuration) inputMovieDuration->draw(window);
        if (inputMovieReleaseDate) inputMovieReleaseDate->draw(window);
        if (inputMovieGenre) inputMovieGenre->draw(window);
        if (btnSelectGenre) btnSelectGenre->draw(window);
        if (inputMoviePoster) {
            inputMoviePoster->draw(window);
            RoundedRectRenderer::render(window, sf::Vector2f(inputX + 68.f, 576.f), sf::Vector2f(16.f, 14.f), 2.5f, sf::Color::White, 1.3f, sf::Color(2, 132, 199));
            sf::CircleShape miniSun(1.6f);
            miniSun.setFillColor(sf::Color(2, 132, 199));
            miniSun.setPosition(inputX + 71.f, 579.f);
            window.draw(miniSun);
        }
        if (inputMovieDesc) inputMovieDesc->draw(window);

        // "phút" suffix inside Thời lượng input
        sf::Text unitTxt;
        unitTxt.setFont(Theme::FontPrimary);
        unitTxt.setCharacterSize(13);
        unitTxt.setStyle(sf::Text::Bold);
        unitTxt.setFillColor(sf::Color(100, 116, 139));
        unitTxt.setString(toSfString("phút"));
        unitTxt.setPosition(rightColX + rightColW - 60.f, 384.f);
        window.draw(unitTxt);

        // Dropdown Age Rating Selector on y = 466.f
        if (movieFormAgeDropdown) {
            movieFormAgeDropdown->draw(window);
        }

        // Dedicated Poster Preview Box (48x64) + Hint text for adding poster images
        std::string previewCode = inputMovieId ? inputMovieId->getText() : "";
        std::string previewPath = inputMoviePoster ? inputMoviePoster->getText() : "";
        drawPosterBox(previewCode, previewPath, inputX, 554.f, 48.f, 64.f);

        sf::Text posterHint;
        posterHint.setFont(Theme::FontPrimary);
        posterHint.setCharacterSize(11);
        posterHint.setFillColor(sf::Color(100, 116, 139));
        posterHint.setString(toSfString("Đặt ảnh vào thư mục assets/posters/<MãPhim>.jpg hoặc nhập đường dẫn"));
        posterHint.setPosition(inputX + 58.f, 604.f);
        window.draw(posterHint);

        // POS Schedule Activation Toggle Card (y = 736.f)
        RoundedRectRenderer::render(window, sf::Vector2f(rightColX + 20.f, 736.f), sf::Vector2f(rightColW - 40.f, 48.f), 8.f, sf::Color(240, 249, 255), 1.f, sf::Color(224, 242, 254));
        sf::Text posTitle;
        posTitle.setFont(Theme::FontPrimary);
        posTitle.setCharacterSize(13);
        posTitle.setStyle(sf::Text::Bold);
        posTitle.setFillColor(sf::Color(15, 23, 42));
        posTitle.setString(toSfString("Kích hoạt lịch chiếu POS"));
        posTitle.setPosition(rightColX + 34.f, 742.f);
        window.draw(posTitle);

        sf::Text posSub;
        posSub.setFont(Theme::FontPrimary);
        posSub.setCharacterSize(11);
        posSub.setFillColor(sf::Color(100, 116, 139));
        posSub.setString(toSfString("Cho phép mở bán vé trực tiếp tại quầy"));
        posSub.setPosition(rightColX + 34.f, 761.f);
        window.draw(posSub);

        // Toggle pill
        float togX = rightColX + rightColW - 76.f;
        float togY = 748.f;
        RoundedRectRenderer::render(window, sf::Vector2f(togX, togY), sf::Vector2f(44.f, 24.f), 12.f, moviePosActive ? sf::Color(22, 163, 74) : sf::Color(203, 213, 225));
        sf::CircleShape togKnob(9.f);
        togKnob.setFillColor(sf::Color::White);
        togKnob.setPosition(moviePosActive ? (togX + 23.f) : (togX + 3.f), togY + 3.f);
        window.draw(togKnob);

        // 4 Action Buttons
        if (btnMovieCreate) btnMovieCreate->draw(window);
        if (btnMovieUpdate) btnMovieUpdate->draw(window);
        if (btnMovieDelete) btnMovieDelete->draw(window);
        if (btnMovieClear) btnMovieClear->draw(window);
    } else {
        // Genre Form (Right Card: Matching Reference UI 100%)
        // 1. Header Icon Badge + Title & Subtitle + Mode Pill Badge
        RoundedRectRenderer::render(
            window,
            sf::Vector2f(rightColX + 24.f, cardY + 20.f),
            sf::Vector2f(38.f, 38.f),
            10.f,
            sf::Color(224, 242, 254)
        );
        // Tag / label vector icon inside badge
        sf::ConvexShape tagIcon(5);
        tagIcon.setPoint(0, sf::Vector2f(rightColX + 35.f, cardY + 31.f));
        tagIcon.setPoint(1, sf::Vector2f(rightColX + 44.f, cardY + 31.f));
        tagIcon.setPoint(2, sf::Vector2f(rightColX + 51.f, cardY + 38.f));
        tagIcon.setPoint(3, sf::Vector2f(rightColX + 43.f, cardY + 46.f));
        tagIcon.setPoint(4, sf::Vector2f(rightColX + 35.f, cardY + 39.f));
        tagIcon.setFillColor(sf::Color(2, 132, 199));
        window.draw(tagIcon);
        sf::CircleShape tagHole(2.f);
        tagHole.setFillColor(sf::Color(224, 242, 254));
        tagHole.setPosition(rightColX + 37.5f, cardY + 33.5f);
        window.draw(tagHole);

        bool isEditMode = (selectedGenreRowIdx >= 0 && selectedGenreRowIdx < static_cast<int>(genreList.size()));

        sf::Text formTitle;
        formTitle.setFont(Theme::FontPrimary);
        formTitle.setCharacterSize(17);
        formTitle.setStyle(sf::Text::Bold);
        formTitle.setFillColor(sf::Color(15, 23, 42));
        formTitle.setString(toSfString(isEditMode ? "Thông tin Thể loại" : "Thêm Thể Loại Mới"));
        formTitle.setPosition(rightColX + 74.f, cardY + 19.f);
        window.draw(formTitle);

        sf::Text formSub;
        formSub.setFont(Theme::FontPrimary);
        formSub.setCharacterSize(12);
        formSub.setFillColor(sf::Color(100, 116, 139));
        formSub.setString(toSfString(isEditMode ? "Xem hoặc cập nhật dữ liệu" : "Nhập tên thể loại để thêm vào hệ thống"));
        formSub.setPosition(rightColX + 74.f, cardY + 42.f);
        window.draw(formSub);
        RoundedRectRenderer::render(
            window,
            sf::Vector2f(rightColX + rightColW - 148.f, cardY + 23.f),
            sf::Vector2f(124.f, 28.f),
            8.f,
            isEditMode ? sf::Color(240, 249, 255) : sf::Color(240, 253, 244),
            1.f,
            isEditMode ? sf::Color(186, 230, 253) : sf::Color(187, 247, 208)
        );
        sf::Text modeTxt;
        modeTxt.setFont(Theme::FontPrimary);
        modeTxt.setCharacterSize(11);
        modeTxt.setStyle(sf::Text::Bold);
        modeTxt.setFillColor(isEditMode ? sf::Color(2, 132, 199) : sf::Color(21, 128, 61));
        modeTxt.setString(toSfString(isEditMode ? "Chế độ: Xem / Sửa" : "Chế độ: Thêm mới"));
        sf::FloatRect mdB = modeTxt.getLocalBounds();
        modeTxt.setPosition(
            std::round(rightColX + rightColW - 148.f + (124.f - mdB.width) / 2.f - mdB.left),
            std::round(cardY + 23.f + (28.f - mdB.height) / 2.f - mdB.top)
        );
        window.draw(modeTxt);

        sf::RectangleShape hdrLine(sf::Vector2f(rightColW - 48.f, 1.f));
        hdrLine.setPosition(rightColX + 24.f, cardY + 74.f);
        hdrLine.setFillColor(sf::Color(241, 245, 249));
        window.draw(hdrLine);

        // 2. Field 1: MÃ THỂ LOẠI * + "🔒 Tự động khóa"
        sf::Text l1;
        l1.setFont(Theme::FontPrimary);
        l1.setCharacterSize(12);
        l1.setStyle(sf::Text::Bold);
        l1.setFillColor(sf::Color(51, 65, 85));
        l1.setString(toSfString("MÃ THỂ LOẠI"));
        l1.setPosition(rightColX + 24.f, 326.f);
        window.draw(l1);

        sf::Text star1;
        star1.setFont(Theme::FontPrimary);
        star1.setCharacterSize(13);
        star1.setStyle(sf::Text::Bold);
        star1.setFillColor(sf::Color(239, 68, 68));
        star1.setString("*");
        star1.setPosition(rightColX + 108.f, 325.f);
        window.draw(star1);

        // Lock icon + "Tự động khóa"
        RoundedRectRenderer::render(window, sf::Vector2f(rightColX + rightColW - 112.f, 331.f), sf::Vector2f(9.f, 8.f), 2.f, sf::Color(148, 163, 184));
        sf::CircleShape lockArch(2.5f);
        lockArch.setPosition(rightColX + rightColW - 110.f, 327.f);
        lockArch.setFillColor(sf::Color::Transparent);
        lockArch.setOutlineThickness(1.3f);
        lockArch.setOutlineColor(sf::Color(148, 163, 184));
        window.draw(lockArch);

        sf::Text lockTxt;
        lockTxt.setFont(Theme::FontPrimary);
        lockTxt.setCharacterSize(11);
        lockTxt.setFillColor(sf::Color(148, 163, 184));
        lockTxt.setString(toSfString("Tự động khóa"));
        lockTxt.setPosition(rightColX + rightColW - 98.f, 326.f);
        window.draw(lockTxt);

        if (inputGenreId) inputGenreId->draw(window);

        // Shield icon inside right edge of inputGenreId
        sf::ConvexShape idShield(6);
        float shX = rightColX + rightColW - 46.f;
        float shY = 367.f;
        idShield.setPoint(0, sf::Vector2f(shX + 6.f, shY));
        idShield.setPoint(1, sf::Vector2f(shX + 12.f, shY + 2.f));
        idShield.setPoint(2, sf::Vector2f(shX + 11.f, shY + 9.f));
        idShield.setPoint(3, sf::Vector2f(shX + 6.f, shY + 14.f));
        idShield.setPoint(4, sf::Vector2f(shX + 1.f, shY + 9.f));
        idShield.setPoint(5, sf::Vector2f(shX, shY + 2.f));
        idShield.setFillColor(sf::Color(148, 163, 184));
        window.draw(idShield);

        // 3. Field 2: TÊN THỂ LOẠI * + helper text
        sf::Text l2;
        l2.setFont(Theme::FontPrimary);
        l2.setCharacterSize(12);
        l2.setStyle(sf::Text::Bold);
        l2.setFillColor(sf::Color(51, 65, 85));
        l2.setString(toSfString("TÊN THỂ LOẠI"));
        l2.setPosition(rightColX + 24.f, 416.f);
        window.draw(l2);

        sf::Text star2;
        star2.setFont(Theme::FontPrimary);
        star2.setCharacterSize(13);
        star2.setStyle(sf::Text::Bold);
        star2.setFillColor(sf::Color(239, 68, 68));
        star2.setString("*");
        star2.setPosition(rightColX + 112.f, 415.f);
        window.draw(star2);

        if (inputGenreName) inputGenreName->draw(window);

        // Keyboard icon inside right edge of inputGenreName
        RoundedRectRenderer::render(window, sf::Vector2f(rightColX + rightColW - 50.f, 458.f), sf::Vector2f(16.f, 11.f), 2.f, sf::Color::Transparent, 1.2f, sf::Color(148, 163, 184));
        for (int kx = 0; kx < 3; ++kx) {
            sf::RectangleShape kDot(sf::Vector2f(2.f, 2.f));
            kDot.setPosition(rightColX + rightColW - 47.f + kx * 4.f, 461.f);
            kDot.setFillColor(sf::Color(148, 163, 184));
            window.draw(kDot);
        }
        sf::RectangleShape kSpace(sf::Vector2f(8.f, 1.5f));
        kSpace.setPosition(rightColX + rightColW - 46.f, 465.f);
        kSpace.setFillColor(sf::Color(148, 163, 184));
        window.draw(kSpace);

        sf::Text helperTxt;
        helperTxt.setFont(Theme::FontPrimary);
        helperTxt.setCharacterSize(11);
        helperTxt.setFillColor(sf::Color(148, 163, 184));
        helperTxt.setString(toSfString("Ví dụ: Hành Động, Khoa Học Viễn Tưởng, Tâm Lý..."));
        helperTxt.setPosition(rightColX + 24.f, 494.f);
        window.draw(helperTxt);

        // 4. Field 3: MÀU SẮC BIỂU THỊ + 5 Swatch Circles
        sf::Text l3;
        l3.setFont(Theme::FontPrimary);
        l3.setCharacterSize(12);
        l3.setStyle(sf::Text::Bold);
        l3.setFillColor(sf::Color(51, 65, 85));
        l3.setString(toSfString("MÀU SẮC BIỂU THỊ"));
        l3.setPosition(rightColX + 24.f, 532.f);
        window.draw(l3);

        const sf::Color swatchColors[5] = {
            sf::Color(239, 68, 68),   // Red
            sf::Color(139, 92, 246),  // Purple
            sf::Color(236, 72, 153),  // Pink
            sf::Color(245, 158, 11),  // Amber
            sf::Color(2, 132, 199)    // Blue
        };
        const char* swatchNames[5] = {
            "Màu Đỏ (Hành Động)",
            "Màu Tím (Kinh Dị)",
            "Màu Hồng (Tình Cảm)",
            "Màu Cam (Hoạt Hình)",
            "Màu Xanh (Tâm Lý)"
        };

        int activeSwatch = (selectedGenreColorIdx >= 0 && selectedGenreColorIdx < 5) ? selectedGenreColorIdx : 0;
        for (int c = 0; c < 5; ++c) {
            float cx = rightColX + 42.f + c * 40.f;
            float cy = 574.f;
            if (c == activeSwatch) {
                sf::CircleShape ring(16.5f);
                ring.setPosition(cx - 16.5f, cy - 16.5f);
                ring.setFillColor(sf::Color::Transparent);
                ring.setOutlineThickness(1.8f);
                ring.setOutlineColor(swatchColors[c]);
                window.draw(ring);
            }
            sf::CircleShape sw(13.f);
            sw.setPosition(cx - 13.f, cy - 13.f);
            sw.setFillColor(swatchColors[c]);
            window.draw(sw);
        }

        sf::Text swLabel;
        swLabel.setFont(Theme::FontPrimary);
        swLabel.setCharacterSize(12);
        swLabel.setFillColor(sf::Color(100, 116, 139));
        swLabel.setString(toSfString(swatchNames[activeSwatch]));
        swLabel.setPosition(rightColX + 234.f, 566.f);
        window.draw(swLabel);

        // 5. Action Buttons
        if (btnGenreCreate) btnGenreCreate->draw(window);
        if (btnGenreDelete) {
            btnGenreDelete->draw(window);
            // Trash icon on btnGenreDelete
            float tx = rightColX + 24.f + (rightColW - 62.f) / 2.f + 14.f + 44.f;
            float ty = 631.f;
            RoundedRectRenderer::render(window, sf::Vector2f(tx, ty + 3.f), sf::Vector2f(10.f, 11.f), 1.5f, sf::Color::Transparent, 1.3f, sf::Color::White);
            sf::RectangleShape tLid(sf::Vector2f(12.f, 1.5f));
            tLid.setPosition(tx - 1.f, ty + 1.f);
            tLid.setFillColor(sf::Color::White);
            window.draw(tLid);
        }
        if (btnGenreUpdate) {
            btnGenreUpdate->draw(window);
            // Save floppy icon on btnGenreUpdate
            float sx = rightColX + 172.f;
            float sy = 687.f;
            RoundedRectRenderer::render(window, sf::Vector2f(sx, sy), sf::Vector2f(13.f, 13.f), 2.f, sf::Color::Transparent, 1.4f, sf::Color::White);
            sf::RectangleShape sTop(sf::Vector2f(7.f, 4.f));
            sTop.setPosition(sx + 3.f, sy + 1.f);
            sTop.setFillColor(sf::Color::White);
            window.draw(sTop);
        }
        if (btnGenreClear) {
            btnGenreClear->draw(window);
            // Refresh circle arrow icon on btnGenreClear
            sf::CircleShape refArc(5.5f);
            refArc.setPosition(rightColX + 164.f, 742.f);
            refArc.setFillColor(sf::Color::Transparent);
            refArc.setOutlineThickness(1.4f);
            refArc.setOutlineColor(sf::Color(71, 85, 105));
            window.draw(refArc);
        }
    }
}

} // namespace UI