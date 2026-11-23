#include "../../include/UI/CinemaUIApp.h"
#include "../../include/ActivityLog.h"
#include <fstream>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <cmath>
#include <ctime>
#include <map>
#include <set>
#include <atomic>
#include <chrono>
#include <sqlite3.h>

namespace UI {

static std::string getTodayDateStr() {
    std::time_t t = std::time(nullptr);
    char buf[32];
    std::strftime(buf, sizeof(buf), "%d/%m/%Y", std::localtime(&t));
    return std::string(buf);
}

static std::string formatVND(double amount) {
    long long val = static_cast<long long>(std::round(amount));
    if (val < 0) val = 0;
    return formatNumberDots(val) + " VNĐ";
}

static std::string formatShortPrice(double amount) {
    long long val = static_cast<long long>(std::round(amount));
    if (val < 0) val = 0;
    return formatNumberDots(val) + "đ";
}

static double parseDigitsToDouble(const std::string& str) {
    std::string digits;
    for (char c : str) {
        if (c >= '0' && c <= '9') digits.push_back(c);
    }
    if (digits.empty()) return 0.0;
    try {
        return std::stod(digits);
    } catch (...) {
        return 0.0;
    }
}

static std::string getRealTimeTodayStr() {
    std::time_t tNow = std::time(nullptr);
    std::tm tmNow;
#ifdef _WIN32
    localtime_s(&tmNow, &tNow);
#else
    localtime_r(&tNow, &tmNow);
#endif
    char buf[32];
    std::strftime(buf, sizeof(buf), "%d/%m/%Y", &tmNow);
    return std::string(buf);
}

static std::string getDayOfWeekNameVN(const std::string& dmy) {
    int d = 0, m = 0, y = 0;
    if (std::sscanf(dmy.c_str(), "%d/%d/%d", &d, &m, &y) == 3 && d > 0 && m > 0 && y > 1900) {
        std::tm timeIn = {};
        timeIn.tm_year = y - 1900;
        timeIn.tm_mon = m - 1;
        timeIn.tm_mday = d;
        timeIn.tm_hour = 12;
        std::mktime(&timeIn);
        const char* wdays[] = {
            "Chủ Nhật", "Thứ Hai", "Thứ Ba", "Thứ Tư", "Thứ Năm", "Thứ Sáu", "Thứ Bảy"
        };
        int w = (timeIn.tm_wday >= 0 && timeIn.tm_wday < 7) ? timeIn.tm_wday : 0;
        return wdays[w];
    }
    return "Thứ Bảy";
}

// Parse DD/MM/YYYY and return short pill label e.g. "Hôm nay, 22/11" or "Chủ Nhật, 22/11"
static std::string formatDatePillLabel(const std::string& dmy, int indexInList) {
    (void)indexInList;
    int d = 0, m = 0, y = 0;
    if (std::sscanf(dmy.c_str(), "%d/%d/%d", &d, &m, &y) == 3 && d > 0 && m > 0 && y > 1900) {
        char dmBuf[16];
        std::snprintf(dmBuf, sizeof(dmBuf), "%02d/%02d", d, m);
        std::string todayReal = getRealTimeTodayStr();
        if (dmy == todayReal) {
            return std::string("Hôm nay, ") + dmBuf;
        }
        std::string wName = getDayOfWeekNameVN(dmy);
        return wName + ", " + dmBuf;
    }
    return dmy;
}

int CinemaUIApp::getPosRemainingSeats(const std::string& maSuatChieu) const {
    if (maSuatChieu.empty()) return 0;
    std::string roomId = "";
    for (const auto& st : showtimeList) {
        if (st.getMaSuatChieu() == maSuatChieu) {
            roomId = st.getMaPhong();
            break;
        }
    }
    if (roomId.empty()) roomId = "R01";

    int totalSeatsInRoom = 0;
    for (const auto& s : allSeats) {
        if (s.getMaPhong() == roomId) {
            bool typeActive = true;
            for (const auto& st : seatTypeList) {
                if (st.tenLoai == s.getLoaiGhe()) {
                    typeActive = st.kichHoatBan;
                    break;
                }
            }
            if (typeActive) totalSeatsInRoom++;
        }
    }
    if (totalSeatsInRoom == 0) {
        for (const auto& rm : roomList) {
            if (rm.getMaPhong() == roomId) {
                totalSeatsInRoom = rm.getSucChua();
                break;
            }
        }
    }
    if (totalSeatsInRoom <= 0) totalSeatsInRoom = 64;

    std::set<std::string> bookedSet;
    sqlite3* dbPtr = nullptr;
    if (sqlite3_open("DATA/cinema.db", &dbPtr) == SQLITE_OK) {
        std::string q1 = "SELECT maGhe FROM tickets WHERE maSuatChieu='" + maSuatChieu + "';";
        sqlite3_stmt* stmt = nullptr;
        if (sqlite3_prepare_v2(dbPtr, q1.c_str(), -1, &stmt, nullptr) == SQLITE_OK) {
            while (sqlite3_step(stmt) == SQLITE_ROW) {
                const char* g = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
                if (g && g[0] != '\0') bookedSet.insert(g);
            }
            sqlite3_finalize(stmt);
        }

        std::string q2 = "SELECT maGhe FROM seat_status WHERE maSuatChieu='" + maSuatChieu + "' AND trangThai='DaDat';";
        if (sqlite3_prepare_v2(dbPtr, q2.c_str(), -1, &stmt, nullptr) == SQLITE_OK) {
            while (sqlite3_step(stmt) == SQLITE_ROW) {
                const char* g = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
                if (g && g[0] != '\0') bookedSet.insert(g);
            }
            sqlite3_finalize(stmt);
        }
        sqlite3_close(dbPtr);
    }

    int rem = totalSeatsInRoom - static_cast<int>(bookedSet.size());
    if (rem < 0) rem = 0;
    return rem;
}

void CinemaUIApp::recalculatePosTotals() {
    posSeatTotal = 0.0;
    for (const auto& code : selectedSeatCodes) {
        for (const auto& s : currentSeats) {
            if (s.maGhe == code) {
                posSeatTotal += s.donGia;
                break;
            }
        }
    }

    posFnbTotal = 0.0;
    for (const auto& item : posSelectedFnb) {
        posFnbTotal += item.second;
    }

    double subTotal = posSeatTotal + posFnbTotal;
    posDiscountAmount = 0.0;

    if (posVoucherApplied && !posVoucherCode.empty() && subTotal > 0.0) {
        std::string upperCode = posVoucherCode;
        std::transform(upperCode.begin(), upperCode.end(), upperCode.begin(), ::toupper);

        bool matchedSql = false;
        // Check in-memory promoList first
        for (const auto& pr : promoList) {
            std::string c = pr.getMaCode();
            std::string id = pr.getMaKhuyenMai();
            std::transform(c.begin(), c.end(), c.begin(), ::toupper);
            std::transform(id.begin(), id.end(), id.begin(), ::toupper);
            if (upperCode == c || upperCode == id) {
                double rawDisc = subTotal * (pr.getPhanTramGiam() / 100.0);
                if (pr.getMucGiamToiDa() > 0.0 && rawDisc > pr.getMucGiamToiDa()) {
                    rawDisc = pr.getMucGiamToiDa();
                }
                posDiscountAmount = rawDisc;
                matchedSql = true;
                break;
            }
        }

        if (!matchedSql) {
            sqlite3* dbPtr = nullptr;
            if (sqlite3_open("DATA/cinema.db", &dbPtr) == SQLITE_OK) {
                std::string q = "SELECT phanTramGiam, mucGiamToiDa, maCode, maKhuyenMai FROM promotions;";
                sqlite3_stmt* stmt = nullptr;
                if (sqlite3_prepare_v2(dbPtr, q.c_str(), -1, &stmt, nullptr) == SQLITE_OK) {
                    while (sqlite3_step(stmt) == SQLITE_ROW) {
                        double pct = sqlite3_column_double(stmt, 0);
                        double maxDisc = sqlite3_column_double(stmt, 1);
                        const char* cStr = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
                        const char* idStr = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
                        std::string codeDb = cStr ? cStr : "";
                        std::string idDb = idStr ? idStr : "";
                        std::transform(codeDb.begin(), codeDb.end(), codeDb.begin(), ::toupper);
                        std::transform(idDb.begin(), idDb.end(), idDb.begin(), ::toupper);

                        if (upperCode == codeDb || upperCode == idDb) {
                            double rawDisc = subTotal * (pct / 100.0);
                            if (maxDisc > 0.0 && rawDisc > maxDisc) rawDisc = maxDisc;
                            posDiscountAmount = rawDisc;
                            matchedSql = true;
                            break;
                        }
                    }
                    sqlite3_finalize(stmt);
                }
                sqlite3_close(dbPtr);
            }
        }

        if (!matchedSql) {
            posDiscountAmount = 0.0;
            posVoucherApplied = false;
        }
        if (posDiscountAmount > subTotal) posDiscountAmount = subTotal;
    } else if (!posMemberName.empty() && posDiscountAmount <= 0.0 && !posVoucherApplied) {
        posDiscountAmount = 0.0;
    }

    posGrandTotal = subTotal - posDiscountAmount;
    if (posGrandTotal < 0.0) posGrandTotal = 0.0;

    if (!posCustomCashEdited) {
        if (posGrandTotal <= 0.0) {
            posCashGivenAmount = 0.0;
        } else {
            // Round up to a realistic banknote step (e.g., 50.000 or 100.000)
            long long gt = static_cast<long long>(std::round(posGrandTotal));
            long long rounded = ((gt + 49999) / 50000) * 50000;
            if (rounded == gt && gt > 0) rounded += 50000;
            if (gt == 245000) rounded = 300000;
            posCashGivenAmount = static_cast<double>(rounded);
        }
        if (inputPosCashGiven) {
            inputPosCashGiven->setText(formatNumberDots(static_cast<long long>(std::round(posCashGivenAmount))));
        }
    } else if (inputPosCashGiven) {
        posCashGivenAmount = parseDigitsToDouble(inputPosCashGiven->getText());
    }
}

void CinemaUIApp::refreshPosShowtimesDropdown() {
    std::string todayReal = getRealTimeTodayStr();

    // 1. Build posAvailableDates: today and next 6 days (7 days of week)
    posAvailableDates.clear();
    int curD = 1, curM = 1, curY = 2026;
    if (std::sscanf(todayReal.c_str(), "%d/%d/%d", &curD, &curM, &curY) == 3) {
        std::tm baseTm = {};
        baseTm.tm_year = curY - 1900;
        baseTm.tm_mon = curM - 1;
        baseTm.tm_mday = curD;
        baseTm.tm_hour = 12;
        std::time_t baseTime = std::mktime(&baseTm);

        for (int dayOff = 0; dayOff < 7; ++dayOff) {
            std::time_t dayT = baseTime + static_cast<std::time_t>(dayOff * 86400);
            std::tm dayTm;
#ifdef _WIN32
            localtime_s(&dayTm, &dayT);
#else
            localtime_r(&dayT, &dayTm);
#endif
            char b[32];
            std::snprintf(b, sizeof(b), "%02d/%02d/%04d", dayTm.tm_mday, dayTm.tm_mon + 1, dayTm.tm_year + 1900);
            posAvailableDates.push_back(std::string(b));
        }
    }

    // Also include any dates in showtimeList that have showtimes
    for (const auto& st : showtimeList) {
        if (!st.getNgayChieu().empty() &&
            std::find(posAvailableDates.begin(), posAvailableDates.end(), st.getNgayChieu()) == posAvailableDates.end()) {
            posAvailableDates.push_back(st.getNgayChieu());
        }
    }

    auto isRoomActive = [this](const std::string& rId) -> bool {
        for (const auto& rm : roomList) {
            if (rm.getMaPhong() == rId) {
                return (rm.getTrangThaiPhong() != "TamNgung");
            }
        }
        return true;
    };

    // Determine initial selected date: default to today if it has showtimes, or first date that has showtimes
    if (posSelectedDateStr.empty() || std::find(posAvailableDates.begin(), posAvailableDates.end(), posSelectedDateStr) == posAvailableDates.end()) {
        bool todayHasSt = false;
        for (const auto& st : showtimeList) {
            if (st.getNgayChieu() == todayReal && isRoomActive(st.getMaPhong())) {
                todayHasSt = true;
                break;
            }
        }
        if (todayHasSt) {
            posSelectedDateStr = todayReal;
        } else {
            std::string dateWithSt = "";
            for (const auto& dStr : posAvailableDates) {
                for (const auto& st : showtimeList) {
                    if (st.getNgayChieu() == dStr && isRoomActive(st.getMaPhong())) {
                        dateWithSt = dStr;
                        break;
                    }
                }
                if (!dateWithSt.empty()) break;
            }
            posSelectedDateStr = !dateWithSt.empty() ? dateWithSt : (!posAvailableDates.empty() ? posAvailableDates[0] : todayReal);
        }
    }

    // 2. Filter posAvailableMovieIds: ONLY movies having active showtimes on posSelectedDateStr
    posAvailableMovieIds.clear();
    for (const auto& m : movieList) {
        bool hasOnDate = false;
        for (const auto& st : showtimeList) {
            if (st.getMaPhim() == m.getMaPhim() && st.getNgayChieu() == posSelectedDateStr && isRoomActive(st.getMaPhong())) {
                hasOnDate = true;
                break;
            }
        }
        if (hasOnDate) {
            posAvailableMovieIds.push_back(m.getMaPhim());
        }
    }

    if (posAvailableMovieIds.empty()) {
        posSelectedMovieId = "";
    } else {
        if (posSelectedMovieId.empty() ||
            std::find(posAvailableMovieIds.begin(), posAvailableMovieIds.end(), posSelectedMovieId) == posAvailableMovieIds.end()) {
            posSelectedMovieId = posAvailableMovieIds[0];
        }
    }

    // 3. Build posFilteredShowtimeIds: ONLY showtimes of posSelectedMovieId on posSelectedDateStr
    posFilteredShowtimeIds.clear();
    if (!posSelectedDateStr.empty() && !posSelectedMovieId.empty()) {
        for (const auto& st : showtimeList) {
            if (!isRoomActive(st.getMaPhong())) continue;
            if (st.getNgayChieu() == posSelectedDateStr && st.getMaPhim() == posSelectedMovieId) {
                posFilteredShowtimeIds.push_back(st.getMaSuatChieu());
            }
        }
    }

    if (posFilteredShowtimeIds.empty()) {
        currentShowtimeId = "";
    } else {
        bool foundCurrent = false;
        for (const auto& id : posFilteredShowtimeIds) {
            if (id == currentShowtimeId) {
                foundCurrent = true;
                break;
            }
        }
        if (!foundCurrent) {
            currentShowtimeId = posFilteredShowtimeIds[0];
        }
    }
}

void CinemaUIApp::setupSeatBookingView() {
    float leftColX = sidebarWidth + 14.f; // 264.f
    float leftColW = 1000.f;
    float rightStartX = leftColX + leftColW + 14.f; // 1278.f
    float rightW = 436.f;

    posShowSeatMap = true;
    posDateScrollOffset = 0;
    posMovieScrollOffset = 0;
    posShowtimeScrollOffset = 0;
    posPaymentMethod = 0; // 0: Tiền mặt
    posCustomCashEdited = false;

    posSelectedDateStr = "";
    posSelectedMovieId = "";
    currentShowtimeId = "";
    posPromoDropdownOpen = false;
    posPromoScrollOffset = 0;

    refreshPosShowtimesDropdown();

    // 1. Top Header Action Buttons ("Làm mới lịch chiếu" & "Đổi ca / Thoát")
    btnPosRefresh = std::make_unique<RoundRectButton>(
        "Làm mới lịch chiếu",
        sf::Vector2f(1728.f - 266.f, 12.f),
        sf::Vector2f(138.f, 38.f),
        ButtonStyle::Primary,
        6.f
    );
    btnPosRefresh->setFontSize(11);
    btnPosRefresh->setCustomColors(
        sf::Color(2, 132, 199),
        sf::Color(3, 105, 161),
        sf::Color(7, 89, 133),
        sf::Color(148, 163, 184)
    );
    btnPosRefresh->setOnClick([this]() {
        loadDataFromDatabase();
        refreshPosShowtimesDropdown();
        refreshSeatsForShowtime(currentShowtimeId);
        if (confirmPopup) {
            confirmPopup->show("ĐỒNG BỘ LỊCH CHIẾU", "Đã cập nhật lịch chiếu, danh sách phim và trạng thái ghế mới nhất từ SQLite!", ButtonStyle::Primary, "Đóng", "");
        }
    });

    btnPosLogout = std::make_unique<RoundRectButton>(
        "Đổi ca / Thoát",
        sf::Vector2f(1728.f - 120.f, 12.f),
        sf::Vector2f(106.f, 38.f),
        ButtonStyle::Secondary,
        6.f
    );
    btnPosLogout->setFontSize(11);
    btnPosLogout->setCustomColors(
        sf::Color(254, 226, 226),
        sf::Color(254, 202, 202),
        sf::Color(252, 165, 165),
        sf::Color(241, 245, 249)
    );
    btnPosLogout->setTextColor(sf::Color(220, 38, 38));
    btnPosLogout->setOnClick([this]() {
        if (currentUserRole != "Admin" && !shiftIsClosed) {
            if (confirmPopup) {
                confirmPopup->show(
                    "CHƯA CHỐT CA TRỰC",
                    "Bạn chưa thực hiện Khóa sổ & Chốt ca trực!\nVui lòng vào mục 'Chốt ca & báo cáo' để hoàn tất bàn giao ca trước khi đăng xuất.",
                    ButtonStyle::Danger,
                    "Đã hiểu",
                    ""
                );
            }
            return;
        }
        if (isLoggedIn) {
            ActivityLog::ghiNhan(currentUserId, currentUserRole, "DANG_XUAT", "Đổi ca / Đăng xuất khỏi quầy POS");
        }
        isLoggedIn = false;
        if (loginInputUser) loginInputUser->setText("");
        if (loginInputPass) loginInputPass->setText("");
        loginErrorMsg = "";
    });

    btnPosLock = std::make_unique<RoundRectButton>("", sf::Vector2f(-1000.f, -1000.f), sf::Vector2f(1.f, 1.f), ButtonStyle::Secondary);
    btnPosDateToday = std::make_unique<RoundRectButton>("", sf::Vector2f(-1000.f, -1000.f), sf::Vector2f(1.f, 1.f), ButtonStyle::Primary);
    btnPosDateTomorrow = std::make_unique<RoundRectButton>("", sf::Vector2f(-1000.f, -1000.f), sf::Vector2f(1.f, 1.f), ButtonStyle::Secondary);
    btnPosMovie1 = std::make_unique<RoundRectButton>("", sf::Vector2f(-1000.f, -1000.f), sf::Vector2f(1.f, 1.f), ButtonStyle::Secondary);
    btnPosMovie2 = std::make_unique<RoundRectButton>("", sf::Vector2f(-1000.f, -1000.f), sf::Vector2f(1.f, 1.f), ButtonStyle::Primary);
    btnPosShowtime = std::make_unique<RoundRectButton>("", sf::Vector2f(-1000.f, -1000.f), sf::Vector2f(1.f, 1.f), ButtonStyle::Primary);
    btnPosSeatRefresh = std::make_unique<RoundRectButton>("", sf::Vector2f(-1000.f, -1000.f), sf::Vector2f(1.f, 1.f), ButtonStyle::Secondary);

    // "Xóa tất cả" button in Card 1 (GHẾ ĐANG CHỌN)
    btnPosSeatClear = std::make_unique<RoundRectButton>(
        "Xóa tất cả",
        sf::Vector2f(rightStartX + rightW - 88.f, 82.f),
        sf::Vector2f(74.f, 24.f),
        ButtonStyle::Secondary,
        4.f
    );
    btnPosSeatClear->setFontSize(10);
    btnPosSeatClear->setCustomColors(
        sf::Color(255, 255, 255),
        sf::Color(254, 242, 242),
        sf::Color(254, 226, 226),
        sf::Color(255, 255, 255)
    );
    btnPosSeatClear->setTextColor(sf::Color(239, 68, 68));
    btnPosSeatClear->setOnClick([this]() {
        for (auto& s : currentSeats) {
            s.isSelected = false;
        }
        selectedSeatCodes.clear();
        recalculatePosTotals();
    });

    // 2. Card 2: F&B Controls ("+ Thêm bắp/nước" & floating F&B Dropdown)
    std::vector<std::string> fnbOptions;
    if (fnbList.empty()) {
        db.docDanhSachFAndB(fnbList);
    }
    for (const auto& item : fnbList) {
        if (item.getTrangThaiKinhDoanh() == "NgungBan") continue;
        std::string label = item.getTenMatHang() + " (" + formatShortPrice(item.getGiaTien()) + ")";
        fnbOptions.push_back(label);
    }
    if (fnbOptions.empty()) {
        fnbOptions = {
            "Combo My Sweet (1 Bắp ngọt + 2 Coke L) (85.000đ)",
            "Combo Solo (1 Bắp + 1 Nước) (70.000đ)",
            "Combo 2 (1 Bắp lớn + 2 Nước) (75.000đ)",
            "Bắp ngọt (45.000đ)",
            "Bắp Phô Mai (45.000đ)",
            "Bắp Caramel (45.000đ)"
        };
    }

    posFnbDropdown = std::make_unique<DropdownBox>(
        sf::Vector2f(rightStartX + 14.f, 254.f),
        sf::Vector2f(rightW - 28.f, 30.f),
        fnbOptions,
        0
    );
    posFnbDropdown->setLightStyle(true);
    posFnbDropdown->setFontSize(11);
    posFnbDropdown->setVisible(true);
    posFnbDropdown->setOnSelect([this](int sel, const std::string&) {
        if (!fnbList.empty() && sel >= 0 && sel < static_cast<int>(fnbList.size())) {
            posSelectedFnb.push_back({fnbList[sel].getTenMatHang(), fnbList[sel].getGiaTien()});
        } else {
            posSelectedFnb.push_back({"Combo My Sweet (1 Bắp ngọt + 2 Coke L)", 85000.0});
        }
        recalculatePosTotals();
    });

    btnPosAddFnb = std::make_unique<RoundRectButton>(
        "+ Thêm bắp/nước",
        sf::Vector2f(rightStartX + rightW - 140.f, 256.f),
        sf::Vector2f(126.f, 26.f),
        ButtonStyle::Primary,
        6.f
    );
    btnPosAddFnb->setFontSize(10);
    btnPosAddFnb->setCustomColors(
        sf::Color(245, 158, 11),
        sf::Color(217, 119, 6),
        sf::Color(180, 83, 9),
        sf::Color(203, 213, 225)
    );
    btnPosAddFnb->setOnClick([this]() {
        if (posFnbDropdown) {
            posFnbDropdown->setExpanded(!posFnbDropdown->getExpanded());
        }
    });

    btnPosAddQuickCombo = std::make_unique<RoundRectButton>("", sf::Vector2f(-1000.f, -1000.f), sf::Vector2f(1.f, 1.f), ButtonStyle::Secondary);
    btnPosClearFnb = std::make_unique<RoundRectButton>("", sf::Vector2f(-1000.f, -1000.f), sf::Vector2f(1.f, 1.f), ButtonStyle::Secondary);

    // 3. Card 3: Customer Loyalty & Promotion Voucher
    inputPosMemberPhone = std::make_unique<UIInputBox>(
        sf::Vector2f(rightStartX + 14.f, 438.f),
        sf::Vector2f(324.f, 32.f),
        "Nhập SĐT hoặc Mã thẻ thành viên...",
        11
    );
    inputPosMemberPhone->setText("");
    posMemberId = "";
    posMemberName = "";
    posMemberTier = "";
    posMemberPoints = 0;

    btnPosApplyMember = std::make_unique<RoundRectButton>(
        "Tra cứu",
        sf::Vector2f(rightStartX + 344.f, 438.f),
        sf::Vector2f(78.f, 32.f),
        ButtonStyle::Secondary,
        6.f
    );
    btnPosApplyMember->setFontSize(11);
    btnPosApplyMember->setCustomColors(
        sf::Color(240, 249, 255),
        sf::Color(224, 242, 254),
        sf::Color(186, 230, 253),
        sf::Color(241, 245, 249)
    );
    btnPosApplyMember->setTextColor(sf::Color(2, 132, 199));
    btnPosApplyMember->setOnClick([this]() {
        std::string query = inputPosMemberPhone ? inputPosMemberPhone->getText() : "";
        if (query.empty()) {
            posMemberId = "";
            posMemberName = "";
            posMemberTier = "";
            posMemberPoints = 0;
            recalculatePosTotals();
            if (confirmPopup) confirmPopup->show("TRA CỨU THÀNH VIÊN", "Vui lòng nhập Số điện thoại hoặc Mã khách hàng để tra cứu!", ButtonStyle::Primary, "Đóng", "");
            return;
        }

        sqlite3* dbPtr = nullptr;
        bool found = false;
        if (sqlite3_open("DATA/cinema.db", &dbPtr) == SQLITE_OK) {
            std::string sql = "SELECT u.maNguoiDung, u.hoTen, c.hangThanhVien, c.diemTichLuy "
                              "FROM users u INNER JOIN customers c ON u.maNguoiDung = c.maKhachHang "
                              "WHERE (u.vaiTro = 'Customer' OR u.vaiTro IS NULL) AND "
                              "(u.soDienThoai = '" + query + "' OR u.soDienThoai LIKE '" + query + "%' OR u.maNguoiDung = '" + query + "' OR c.maKhachHang = '" + query + "') LIMIT 1;";
            sqlite3_stmt* stmt = nullptr;
            if (sqlite3_prepare_v2(dbPtr, sql.c_str(), -1, &stmt, nullptr) == SQLITE_OK) {
                if (sqlite3_step(stmt) == SQLITE_ROW) {
                    const char* uid = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
                    const char* name = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
                    const char* tier = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
                    int pts = sqlite3_column_int(stmt, 3);
                    posMemberId = uid ? uid : "";
                    posMemberName = name ? name : "Khách thành viên";
                    posMemberTier = tier ? tier : "Thường";
                    posMemberPoints = pts;
                    found = true;
                }
                sqlite3_finalize(stmt);
            }
            sqlite3_close(dbPtr);
        }

        if (!found) {
            posMemberId = "";
            posMemberName = "";
            posMemberTier = "";
            posMemberPoints = 0;
            if (confirmPopup) {
                confirmPopup->show("KHÔNG TÌM THẤY", "Không tìm thấy khách hàng thành viên với SĐT / Mã thẻ: " + query, ButtonStyle::Primary, "Đóng", "");
            }
        }
        recalculatePosTotals();
    });

    // Voucher Input & Button
    inputPosVoucher = std::make_unique<UIInputBox>(
        sf::Vector2f(rightStartX + 14.f, 530.f),
        sf::Vector2f(304.f, 32.f),
        "Nhập hoặc tìm mã khuyến mãi...",
        11
    );
    inputPosVoucher->setText("");
    posVoucherCode = "";
    posVoucherApplied = false;

    btnPosApplyVoucher = std::make_unique<RoundRectButton>(
        "Áp dụng",
        sf::Vector2f(rightStartX + 324.f, 530.f),
        sf::Vector2f(98.f, 32.f),
        ButtonStyle::Primary,
        6.f
    );
    btnPosApplyVoucher->setFontSize(11);
    btnPosApplyVoucher->setCustomColors(
        sf::Color(2, 132, 199),
        sf::Color(3, 105, 161),
        sf::Color(7, 89, 133),
        sf::Color(148, 163, 184)
    );
    btnPosApplyVoucher->setOnClick([this]() {
        std::string vCode = inputPosVoucher ? inputPosVoucher->getText() : "";
        // Trim whitespace
        vCode.erase(0, vCode.find_first_not_of(" \t\r\n"));
        if (!vCode.empty()) vCode.erase(vCode.find_last_not_of(" \t\r\n") + 1);

        if (vCode.empty()) {
            posVoucherApplied = false;
            posVoucherCode = "";
            if (btnPosApplyVoucher) {
                btnPosApplyVoucher->setText("Áp dụng");
                btnPosApplyVoucher->setCustomColors(sf::Color(2, 132, 199), sf::Color(3, 105, 161), sf::Color(7, 89, 133), sf::Color(148, 163, 184));
            }
            recalculatePosTotals();
            return;
        }

        std::string upperV = vCode;
        std::transform(upperV.begin(), upperV.end(), upperV.begin(), ::toupper);

        bool validInDb = false;
        // Check in-memory promoList
        for (const auto& pr : promoList) {
            std::string c = pr.getMaCode();
            std::string id = pr.getMaKhuyenMai();
            std::transform(c.begin(), c.end(), c.begin(), ::toupper);
            std::transform(id.begin(), id.end(), id.begin(), ::toupper);
            if (upperV == c || upperV == id) {
                validInDb = true;
                upperV = pr.getMaCode();
                break;
            }
        }

        bool isExpired = false;
        std::string expDate = "";
        if (!validInDb) {
            sqlite3* dbPtr = nullptr;
            if (sqlite3_open("DATA/cinema.db", &dbPtr) == SQLITE_OK) {
                std::string q = "SELECT maCode, maKhuyenMai, ngayBatDau, ngayKetThuc FROM promotions;";
                sqlite3_stmt* stmt = nullptr;
                if (sqlite3_prepare_v2(dbPtr, q.c_str(), -1, &stmt, nullptr) == SQLITE_OK) {
                    std::string todayNorm = DateFormat::normalizeDate(getTodayDateStr());
                    while (sqlite3_step(stmt) == SQLITE_ROW) {
                        const char* c1 = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
                        const char* c2 = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
                        const char* cStart = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
                        const char* cEnd = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
                        std::string s1 = c1 ? c1 : "";
                        std::string s2 = c2 ? c2 : "";
                        std::string dStart = cStart ? cStart : "";
                        std::string dEnd = cEnd ? cEnd : "";
                        std::transform(s1.begin(), s1.end(), s1.begin(), ::toupper);
                        std::transform(s2.begin(), s2.end(), s2.begin(), ::toupper);
                        if (upperV == s1 || upperV == s2) {
                            validInDb = true;
                            if (!s1.empty()) upperV = s1;
                            expDate = dEnd;
                            std::string startNorm = DateFormat::normalizeDate(dStart);
                            std::string endNorm = DateFormat::normalizeDate(dEnd);
                            if (!startNorm.empty() && !endNorm.empty()) {
                                if (todayNorm < startNorm || todayNorm > endNorm) {
                                    isExpired = true;
                                }
                            }
                            break;
                        }
                    }
                    sqlite3_finalize(stmt);
                }
                sqlite3_close(dbPtr);
            }
        }

        if (validInDb && !isExpired) {
            posVoucherCode = upperV;
            posVoucherApplied = true;
            if (inputPosVoucher) inputPosVoucher->setText(upperV);
            if (btnPosApplyVoucher) {
                btnPosApplyVoucher->setText("Đã áp dụng");
                btnPosApplyVoucher->setCustomColors(sf::Color(5, 150, 105), sf::Color(4, 120, 87), sf::Color(6, 95, 70), sf::Color(148, 163, 184));
            }
            posPromoDropdownOpen = false;
            recalculatePosTotals();
        } else if (validInDb && isExpired) {
            posVoucherApplied = false;
            posVoucherCode = "";
            if (btnPosApplyVoucher) {
                btnPosApplyVoucher->setText("Áp dụng");
                btnPosApplyVoucher->setCustomColors(sf::Color(2, 132, 199), sf::Color(3, 105, 161), sf::Color(7, 89, 133), sf::Color(148, 163, 184));
            }
            recalculatePosTotals();
            if (confirmPopup) {
                confirmPopup->show("MÃ KHUYẾN MÃI HẾT HẠN", "Mã khuyến mãi '" + vCode + "' đã hết hạn áp dụng (Hạn cuối: " + expDate + ")!\nVui lòng chọn mã khuyến mãi khác.", ButtonStyle::Danger, "Đóng", "");
            }
        } else {
            posVoucherApplied = false;
            posVoucherCode = "";
            if (btnPosApplyVoucher) {
                btnPosApplyVoucher->setText("Áp dụng");
                btnPosApplyVoucher->setCustomColors(sf::Color(2, 132, 199), sf::Color(3, 105, 161), sf::Color(7, 89, 133), sf::Color(148, 163, 184));
            }
            recalculatePosTotals();
            if (confirmPopup) {
                confirmPopup->show("MÃ KHÔNG HỢP LỆ", "Mã khuyến mãi '" + vCode + "' không tồn tại trong hệ thống!\nVui lòng nhấp vào ô tìm kiếm để chọn mã hợp lệ từ danh sách.", ButtonStyle::Primary, "Đóng", "");
            }
        }
    });

    // 4. Card 4: Payment & Cash Input & Checkout Buttons
    inputPosCashGiven = std::make_unique<UIInputBox>(
        sf::Vector2f(rightStartX + 14.f, 704.f),
        sf::Vector2f(198.f, 34.f),
        "Nhập tiền khách đưa...",
        13
    );

    btnPosCheckout = std::make_unique<RoundRectButton>(
        "THANH TOÁN & IN VÉ (F5)",
        sf::Vector2f(rightStartX + 14.f, 858.f),
        sf::Vector2f(rightW - 28.f, 44.f),
        ButtonStyle::Primary,
        8.f
    );
    btnPosCheckout->setFontSize(13);
    btnPosCheckout->setCustomColors(
        sf::Color(5, 150, 105),
        sf::Color(4, 120, 87),
        sf::Color(6, 95, 70),
        sf::Color(148, 163, 184)
    );
    btnPosCheckout->setOnClick([this]() {
        if (selectedSeatCodes.empty() && posSelectedFnb.empty()) {
            if (confirmPopup) {
                confirmPopup->show("CHƯA CHỌN VÉ / DỊCH VỤ", "Vui lòng chọn ít nhất 1 ghế trên sơ đồ hoặc món F&B trước khi thanh toán!", ButtonStyle::Primary, "Đóng", "");
            }
            return;
        }

        if (posPaymentMethod == 0) { // CASH
            if (posCashGivenAmount < posGrandTotal) {
                if (confirmPopup) {
                    confirmPopup->show(
                        "TIỀN KHÁCH ĐƯA KHÔNG ĐỦ",
                        "Số tiền khách đưa (" + formatVND(posCashGivenAmount) + ") không đủ để thanh toán đơn hàng (" + formatVND(posGrandTotal) + ")!\nVui lòng nhập đúng số tiền khách đưa trước khi thanh toán.",
                        ButtonStyle::Danger,
                        "Đóng",
                        ""
                    );
                }
                return;
            }
        }

        std::string payMethodStr = (posPaymentMethod == 0) ? "CASH" : ((posPaymentMethod == 1) ? "QR_PAY" : "CARD");
        std::string payMethodVN = (posPaymentMethod == 0) ? "Tiền mặt" : ((posPaymentMethod == 1) ? "Quét QR" : "Thẻ POS");

        std::string seatListStr = "";
        for (size_t i = 0; i < selectedSeatCodes.size(); ++i) {
            seatListStr += selectedSeatCodes[i];
            if (i + 1 < selectedSeatCodes.size()) seatListStr += ", ";
        }

        if (confirmPopup) {
            std::string popupTitle = "XÁC NHẬN THANH TOÁN TIỀN MẶT & IN VÉ";
            std::string popupBody = "Suất chiếu: " + currentShowtimeId +
                "\nGhế đã chọn (" + std::to_string(selectedSeatCodes.size()) + " vé): " + (seatListStr.empty() ? "Không" : seatListStr) +
                "\nPhương thức: Tiền mặt" +
                "\nTổng đơn hàng: " + formatVND(posGrandTotal) +
                "\nTiền khách đưa: " + formatVND(posCashGivenAmount) +
                "\nTiền thối lại: " + formatVND(std::max(0.0, posCashGivenAmount - posGrandTotal));
            std::string confirmBtnText = "Xác nhận & In vé";

            if (posPaymentMethod == 1) {
                popupTitle = "XÁC NHẬN THANH TOÁN QUÉT MÃ QR";
                popupBody = "Yêu cầu khách quét mã QR để chuyển khoản:\n"
                            "• Số tiền thanh toán: " + formatVND(posGrandTotal) + "\n"
                            "• Cổng thanh toán: VietQR / MoMo POS\n"
                            "• Suất chiếu: " + currentShowtimeId + " (" + std::to_string(selectedSeatCodes.size()) + " vé)\n\n"
                            "Chỉ nhấn 'Xác nhận Đã Nhận Tiền' sau khi hệ thống cổng thanh toán báo Giao dịch thành công.";
                confirmBtnText = "Xác nhận Đã Nhận Tiền";
            } else if (posPaymentMethod == 2) {
                popupTitle = "XÁC NHẬN THANH TOÁN THẺ POS";
                popupBody = "Quẹt hoặc cắm thẻ qua thiết bị POS thanh toán:\n"
                            "• Số tiền thanh toán: " + formatVND(posGrandTotal) + "\n"
                            "• Thiết bị: Máy POS quầy " + (currentPosTerminal.empty() ? "01" : currentPosTerminal) + "\n"
                            "• Suất chiếu: " + currentShowtimeId + " (" + std::to_string(selectedSeatCodes.size()) + " vé)\n\n"
                            "Chỉ nhấn 'Xác nhận Đã Nhận Tiền' sau khi máy POS in hóa đơn cà thẻ thành công.";
                confirmBtnText = "Xác nhận Đã Nhận Tiền";
            }

            confirmPopup->show(
                popupTitle,
                popupBody,
                ButtonStyle::Primary,
                confirmBtnText,
                "Quay lại"
            );
            confirmPopup->setOnConfirm([this, payMethodStr]() {
                sqlite3* dbConn = nullptr;
                static std::atomic<unsigned long long> s_invSeq{100};
                auto nowMs = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
                std::string invId = "INV-" + std::to_string(nowMs % 10000000) + "-" + std::to_string(s_invSeq++);

                if (sqlite3_open("DATA/cinema.db", &dbConn) == SQLITE_OK) {
                    sqlite3_exec(dbConn, "BEGIN IMMEDIATE TRANSACTION;", nullptr, nullptr, nullptr);

                    // Re-verify Double Booking in SQLite directly inside the active transaction
                    bool conflict = false;
                    std::vector<std::string> conflictSeats;
                    if (!selectedSeatCodes.empty()) {
                        for (const auto& code : selectedSeatCodes) {
                            std::string chkSql = "SELECT trangThai FROM seat_status WHERE maSuatChieu = '" + currentShowtimeId + "' AND maGhe = '" + code + "';";
                            sqlite3_stmt* chkStmt = nullptr;
                            if (sqlite3_prepare_v2(dbConn, chkSql.c_str(), -1, &chkStmt, nullptr) == SQLITE_OK) {
                                if (sqlite3_step(chkStmt) == SQLITE_ROW) {
                                    const char* st = reinterpret_cast<const char*>(sqlite3_column_text(chkStmt, 0));
                                    if (st && std::string(st) == "DaDat") {
                                        conflict = true;
                                        conflictSeats.push_back(code);
                                    }
                                }
                                sqlite3_finalize(chkStmt);
                            }
                        }
                        if (conflict) {
                            sqlite3_exec(dbConn, "ROLLBACK;", nullptr, nullptr, nullptr);
                            sqlite3_close(dbConn);
                            refreshSeatsForShowtime(currentShowtimeId);
                            std::string errSeats = "";
                            for (size_t i = 0; i < conflictSeats.size(); ++i) {
                                errSeats += conflictSeats[i] + (i + 1 < conflictSeats.size() ? ", " : "");
                            }
                            if (confirmPopup) {
                                confirmPopup->show(
                                    "GHẾ ĐÃ ĐƯỢC ĐẶT TRƯỚC",
                                    "Ghế [" + errSeats + "] vừa được quầy khác hoặc khách online thanh toán thành công!\nVui lòng chọn ghế khác.",
                                    ButtonStyle::Danger,
                                    "Đóng",
                                    ""
                                );
                            }
                            return;
                        }
                    }

                    // Look up or assign current active shift ID
                    if (currentShiftId.empty()) {
                        sqlite3_stmt* shStmt = nullptr;
                        std::string qActiveShift = "SELECT maCa FROM shift_managements WHERE (trangThai = 'DangLam' OR trangThai = 'ChoDuyet') AND maNhanVien = '" + currentUserId + "' ORDER BY maCa DESC LIMIT 1;";
                        if (sqlite3_prepare_v2(dbConn, qActiveShift.c_str(), -1, &shStmt, nullptr) == SQLITE_OK) {
                            if (sqlite3_step(shStmt) == SQLITE_ROW) {
                                const char* cCa = reinterpret_cast<const char*>(sqlite3_column_text(shStmt, 0));
                                if (cCa) currentShiftId = cCa;
                            }
                            sqlite3_finalize(shStmt);
                        }
                        if (currentShiftId.empty()) currentShiftId = "CA09";
                    }

                    std::time_t tNow = std::time(nullptr);
                    char tBuf[32];
                    std::strftime(tBuf, sizeof(tBuf), "%d/%m/%Y %H:%M:%S", std::localtime(&tNow));

                    std::string qInv = "INSERT INTO invoices (maHoaDon, maKhachHang, maNhanVien, thoiGianLap, tongTien, hinhThucThanhToan, maGiaoDich, trangThaiThanhToan, maCa) VALUES ('"
                        + invId + "', '"
                        + (posMemberId.empty() ? "KHACH_LE" : posMemberId) + "', '"
                        + currentUserId + "', '"
                        + std::string(tBuf) + "', '"
                        + std::to_string(static_cast<long long>(std::round(posGrandTotal))) + "', '" + payMethodStr + "', 'POS-" + invId.substr(4) + "', 'DaThanhToan', '" + currentShiftId + "');";
                    sqlite3_exec(dbConn, qInv.c_str(), nullptr, nullptr, nullptr);

                    double subTotal = posSeatTotal + posFnbTotal;
                    double discountFactor = (subTotal > 0.0) ? (posGrandTotal / subTotal) : 1.0;
                    long long accumulatedItemTotal = 0;

                    for (size_t i = 0; i < selectedSeatCodes.size(); ++i) {
                        const auto& code = selectedSeatCodes[i];
                        double originalTicketPrice = 75000.0;
                        for (const auto& s : currentSeats) {
                            if (s.maGhe == code) {
                                originalTicketPrice = s.donGia;
                                break;
                            }
                        }
                        long long ticketPrice = static_cast<long long>(std::round(originalTicketPrice * discountFactor));
                        if (i + 1 == selectedSeatCodes.size() && posSelectedFnb.empty()) {
                            long long expectedGrand = static_cast<long long>(std::round(posGrandTotal));
                            if (accumulatedItemTotal + ticketPrice != expectedGrand) {
                                ticketPrice = expectedGrand - accumulatedItemTotal;
                            }
                        }
                        accumulatedItemTotal += ticketPrice;

                        std::string tId = "T-" + invId.substr(4) + "-" + code;
                        std::string qTick = "INSERT INTO tickets (maVe, maHoaDon, maSuatChieu, maGhe, giaVeThucTe, maQRCode, trangThaiSuDung) VALUES ('"
                            + tId + "', '"
                            + invId + "', '"
                            + currentShowtimeId + "', '"
                            + code + "', '" + std::to_string(ticketPrice) + "', 'QR-" + code + "', 'ChuaCheckIn');";
                        sqlite3_exec(dbConn, qTick.c_str(), nullptr, nullptr, nullptr);

                        std::string qStat = "INSERT OR REPLACE INTO seat_status (maSuatChieu, maGhe, trangThai) VALUES ('"
                            + currentShowtimeId + "', '" + code + "', 'DaDat');";
                        sqlite3_exec(dbConn, qStat.c_str(), nullptr, nullptr, nullptr);
                    }

                    for (size_t i = 0; i < posSelectedFnb.size(); ++i) {
                        const auto& f = posSelectedFnb[i];
                        std::string realFbId = "FB_POS";
                        for (const auto& fbItem : fnbList) {
                            if (fbItem.getTenMatHang() == f.first) {
                                realFbId = fbItem.getMaMatHang();
                                break;
                            }
                        }
                        long long fnbPrice = static_cast<long long>(std::round(f.second * discountFactor));
                        if (i + 1 == posSelectedFnb.size()) {
                            long long expectedGrand = static_cast<long long>(std::round(posGrandTotal));
                            if (accumulatedItemTotal + fnbPrice != expectedGrand) {
                                fnbPrice = expectedGrand - accumulatedItemTotal;
                            }
                        }
                        accumulatedItemTotal += fnbPrice;

                        std::string qFb = "INSERT INTO invoice_fb_items (maHoaDon, maMatHang, tenMatHangTaiThoiDiem, soLuong, donGiaTaiThoiDiem, thanhTien) VALUES ('"
                            + invId + "', '" + realFbId + "', '" + f.first + "', '1', '" + std::to_string(fnbPrice) + "', '" + std::to_string(fnbPrice) + "');";
                        sqlite3_exec(dbConn, qFb.c_str(), nullptr, nullptr, nullptr);
                    }

                    // Update customer loyalty points and total spending in SQLite if member was looked up
                    if (!posMemberId.empty()) {
                        int addedPts = static_cast<int>(std::round(posGrandTotal / 10000.0));
                        if (addedPts > 0) {
                            posMemberPoints += addedPts;
                            std::string qUpd = "UPDATE customers SET diemTichLuy = CAST(CAST(diemTichLuy AS INTEGER) + " + std::to_string(addedPts) + " AS TEXT), "
                                               "tongChiTieu = CAST(CAST(tongChiTieu AS INTEGER) + " + std::to_string(static_cast<long long>(std::round(posGrandTotal))) + " AS TEXT) "
                                               "WHERE maKhachHang = '" + posMemberId + "';";
                            sqlite3_exec(dbConn, qUpd.c_str(), nullptr, nullptr, nullptr);

                            std::string gdId = "GD-" + std::to_string(nowMs % 10000000) + "-" + std::to_string(s_invSeq++);
                            std::string qHist = "INSERT INTO customer_points_history (maGiaoDichDiem, maKhachHang, loaiGiaoDich, soDiem, noiDung, thoiGian) VALUES ('"
                                + gdId + "', '" + posMemberId + "', 'TichDiem', '" + std::to_string(addedPts) + "', 'Tích điểm từ hóa đơn " + invId + "', '" + std::string(tBuf) + "');";
                            sqlite3_exec(dbConn, qHist.c_str(), nullptr, nullptr, nullptr);
                        }
                    }

                    sqlite3_exec(dbConn, "COMMIT;", nullptr, nullptr, nullptr);
                    sqlite3_close(dbConn);
                }

                // Mark visual seats as booked
                for (auto& s : currentSeats) {
                    for (const auto& c : selectedSeatCodes) {
                        if (s.maGhe == c) {
                            s.isSelected = false;
                            s.trangThai = "DaDat";
                            break;
                        }
                    }
                }

                ActivityLog::ghiNhan(currentUserId, currentUserRole, "XUAT_VE_POS", "Bán " + std::to_string(selectedSeatCodes.size()) + " vé (" + invId + "), tổng: " + formatVND(posGrandTotal));

                selectedSeatCodes.clear();
                posSelectedFnb.clear();
                posCustomCashEdited = false;
                posVoucherApplied = false;
                posVoucherCode = "";
                if (inputPosVoucher) inputPosVoucher->setText("");
                if (btnPosApplyVoucher) {
                    btnPosApplyVoucher->setText("Áp dụng");
                    btnPosApplyVoucher->setCustomColors(sf::Color(2, 132, 199), sf::Color(3, 105, 161), sf::Color(7, 89, 133), sf::Color(148, 163, 184));
                }
                posMemberId = "";
                posMemberName = "";
                posMemberTier = "";
                posMemberPoints = 0;
                if (inputPosMemberPhone) inputPosMemberPhone->setText("");
                recalculatePosTotals();

                if (confirmPopup) {
                    confirmPopup->show(
                        "THANH TOÁN & IN VÉ THÀNH CÔNG",
                        "Hóa đơn " + invId + " đã được lưu vào hệ thống SQLite!\nVé xem phim và hóa đơn F&B đã được in tại quầy.",
                        ButtonStyle::Primary,
                        "Hoàn tất",
                        ""
                    );
                }
            });
        }
    });

    btnPosPrintProforma = std::make_unique<RoundRectButton>(
        "In vé thử / Giữ ghế",
        sf::Vector2f(rightStartX + 14.f, 910.f),
        sf::Vector2f(198.f, 34.f),
        ButtonStyle::Secondary,
        6.f
    );
    btnPosPrintProforma->setFontSize(11);
    btnPosPrintProforma->setOnClick([this]() {
        std::ofstream f("DATA/phieu_tam_tinh.txt");
        if (f.is_open()) {
            f << "==========================================\n";
            f << "   BÁN VÉ - POS (QUẦY NHÂN VIÊN)          \n";
            f << "   PHIẾU IN VÉ THỬ / GIỮ GHẾ TẠM TÍNH    \n";
            f << "==========================================\n";
            f << "Suất chiếu: " << currentShowtimeId << "\n";
            f << "Số vé chọn: " << selectedSeatCodes.size() << " vé\n";
            for (const auto& c : selectedSeatCodes) f << "  - Ghế: " << c << "\n";
            f << "Tiền vé ghế: " << formatVND(posSeatTotal) << "\n";
            f << "Tiền F&B: " << formatVND(posFnbTotal) << "\n";
            if (posDiscountAmount > 0) f << "Ưu đãi Voucher (" << posVoucherCode << "): -" << formatVND(posDiscountAmount) << "\n";
            f << "TỔNG CỘNG: " << formatVND(posGrandTotal) << "\n";
            f.close();
            if (confirmPopup) {
                confirmPopup->show("IN VÉ THỬ & GIỮ GHẾ", "Đã xuất phiếu in vé thử và giữ chỗ tạm thời ra file DATA/phieu_tam_tinh.txt!", ButtonStyle::Primary, "Đóng", "");
            }
        }
    });

    btnPosCancel = std::make_unique<RoundRectButton>(
        "Hủy đơn (Esc)",
        sf::Vector2f(rightStartX + 224.f, 910.f),
        sf::Vector2f(198.f, 34.f),
        ButtonStyle::Secondary,
        6.f
    );
    btnPosCancel->setFontSize(11);
    btnPosCancel->setCustomColors(
        sf::Color(254, 226, 226),
        sf::Color(254, 202, 202),
        sf::Color(252, 165, 165),
        sf::Color(241, 245, 249)
    );
    btnPosCancel->setTextColor(sf::Color(220, 38, 38));
    btnPosCancel->setOnClick([this]() {
        for (auto& s : currentSeats) s.isSelected = false;
        selectedSeatCodes.clear();
        posSelectedFnb.clear();
        posCustomCashEdited = false;
        posVoucherApplied = false;
        posVoucherCode = "";
        if (inputPosVoucher) inputPosVoucher->setText("");
        if (btnPosApplyVoucher) {
            btnPosApplyVoucher->setText("Áp dụng");
            btnPosApplyVoucher->setCustomColors(sf::Color(2, 132, 199), sf::Color(3, 105, 161), sf::Color(7, 89, 133), sf::Color(148, 163, 184));
        }
        posMemberId = "";
        posMemberName = "";
        posMemberTier = "";
        posMemberPoints = 0;
        if (inputPosMemberPhone) inputPosMemberPhone->setText("");
        recalculatePosTotals();
    });

    refreshSeatsForShowtime(currentShowtimeId);
    recalculatePosTotals();
}

void CinemaUIApp::refreshSeatsForShowtime(const std::string& maSuatChieu) {
    currentSeats.clear();
    selectedSeatCodes.clear();

    if (maSuatChieu.empty()) {
        recalculatePosTotals();
        return;
    }

    // Determine target room, showtime date and base price from showtimeList
    std::string roomId = "";
    std::string stDateStr = posSelectedDateStr;
    double basePrice = 75000.0;
    for (const auto& st : showtimeList) {
        if (st.getMaSuatChieu() == maSuatChieu) {
            roomId = st.getMaPhong();
            if (!st.getNgayChieu().empty()) stDateStr = st.getNgayChieu();
            if (st.getGiaVeCoBan() > 0.0) basePrice = st.getGiaVeCoBan();
            break;
        }
    }
    if (roomId.empty()) roomId = "R01";
    std::string dowName = getDayOfWeekNameVN(stDateStr);
    bool isWeekendShow = (dowName == "Thứ Sáu" || dowName == "Thứ Bảy" || dowName == "Chủ Nhật");

    double stdPrice = basePrice > 0.0 ? basePrice : 75000.0;
    double vipPrice = stdPrice + 15000.0;
    double swPrice = stdPrice * 2.0 + 10000.0;
    for (const auto& st : seatTypeList) {
        if (st.tenLoai == "Standard") stdPrice = st.donGia;
        else if (st.tenLoai == "VIP") vipPrice = st.donGia;
        else if (st.tenLoai == "Sweetbox" || st.tenLoai == "Couple") swPrice = st.donGia;
    }

    // Read booked tickets & seat_status for this showtime from SQLite
    std::set<std::string> bookedSeats;
    sqlite3* dbPtr = nullptr;
    if (sqlite3_open("DATA/cinema.db", &dbPtr) == SQLITE_OK) {
        std::string q1 = "SELECT maGhe FROM tickets WHERE maSuatChieu='" + maSuatChieu + "';";
        sqlite3_stmt* stmt = nullptr;
        if (sqlite3_prepare_v2(dbPtr, q1.c_str(), -1, &stmt, nullptr) == SQLITE_OK) {
            while (sqlite3_step(stmt) == SQLITE_ROW) {
                const char* g = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
                if (g && g[0] != '\0') bookedSeats.insert(g);
            }
            sqlite3_finalize(stmt);
        }
        std::string q2 = "SELECT maGhe FROM seat_status WHERE maSuatChieu='" + maSuatChieu + "' AND trangThai='DaDat';";
        if (sqlite3_prepare_v2(dbPtr, q2.c_str(), -1, &stmt, nullptr) == SQLITE_OK) {
            while (sqlite3_step(stmt) == SQLITE_ROW) {
                const char* g = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
                if (g && g[0] != '\0') bookedSeats.insert(g);
            }
            sqlite3_finalize(stmt);
        }
        sqlite3_close(dbPtr);
    }

    // Collect seats for target roomId from allSeats
    std::vector<Seat> roomSeats;
    for (const auto& s : allSeats) {
        if (s.getMaPhong() == roomId) {
            roomSeats.push_back(s);
        }
    }

    // If allSeats has no seats for this room, query SQLite table seats
    if (roomSeats.empty()) {
        if (sqlite3_open("DATA/cinema.db", &dbPtr) == SQLITE_OK) {
            std::string qs = "SELECT maGhe, maPhong, loaiGhe, donGiaGhe FROM seats WHERE maPhong='" + roomId + "';";
            sqlite3_stmt* stmt = nullptr;
            if (sqlite3_prepare_v2(dbPtr, qs.c_str(), -1, &stmt, nullptr) == SQLITE_OK) {
                while (sqlite3_step(stmt) == SQLITE_ROW) {
                    const char* mg = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
                    const char* mp = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
                    const char* lg = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
                    double dg = sqlite3_column_double(stmt, 3);
                    roomSeats.push_back(Seat(mg ? mg : "", mp ? mp : "", lg ? lg : "Standard", dg, "Trong"));
                }
                sqlite3_finalize(stmt);
            }
            sqlite3_close(dbPtr);
        }
    }

    // Structure for seat positioning
    struct RoomParsedSeat {
        std::string maGhe;
        char rowChar;
        int colStart;
        int colEnd;
        std::string loaiGhe;
        double donGia;
        bool isBooked;
    };

    auto parseSeatCode = [](const std::string& rawCode, char& rc, int& c1, int& c2) -> bool {
        std::string code;
        for (char ch : rawCode) {
            if (!std::isspace(static_cast<unsigned char>(ch))) code.push_back(ch);
        }
        if (code.size() < 2 || !std::isalpha(static_cast<unsigned char>(code[0]))) return false;

        size_t alphaLen = 0;
        std::string prefix;
        while (alphaLen < code.size() && std::isalpha(static_cast<unsigned char>(code[alphaLen]))) {
            prefix.push_back(static_cast<char>(std::toupper(static_cast<unsigned char>(code[alphaLen]))));
            ++alphaLen;
        }
        if (alphaLen == 0 || alphaLen >= code.size()) return false;

        if (prefix == "VIP") {
            rc = 'V';
        } else if (prefix == "SW") {
            rc = 'S';
        } else if (prefix.size() == 1) {
            rc = prefix[0];
        } else {
            return false;
        }

        std::string rest = code.substr(alphaLen);
        size_t dashPos = rest.find('-');
        if (dashPos == std::string::npos) {
            int a = 0;
            char extra = 0;
            if (std::sscanf(rest.c_str(), "%d%c", &a, &extra) != 1 || a <= 0) return false;
            c1 = c2 = a;
            return true;
        }

        std::string leftPart = rest.substr(0, dashPos);
        std::string rightPart = rest.substr(dashPos + 1);
        size_t rightAlpha = 0;
        std::string rightPrefix;
        while (rightAlpha < rightPart.size() && std::isalpha(static_cast<unsigned char>(rightPart[rightAlpha]))) {
            rightPrefix.push_back(static_cast<char>(std::toupper(static_cast<unsigned char>(rightPart[rightAlpha]))));
            ++rightAlpha;
        }
        if (!rightPrefix.empty()) {
            if (rightPrefix != prefix) return false;
            rightPart = rightPart.substr(rightAlpha);
        }

        int a = 0, b = 0;
        char extra1 = 0, extra2 = 0;
        if (std::sscanf(leftPart.c_str(), "%d%c", &a, &extra1) != 1 ||
            std::sscanf(rightPart.c_str(), "%d%c", &b, &extra2) != 1 ||
            a <= 0 || b < a) {
            return false;
        }
        c1 = a;
        c2 = b;
        return true;
    };

    std::map<char, std::vector<RoomParsedSeat>> rowsMap;
    for (const auto& s : roomSeats) {
        char rc = 'A';
        int c1 = 1, c2 = 1;
        if (parseSeatCode(s.getMaGhe(), rc, c1, c2)) {
            bool typeActive = true;
            double configuredTypePrice = 0.0;
            for (const auto& st : seatTypeList) {
                if (st.tenLoai == s.getLoaiGhe()) {
                    typeActive = st.kichHoatBan;
                    configuredTypePrice = st.donGia;
                    break;
                }
            }
            // Do not allow selling seat types that have been deactivated in Seat Management
            if (!typeActive) continue;

            bool booked = (bookedSeats.count(s.getMaGhe()) > 0);
            double seatPrice = s.getDonGiaGhe();
            if (configuredTypePrice > 0.0) {
                seatPrice = configuredTypePrice;
            } else if (seatPrice <= 0.0) {
                if (s.getLoaiGhe() == "VIP") seatPrice = vipPrice;
                else if (s.getLoaiGhe() == "Sweetbox" || s.getLoaiGhe() == "Couple") seatPrice = swPrice;
                else seatPrice = stdPrice;
            }
            // Apply +15.000 VND weekend surcharge (Fri, Sat, Sun)
            if (isWeekendShow) {
                seatPrice += 15000.0;
            }
            rowsMap[rc].push_back({s.getMaGhe(), rc, c1, c2, s.getLoaiGhe(), seatPrice, booked});
        }
    }

    // Sort rows ascending: 'A' nearest to screen, followed by 'B', 'C'...
    std::vector<char> rowOrder;
    for (const auto& kv : rowsMap) rowOrder.push_back(kv.first);
    std::sort(rowOrder.begin(), rowOrder.end());

    int maxCols = 1;
    for (auto& kv : rowsMap) {
        std::sort(kv.second.begin(), kv.second.end(), [](const RoomParsedSeat& a, const RoomParsedSeat& b) {
            return a.colStart < b.colStart;
        });
        int count = static_cast<int>(kv.second.size());
        if (count > maxCols) maxCols = count;
    }

    float leftColX = sidebarWidth + 14.f; // 264.f
    float leftColW = 1000.f;
    float startGridY = 430.f;

    int numRows = static_cast<int>(rowOrder.size());
    float seatW = 42.f;
    float seatH = 36.f;
    float gapX = 8.f;
    float gapY = 10.f;

    if (maxCols <= 4) {
        seatW = 60.f; seatH = 46.f; gapX = 14.f; gapY = 16.f;
    } else if (maxCols <= 6) {
        seatW = 52.f; seatH = 42.f; gapX = 12.f; gapY = 14.f;
    } else if (maxCols <= 10) {
        seatW = 44.f; seatH = 36.f; gapX = 8.f; gapY = 10.f;
    } else if (maxCols <= 15) {
        seatW = 36.f; seatH = 30.f; gapX = 6.f; gapY = 7.f;
    } else {
        seatW = 30.f; seatH = 26.f; gapX = 5.f; gapY = 6.f;
    }

    if (numRows > 8) {
        seatH = std::min(seatH, 28.f);
        gapY = std::min(gapY, 6.f);
    }

    for (int r = 0; r < numRows; ++r) {
        char rc = rowOrder[r];
        const auto& rSeats = rowsMap[rc];
        float rowY = startGridY + static_cast<float>(r) * (seatH + gapY);

        float rowTotalW = 0.f;
        for (size_t i = 0; i < rSeats.size(); ++i) {
            bool isCouple = (rSeats[i].colEnd > rSeats[i].colStart || rSeats[i].loaiGhe == "Sweetbox" || rSeats[i].loaiGhe == "Couple");
            float curW = isCouple ? (2.f * seatW + gapX) : seatW;
            rowTotalW += curW;
            if (i + 1 < rSeats.size()) rowTotalW += gapX;
        }

        float rowStartX = leftColX + (leftColW - rowTotalW) / 2.0f;
        float curX = rowStartX;

        for (size_t i = 0; i < rSeats.size(); ++i) {
            const auto& ps = rSeats[i];
            bool isCouple = (ps.colEnd > ps.colStart || ps.loaiGhe == "Sweetbox" || ps.loaiGhe == "Couple");
            float curW = isCouple ? (2.f * seatW + gapX) : seatW;

            VisualSeat vs;
            vs.maGhe = ps.maGhe;
            vs.isSelected = false;
            vs.trangThai = ps.isBooked ? "DaDat" : "Trong";
            vs.loaiGhe = ps.loaiGhe;
            vs.donGia = ps.donGia;
            vs.position = sf::Vector2f(curX, rowY);
            vs.size = sf::Vector2f(curW, seatH);

            currentSeats.push_back(vs);
            curX += curW + gapX;
        }
    }

    recalculatePosTotals();
}

void CinemaUIApp::renderSeatBookingView() {
    float contentAreaW = 1728.f - sidebarWidth;
    float contentAreaH = 972.f;

    if (posCustomCashEdited && inputPosCashGiven) {
        posCashGivenAmount = parseDigitsToDouble(inputPosCashGiven->getText());
    }

    // Background Canvas
    sf::RectangleShape posBg(sf::Vector2f(contentAreaW, contentAreaH));
    posBg.setPosition(sidebarWidth, 0.f);
    posBg.setFillColor(sf::Color(241, 245, 249)); // #f1f5f9
    window.draw(posBg);

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
    // 1. TOP HEADER BAR (White Card Header)
    // ==========================================
    float headerH = 62.f;
    sf::RectangleShape topBar(sf::Vector2f(contentAreaW, headerH));
    topBar.setPosition(sidebarWidth, 0.f);
    topBar.setFillColor(sf::Color::White);
    window.draw(topBar);

    sf::RectangleShape topBarBorder(sf::Vector2f(contentAreaW, 1.f));
    topBarBorder.setPosition(sidebarWidth, headerH - 1.f);
    topBarBorder.setFillColor(sf::Color(226, 232, 240));
    window.draw(topBarBorder);

    // Blue POS Terminal Icon
    float iconX = sidebarWidth + 16.f;
    RoundedRectRenderer::render(window, sf::Vector2f(iconX, 13.f), sf::Vector2f(36.f, 36.f), 8.f, sf::Color(2, 132, 199));
    RoundedRectRenderer::render(window, sf::Vector2f(iconX + 8.f, 20.f), sf::Vector2f(20.f, 14.f), 2.f, sf::Color::White);
    RoundedRectRenderer::render(window, sf::Vector2f(iconX + 10.f, 22.f), sf::Vector2f(16.f, 10.f), 1.f, sf::Color(2, 132, 199));
    sf::RectangleShape standBar(sf::Vector2f(12.f, 3.f));
    standBar.setPosition(iconX + 12.f, 36.f);
    standBar.setFillColor(sf::Color::White);
    window.draw(standBar);

    // Header Title (2 lines matching mockup)
    drawTxt("BÁN VÉ – POS (QUẦY", sidebarWidth + 62.f, 11.f, 14, sf::Color(15, 23, 42), true);
    drawTxt("NHÂN VIÊN)", sidebarWidth + 62.f, 31.f, 14, sf::Color(15, 23, 42), true);

    // Status Pill: HỆ THỐNG TRỰC TUYẾN
    float pillX = sidebarWidth + 246.f;
    RoundedRectRenderer::render(
        window,
        sf::Vector2f(pillX, 13.f),
        sf::Vector2f(154.f, 36.f),
        18.f,
        sf::Color(236, 253, 245),
        1.f,
        sf::Color(167, 243, 208)
    );
    sf::CircleShape greenDot(4.f);
    greenDot.setFillColor(sf::Color(16, 185, 129));
    greenDot.setPosition(pillX + 12.f, 27.f);
    window.draw(greenDot);
    drawTxt("HỆ THỐNG TRỰC", pillX + 26.f, 16.f, 10, sf::Color(5, 150, 105), true);
    drawTxt("TUYẾN", pillX + 26.f, 30.f, 10, sf::Color(5, 150, 105), true);

    // Date Info in Header
    float hDateX = sidebarWidth + 418.f;
    RoundedRectRenderer::render(window, sf::Vector2f(hDateX, 22.f), sf::Vector2f(16.f, 16.f), 3.f, sf::Color(241, 245, 249), 1.f, sf::Color(148, 163, 184));
    sf::RectangleShape calTop(sf::Vector2f(16.f, 4.f));
    calTop.setPosition(hDateX, 22.f);
    calTop.setFillColor(sf::Color(100, 116, 139));
    window.draw(calTop);

    std::string dispHeaderDate = posSelectedDateStr.empty() ? getRealTimeTodayStr() : posSelectedDateStr;
    std::string dispHeaderDay = "";
    if (dispHeaderDate == getRealTimeTodayStr()) {
        dispHeaderDay = "Hôm nay, " + getDayOfWeekNameVN(dispHeaderDate);
    } else {
        dispHeaderDay = getDayOfWeekNameVN(dispHeaderDate);
    }

    drawTxt(dispHeaderDay, hDateX + 24.f, 13.f, 11, sf::Color(71, 85, 105));
    drawTxt(dispHeaderDate, hDateX + 24.f, 31.f, 11, sf::Color(30, 41, 59), true);

    // Vertical divider
    sf::RectangleShape div1(sf::Vector2f(1.f, 30.f));
    div1.setPosition(sidebarWidth + 572.f, 16.f);
    div1.setFillColor(sf::Color(226, 232, 240));
    window.draw(div1);

    // Shift Info
    float shiftX = sidebarWidth + 588.f;
    drawTxt("Ca: Sáng", shiftX, 13.f, 11, sf::Color(217, 119, 6), true);
    drawTxt("(Quầy 01)", shiftX, 31.f, 11, sf::Color(217, 119, 6));

    // Vertical divider
    sf::RectangleShape div2(sf::Vector2f(1.f, 30.f));
    div2.setPosition(sidebarWidth + 672.f, 16.f);
    div2.setFillColor(sf::Color(226, 232, 240));
    window.draw(div2);

    // Cashier Info (Linked to logged-in user in SQLite)
    std::string cashierName = !currentUserName.empty() ? currentUserName : (currentUserId == "U002" ? "Phạm Như Anh Thư" : (currentUserId == "U001" ? "Lê Minh Tuấn" : "Nguyễn Văn A"));
    for (const auto& u : staffUserList) {
        if (u.maNguoiDung == currentUserId && !u.hoTen.empty()) {
            cashierName = u.hoTen;
            break;
        }
    }
    float cashX = sidebarWidth + 688.f;
    drawTxt("Thu ngân:", cashX, 13.f, 11, sf::Color(71, 85, 105));
    drawTxt(cashierName, cashX, 31.f, 11, sf::Color(2, 132, 199), true);

    // Live Digital Clock Box
    std::time_t nowT = std::time(nullptr);
    char clockBuf[16];
    std::strftime(clockBuf, sizeof(clockBuf), "%H : %M : %S", std::localtime(&nowT));
    float clkX = sidebarWidth + 835.f;
    RoundedRectRenderer::render(
        window,
        sf::Vector2f(clkX, 14.f),
        sf::Vector2f(102.f, 34.f),
        6.f,
        sf::Color(240, 249, 255),
        1.f,
        sf::Color(186, 230, 253)
    );
    drawCenteredTxt(clockBuf, clkX + 51.f, 31.f, 12, sf::Color(3, 105, 161), true);

    if (btnPosRefresh) btnPosRefresh->draw(window);
    if (btnPosLogout) btnPosLogout->draw(window);

    // ==========================================
    // 2. LEFT COLUMN: FILTER BAR + SEAT MATRIX
    // ==========================================
    float leftColX = sidebarWidth + 14.f; // 264.f
    float leftColW = 1000.f;
    float rightStartX = leftColX + leftColW + 14.f; // 1278.f
    float rightW = 436.f;

    // Main Left Card (y = 74.f, h = 818.f -> 74.f to 892.f)
    float mainCardY = 74.f;
    float mainCardH = 818.f;
    RoundedRectRenderer::render(
        window,
        sf::Vector2f(leftColX, mainCardY),
        sf::Vector2f(leftColW, mainCardH),
        10.f,
        sf::Color::White,
        1.f,
        sf::Color(226, 232, 240)
    );

    // ------------------------------------------
    // 2.1 ROW 1: NGÀY (y = 86.f, h = 34.f)
    // ------------------------------------------
    float dateRowY = 86.f;
    drawTxt("NGÀY:", leftColX + 18.f, dateRowY + 9.f, 11, sf::Color(100, 116, 139), true);

    int maxVisibleDates = 5;
    int totalDates = static_cast<int>(posAvailableDates.size());
    if (posDateScrollOffset > std::max(0, totalDates - maxVisibleDates)) {
        posDateScrollOffset = std::max(0, totalDates - maxVisibleDates);
    }

    float dateStartX = leftColX + 82.f;
    float dateBtnW = 136.f;
    float dateBtnH = 34.f;
    float dateGap = 10.f;

    for (int i = 0; i < maxVisibleDates && (posDateScrollOffset + i) < totalDates; ++i) {
        int dIdx = posDateScrollOffset + i;
        std::string dStr = posAvailableDates[dIdx];
        bool isSel = (dStr == posSelectedDateStr);

        float bx = dateStartX + static_cast<float>(i) * (dateBtnW + dateGap);
        RoundedRectRenderer::render(
            window,
            sf::Vector2f(bx, dateRowY),
            sf::Vector2f(dateBtnW, dateBtnH),
            6.f,
            isSel ? sf::Color(2, 132, 199) : sf::Color::White,
            1.f,
            isSel ? sf::Color(2, 132, 199) : sf::Color(203, 213, 225)
        );
        std::string pillLabel = formatDatePillLabel(dStr, dIdx);
        drawCenteredTxt(pillLabel, bx + dateBtnW / 2.f, dateRowY + dateBtnH / 2.f, 11, isSel ? sf::Color::White : sf::Color(51, 65, 85), isSel);
    }

    // Optional Scroll Arrows for NGÀY if > 5 dates
    if (totalDates > maxVisibleDates) {
        float arrX = leftColX + leftColW - 68.f;
        RoundedRectRenderer::render(window, sf::Vector2f(arrX, dateRowY + 3.f), sf::Vector2f(24.f, 28.f), 4.f, sf::Color(248, 250, 252), 1.f, sf::Color(203, 213, 225));
        drawCenteredTxt("<", arrX + 12.f, dateRowY + 17.f, 12, sf::Color(71, 85, 105), true);
        RoundedRectRenderer::render(window, sf::Vector2f(arrX + 28.f, dateRowY + 3.f), sf::Vector2f(24.f, 28.f), 4.f, sf::Color(248, 250, 252), 1.f, sf::Color(203, 213, 225));
        drawCenteredTxt(">", arrX + 40.f, dateRowY + 17.f, 12, sf::Color(71, 85, 105), true);
    }

    // ------------------------------------------
    // 2.2 ROW 2: PHIM (y = 130.f, h = 46.f)
    // ------------------------------------------
    float movieRowY = 130.f;
    drawTxt("PHIM:", leftColX + 18.f, movieRowY + 15.f, 11, sf::Color(100, 116, 139), true);

    int maxVisibleMovies = 4;
    int totalMovies = static_cast<int>(posAvailableMovieIds.size());
    if (posMovieScrollOffset > std::max(0, totalMovies - maxVisibleMovies)) {
        posMovieScrollOffset = std::max(0, totalMovies - maxVisibleMovies);
    }

    float movieStartX = leftColX + 82.f;
    float movieCardW = 204.f;
    float movieCardH = 46.f;
    float movieGap = 10.f;

    if (totalMovies == 0) {
        sf::Text noM;
        noM.setFont(Theme::FontPrimary);
        noM.setCharacterSize(11);
        noM.setFillColor(sf::Color(100, 116, 139));
        noM.setString(toSfString("Không có phim nào có suất chiếu trong ngày đã chọn (" + posSelectedDateStr + ")"));
        noM.setPosition(movieStartX, movieRowY + 14.f);
        window.draw(noM);
    }

    for (int i = 0; i < maxVisibleMovies && (posMovieScrollOffset + i) < totalMovies; ++i) {
        int mIdx = posMovieScrollOffset + i;
        std::string mId = posAvailableMovieIds[mIdx];
        bool isSel = (mId == posSelectedMovieId);

        std::string mTitle = mId;
        int mDur = 120;
        int mAge = 13;
        for (const auto& m : movieList) {
            if (m.getMaPhim() == mId) {
                mTitle = m.getTenPhim();
                mDur = m.getThoiLuong();
                mAge = m.getNhanDoTuoi();
                break;
            }
        }

        float mx = movieStartX + static_cast<float>(i) * (movieCardW + movieGap);
        RoundedRectRenderer::render(
            window,
            sf::Vector2f(mx, movieRowY),
            sf::Vector2f(movieCardW, movieCardH),
            8.f,
            isSel ? sf::Color(240, 249, 255) : sf::Color::White,
            isSel ? 2.f : 1.f,
            isSel ? sf::Color(2, 132, 199) : sf::Color(203, 213, 225)
        );

        // Age Badge Sticker (T18 / T16 / T13 / P)
        std::string ageStr = (mAge <= 0) ? "P" : ("T" + std::to_string(mAge));
        sf::Color ageCol = sf::Color(22, 163, 74); // Green P
        if (mAge >= 16) ageCol = sf::Color(220, 38, 38); // Red T16/T18
        else if (mAge >= 13) ageCol = sf::Color(234, 88, 12); // Orange T13

        RoundedRectRenderer::render(window, sf::Vector2f(mx + 8.f, movieRowY + 13.f), sf::Vector2f(28.f, 20.f), 4.f, ageCol);
        drawCenteredTxt(ageStr, mx + 22.f, movieRowY + 23.f, 9, sf::Color::White, true);

        // Duration on the right e.g. "(138')"
        std::string durStr = "(" + std::to_string(mDur) + "')";
        drawRightTxt(durStr, mx + movieCardW - 8.f, movieRowY + 16.f, 10, sf::Color(100, 116, 139));

        // Movie Title in the middle (wrap into 2 lines if > 14 bytes so font size is preserved and never overflows)
        sf::String sfTitle = toSfString(mTitle);
        if (sfTitle.getSize() <= 14) {
            drawCenteredTxt(mTitle, mx + 38.f + (movieCardW - 76.f) / 2.f, movieRowY + 23.f, 11, sf::Color(15, 23, 42), true);
        } else {
            // Split cleanly by word around midpoint
            size_t mid = mTitle.find(' ', mTitle.size() / 2);
            if (mid == std::string::npos) mid = mTitle.find(' ');
            if (mid != std::string::npos) {
                std::string line1 = mTitle.substr(0, mid);
                std::string line2 = mTitle.substr(mid + 1);
                drawCenteredTxt(line1, mx + 38.f + (movieCardW - 76.f) / 2.f, movieRowY + 15.f, 10, sf::Color(15, 23, 42), true);
                drawCenteredTxt(line2, mx + 38.f + (movieCardW - 76.f) / 2.f, movieRowY + 31.f, 10, sf::Color(15, 23, 42), true);
            } else {
                drawCenteredTxt(mTitle, mx + 38.f + (movieCardW - 76.f) / 2.f, movieRowY + 23.f, 10, sf::Color(15, 23, 42), true);
            }
        }
    }

    // Scroll arrows for PHIM if > 4 movies
    if (totalMovies > maxVisibleMovies) {
        float arrX = leftColX + leftColW - 68.f;
        RoundedRectRenderer::render(window, sf::Vector2f(arrX, movieRowY + 9.f), sf::Vector2f(24.f, 28.f), 4.f, sf::Color(248, 250, 252), 1.f, sf::Color(203, 213, 225));
        drawCenteredTxt("<", arrX + 12.f, movieRowY + 23.f, 12, sf::Color(71, 85, 105), true);
        RoundedRectRenderer::render(window, sf::Vector2f(arrX + 28.f, movieRowY + 9.f), sf::Vector2f(24.f, 28.f), 4.f, sf::Color(248, 250, 252), 1.f, sf::Color(203, 213, 225));
        drawCenteredTxt(">", arrX + 40.f, movieRowY + 23.f, 12, sf::Color(71, 85, 105), true);
    }

    // ------------------------------------------
    // 2.3 ROW 3: SUẤT (y = 186.f, h = 54.f)
    // ------------------------------------------
    float stRowY = 186.f;
    drawTxt("SUẤT:", leftColX + 18.f, stRowY + 19.f, 11, sf::Color(100, 116, 139), true);

    float stStartX = leftColX + 82.f;
    float stCardW = 172.f;
    float stCardH = 54.f;
    float stGap = 10.f;

    int maxVisibleSt = 5;
    int totalSt = static_cast<int>(posFilteredShowtimeIds.size());

    if (totalSt == 0) {
        sf::Text noSt(toSfString("⚠️ Phòng chiếu của các suất chiếu phim này đang TẠM NGƯNG BẢO TRÌ! Vui lòng liên hệ Admin để đổi phòng."), Theme::FontPrimary, 11);
        noSt.setFillColor(sf::Color(220, 38, 38));
        noSt.setStyle(sf::Text::Bold);
        noSt.setPosition(stStartX, stRowY + 18.f);
        window.draw(noSt);
    }

    for (int i = 0; i < maxVisibleSt && i < totalSt; ++i) {
        std::string scId = posFilteredShowtimeIds[i];
        bool isSel = (scId == currentShowtimeId);

        std::string gioBD = "09:00";
        std::string maPhong = "R01";
        for (const auto& st : showtimeList) {
            if (st.getMaSuatChieu() == scId) {
                gioBD = st.getGioBatDau();
                maPhong = st.getMaPhong();
                break;
            }
        }

        std::string loaiPhong = "Standard 2D";
        for (const auto& r : roomList) {
            if (r.getMaPhong() == maPhong) {
                if (r.getLoaiPhong() == "2D") loaiPhong = "Standard 2D";
                else if (r.getLoaiPhong() == "IMAX") loaiPhong = "IMAX 3D";
                else if (r.getLoaiPhong() == "3D") loaiPhong = "Standard 3D";
                else loaiPhong = r.getLoaiPhong();
                break;
            }
        }

        int remSeats = getPosRemainingSeats(scId);
        float sx = stStartX + static_cast<float>(i) * (stCardW + stGap);

        RoundedRectRenderer::render(
            window,
            sf::Vector2f(sx, stRowY),
            sf::Vector2f(stCardW, stCardH),
            8.f,
            isSel ? sf::Color(2, 132, 199) : sf::Color::White,
            isSel ? 1.5f : 1.f,
            isSel ? sf::Color(2, 132, 199) : sf::Color(203, 213, 225)
        );

        // Left info inside showtime card
        drawCenteredTxt(gioBD + " - " + maPhong, sx + 58.f, stRowY + 18.f, 11, isSel ? sf::Color::White : sf::Color(15, 23, 42), true);
        drawCenteredTxt("(" + loaiPhong + ")", sx + 58.f, stRowY + 36.f, 10, isSel ? sf::Color(224, 242, 254) : sf::Color(100, 116, 139));

        // Right remaining seats badge inside showtime card
        RoundedRectRenderer::render(
            window,
            sf::Vector2f(sx + 116.f, stRowY + 7.f),
            sf::Vector2f(48.f, 40.f),
            6.f,
            isSel ? sf::Color(3, 105, 161) : sf::Color(241, 245, 249),
            1.f,
            isSel ? sf::Color(7, 89, 133) : sf::Color(226, 232, 240)
        );
        drawCenteredTxt("Còn " + std::to_string(remSeats), sx + 140.f, stRowY + 19.f, 9, isSel ? sf::Color::White : sf::Color(51, 65, 85), true);
        drawCenteredTxt("ghế", sx + 140.f, stRowY + 34.f, 9, isSel ? sf::Color(224, 242, 254) : sf::Color(100, 116, 139));
    }

    // Horizontal Divider below Filters (y = 254.f)
    sf::RectangleShape filterDiv(sf::Vector2f(leftColW - 2.f, 1.f));
    filterDiv.setPosition(leftColX + 1.f, 254.f);
    filterDiv.setFillColor(sf::Color(226, 232, 240));
    window.draw(filterDiv);

    // ------------------------------------------
    // 2.4 CURVED SCREEN ARC & SEAT MATRIX
    // ------------------------------------------
    float scrCenterX = leftColX + leftColW / 2.f; // 764.f
    float screenY = 282.f;

    // Draw smooth curved screen arc with subtle blue glow
    const int arcSegments = 40;
    float arcHalfW = 225.f;
    float arcCurveH = 14.f;
    for (int s = 0; s < arcSegments; ++s) {
        float t1 = static_cast<float>(s) / arcSegments;
        float t2 = static_cast<float>(s + 1) / arcSegments;
        float x1 = scrCenterX - arcHalfW + t1 * (2.f * arcHalfW);
        float x2 = scrCenterX - arcHalfW + t2 * (2.f * arcHalfW);
        float nx1 = (t1 - 0.5f) * 2.f;
        float nx2 = (t2 - 0.5f) * 2.f;
        float y1 = screenY + (nx1 * nx1) * arcCurveH;
        float y2 = screenY + (nx2 * nx2) * arcCurveH;

        // Soft blue glow below arc
        sf::ConvexShape glow;
        glow.setPointCount(4);
        glow.setPoint(0, sf::Vector2f(x1, y1));
        glow.setPoint(1, sf::Vector2f(x2, y2));
        glow.setPoint(2, sf::Vector2f(x2, y2 + 12.f));
        glow.setPoint(3, sf::Vector2f(x1, y1 + 12.f));
        glow.setFillColor(sf::Color(186, 230, 253, 55));
        window.draw(glow);

        // Main blue arc line (3px thick)
        sf::ConvexShape seg;
        seg.setPointCount(4);
        seg.setPoint(0, sf::Vector2f(x1, y1));
        seg.setPoint(1, sf::Vector2f(x2, y2));
        seg.setPoint(2, sf::Vector2f(x2, y2 + 3.5f));
        seg.setPoint(3, sf::Vector2f(x1, y1 + 3.5f));
        seg.setFillColor(sf::Color(2, 132, 199));
        window.draw(seg);
    }

    std::string curRoomCode = "01";
    for (const auto& st : showtimeList) {
        if (st.getMaSuatChieu() == currentShowtimeId) {
            curRoomCode = st.getMaPhong();
            break;
        }
    }
    drawCenteredTxt("MÀN HÌNH CHIẾU (SCREEN " + curRoomCode + ")", scrCenterX, screenY + 26.f, 11, sf::Color(100, 116, 139), true);

    // Draw Row Labels dynamically from currentSeats on Left and Right of Grid
    std::map<char, std::pair<float, float>> rowXBounds; // rowChar -> (minX, maxX)
    std::map<char, float> rowYPositions;
    for (const auto& s : currentSeats) {
        if (!s.maGhe.empty()) {
            char rc = s.maGhe[0];
            if (s.maGhe.rfind("SW", 0) == 0) rc = 'S';
            if (rowXBounds.find(rc) == rowXBounds.end()) {
                rowXBounds[rc] = {s.position.x, s.position.x + s.size.x};
                rowYPositions[rc] = s.position.y + s.size.y / 2.f;
            } else {
                rowXBounds[rc].first = std::min(rowXBounds[rc].first, s.position.x);
                rowXBounds[rc].second = std::max(rowXBounds[rc].second, s.position.x + s.size.x);
            }
        }
    }
    for (const auto& kv : rowXBounds) {
        char rc = kv.first;
        float yMid = rowYPositions[rc];
        std::string lbl(1, rc);
        if (rc == 'S') lbl = "SW";
        sf::Color lblCol = (rc == 'S') ? sf::Color(190, 24, 93) : ((rc >= 'D') ? sf::Color(180, 83, 9) : sf::Color(100, 116, 139));
        drawCenteredTxt(lbl, kv.second.first - 24.f, yMid, 11, lblCol, true);
        drawCenteredTxt(lbl, kv.second.second + 24.f, yMid, 11, lblCol, true);
    }

    // Draw All Seats in Matrix
    for (const auto& s : currentSeats) {
        sf::Color fCol = sf::Color(226, 232, 240);
        sf::Color oCol = sf::Color(203, 213, 225);
        sf::Color tCol = sf::Color(51, 65, 85);

        if (s.trangThai == "DaDat") {
            fCol = sf::Color(241, 245, 249);
            oCol = sf::Color(226, 232, 240);
            tCol = sf::Color(148, 163, 184);
        } else if (s.isSelected) {
            fCol = sf::Color(5, 150, 105); // Emerald Green #059669
            oCol = sf::Color(4, 120, 87);
            tCol = sf::Color::White;
        } else if (s.loaiGhe == "VIP") {
            fCol = sf::Color(254, 249, 195); // Warm cream yellow #fef9c3
            oCol = sf::Color(245, 158, 11);  // Amber border #f59e0b
            tCol = sf::Color(146, 64, 14);   // Dark amber text #92400e
        } else if (s.loaiGhe == "Sweetbox" || s.loaiGhe == "Couple") {
            fCol = sf::Color(252, 231, 243); // Soft pink #fce7f3
            oCol = sf::Color(236, 72, 153);  // Pink border #ec4899
            tCol = sf::Color(190, 24, 93);   // Magenta text #be185d
        } else if (s.loaiGhe != "Standard") {
            fCol = sf::Color(240, 249, 255);
            oCol = sf::Color(14, 165, 233);
            tCol = sf::Color(3, 105, 161);
            for (const auto& st : seatTypeList) {
                if (st.tenLoai == s.loaiGhe) {
                    oCol = st.mauSac;
                    break;
                }
            }
        }

        RoundedRectRenderer::render(
            window,
            s.position,
            s.size,
            6.f,
            fCol,
            1.2f,
            oCol
        );

        std::string dispLabel = s.maGhe;
        if (s.trangThai == "DaDat") {
            dispLabel = (s.loaiGhe == "Sweetbox") ? "X ĐÃ BÁN" : "X";
        }
        drawCenteredTxt(
            dispLabel,
            s.position.x + s.size.x / 2.f,
            s.position.y + s.size.y / 2.f,
            (s.loaiGhe == "Sweetbox" ? 10 : 11),
            tCol,
            true
        );
    }

    // ==========================================
    // 3. BOTTOM LEGEND BAR (Separate Card at y = 902.f)
    // ==========================================
    float legCardX = leftColX + 70.f;
    float legCardW = 860.f;
    float legCardY = 902.f;
    float legCardH = 50.f;
    RoundedRectRenderer::render(
        window,
        sf::Vector2f(legCardX, legCardY),
        sf::Vector2f(legCardW, legCardH),
        10.f,
        sf::Color::White,
        1.f,
        sf::Color(226, 232, 240)
    );

    // Get dynamic prices for legend
    double stdPrice = 75000.0, vipPrice = 90000.0, swPrice = 160000.0;
    for (const auto& s : currentSeats) {
        if (s.loaiGhe == "Standard") stdPrice = s.donGia;
        else if (s.loaiGhe == "VIP") vipPrice = s.donGia;
        else if (s.loaiGhe == "Sweetbox") swPrice = s.donGia;
    }

    float legBoxY = legCardY + 15.f;
    auto drawLegItem = [&](float lx, const std::string& label, const std::string& pricePart, const sf::Color& fCol, const sf::Color& oCol, const sf::Color& txtCol, bool showX = false) {
        RoundedRectRenderer::render(window, sf::Vector2f(lx, legBoxY), sf::Vector2f(20.f, 20.f), 4.f, fCol, 1.2f, oCol);
        if (showX) {
            drawCenteredTxt("X", lx + 10.f, legBoxY + 10.f, 10, sf::Color(148, 163, 184), true);
        }
        drawTxt(label, lx + 28.f, legBoxY + 2.f, 11, txtCol, !pricePart.empty());
    };

    std::string stdK = std::to_string(static_cast<int>(std::round(stdPrice / 1000.0))) + "k";
    std::string vipK = std::to_string(static_cast<int>(std::round(vipPrice / 1000.0))) + "k";
    std::string swK  = std::to_string(static_cast<int>(std::round(swPrice / 1000.0))) + "k";

    drawLegItem(legCardX + 36.f,  "Thường: " + stdK,    stdK, sf::Color(226, 232, 240), sf::Color(203, 213, 225), sf::Color(51, 65, 85));
    drawLegItem(legCardX + 206.f, "VIP: " + vipK,       vipK, sf::Color(254, 249, 195), sf::Color(245, 158, 11),  sf::Color(146, 64, 14));
    drawLegItem(legCardX + 356.f, "Sweetbox: " + swK,   swK,  sf::Color(252, 231, 243), sf::Color(236, 72, 153),  sf::Color(190, 24, 93));
    drawLegItem(legCardX + 546.f, "Đang chọn",          "1",  sf::Color(5, 150, 105),   sf::Color(4, 120, 87),    sf::Color(5, 150, 105));
    drawLegItem(legCardX + 696.f, "Đã bán",             "",   sf::Color(241, 245, 249), sf::Color(203, 213, 225), sf::Color(100, 116, 139), true);

    // ==========================================
    // 4. RIGHT PANEL: CART, F&B, LOYALTY, PAYMENT
    // ==========================================
    // Find current movie short title for seat description
    std::string curMovieShort = "Phim";
    for (const auto& m : movieList) {
        if (m.getMaPhim() == posSelectedMovieId) {
            curMovieShort = m.getTenPhim();
            size_t colonPos = curMovieShort.find(':');
            if (colonPos != std::string::npos) {
                curMovieShort = curMovieShort.substr(0, colonPos);
            }
            break;
        }
    }

    // ------------------------------------------
    // 4.1 CARD 1: GHẾ ĐANG CHỌN (y = 74.f, h = 166.f)
    // ------------------------------------------
    float rc1Y = 74.f;
    float rc1H = 166.f;
    RoundedRectRenderer::render(
        window,
        sf::Vector2f(rightStartX, rc1Y),
        sf::Vector2f(rightW, rc1H),
        10.f,
        sf::Color::White,
        1.f,
        sf::Color(226, 232, 240)
    );

    // Ticket icon + title
    RoundedRectRenderer::render(window, sf::Vector2f(rightStartX + 14.f, rc1Y + 13.f), sf::Vector2f(16.f, 14.f), 3.f, sf::Color(224, 242, 254), 1.f, sf::Color(2, 132, 199));
    drawTxt("GHẾ ĐANG CHỌN (" + std::to_string(selectedSeatCodes.size()) + " VÉ)", rightStartX + 36.f, rc1Y + 12.f, 11, sf::Color(2, 132, 199), true);
    if (btnPosSeatClear) btnPosSeatClear->draw(window);

    int maxSeatPages = std::max(1, (static_cast<int>(selectedSeatCodes.size()) + 1) / 2);
    if (posSeatScrollPage >= maxSeatPages) posSeatScrollPage = maxSeatPages - 1;
    if (posSeatScrollPage < 0) posSeatScrollPage = 0;
    if (maxSeatPages > 1) {
        drawTxt("◀", rightStartX + rightW - 74.f, rc1Y + 12.f, 11, sf::Color(2, 132, 199), true);
        drawTxt(std::to_string(posSeatScrollPage + 1) + "/" + std::to_string(maxSeatPages), rightStartX + rightW - 56.f, rc1Y + 13.f, 10, sf::Color(100, 116, 139), false);
        drawTxt("▶", rightStartX + rightW - 28.f, rc1Y + 12.f, 11, sf::Color(2, 132, 199), true);
    }

    if (!selectedSeatCodes.empty()) {
        size_t startSeatIdx = static_cast<size_t>(posSeatScrollPage * 2);
        size_t endSeatIdx = std::min(startSeatIdx + 2, selectedSeatCodes.size());
        for (size_t i = startSeatIdx; i < endSeatIdx; ++i) {
            float itemY = rc1Y + 38.f + static_cast<float>(i - startSeatIdx) * 44.f;
            std::string code = selectedSeatCodes[i];
            std::string sType = "VIP";
            double sPrice = 90000.0;
            for (const auto& s : currentSeats) {
                if (s.maGhe == code) {
                    sType = s.loaiGhe;
                    sPrice = s.donGia;
                    break;
                }
            }

            RoundedRectRenderer::render(
                window,
                sf::Vector2f(rightStartX + 14.f, itemY),
                sf::Vector2f(rightW - 28.f, 38.f),
                6.f,
                sf::Color(248, 250, 252),
                1.f,
                sf::Color(226, 232, 240)
            );

            // Seat code badge (E5, E6, etc.)
            sf::Color bFill = sf::Color(254, 243, 199);
            sf::Color bOut  = sf::Color(245, 158, 11);
            sf::Color bTxt  = sf::Color(180, 83, 9);
            std::string typeLabelVN = "Ghế VIP";
            if (sType == "Standard") {
                bFill = sf::Color(224, 242, 254);
                bOut  = sf::Color(56, 189, 248);
                bTxt  = sf::Color(3, 105, 161);
                typeLabelVN = "Ghế Thường";
            } else if (sType == "Sweetbox") {
                bFill = sf::Color(252, 231, 243);
                bOut  = sf::Color(244, 114, 182);
                bTxt  = sf::Color(190, 24, 93);
                typeLabelVN = "Ghế Đôi";
            }

            float badgeW = (code.size() > 3) ? 62.f : 36.f;
            RoundedRectRenderer::render(window, sf::Vector2f(rightStartX + 22.f, itemY + 7.f), sf::Vector2f(badgeW, 24.f), 4.f, bFill, 1.f, bOut);
            drawCenteredTxt(code, rightStartX + 22.f + badgeW / 2.f, itemY + 19.f, 10, bTxt, true);

            drawTxt(typeLabelVN + " (" + curMovieShort + ")", rightStartX + 28.f + badgeW, itemY + 11.f, 11, sf::Color(51, 65, 85));
            drawRightTxt(formatVND(sPrice), rightStartX + rightW - 42.f, itemY + 11.f, 11, sf::Color(15, 23, 42), true);
            drawCenteredTxt("×", rightStartX + rightW - 26.f, itemY + 19.f, 14, sf::Color(148, 163, 184), true);
        }
        if (selectedSeatCodes.size() > 2) {
            drawTxt("Trang " + std::to_string(posSeatScrollPage + 1) + "/" + std::to_string(maxSeatPages) + " (" + std::to_string(selectedSeatCodes.size()) + " ghế)", rightStartX + 18.f, rc1Y + 136.f, 10, sf::Color(100, 116, 139));
            drawTxt("[Xóa ghế hiển thị]", rightStartX + 120.f, rc1Y + 136.f, 10, sf::Color(239, 68, 68), true);
        }
    } else {
        RoundedRectRenderer::render(
            window,
            sf::Vector2f(rightStartX + 14.f, rc1Y + 44.f),
            sf::Vector2f(rightW - 28.f, 70.f),
            6.f,
            sf::Color(248, 250, 252),
            1.f,
            sf::Color(226, 232, 240)
        );
        drawCenteredTxt("Chưa chọn ghế nào trên sơ đồ", rightStartX + rightW / 2.f, rc1Y + 79.f, 11, sf::Color(148, 163, 184));
    }

    drawRightTxt("Tiền ghế: " + formatVND(posSeatTotal), rightStartX + rightW - 14.f, rc1Y + 136.f, 12, sf::Color(2, 132, 199), true);

    // ------------------------------------------
    // 4.2 CARD 2: F&B ĐÃ CHỌN (y = 248.f, h = 152.f)
    // ------------------------------------------
    float rc2Y = 248.f;
    float rc2H = 152.f;
    RoundedRectRenderer::render(
        window,
        sf::Vector2f(rightStartX, rc2Y),
        sf::Vector2f(rightW, rc2H),
        10.f,
        sf::Color::White,
        1.f,
        sf::Color(226, 232, 240)
    );

    RoundedRectRenderer::render(window, sf::Vector2f(rightStartX + 14.f, rc2Y + 11.f), sf::Vector2f(16.f, 16.f), 3.f, sf::Color(254, 243, 199), 1.f, sf::Color(245, 158, 11));
    drawTxt("F&B ĐÃ CHỌN", rightStartX + 36.f, rc2Y + 11.f, 11, sf::Color(194, 65, 12), true);
    if (btnPosAddFnb) btnPosAddFnb->draw(window);

    // Aggregate F&B items by name so quantity is displayed cleanly ("1x Combo My Sweet...")
    struct AggFnb {
        std::string name;
        int qty;
        double total;
    };
    std::vector<AggFnb> groupedFnb;
    for (const auto& f : posSelectedFnb) {
        bool found = false;
        for (auto& g : groupedFnb) {
            if (g.name == f.first) {
                g.qty++;
                g.total += f.second;
                found = true;
                break;
            }
        }
        if (!found) {
            groupedFnb.push_back({f.first, 1, f.second});
        }
    }

    int maxFnbPages = std::max(1, (static_cast<int>(groupedFnb.size()) + 1) / 2);
    if (posFnbScrollPage >= maxFnbPages) posFnbScrollPage = maxFnbPages - 1;
    if (posFnbScrollPage < 0) posFnbScrollPage = 0;
    if (maxFnbPages > 1) {
        drawTxt("◀", rightStartX + rightW - 74.f, rc2Y + 11.f, 12, sf::Color(194, 65, 12), true);
        drawTxt(std::to_string(posFnbScrollPage + 1) + "/" + std::to_string(maxFnbPages), rightStartX + rightW - 56.f, rc2Y + 12.f, 10, sf::Color(100, 116, 139), false);
        drawTxt("▶", rightStartX + rightW - 28.f, rc2Y + 11.f, 12, sf::Color(194, 65, 12), true);
    }

    if (!groupedFnb.empty()) {
        size_t startFnbIdx = static_cast<size_t>(posFnbScrollPage * 2);
        size_t endFnbIdx = std::min(startFnbIdx + 2, groupedFnb.size());
        for (size_t i = startFnbIdx; i < endFnbIdx; ++i) {
            float fy = rc2Y + 38.f + static_cast<float>(i - startFnbIdx) * 42.f;
            float boxH = (groupedFnb.size() == 1) ? 54.f : 38.f;
            RoundedRectRenderer::render(
                window,
                sf::Vector2f(rightStartX + 14.f, fy),
                sf::Vector2f(rightW - 28.f, boxH),
                6.f,
                sf::Color(248, 250, 252),
                1.f,
                sf::Color(226, 232, 240)
            );
            std::string titleLine = std::to_string(groupedFnb[i].qty) + "x " + groupedFnb[i].name;
            drawTxt(titleLine, rightStartX + 22.f, fy + 8.f, 11, sf::Color(15, 23, 42), true);
            if (groupedFnb.size() == 1) {
                drawTxt("Bắp ngọt vị phô mai (+10k)", rightStartX + 22.f, fy + 28.f, 10, sf::Color(217, 119, 6));
            }
            drawRightTxt(formatVND(groupedFnb[i].total), rightStartX + rightW - 42.f, fy + (boxH - 16.f) / 2.f, 11, sf::Color(15, 23, 42), true);
            drawCenteredTxt("×", rightStartX + rightW - 26.f, fy + boxH / 2.f, 14, sf::Color(148, 163, 184), true);
        }
        if (groupedFnb.size() > 2) {
            drawTxt("Trang " + std::to_string(posFnbScrollPage + 1) + "/" + std::to_string(maxFnbPages) + " (" + std::to_string(groupedFnb.size()) + " món)", rightStartX + 18.f, rc2Y + 124.f, 10, sf::Color(100, 116, 139));
            drawTxt("[Xóa món hiển thị]", rightStartX + 120.f, rc2Y + 124.f, 10, sf::Color(239, 68, 68), true);
        }
    } else {
        RoundedRectRenderer::render(
            window,
            sf::Vector2f(rightStartX + 14.f, rc2Y + 42.f),
            sf::Vector2f(rightW - 28.f, 54.f),
            6.f,
            sf::Color(248, 250, 252),
            1.f,
            sf::Color(226, 232, 240)
        );
        drawCenteredTxt("Chưa chọn bắp nước / Combo F&B", rightStartX + rightW / 2.f, rc2Y + 69.f, 11, sf::Color(148, 163, 184));
    }

    drawRightTxt("Tiền F&B: " + formatVND(posFnbTotal), rightStartX + rightW - 14.f, rc2Y + 124.f, 12, sf::Color(217, 119, 6), true);

    // ------------------------------------------
    // 4.3 CARD 3: KHÁCH HÀNG & VOUCHER (y = 408.f, h = 166.f)
    // ------------------------------------------
    float rc3Y = 408.f;
    float rc3H = 166.f;
    RoundedRectRenderer::render(
        window,
        sf::Vector2f(rightStartX, rc3Y),
        sf::Vector2f(rightW, rc3H),
        10.f,
        sf::Color::White,
        1.f,
        sf::Color(226, 232, 240)
    );

    drawTxt("Khách hàng thành viên (SĐT / Mã thẻ)", rightStartX + 14.f, rc3Y + 10.f, 10.5, sf::Color(51, 65, 85), true);
    if (inputPosMemberPhone) inputPosMemberPhone->draw(window);
    if (btnPosApplyMember) btnPosApplyMember->draw(window);

    // Member info bar
    RoundedRectRenderer::render(
        window,
        sf::Vector2f(rightStartX + 14.f, rc3Y + 68.f),
        sf::Vector2f(rightW - 28.f, 26.f),
        5.f,
        sf::Color(240, 249, 255),
        1.f,
        sf::Color(186, 230, 253)
    );
    std::string memDisp = posMemberName.empty() ? "Khách lẻ (Chưa tích điểm)" : posMemberName;
    drawTxt("Khách: " + memDisp, rightStartX + 22.f, rc3Y + 74.f, 10.5, sf::Color(15, 23, 42), true);
    drawRightTxt("Điểm: " + formatNumberDots(posMemberPoints) + " đ", rightStartX + rightW - 22.f, rc3Y + 74.f, 10.5, sf::Color(2, 132, 199), true);

    drawTxt("Mã khuyến mãi / Voucher", rightStartX + 14.f, rc3Y + 102.f, 10.5, sf::Color(51, 65, 85), true);
    if (inputPosVoucher) inputPosVoucher->draw(window);
    if (btnPosApplyVoucher) btnPosApplyVoucher->draw(window);

    // ------------------------------------------
    // 4.4 CARD 4: PAYMENT METHOD & CHECKOUT (y = 582.f, h = 374.f)
    // ------------------------------------------
    float rc4Y = 582.f;
    float rc4H = 374.f;
    RoundedRectRenderer::render(
        window,
        sf::Vector2f(rightStartX, rc4Y),
        sf::Vector2f(rightW, rc4H),
        10.f,
        sf::Color::White,
        1.f,
        sf::Color(226, 232, 240)
    );

    drawTxt("PHƯƠNG THỨC THANH TOÁN:", rightStartX + 14.f, rc4Y + 12.f, 10.5, sf::Color(71, 85, 105), true);

    // 3 Payment Method Cards (Tiền mặt, Quét QR, Thẻ POS)
    float pmY = rc4Y + 32.f;
    float pmW = 130.f;
    float pmH = 50.f;
    float pmGap = 9.f;
    const char* pmNames[3] = {"Tiền mặt", "Quét QR", "Thẻ POS"};
    const char* pmIcons[3] = {"[đ]", "[QR]", "[POS]"};

    for (int p = 0; p < 3; ++p) {
        float px = rightStartX + 14.f + static_cast<float>(p) * (pmW + pmGap);
        bool isSel = (posPaymentMethod == p);
        RoundedRectRenderer::render(
            window,
            sf::Vector2f(px, pmY),
            sf::Vector2f(pmW, pmH),
            6.f,
            isSel ? sf::Color(2, 132, 199) : sf::Color::White,
            1.f,
            isSel ? sf::Color(2, 132, 199) : sf::Color(203, 213, 225)
        );
        drawCenteredTxt(pmIcons[p], px + pmW / 2.f, pmY + 15.f, 10, isSel ? sf::Color::White : sf::Color(2, 132, 199), true);
        drawCenteredTxt(pmNames[p], px + pmW / 2.f, pmY + 35.f, 11, isSel ? sf::Color::White : sf::Color(51, 65, 85), true);
    }

    // Cash Given & Change Returned
    drawTxt("Tiền khách đưa:", rightStartX + 14.f, rc4Y + 90.f, 10.5, sf::Color(100, 116, 139));
    drawTxt("Tiền thừa trả khách:", rightStartX + 224.f, rc4Y + 90.f, 10.5, sf::Color(100, 116, 139));

    if (inputPosCashGiven) inputPosCashGiven->draw(window);

    RoundedRectRenderer::render(
        window,
        sf::Vector2f(rightStartX + 224.f, 704.f),
        sf::Vector2f(198.f, 34.f),
        6.f,
        sf::Color(236, 253, 245),
        1.f,
        sf::Color(167, 243, 208)
    );
    double changeAmt = std::max(0.0, posCashGivenAmount - posGrandTotal);
    drawRightTxt(formatVND(changeAmt), rightStartX + 224.f + 188.f, 712.f, 12, sf::Color(5, 150, 105), true);

    // Divider line
    sf::RectangleShape sumDiv(sf::Vector2f(rightW - 28.f, 1.f));
    sumDiv.setPosition(rightStartX + 14.f, 748.f);
    sumDiv.setFillColor(sf::Color(226, 232, 240));
    window.draw(sumDiv);

    // 3 Breakdown Rows
    drawTxt("Tiền vé ghế (" + std::to_string(selectedSeatCodes.size()) + " vé):", rightStartX + 14.f, 756.f, 10.5, sf::Color(71, 85, 105));
    drawRightTxt(formatVND(posSeatTotal), rightStartX + rightW - 14.f, 756.f, 10.5, sf::Color(30, 41, 59));

    drawTxt("Tiền F&B bắp nước:", rightStartX + 14.f, 776.f, 10.5, sf::Color(71, 85, 105));
    drawRightTxt(formatVND(posFnbTotal), rightStartX + rightW - 14.f, 776.f, 10.5, sf::Color(30, 41, 59));

    std::string vLabel = "Ưu đãi Voucher (" + (posVoucherApplied && !posVoucherCode.empty() ? posVoucherCode : "Không") + "):";
    drawTxt(vLabel, rightStartX + 14.f, 796.f, 10.5, sf::Color(5, 150, 105));
    drawRightTxt("-" + formatVND(posDiscountAmount), rightStartX + rightW - 14.f, 796.f, 10.5, sf::Color(5, 150, 105), true);

    // Grand Total Row
    drawTxt("TỔNG CỘNG:", rightStartX + 14.f, 824.f, 13, sf::Color(15, 23, 42), true);
    drawRightTxt(formatVND(posGrandTotal), rightStartX + rightW - 14.f, 818.f, 19, sf::Color(5, 150, 105), true);

    // Action Buttons
    if (btnPosCheckout) btnPosCheckout->draw(window);
    if (btnPosPrintProforma) btnPosPrintProforma->draw(window);
    if (btnPosCancel) btnPosCancel->draw(window);

    // -------------------------------------------------------------
    // Floating Promotion Dropdown Popup (Scrollable Search List)
    // -------------------------------------------------------------
    if (posPromoDropdownOpen) {
        float popX = rightStartX + 14.f;
        float popY = 566.f;
        float popW = 408.f;
        float popH = 224.f;

        // Shadow
        sf::RectangleShape popShadow(sf::Vector2f(popW + 4.f, popH + 4.f));
        popShadow.setPosition(popX - 2.f, popY - 2.f);
        popShadow.setFillColor(sf::Color(15, 23, 42, 35));
        window.draw(popShadow);

        // Card Frame
        RoundedRectRenderer::render(
            window,
            sf::Vector2f(popX, popY),
            sf::Vector2f(popW, popH),
            8.f,
            sf::Color::White,
            1.5f,
            sf::Color(2, 132, 199)
        );

        // Header
        RoundedRectRenderer::render(
            window,
            sf::Vector2f(popX, popY),
            sf::Vector2f(popW, 26.f),
            8.f,
            sf::Color(240, 249, 255)
        );
        sf::RectangleShape hdrPatch(sf::Vector2f(popW, 8.f));
        hdrPatch.setPosition(popX, popY + 18.f);
        hdrPatch.setFillColor(sf::Color(240, 249, 255));
        window.draw(hdrPatch);
        drawTxt("DANH SÁCH MÃ KHUYẾN MÃI (CHỌN ĐỂ ÁP DỤNG)", popX + 12.f, popY + 6.f, 10.f, sf::Color(3, 105, 161), true);

        // Filter promotions from DATA
        std::string query = inputPosVoucher ? inputPosVoucher->getText() : "";
        std::string qUpper = query;
        std::transform(qUpper.begin(), qUpper.end(), qUpper.begin(), ::toupper);
        qUpper.erase(0, qUpper.find_first_not_of(" \t\r\n"));
        if (!qUpper.empty()) qUpper.erase(qUpper.find_last_not_of(" \t\r\n") + 1);

        struct PromoItemDisplay {
            std::string maCode;
            std::string tenKM;
            int phanTram;
            int mucGiam;
            std::string hanDung;
        };
        std::vector<PromoItemDisplay> matchingPromos;

        sqlite3* dbConn = nullptr;
        if (sqlite3_open("DATA/cinema.db", &dbConn) == SQLITE_OK) {
            std::string q = "SELECT maCode, tenKhuyenMai, phanTramGiam, mucGiamToiDa, ngayKetThuc FROM promotions;";
            sqlite3_stmt* stmt = nullptr;
            if (sqlite3_prepare_v2(dbConn, q.c_str(), -1, &stmt, nullptr) == SQLITE_OK) {
                while (sqlite3_step(stmt) == SQLITE_ROW) {
                    const char* cCode = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
                    const char* cTen = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
                    int pt = sqlite3_column_int(stmt, 2);
                    int mg = sqlite3_column_int(stmt, 3);
                    const char* cHan = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4));

                    std::string codeStr = cCode ? cCode : "";
                    std::string tenStr = cTen ? cTen : "";
                    std::string hanStr = cHan ? cHan : "";

                    std::string codeUpper = codeStr;
                    std::string tenUpper = tenStr;
                    std::transform(codeUpper.begin(), codeUpper.end(), codeUpper.begin(), ::toupper);
                    std::transform(tenUpper.begin(), tenUpper.end(), tenUpper.begin(), ::toupper);

                    if (qUpper.empty() || codeUpper.find(qUpper) != std::string::npos || tenUpper.find(qUpper) != std::string::npos) {
                        matchingPromos.push_back({codeStr, tenStr, pt, mg, hanStr});
                    }
                }
                sqlite3_finalize(stmt);
            }
            sqlite3_close(dbConn);
        }

