#include "../../include/UI/CinemaUIApp.h"
#include "../../include/ActivityLog.h"
#include <fstream>
#include <algorithm>
#include <cmath>

namespace UI {

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
void CinemaUIApp::setupSeatBookingView() {
    // 1. Refresh button in topbar ("Làm mới lịch chiếu")
    btnPosRefresh = std::unique_ptr<RoundRectButton>(new RoundRectButton(
        "Làm mới lịch chiếu",
        sf::Vector2f(1728.f - 230.f, (topbarHeight - 36.f) / 2.0f),
        sf::Vector2f(205.f, 36.f),
        ButtonStyle::Secondary,
        18.f
    ));
    btnPosRefresh->setCustomColors(
        sf::Color(14, 165, 233), // Sky blue #0ea5e9
        sf::Color(2, 132, 199),  // #0284c7
        sf::Color(3, 105, 161),  // #0369a1
        sf::Color(148, 163, 184)
    );
    btnPosRefresh->setOnClick([this]() {
        loadDataFromDatabase();
        if (!showtimeList.empty()) {
            if (posSelectedShowtimeIdx >= static_cast<int>(showtimeList.size())) posSelectedShowtimeIdx = 0;
            currentShowtimeId = showtimeList[posSelectedShowtimeIdx].getMaSuatChieu();
            refreshSeatsForShowtime(currentShowtimeId);
        }
    });

    // 2. Filter buttons
    float filterStartX = sidebarWidth + 24.f;
    float filterStartY = topbarHeight + 16.f;

    std::time_t now = std::time(nullptr);
    std::tm* lt = std::localtime(&now);
    char dateBuf[32];
    std::strftime(dateBuf, sizeof(dateBuf), "Hôm nay, %d/%m", lt);

    btnPosDate = std::unique_ptr<RoundRectButton>(new RoundRectButton(
        dateBuf,
        sf::Vector2f(filterStartX + 90.f, filterStartY + 14.f),
        sf::Vector2f(140.f, 32.f),
        ButtonStyle::Secondary,
        6.f
    ));
    btnPosDate->setCustomColors(
        sf::Color(14, 165, 233),
        sf::Color(2, 132, 199),
        sf::Color(3, 105, 161),
        sf::Color(148, 163, 184)
    );

    std::string currentMovieTitle = "Thỏ Ơi";
    if (!movieList.empty()) {
        currentMovieTitle = movieList[posSelectedMovieIdx % movieList.size()].getTenPhim();
    }

    btnPosMovie = std::unique_ptr<RoundRectButton>(new RoundRectButton(
        currentMovieTitle,
        sf::Vector2f(filterStartX + 90.f, filterStartY + 54.f),
        sf::Vector2f(140.f, 32.f),
        ButtonStyle::Secondary,
        6.f
    ));
    btnPosMovie->setCustomColors(
        sf::Color(14, 165, 233),
        sf::Color(2, 132, 199),
        sf::Color(3, 105, 161),
        sf::Color(148, 163, 184)
    );
    btnPosMovie->setOnClick([this]() {
        if (!movieList.empty()) {
            posSelectedMovieIdx = (posSelectedMovieIdx + 1) % movieList.size();
            std::string mTitle = movieList[posSelectedMovieIdx].getTenPhim();
            std::string mId = movieList[posSelectedMovieIdx].getMaPhim();
            btnPosMovie->setText(mTitle);

            for (size_t i = 0; i < showtimeList.size(); ++i) {
                if (showtimeList[i].getMaPhim() == mId) {
                    posSelectedShowtimeIdx = static_cast<int>(i);
                    currentShowtimeId = showtimeList[i].getMaSuatChieu();
                    std::string stLabel = showtimeList[i].getGioBatDau() + " - " + showtimeList[i].getMaPhong() + " - Standard";
                    btnPosShowtime->setText(stLabel);
                    posShowSeatMap = true;
                    refreshSeatsForShowtime(currentShowtimeId);
                    return;
                }
            }
        }
    });

    std::string currentShowtimeLabel = "17:00 - Phòng 1 - Standard";
    if (!showtimeList.empty()) {
        const auto& st = showtimeList[posSelectedShowtimeIdx % showtimeList.size()];
        currentShowtimeLabel = st.getGioBatDau() + " - " + st.getMaPhong() + " - Standard";
        currentShowtimeId = st.getMaSuatChieu();
    }

    btnPosShowtime = std::unique_ptr<RoundRectButton>(new RoundRectButton(
        currentShowtimeLabel,
        sf::Vector2f(filterStartX + 90.f, filterStartY + 94.f),
        sf::Vector2f(235.f, 34.f),
        ButtonStyle::Ghost,
        6.f
    ));
    btnPosShowtime->setCustomColors(
        sf::Color(255, 255, 255),
        sf::Color(248, 250, 252),
        sf::Color(241, 245, 249),
        sf::Color(226, 232, 240)
    );
    btnPosShowtime->setTextColor(sf::Color(15, 23, 42));
    btnPosShowtime->setOutline(1.0f, sf::Color(203, 213, 225));
    btnPosShowtime->setOnClick([this]() {
        if (!showtimeList.empty()) {
            posSelectedShowtimeIdx = (posSelectedShowtimeIdx + 1) % showtimeList.size();
            const auto& st = showtimeList[posSelectedShowtimeIdx];
            currentShowtimeId = st.getMaSuatChieu();
            std::string stLabel = st.getGioBatDau() + " - " + st.getMaPhong() + " - Standard";
            btnPosShowtime->setText(stLabel);
            for (size_t m = 0; m < movieList.size(); ++m) {
                if (movieList[m].getMaPhim() == st.getMaPhim()) {
                    posSelectedMovieIdx = static_cast<int>(m);
                    btnPosMovie->setText(movieList[m].getTenPhim());
                    break;
                }
            }
        }
        posShowSeatMap = true;
        refreshSeatsForShowtime(currentShowtimeId);
    });

    // Right Column Buttons
    float rightStartX = sidebarWidth + 1084.f;

    btnPosAddFnb = std::unique_ptr<RoundRectButton>(new RoundRectButton(
        "+ Thêm bắp/nước",
        sf::Vector2f(rightStartX + 14.f, topbarHeight + 434.f),
        sf::Vector2f(175.f, 34.f),
        ButtonStyle::Primary,
        6.f
    ));
    btnPosAddFnb->setCustomColors(
        sf::Color(245, 158, 11), // Amber #f59e0b
        sf::Color(217, 119, 6),  // #d97706
        sf::Color(180, 83, 9),   // #b45309
        sf::Color(203, 213, 225)
    );
    btnPosAddFnb->setOnClick([this]() {
        if (posSelectedFnb.empty()) {
            posSelectedFnb.push_back({"1x Combo Bắp Nước CineStar", 75000.0});
        } else if (posSelectedFnb.size() == 1) {
            posSelectedFnb.push_back({"1x Bắp Rang Phô Mai Lớn", 45000.0});
        } else if (posSelectedFnb.size() == 2) {
            posSelectedFnb.push_back({"1x Nước Ngọt Có Ga Lạnh", 25000.0});
        } else {
            posSelectedFnb.clear();
        }
        posFnbTotal = 0.0;
        for (const auto& item : posSelectedFnb) {
            posFnbTotal += item.second;
        }
        posGrandTotal = posSeatTotal + posFnbTotal;
    });

    btnPosCheckout = std::unique_ptr<RoundRectButton>(new RoundRectButton(
        "Thanh toán (F5)",
        sf::Vector2f(rightStartX + 14.f, topbarHeight + 560.f),
        sf::Vector2f(340.f, 48.f),
        ButtonStyle::Success,
        8.f
    ));
    btnPosCheckout->setCustomColors(
        sf::Color(34, 197, 94), // Emerald green #22c55e
        sf::Color(22, 163, 74), // #16a34a
        sf::Color(21, 128, 61), // #15803d
        sf::Color(203, 213, 225)
    );
    btnPosCheckout->setOnClick([this]() {
        if (selectedSeatCodes.empty() && posSelectedFnb.empty()) {
            confirmPopup->show("Thông báo", "Vui lòng chọn ít nhất 1 ghế hoặc món F&B trước khi thanh toán!", ButtonStyle::Primary, "Đồng ý", "Đóng");
            return;
        }
        std::ostringstream oss;
        oss << "XÁC NHẬN THANH TOÁN ĐƠN HÀNG POS (KIOSK)\n"
            << "Số vé: " << selectedSeatCodes.size() << " | F&B: " << posSelectedFnb.size() << " món\n"
            << "Tổng cộng: " << formatVND(posGrandTotal);

        confirmPopup->show("XÁC NHẬN BÁN VÉ", oss.str(), ButtonStyle::Success, "In Hóa Đơn & Xác Nhận", "Hủy");
        confirmPopup->setOnConfirm([this]() {
            for (auto& s : currentSeats) {
                if (s.isSelected) {
                    s.isSelected = false;
                    s.trangThai = "DaDat";
                    db.capNhatTrangThaiGheSuatChieu(currentShowtimeId, s.maGhe, "DaDat");
                }
            }
            std::string logMsg = "Xuất vé POS (" + std::to_string(selectedSeatCodes.size()) + " vé), F&B (" + std::to_string(posSelectedFnb.size()) + " món), thu: " + std::to_string((long long)posGrandTotal) + " VND";
            ActivityLog::ghiNhan(currentUserId, currentUserRole, "BAN_VE_POS", logMsg);
            setupLogsView();

            selectedSeatCodes.clear();
            posSelectedFnb.clear();
            posSeatTotal = 0.0;
            posFnbTotal = 0.0;
            posGrandTotal = 0.0;
        });
    });

    btnPosCancel = std::unique_ptr<RoundRectButton>(new RoundRectButton(
        "Hủy đơn",
        sf::Vector2f(rightStartX + 14.f, topbarHeight + 618.f),
        sf::Vector2f(340.f, 44.f),
        ButtonStyle::Dark,
        8.f
    ));
    btnPosCancel->setCustomColors(
        sf::Color(100, 116, 139), // Slate grey #64748b
        sf::Color(71, 85, 105),   // #475569
        sf::Color(51, 65, 85),    // #334155
        sf::Color(203, 213, 225)
    );
    btnPosCancel->setOnClick([this]() {
        for (auto& s : currentSeats) {
            s.isSelected = false;
        }
        selectedSeatCodes.clear();
        posSelectedFnb.clear();
        posSeatTotal = 0.0;
        posFnbTotal = 0.0;
        posGrandTotal = 0.0;
    });

    if (!showtimeList.empty()) {
        currentShowtimeId = showtimeList[0].getMaSuatChieu();
        refreshSeatsForShowtime(currentShowtimeId);
    }
}

void CinemaUIApp::refreshSeatsForShowtime(const std::string& maSuatChieu) {
    currentSeats.clear();
    selectedSeatCodes.clear();
    bookingTotalPrice = 0.0;

    // 1. Find showtime to get room ID
    std::string roomId = "R01";
    for (const auto& st : showtimeList) {
        if (st.getMaSuatChieu() == maSuatChieu) {
            roomId = st.getMaPhong();
            break;
        }
    }

    // 2. Read REAL seat status from DATA/lich_va_ve.csv
    vector<pair<string, string>> seatStatusList;
    db.docTrangThaiGheSuatChieu(maSuatChieu, seatStatusList);

    // 3. Filter REAL seats from DATA/quan_ly_phong_ghe.csv for this room
    std::vector<Seat> roomSeats;
    for (const auto& s : allSeats) {
        if (s.getMaPhong() == roomId) {
            roomSeats.push_back(s);
        }
    }

    // Layout configuration inside White Seat Card
    float cardX = sidebarWidth + 24.f;
    float cardY = topbarHeight + 172.f;
    float cardW = 1040.f;

    float seatW = 54.f;
    float seatH = 42.f;
    float gapX = 14.f;
    float gapY = 14.f;

    float gridStartX = cardX + (cardW - (10.f * seatW + 9.f * gapX)) / 2.0f;
    float gridStartY = cardY + 98.f;

    if (!roomSeats.empty()) {
        int colsPerRow = 10;
        if (roomSeats.size() <= 12) colsPerRow = 4;
        else if (roomSeats.size() <= 30) colsPerRow = 6;

        gridStartX = cardX + (cardW - (colsPerRow * seatW + (colsPerRow - 1) * gapX)) / 2.0f;

        for (size_t i = 0; i < roomSeats.size(); ++i) {
            VisualSeat vs;
            vs.maGhe = roomSeats[i].getMaGhe();
            vs.loaiGhe = roomSeats[i].getLoaiGhe();
            vs.donGia = roomSeats[i].getDonGiaGhe();
            vs.isSelected = false;
            vs.trangThai = "Trong";

            // Check if booked in CSV
            for (const auto& stat : seatStatusList) {
                if (stat.first == vs.maGhe && (stat.second == "DaDat" || stat.second == "1")) {
                    vs.trangThai = "DaDat";
                    break;
                }
            }

            int r = static_cast<int>(i) / colsPerRow;
            int c = static_cast<int>(i) % colsPerRow;

            vs.position = sf::Vector2f(
                gridStartX + static_cast<float>(c) * (seatW + gapX),
                gridStartY + static_cast<float>(r) * (seatH + gapY)
            );
            vs.size = sf::Vector2f(seatW, seatH);
            currentSeats.push_back(vs);
        }
    } else {
        // Fallback standard layout
        char rows[7] = {'A', 'B', 'C', 'D', 'E', 'F', 'G'};
        for (int r = 0; r < 7; ++r) {
            for (int c = 1; c <= 10; ++c) {
                VisualSeat s;
                std::string code = std::string(1, rows[r]) + (c < 10 ? "0" : "") + std::to_string(c);
                s.maGhe = code;
                s.isSelected = false;
                s.trangThai = "Trong";
                s.loaiGhe = (r < 3) ? "Standard" : ((r < 6) ? "VIP" : "Sweetbox");
                s.donGia = (r < 3) ? 60000.0 : ((r < 6) ? 90000.0 : 140000.0);

                for (const auto& stat : seatStatusList) {
                    if (stat.first == code && (stat.second == "DaDat" || stat.second == "1")) {
                        s.trangThai = "DaDat";
                        break;
                    }
                }

                s.position = sf::Vector2f(
                    gridStartX + static_cast<float>(c - 1) * (seatW + gapX),
                    gridStartY + static_cast<float>(r) * (seatH + gapY)
                );
                s.size = sf::Vector2f(seatW, seatH);
                currentSeats.push_back(s);
            }
        }
    }
}

std::vector<int> CinemaUIApp::getRetailFnbIndices() const {
    std::vector<int> indices;
    for (size_t i = 0; i < fnbList.size(); ++i) {
        if (fnbList[i].getLoai() != "Combo") {
            indices.push_back(static_cast<int>(i));
        }
    }
    return indices;
}


void CinemaUIApp::renderSeatBookingView() {
    float contentAreaW = 1728.f - sidebarWidth;
    float contentAreaH = 972.f - topbarHeight;

    // 1. Soft grey canvas background matching reference
    sf::RectangleShape posBg(sf::Vector2f(contentAreaW, contentAreaH));
    posBg.setPosition(sidebarWidth, topbarHeight);
    posBg.setFillColor(sf::Color(241, 245, 249)); // #f1f5f9
    window.draw(posBg);

    // 2. Top Header Bar for POS Kiosk Mode
    sf::RectangleShape topBar(sf::Vector2f(contentAreaW, topbarHeight));
    topBar.setPosition(sidebarWidth, 0.f);
    topBar.setFillColor(sf::Color(15, 23, 42)); // Dark Navy #0f172a
    window.draw(topBar);

    // Wallet / Ticket Vector Icon
    RoundedRectRenderer::render(
        window,
        sf::Vector2f(sidebarWidth + 24.f, (topbarHeight - 20.f) / 2.0f),
        sf::Vector2f(24.f, 20.f),
        3.f,
        sf::Color::Transparent,
        2.f,
        sf::Color::White
    );
    sf::RectangleShape walletClasp(sf::Vector2f(8.f, 6.f));
    walletClasp.setPosition(sidebarWidth + 38.f, (topbarHeight - 6.f) / 2.0f);
    walletClasp.setFillColor(sf::Color::White);
    window.draw(walletClasp);

    // Header Title
    sf::Text title;
    title.setFont(Theme::FontPrimary);
    title.setCharacterSize(18);
    title.setStyle(sf::Text::Bold);
    title.setFillColor(sf::Color::White);
    title.setString(toSfString("BÁN VÉ – POS (KIOSK MODE)"));
    title.setPosition(sidebarWidth + 56.f, (topbarHeight - 22.f) / 2.0f);
    window.draw(title);

    if (btnPosRefresh) btnPosRefresh->draw(window);

    // ==========================================
    // 3. LEFT COLUMN: FILTER CARD + SEAT MAP
    // ==========================================
    float leftColX = sidebarWidth + 20.f;
    float leftColW = 1040.f;

    // Top Filter Card (White)
    float filterCardY = topbarHeight + 14.f;
    float filterCardH = 138.f;
    RoundedRectRenderer::render(
        window,
        sf::Vector2f(leftColX, filterCardY),
        sf::Vector2f(leftColW, filterCardH),
        8.f,
        sf::Color::White,
        1.f,
        sf::Color(226, 232, 240) // #e2e8f0
    );

    // Labels
    sf::Text lblDate;
    lblDate.setFont(Theme::FontPrimary);
    lblDate.setCharacterSize(14);
    lblDate.setStyle(sf::Text::Bold);
    lblDate.setFillColor(sf::Color(15, 23, 42));
    lblDate.setString(toSfString("Ngày:"));
    lblDate.setPosition(leftColX + 24.f, filterCardY + 20.f);
    window.draw(lblDate);

    sf::Text lblMovie;
    lblMovie.setFont(Theme::FontPrimary);
    lblMovie.setCharacterSize(14);
    lblMovie.setStyle(sf::Text::Bold);
    lblMovie.setFillColor(sf::Color(15, 23, 42));
    lblMovie.setString(toSfString("Phim:"));
    lblMovie.setPosition(leftColX + 24.f, filterCardY + 60.f);
    window.draw(lblMovie);

    sf::Text lblShowtime;
    lblShowtime.setFont(Theme::FontPrimary);
    lblShowtime.setCharacterSize(14);
    lblShowtime.setStyle(sf::Text::Bold);
    lblShowtime.setFillColor(sf::Color(15, 23, 42));
    lblShowtime.setString(toSfString("Suất:"));
    lblShowtime.setPosition(leftColX + 24.f, filterCardY + 100.f);
    window.draw(lblShowtime);

    if (btnPosDate) btnPosDate->draw(window);
    if (btnPosMovie) btnPosMovie->draw(window);
    if (btnPosShowtime) btnPosShowtime->draw(window);

    // Bottom Seat Map Container Card (White)
    float seatCardY = topbarHeight + 162.f;
    float seatCardH = 718.f;
    RoundedRectRenderer::render(
        window,
        sf::Vector2f(leftColX, seatCardY),
        sf::Vector2f(leftColW, seatCardH),
        8.f,
        sf::Color::White,
        1.f,
        sf::Color(226, 232, 240)
    );

    if (!posShowSeatMap) {
        // Centered hint matching reference image
        sf::Text hintText;
        hintText.setFont(Theme::FontPrimary);
        hintText.setCharacterSize(15);
        hintText.setStyle(sf::Text::Italic);
        hintText.setFillColor(sf::Color(148, 163, 184)); // #94a3b8
        hintText.setString(toSfString("← Chọn suất chiếu ở trên để hiển thị sơ đồ ghế"));
        sf::FloatRect hb = hintText.getLocalBounds();
        hintText.setPosition(leftColX + (leftColW - hb.width) / 2.0f, seatCardY + 330.f);
        window.draw(hintText);
    } else {
        // Screen projection bar
        float screenW = 600.f;
        float screenX = leftColX + (leftColW - screenW) / 2.0f;
        float screenY = seatCardY + 28.f;

        RoundedRectRenderer::render(
            window,
            sf::Vector2f(screenX, screenY),
            sf::Vector2f(screenW, 10.f),
            5.f,
            sf::Color(14, 165, 233)
        );

        sf::Text screenText;
        screenText.setFont(Theme::FontPrimary);
        screenText.setCharacterSize(12);
        screenText.setStyle(sf::Text::Bold);
        screenText.setFillColor(sf::Color(100, 116, 139));
        screenText.setString(toSfString("MÀN HÌNH CHIẾU (SCREEN)"));
        sf::FloatRect stb = screenText.getLocalBounds();
        screenText.setPosition(screenX + (screenW - stb.width) / 2.0f, screenY + 18.f);
        window.draw(screenText);

        // Seat Grid
        for (const auto& s : currentSeats) {
            sf::Color seatBg = sf::Color::White;
            sf::Color seatBorder = sf::Color(203, 213, 225);
            sf::Color seatTxt = sf::Color(51, 65, 85);

            if (s.trangThai == "DaDat") {
                seatBg = sf::Color(226, 232, 240); // Grey booked
                seatBorder = sf::Color(203, 213, 225);
                seatTxt = sf::Color(148, 163, 184);
            } else if (s.isSelected) {
                seatBg = sf::Color(34, 197, 94);   // Green selected
                seatBorder = sf::Color(22, 163, 74);
                seatTxt = sf::Color::White;
            } else if (s.loaiGhe == "VIP") {
                seatBg = sf::Color(254, 243, 199); // Yellow VIP
                seatBorder = sf::Color(245, 158, 11);
                seatTxt = sf::Color(180, 83, 9);
            } else if (s.loaiGhe == "Sweetbox") {
                seatBg = sf::Color(252, 231, 243); // Pink Sweetbox
                seatBorder = sf::Color(236, 72, 153);
                seatTxt = sf::Color(190, 24, 93);
            }

            RoundedRectRenderer::render(
                window,
                s.position,
                s.size,
                6.f,
                seatBg,
                1.0f,
                seatBorder
            );

            sf::Text codeText;
            codeText.setFont(Theme::FontPrimary);
            codeText.setCharacterSize(11);
            codeText.setStyle(sf::Text::Bold);
            codeText.setFillColor(seatTxt);
            codeText.setString(s.maGhe);
            sf::FloatRect cb = codeText.getLocalBounds();
            codeText.setPosition(
                s.position.x + (s.size.x - cb.width) / 2.f - cb.left,
                s.position.y + (s.size.y - cb.height) / 2.f - cb.top
            );
            window.draw(codeText);
        }

        // Legend at bottom of seat card
        float legY = seatCardY + seatCardH - 46.f;
        float legX = leftColX + 40.f;
        std::vector<std::tuple<std::string, sf::Color, sf::Color>> legends = {
            {"Ghế Thường", sf::Color::White, sf::Color(203, 213, 225)},
            {"Ghế VIP", sf::Color(254, 243, 199), sf::Color(245, 158, 11)},
            {"Sweetbox", sf::Color(252, 231, 243), sf::Color(236, 72, 153)},
            {"Đang Chọn", sf::Color(34, 197, 94), sf::Color(22, 163, 74)},
            {"Đã Đặt", sf::Color(226, 232, 240), sf::Color(203, 213, 225)}
        };

        for (const auto& leg : legends) {
            RoundedRectRenderer::render(
                window,
                sf::Vector2f(legX, legY),
                sf::Vector2f(20.f, 20.f),
                4.f,
                std::get<1>(leg),
                1.f,
                std::get<2>(leg)
            );

            sf::Text lText;
            lText.setFont(Theme::FontPrimary);
            lText.setCharacterSize(12);
            lText.setFillColor(sf::Color(71, 85, 105));
            lText.setString(toSfString(std::get<0>(leg)));
            lText.setPosition(legX + 28.f, legY + 2.f);
            window.draw(lText);

            legX += 160.f;
        }
    }

    // ==========================================
    // 4. RIGHT COLUMN: CART & CHECKOUT PANEL
    // ==========================================
    float rightColX = sidebarWidth + 1076.f;
    float rightColW = 394.f;

    // Card 1: Ghế đang chọn
    float c1Y = topbarHeight + 14.f;
    float c1H = 246.f;
    RoundedRectRenderer::render(
        window,
        sf::Vector2f(rightColX, c1Y),
        sf::Vector2f(rightColW, c1H),
        8.f,
        sf::Color::White,
        1.f,
        sf::Color(226, 232, 240)
    );

    sf::Text t1;
    t1.setFont(Theme::FontPrimary);
    t1.setCharacterSize(14);
    t1.setStyle(sf::Text::Bold);
    t1.setFillColor(sf::Color(2, 132, 199)); // Sky blue #0284c7
    t1.setString(toSfString("Ghế đang chọn"));
    t1.setPosition(rightColX + 16.f, c1Y + 14.f);
    window.draw(t1);

    sf::RectangleShape div1(sf::Vector2f(rightColW - 32.f, 1.f));
    div1.setPosition(rightColX + 16.f, c1Y + 38.f);
    div1.setFillColor(sf::Color(241, 245, 249));
    window.draw(div1);

    for (size_t i = 0; i < selectedSeatCodes.size() && i < 6; ++i) {
        sf::Text sItem;
        sItem.setFont(Theme::FontPrimary);
        sItem.setCharacterSize(13);
        sItem.setFillColor(sf::Color(51, 65, 85));
        sItem.setString(toSfString("• Ghế " + selectedSeatCodes[i] + " (VIP / Standard)"));
        sItem.setPosition(rightColX + 16.f, c1Y + 48.f + static_cast<float>(i) * 24.f);
        window.draw(sItem);
    }

    sf::Text seatSubtotal;
    seatSubtotal.setFont(Theme::FontPrimary);
    seatSubtotal.setCharacterSize(14);
    seatSubtotal.setStyle(sf::Text::Bold);
    seatSubtotal.setFillColor(sf::Color(2, 132, 199));
    seatSubtotal.setString(toSfString("Tiền ghế: " + formatVND(posSeatTotal)));
    sf::FloatRect ssb = seatSubtotal.getLocalBounds();
    seatSubtotal.setPosition(rightColX + rightColW - 16.f - ssb.width, c1Y + c1H - 28.f);
    window.draw(seatSubtotal);

    // Card 2: F&B đã chọn
    float c2Y = topbarHeight + 270.f;
    float c2H = 236.f;
    RoundedRectRenderer::render(
        window,
        sf::Vector2f(rightColX, c2Y),
        sf::Vector2f(rightColW, c2H),
        8.f,
        sf::Color::White,
        1.f,
        sf::Color(226, 232, 240)
    );

    sf::Text t2;
    t2.setFont(Theme::FontPrimary);
    t2.setCharacterSize(14);
    t2.setStyle(sf::Text::Bold);
    t2.setFillColor(sf::Color(217, 119, 6)); // Amber #d97706
    t2.setString(toSfString("F&B đã chọn"));
    t2.setPosition(rightColX + 16.f, c2Y + 14.f);
    window.draw(t2);

    sf::RectangleShape div2(sf::Vector2f(rightColW - 32.f, 1.f));
    div2.setPosition(rightColX + 16.f, c2Y + 38.f);
    div2.setFillColor(sf::Color(241, 245, 249));
    window.draw(div2);

    for (size_t i = 0; i < posSelectedFnb.size() && i < 4; ++i) {
        sf::Text fItem;
        fItem.setFont(Theme::FontPrimary);
        fItem.setCharacterSize(13);
        fItem.setFillColor(sf::Color(51, 65, 85));
        fItem.setString(toSfString("• " + posSelectedFnb[i].first));
        fItem.setPosition(rightColX + 16.f, c2Y + 48.f + static_cast<float>(i) * 24.f);
        window.draw(fItem);
    }

    if (btnPosAddFnb) btnPosAddFnb->draw(window);

    sf::Text fnbSubtotal;
    fnbSubtotal.setFont(Theme::FontPrimary);
    fnbSubtotal.setCharacterSize(14);
    fnbSubtotal.setStyle(sf::Text::Bold);
    fnbSubtotal.setFillColor(sf::Color(217, 119, 6));
    fnbSubtotal.setString(toSfString("Tiền F&B: " + formatVND(posFnbTotal)));
    sf::FloatRect fsb = fnbSubtotal.getLocalBounds();
    fnbSubtotal.setPosition(rightColX + rightColW - 16.f - fsb.width, c2Y + c2H - 28.f);
    window.draw(fnbSubtotal);

    // Card 3: TỔNG CỘNG & Actions
    float c3Y = topbarHeight + 516.f;
    float c3H = 200.f;
    RoundedRectRenderer::render(
        window,
        sf::Vector2f(rightColX, c3Y),
        sf::Vector2f(rightColW, c3H),
        8.f,
        sf::Color::White,
        1.f,
        sf::Color(226, 232, 240)
    );

    sf::Text totTitle;
    totTitle.setFont(Theme::FontPrimary);
    totTitle.setCharacterSize(16);
    totTitle.setStyle(sf::Text::Bold);
    totTitle.setFillColor(sf::Color(15, 23, 42));
    totTitle.setString(toSfString("TỔNG CỘNG"));
    totTitle.setPosition(rightColX + 16.f, c3Y + 16.f);
    window.draw(totTitle);

    sf::Text totVal;
    totVal.setFont(Theme::FontPrimary);
    totVal.setCharacterSize(22);
    totVal.setStyle(sf::Text::Bold);
    totVal.setFillColor(sf::Color(220, 38, 38)); // Red #dc2626
    totVal.setString(toSfString(formatVND(posGrandTotal)));
    sf::FloatRect tvb = totVal.getLocalBounds();
    totVal.setPosition(rightColX + rightColW - 16.f - tvb.width, c3Y + 12.f);
    window.draw(totVal);

    if (btnPosCheckout) btnPosCheckout->draw(window);
    if (btnPosCancel) btnPosCancel->draw(window);
}

} // namespace UI