        if (matchingPromos.empty()) {
            drawCenteredTxt("Không tìm thấy mã khuyến mãi phù hợp!", popX + popW / 2.f, popY + 110.f, 11.5f, sf::Color(148, 163, 184));
        } else {
            float itemH = 46.f;
            int maxItems = 4;
            int totalMatching = static_cast<int>(matchingPromos.size());
            if (posPromoScrollOffset > std::max(0, totalMatching - maxItems)) {
                posPromoScrollOffset = std::max(0, totalMatching - maxItems);
            }
            if (posPromoScrollOffset < 0) posPromoScrollOffset = 0;

            sf::Vector2i mPos = sf::Mouse::getPosition(window);
            for (int i = 0; i < maxItems && (posPromoScrollOffset + i) < totalMatching; ++i) {
                const auto& item = matchingPromos[posPromoScrollOffset + i];
                float iy = popY + 28.f + static_cast<float>(i) * itemH;

                bool isHover = (mPos.x >= popX + 6.f && mPos.x <= popX + popW - 6.f && mPos.y >= iy && mPos.y <= iy + itemH - 2.f);

                RoundedRectRenderer::render(
                    window,
                    sf::Vector2f(popX + 6.f, iy + 2.f),
                    sf::Vector2f(popW - 12.f, itemH - 4.f),
                    6.f,
                    isHover ? sf::Color(240, 249, 255) : sf::Color::White,
                    1.f,
                    isHover ? sf::Color(186, 230, 253) : sf::Color(241, 245, 249)
                );

                // Code badge
                RoundedRectRenderer::render(
                    window,
                    sf::Vector2f(popX + 12.f, iy + 6.f),
                    sf::Vector2f(92.f, 20.f),
                    4.f,
                    sf::Color(254, 243, 199),
                    1.f,
                    sf::Color(251, 191, 36)
                );
                drawCenteredTxt(item.maCode, popX + 58.f, iy + 14.f, 10.5f, sf::Color(180, 83, 9), true);

                // Discount tag
                std::string discountTag = "-" + std::to_string(item.phanTram) + "%";
                if (item.mucGiam > 0) discountTag += " (Tối đa " + formatNumberDots(item.mucGiam) + " đ)";
                drawTxt(discountTag, popX + 114.f, iy + 7.f, 11.f, sf::Color(5, 150, 105), true);

                // Title & expiry
                std::string desc = item.tenKM;
                if (!item.hanDung.empty()) desc += " • HSD: " + item.hanDung;
                drawTxt(desc, popX + 12.f, iy + 27.f, 10.f, sf::Color(100, 116, 139));
            }

            // Scroll indicator if > 4 items
            if (totalMatching > maxItems) {
                float scrollAreaH = popH - 36.f;
                float barH = (static_cast<float>(maxItems) / totalMatching) * scrollAreaH;
                float barY = popY + 30.f + (static_cast<float>(posPromoScrollOffset) / (totalMatching - maxItems)) * (scrollAreaH - barH);
                RoundedRectRenderer::render(
                    window,
                    sf::Vector2f(popX + popW - 8.f, barY),
                    sf::Vector2f(4.f, barH),
                    2.f,
                    sf::Color(148, 163, 184)
                );
            }
        }
    }
}

} // namespace UI
