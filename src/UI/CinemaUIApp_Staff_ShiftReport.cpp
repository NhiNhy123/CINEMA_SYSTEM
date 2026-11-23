#include "../../include/UI/CinemaUIApp.h"
#include "../../include/ActivityLog.h"
#include "../../include/ShilftManagement.h"
#include <fstream>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <cmath>
#include <ctime>
#include <set>
#include <sqlite3.h>

namespace UI {

static std::string formatDotNumber(long long val) {
    bool neg = false;
    if (val < 0) {
        neg = true;
        val = -val;
    }
    std::string s = std::to_string(val);
    int n = static_cast<int>(s.length()) - 3;
    while (n > 0) {
        s.insert(n, ".");
        n -= 3;
    }
    return (neg ? "-" : "") + s;
}

static std::string formatVnd(long long val) {
    return formatDotNumber(val) + " VNĐ";
}

static std::string formatDongShort(long long val) {
    return formatDotNumber(val) + " đ";
}

static std::string getTodayDateStr() {
    std::time_t t = std::time(nullptr);
    std::tm tmNow;
#ifdef _WIN32
    localtime_s(&tmNow, &t);
#else
    localtime_r(&t, &tmNow);
#endif
    char buf[32];
    std::strftime(buf, sizeof(buf), "%d/%m/%Y", &tmNow);
    return std::string(buf);
}

static std::string getCurrentTimeStr() {
    std::time_t t = std::time(nullptr);
    std::tm tmNow;
#ifdef _WIN32
    localtime_s(&tmNow, &t);
#else
    localtime_r(&t, &tmNow);
#endif
    char buf[32];
    std::strftime(buf, sizeof(buf), "%H:%M:%S", &tmNow);
    return std::string(buf);
}

struct ShiftRecord {
    std::string maCa;
    std::string maNV;
    std::string tenNV;
    std::string ngayLam;
    std::string gioBatDau;
    std::string gioKetThuc;
    long long doanhThu;
    long long tienThucTe;
    long long chenhLech;
    std::string trangThai;
    bool daKhoa;
};

struct AdminShiftMovieStat {
    std::string title;
    std::string detail;
    std::string revenue;
    std::string share;
    bool highlightShare;
};

struct AdminTopFnbStat {
    std::string qty;
    std::string name;
};

struct AdminShiftProfile {
    std::string maCa;
    std::string tenCa;
    std::string quay;
    std::string thuNgan;
    std::string maNV;
    std::string khungGio;
    std::string gioBatDau;
    std::string gioKetThuc;
    std::string ngay;
    long long tongDoanhThu;
    long long tienThucTe;
    long long chenhLech;
    std::string trangThai;
    bool daKhoa;
    int tongLuotGD;
    int phanTramTarget;
    long long doanhThuVe;
    int tongSoVe;
    int veVip;
    int veThuong;
    int veSweetbox;
    long long doanhThuFnb;
    int soComboMonLe;
    long long voucherGiamGia;
    int soLuotVoucher;
    long long tienMatHT;
    long long tienMatThucTe;
    long long quetQrHT;
    long long quetQrThucTe;
    int soGdQR;
    long long caThePosHT;
    long long caThePosThucTe;
    int soGdCard;
    std::string maEdc;
    long long tienDauCa;
    std::vector<AdminShiftMovieStat> mvStats;
    std::vector<AdminTopFnbStat> topFnbs;
};

static int parseTimeSeconds(const std::string& tStr, int defaultSec = 0) {
    if (tStr.empty()) return defaultSec;
    int h = 0, m = 0, s = 0;
    int n = std::sscanf(tStr.c_str(), "%d:%d:%d", &h, &m, &s);
    if (n >= 2) {
        return h * 3600 + m * 60 + (n >= 3 ? s : 0);
    }
    if (n == 1) {
        return h * 3600;
    }
    return defaultSec;
}

static bool staffIdMatches(const std::string& a, const std::string& b) {
    if (a == b) return true;
    if ((a == "U002" && b == "U02") || (a == "U02" && b == "U002")) return true;
    if ((a == "U001" && b == "U01") || (a == "U01" && b == "U001")) return true;
    return false;
}

static std::string inferShiftNameFromStart(const std::string& gioBD) {
    int sec = parseTimeSeconds(gioBD, 8 * 3600);
    int h = sec / 3600;
    if (h < 12) return "Ca Sáng";
    if (h < 18) return "Ca Chiều";
    return "Ca Tối";
}

static std::string formatShiftTimeSpan(const std::string& gioBD, const std::string& gioKT) {
    std::string b = gioBD.size() >= 5 ? gioBD.substr(0, 5) : gioBD;
    std::string e = gioKT.size() >= 5 ? gioKT.substr(0, 5) : gioKT;
    int s1 = parseTimeSeconds(gioBD, 0);
    int s2 = parseTimeSeconds(gioKT, 0);
    int diffMin = (s2 >= s1) ? ((s2 - s1) / 60) : 0;
    int dh = diffMin / 60;
    int dm = diffMin % 60;
    if (dh > 0 || dm > 0) {
        return b + " – " + e + " (" + std::to_string(dh) + " giờ " + std::to_string(dm) + " phút)";
    }
    return b + " – " + e;
}

static std::vector<AdminShiftProfile> s_cachedAdminProfiles;

static std::vector<AdminShiftProfile> loadAdminShiftProfilesFromDB() {
    std::vector<AdminShiftProfile> result;
    sqlite3* dbConn = nullptr;
    if (sqlite3_open("DATA/cinema.db", &dbConn) != SQLITE_OK || !dbConn) {
        if (dbConn) sqlite3_close(dbConn);
        return result;
    }

    // Load staff names map from users
    std::map<std::string, std::string> userNames;
    {
        sqlite3_stmt* uStmt = nullptr;
        if (sqlite3_prepare_v2(dbConn, "SELECT maNguoiDung, hoTen FROM users;", -1, &uStmt, nullptr) == SQLITE_OK) {
            while (sqlite3_step(uStmt) == SQLITE_ROW) {
                const char* id = reinterpret_cast<const char*>(sqlite3_column_text(uStmt, 0));
                const char* nm = reinterpret_cast<const char*>(sqlite3_column_text(uStmt, 1));
                if (id && nm && nm[0] != '\0') userNames[id] = nm;
            }
            sqlite3_finalize(uStmt);
        }
    }

    // Load all shift records ordered by maCa DESC
    struct RawShiftRow {
        std::string maCa, maNV, tenNV, ngayLam, gioBD, gioKT, trangThai;
        long long doanhThu = 0, tienThucTe = 0, chenhLech = 0;
        bool daKhoa = false;
    };
    std::vector<RawShiftRow> shiftRows;
    {
        sqlite3_stmt* stmt = nullptr;
        const char* q = "SELECT maCa, maNhanVien, tenNhanVien, ngayLam, gioBatDau, gioKetThuc, doanhThuHeThong, tienThucTe, chenhLech, trangThai, daKhoaBaoCao FROM shift_managements ORDER BY maCa DESC;";
        if (sqlite3_prepare_v2(dbConn, q, -1, &stmt, nullptr) == SQLITE_OK) {
            while (sqlite3_step(stmt) == SQLITE_ROW) {
                auto colStr = [&](int c) -> std::string {
                    const char* pStr = reinterpret_cast<const char*>(sqlite3_column_text(stmt, c));
                    return pStr ? std::string(pStr) : "";
                };
                RawShiftRow r;
                r.maCa = colStr(0);
                r.maNV = colStr(1);
                r.tenNV = colStr(2);
                if (r.tenNV == "Pham Nhu Anh Thu" || r.tenNV.empty()) {
                    r.tenNV = userNames.count(r.maNV) ? userNames[r.maNV] : "Phạm Như Anh Thư";
                }
                r.ngayLam = DateFormat::normalizeDate(colStr(3));
                r.gioBD = colStr(4);
                r.gioKT = colStr(5);
                try { r.doanhThu = std::stoll(colStr(6)); } catch (...) { r.doanhThu = 0; }
                try { r.tienThucTe = std::stoll(colStr(7)); } catch (...) { r.tienThucTe = 0; }
                try { r.chenhLech = std::stoll(colStr(8)); } catch (...) { r.chenhLech = r.tienThucTe - r.doanhThu; }
                r.trangThai = colStr(9);
                r.daKhoa = (colStr(10) == "true" || colStr(10) == "1");
                shiftRows.push_back(r);
            }
            sqlite3_finalize(stmt);
        }
    }

    // Load all invoices
    struct RawInv {
        std::string maHD, maNV, dateStr, timeStr, method;
        long long tongTien = 0;
        int timeSec = 0;
    };
    std::vector<RawInv> allInvs;
    {
        sqlite3_stmt* stmt = nullptr;
        const char* q = "SELECT maHoaDon, maNhanVien, thoiGianLap, tongTien, hinhThucThanhToan FROM invoices WHERE trangThaiThanhToan != 'DaHuy';";
        if (sqlite3_prepare_v2(dbConn, q, -1, &stmt, nullptr) == SQLITE_OK) {
            while (sqlite3_step(stmt) == SQLITE_ROW) {
                auto colStr = [&](int c) -> std::string {
                    const char* pStr = reinterpret_cast<const char*>(sqlite3_column_text(stmt, c));
                    return pStr ? std::string(pStr) : "";
                };
                RawInv inv;
                inv.maHD = colStr(0);
                inv.maNV = colStr(1);
                std::string dt = colStr(2);
                size_t sp = dt.find(' ');
                if (sp != std::string::npos) {
                    inv.dateStr = DateFormat::normalizeDate(dt.substr(0, sp));
                    inv.timeStr = dt.substr(sp + 1);
                } else {
                    inv.dateStr = DateFormat::normalizeDate(dt);
                    inv.timeStr = "12:00:00";
                }
                inv.timeSec = parseTimeSeconds(inv.timeStr, 12 * 3600);
                try { inv.tongTien = std::stoll(colStr(3)); } catch (...) { inv.tongTien = 0; }
                inv.method = colStr(4);
                allInvs.push_back(inv);
            }
            sqlite3_finalize(stmt);
        }
    }

    for (const auto& sr : shiftRows) {
        AdminShiftProfile p;
        p.maCa = sr.maCa;
        p.tenCa = inferShiftNameFromStart(sr.gioBD);
        p.quay = "Quầy 01";
        p.thuNgan = sr.tenNV;
        p.maNV = sr.maNV;
        p.gioBatDau = sr.gioBD;
        p.gioKetThuc = sr.gioKT;
        p.khungGio = formatShiftTimeSpan(sr.gioBD, sr.gioKT);
        p.ngay = sr.ngayLam;
        p.tongDoanhThu = sr.doanhThu;
        p.tienThucTe = sr.tienThucTe;
        p.chenhLech = sr.tienThucTe - sr.doanhThu;
        p.trangThai = sr.trangThai;
        p.daKhoa = sr.daKhoa;
        p.maEdc = "#" + sr.maCa;
        p.tienDauCa = 1500000;

        int startSec = parseTimeSeconds(sr.gioBD, 0);
        int endSec = parseTimeSeconds(sr.gioKT, 24 * 3600 - 1);
        if (endSec < startSec) endSec = 24 * 3600 - 1;

        std::vector<std::string> matchedInvIds;
        long long invCash = 0, invQr = 0, invCard = 0;
        int qrCnt = 0, cardCnt = 0;

        for (const auto& inv : allInvs) {
            if (inv.dateStr != sr.ngayLam) continue;
            if (!staffIdMatches(inv.maNV, sr.maNV)) continue;
            if (inv.timeSec >= startSec && inv.timeSec <= endSec + 300) {
                matchedInvIds.push_back(inv.maHD);
                if (inv.method == "QueQR" || inv.method == "QR_PAY") {
                    invQr += inv.tongTien;
                    qrCnt++;
                } else if (inv.method == "TheNganHang" || inv.method == "CARD") {
                    invCard += inv.tongTien;
                    cardCnt++;
                } else {
                    invCash += inv.tongTien;
                }
            }
        }

        p.tongLuotGD = static_cast<int>(matchedInvIds.size());
        p.phanTramTarget = (p.tongDoanhThu > 0) ? std::min(150, std::max(10, static_cast<int>((p.tongDoanhThu * 100) / 200000))) : 0;

        // Payment breakdown reconciled against the shift record's actual doanhThuHeThong & tienThucTe
        p.quetQrHT = std::min(p.tongDoanhThu, invQr);
        p.quetQrThucTe = p.quetQrHT;
        p.soGdQR = qrCnt;

        p.caThePosHT = std::min(std::max(0LL, p.tongDoanhThu - p.quetQrHT), invCard);
        p.caThePosThucTe = p.caThePosHT;
        p.soGdCard = cardCnt;

        p.tienMatHT = std::max(0LL, p.tongDoanhThu - p.quetQrHT - p.caThePosHT);
        p.tienMatThucTe = std::max(0LL, p.tienThucTe - p.quetQrThucTe - p.caThePosThucTe);

        // Ticket & F&B breakdown for matched invoices
        long long rawTicketRev = 0;
        int totalTickets = 0, vipCnt = 0, stdCnt = 0, swCnt = 0;

        struct MovieAgg {
            std::string title;
            std::set<std::string> showtimes;
            int total = 0, vip = 0, std = 0, sw = 0;
            long long rev = 0;
        };
        std::map<std::string, MovieAgg> movieAggMap;

        long long rawFnbRev = 0;
        int fnbQtyTotal = 0;
        std::map<std::string, int> fnbItemQty;

        for (const auto& hdId : matchedInvIds) {
            // Query tickets for this invoice
            std::string qTk = "SELECT t.maSuatChieu, t.maGhe, CAST(t.giaVeThucTe AS INTEGER), "
                              "COALESCE(m.tenPhim, s.maPhim, 'Phim chiếu rạp'), "
                              "COALESCE(st.loaiGhe, '') "
                              "FROM tickets t "
                              "LEFT JOIN showtimes s ON t.maSuatChieu = s.maSuatChieu "
                              "LEFT JOIN movies m ON s.maPhim = m.maPhim "
                              "LEFT JOIN seats st ON st.maPhong = s.maPhong AND st.maGhe = t.maGhe "
                              "WHERE t.maHoaDon = '" + hdId + "';";
            sqlite3_stmt* tkStmt = nullptr;
            if (sqlite3_prepare_v2(dbConn, qTk.c_str(), -1, &tkStmt, nullptr) == SQLITE_OK) {
                while (sqlite3_step(tkStmt) == SQLITE_ROW) {
                    const char* cSc = reinterpret_cast<const char*>(sqlite3_column_text(tkStmt, 0));
                    const char* cGhe = reinterpret_cast<const char*>(sqlite3_column_text(tkStmt, 1));
                    long long gGia = sqlite3_column_int64(tkStmt, 2);
                    const char* cPhim = reinterpret_cast<const char*>(sqlite3_column_text(tkStmt, 3));
                    const char* cLoai = reinterpret_cast<const char*>(sqlite3_column_text(tkStmt, 4));

                    std::string scStr = cSc ? cSc : "";
                    std::string gheStr = cGhe ? cGhe : "";
                    std::string phimStr = (cPhim && cPhim[0] != '\0') ? cPhim : "Suất chiếu tại rạp";
                    std::string loaiStr = cLoai ? cLoai : "";

                    rawTicketRev += gGia;
                    totalTickets++;

                    bool isSw = (loaiStr == "Couple" || loaiStr == "Sweetbox" || gheStr.find('-') != std::string::npos || gheStr.rfind("SW", 0) == 0);
                    bool isVip = (!isSw && (loaiStr == "VIP" || loaiStr == "IMAX" || loaiStr == "4DX" || gheStr.rfind("VIP", 0) == 0));

                    if (isSw) swCnt++;
                    else if (isVip) vipCnt++;
                    else stdCnt++;

                    auto& ma = movieAggMap[phimStr];
                    ma.title = phimStr;
                    if (!scStr.empty()) ma.showtimes.insert(scStr);
                    ma.total++;
                    if (isSw) ma.sw++;
                    else if (isVip) ma.vip++;
                    else ma.std++;
                    ma.rev += gGia;
                }
                sqlite3_finalize(tkStmt);
            }

            // Query F&B items for this invoice
            std::string qFb = "SELECT COALESCE(tenMatHang, maMatHang), CAST(soLuong AS INTEGER), CAST(thanhTien AS INTEGER) "
                              "FROM invoice_fb_items WHERE maHoaDon = '" + hdId + "';";
            sqlite3_stmt* fbStmt = nullptr;
            if (sqlite3_prepare_v2(dbConn, qFb.c_str(), -1, &fbStmt, nullptr) == SQLITE_OK) {
                while (sqlite3_step(fbStmt) == SQLITE_ROW) {
                    const char* cName = reinterpret_cast<const char*>(sqlite3_column_text(fbStmt, 0));
                    int qty = sqlite3_column_int(fbStmt, 1);
                    long long tt = sqlite3_column_int64(fbStmt, 2);
                    std::string nm = (cName && cName[0] != '\0') ? cName : "Món F&B";
                    if (qty <= 0) qty = 1;
                    rawFnbRev += tt;
                    fnbQtyTotal += qty;
                    fnbItemQty[nm] += qty;
                }
                sqlite3_finalize(fbStmt);
            }
        }

        if (!matchedInvIds.empty() && (rawTicketRev + rawFnbRev > 0)) {
            p.doanhThuVe = rawTicketRev;
            p.tongSoVe = totalTickets;
            p.veVip = vipCnt;
            p.veThuong = stdCnt;
            p.veSweetbox = swCnt;
            p.doanhThuFnb = rawFnbRev;
            p.soComboMonLe = fnbQtyTotal;

            long long grossSum = rawTicketRev + rawFnbRev;
            if (grossSum > p.tongDoanhThu) {
                p.voucherGiamGia = grossSum - p.tongDoanhThu;
                p.soLuotVoucher = std::max(1, static_cast<int>(matchedInvIds.size()));
            } else {
                p.voucherGiamGia = 0;
                p.soLuotVoucher = 0;
                if (grossSum < p.tongDoanhThu) {
                    p.doanhThuVe += (p.tongDoanhThu - grossSum);
                }
            }
        } else {
            // Shift record in shift_managements has no detailed invoice child rows in DB
            p.doanhThuVe = p.tongDoanhThu;
            p.doanhThuFnb = 0;
            p.soComboMonLe = 0;
            p.voucherGiamGia = 0;
            p.soLuotVoucher = 0;
            p.tongSoVe = totalTickets;
            p.veVip = vipCnt;
            p.veThuong = stdCnt;
            p.veSweetbox = swCnt;
            if (p.tongLuotGD == 0 && p.tongDoanhThu > 0) {
                p.tongLuotGD = 1;
            }
        }

        for (const auto& kv : movieAggMap) {
            const auto& ma = kv.second;
            AdminShiftMovieStat ms;
            ms.title = ma.title;
            int scCount = std::max(1, static_cast<int>(ma.showtimes.size()));
            std::string brk;
            if (ma.vip > 0) brk += (brk.empty() ? "" : ", ") + std::string("VIP: ") + std::to_string(ma.vip);
            if (ma.std > 0) brk += (brk.empty() ? "" : ", ") + std::string("Thường: ") + std::to_string(ma.std);
            if (ma.sw > 0)  brk += (brk.empty() ? "" : ", ") + std::string("Sweetbox: ") + std::to_string(ma.sw);
            ms.detail = std::to_string(scCount) + " suất  •  " + std::to_string(ma.total) + " vé" + (brk.empty() ? "" : (" (" + brk + ")"));
            ms.revenue = formatDongShort(ma.rev);
            double pct = (p.tongSoVe > 0) ? (ma.total * 100.0 / p.tongSoVe) : 100.0;
            char pctBuf[32];
            std::snprintf(pctBuf, sizeof(pctBuf), "Chiếm %.1f%% vé", pct);
            ms.share = pctBuf;
            ms.highlightShare = (pct >= 50.0);
            p.mvStats.push_back(ms);
        }
        if (p.mvStats.empty()) {
            p.mvStats.push_back({
                p.tongDoanhThu > 0 ? ("Doanh thu ca " + p.maCa + " (Ghi nhận tổng hợp)") : "Chưa phát sinh giao dịch vé trong ca",
                p.tongDoanhThu > 0 ? ("Tổng hợp từ dữ liệu ca " + p.maCa + " • Ngày " + p.ngay) : ("Ca trực " + p.maCa + " • Ngày " + p.ngay),
                formatDongShort(p.doanhThuVe),
                p.tongDoanhThu > 0 ? "100% doanh thu vé" : "0 vé",
                p.tongDoanhThu > 0
            });
        }

        for (const auto& kv : fnbItemQty) {
            p.topFnbs.push_back({std::to_string(kv.second) + " phần", kv.first});
            if (p.topFnbs.size() >= 3) break;
        }
        if (p.topFnbs.empty()) {
            p.topFnbs.push_back({"0 phần", "Chưa phát sinh F&B"});
        }

        result.push_back(p);
    }

    sqlite3_close(dbConn);

    if (result.empty()) {
        AdminShiftProfile emptyP{};
        emptyP.maCa = "CA00";
        emptyP.tenCa = "Chưa có ca";
        emptyP.quay = "Quầy 01";
        emptyP.thuNgan = "Chưa có dữ liệu";
        emptyP.maNV = "N/A";
        emptyP.khungGio = "--:-- – --:--";
        emptyP.gioBatDau = "08:00";
        emptyP.gioKetThuc = "16:00";
        emptyP.ngay = getTodayDateStr();
        emptyP.trangThai = "ChoDuyet";
        emptyP.maEdc = "#00";
        emptyP.tienDauCa = 1500000;
        emptyP.mvStats.push_back({"Chưa có dữ liệu ca trực", "0 suất • 0 vé", "0 đ", "0% vé", false});
        emptyP.topFnbs.push_back({"0 phần", "Chưa có dữ liệu"});
        result.push_back(emptyP);
    }
    return result;
}

static const std::vector<AdminShiftProfile>& getAdminShiftProfiles() {
    if (s_cachedAdminProfiles.empty()) {
        s_cachedAdminProfiles = loadAdminShiftProfilesFromDB();
    }
    return s_cachedAdminProfiles;
}

void CinemaUIApp::setupShiftReportView() {
    float startX = sidebarWidth + 24.f; // 274.f
    float contentW = 1728.f - startX - 24.f; // 1430.f

    shiftCalculatedSysRev = 222000;
    shiftCalculatedActualCash = 222000;
    shiftCalculatedDiff = 0;
    shiftIsClosed = false;
    shiftCurrentPage = 0;

    adminShiftApproved = false;
    adminShowHistorySubView = false;
    adminSelectedShiftIdx = 0;

    // =========================================================================
    // A. ADMIN SHIFT RECONCILIATION CONTROLS (media_1795197286781_0606a2d9.jpg)
    // =========================================================================
    float admStartX = sidebarWidth + 18.f; // 268.f
    float admContentW = 1728.f - admStartX - 18.f; // 1442.f

    btnAdminShiftRefresh = std::make_unique<RoundRectButton>(
        "↻  Làm mới lịch chiếu",
        sf::Vector2f(admStartX + admContentW - 252.f, 18.f),
        sf::Vector2f(136.f, 38.f),
        ButtonStyle::Primary,
        8.f
    );
    btnAdminShiftRefresh->setFontSize(11);
    btnAdminShiftRefresh->setCustomColors(
        sf::Color(2, 132, 199),
        sf::Color(3, 105, 161),
        sf::Color(7, 89, 133),
        sf::Color(148, 163, 184)
    );
    btnAdminShiftRefresh->setOnClick([this]() {
        reloadShiftReportData();
        ActivityLog::ghiNhan(currentUserId, currentUserRole, "LAM_MOI_DOI_SOAT_CA", "Đồng bộ dữ liệu báo cáo kết ca & đối soát doanh thu từ SQLite");
        if (confirmPopup) {
            confirmPopup->show(
                "ĐỒNG BỘ DỮ LIỆU ĐỐI SOÁT",
                "Đã cập nhật mới nhất số liệu doanh thu vé, bắp nước (F&B), bảng đối soát phương thức thanh toán và kiểm kê két tiền từ SQLite!",
                ButtonStyle::Primary,
                "Hoàn tất",
                ""
            );
        }
    });

    btnAdminShiftExit = std::make_unique<RoundRectButton>(
        "Đổi ca / Thoát",
        sf::Vector2f(admStartX + admContentW - 108.f, 18.f),
        sf::Vector2f(96.f, 38.f),
        ButtonStyle::Secondary,
        8.f
    );
    btnAdminShiftExit->setFontSize(11);
    btnAdminShiftExit->setCustomColors(
        sf::Color(254, 242, 242),
        sf::Color(254, 226, 226),
        sf::Color(254, 202, 202),
        sf::Color(254, 202, 202)
    );
    btnAdminShiftExit->setTextColor(sf::Color(220, 38, 38));
    btnAdminShiftExit->setOnClick([this]() {
        s_cachedAdminProfiles = loadAdminShiftProfilesFromDB();
        const auto& profiles = getAdminShiftProfiles();
        adminSelectedShiftIdx = (adminSelectedShiftIdx + 1) % static_cast<int>(profiles.size());
        const auto& p = profiles[adminSelectedShiftIdx];
        adminShiftApproved = (p.trangThai == "DaDuyet");
        ActivityLog::ghiNhan(currentUserId, currentUserRole, "CHUYEN_CA_DOI_SOAT", "Chuyển sang đối soát ca " + p.maCa + " (" + p.tenCa + ") - Thu ngân: " + p.thuNgan);
        if (confirmPopup) {
            confirmPopup->show(
                "ĐÃ CHUYỂN PHIÊN CA ĐỐI SOÁT",
                "Đang hiển thị báo cáo kết ca & đối soát doanh thu từ SQLite:\n• Mã ca: " + p.maCa + " - " + p.tenCa + " (" + p.ngay + ")\n• Thu ngân: " + p.thuNgan + " (" + p.maNV + ")\n• Doanh thu HT: " + formatVnd(p.tongDoanhThu) + " | Thực tế: " + formatVnd(p.tienThucTe),
                ButtonStyle::Primary,
                "Đóng",
                ""
            );
        }
    });

    btnAdminShiftHistory = std::make_unique<RoundRectButton>(
        "Lịch sử giao dịch ca",
        sf::Vector2f(admStartX + admContentW - 178.f, 87.f),
        sf::Vector2f(162.f, 34.f),
        ButtonStyle::Secondary,
        8.f
    );
    btnAdminShiftHistory->setFontSize(11);
    btnAdminShiftHistory->setCustomColors(
        sf::Color(240, 249, 255),
        sf::Color(224, 242, 254),
        sf::Color(186, 230, 253),
        sf::Color(186, 230, 253)
    );
    btnAdminShiftHistory->setTextColor(sf::Color(2, 132, 199));
    btnAdminShiftHistory->setOnClick([this]() {
        adminShowHistorySubView = !adminShowHistorySubView;
        if (adminShowHistorySubView) {
            btnAdminShiftHistory->setText("← Báo cáo kết ca");
            btnAdminShiftHistory->setPosition(sf::Vector2f( sidebarWidth + 24.f + (1728.f - sidebarWidth - 48.f) - 438.f, 84.f ));
            btnAdminShiftHistory->setSize(sf::Vector2f(160.f, 38.f));
        } else {
            float aStartX = sidebarWidth + 18.f;
            float aContentW = 1728.f - aStartX - 18.f;
            btnAdminShiftHistory->setText("Lịch sử giao dịch ca");
            btnAdminShiftHistory->setPosition(sf::Vector2f(aStartX + aContentW - 178.f, 87.f));
            btnAdminShiftHistory->setSize(sf::Vector2f(162.f, 34.f));
        }
    });

    // Right Bottom Card Controls: Next Cashier Dropdown, Manager PIN Input, Handover & Approve Buttons
    float rColX = admStartX + 846.f + 14.f; // 1128.f

    std::vector<std::string> nextCashierOpts;
    for (const auto& u : staffUserList) {
        if ((u.vaiTro == "Staff" || u.vaiTro == "Admin") && u.trangThaiLamViec != "DaNghi") {
            nextCashierOpts.push_back(u.hoTen + " (" + u.maNguoiDung + ")");
        }
    }
    if (nextCashierOpts.empty()) {
        nextCashierOpts = {
            "Phạm Như Anh Thư (U002)",
            "Lê Tuyết Nhi (U003)"
        };
    }

    adminNextCashierDropdown = std::make_unique<DropdownBox>(
        sf::Vector2f(rColX + 18.f, 840.f),
        sf::Vector2f(312.f, 34.f),
        nextCashierOpts,
        0
    );
    adminNextCashierDropdown->setSelectedIndex(0);

    inputAdminManagerPin = std::make_unique<UIInputBox>(
        sf::Vector2f(rColX + 342.f, 840.f),
        sf::Vector2f(222.f, 34.f),
        "Nhập mã PIN...",
        13
    );
    inputAdminManagerPin->setText("");

    btnAdminCashierHandover = std::make_unique<RoundRectButton>(
        "THU NGÂN BÀN GIAO",
        sf::Vector2f(rColX + 18.f, 892.f),
        sf::Vector2f(266.f, 40.f),
        ButtonStyle::Secondary,
        8.f
    );
    btnAdminCashierHandover->setFontSize(11);
    btnAdminCashierHandover->setCustomColors(
        sf::Color(248, 250, 252),
        sf::Color(241, 245, 249),
        sf::Color(226, 232, 240),
        sf::Color(203, 213, 225)
    );
    btnAdminCashierHandover->setTextColor(sf::Color(51, 65, 85));
    btnAdminCashierHandover->setOnClick([this]() {
        const auto& profiles = getAdminShiftProfiles();
        const auto& p = profiles[adminSelectedShiftIdx % profiles.size()];
        if (p.maCa == "CA00") return;
        std::string nextStaff = adminNextCashierDropdown ? adminNextCashierDropdown->getSelectedValue() : "Phạm Như Anh Thư (U002)";

        // Invoke backend ShiftReport logic preserving the real shift record values
        ShiftReport rep(p.maCa, p.maNV, p.thuNgan, p.ngay, p.gioBatDau, p.gioKetThuc, p.tongDoanhThu, p.tienThucTe, p.trangThai, p.daKhoa);
        rep.tinhChenhLech();
        rep.khoaBaoCao();

        // Save handover receipt file with actual DB shift numbers
        std::ofstream f("DATA/bien_ban_ban_giao_ca.txt");
        if (f.is_open()) {
            f << "=================================================================\n";
            f << "          CINESTAR CINEMA - BIÊN BẢN KẾT CA & BÀN GIAO           \n";
            f << "=================================================================\n";
            f << "Mã ca / Quầy           : " << p.maCa << " - " << p.tenCa << " (" << p.quay << ")\n";
            f << "Thu ngân bàn giao      : " << p.thuNgan << " (" << p.maNV << ")\n";
            f << "Nhân viên nhận ca sau  : " << nextStaff << "\n";
            f << "Khung giờ trực         : " << p.khungGio << " • " << p.ngay << "\n";
            f << "-----------------------------------------------------------------\n";
            f << "1. TỔNG DOANH THU CA (HT)          : " << formatVnd(p.tongDoanhThu) << " (" << p.tongLuotGD << " GD)\n";
            f << "   - Doanh thu vé xem phim         : " << formatVnd(p.doanhThuVe) << " (" << p.tongSoVe << " vé)\n";
            f << "   - Doanh thu bắp nước (F&B)      : " << formatVnd(p.doanhThuFnb) << " (" << p.soComboMonLe << " combo/món)\n";
            f << "   - Voucher / Giảm giá            : -" << formatVnd(p.voucherGiamGia) << " (" << p.soLuotVoucher << " lượt)\n";
            f << "-----------------------------------------------------------------\n";
            f << "2. ĐỐI SOÁT PHƯƠNG THỨC THANH TOÁN :\n";
            f << "   - Tiền mặt (Cash)               : " << formatDongShort(p.tienMatHT) << " | Thực tế: " << formatDongShort(p.tienMatThucTe) << " (Chênh lệch: " << formatDongShort(p.chenhLech) << ")\n";
            f << "   - Quét mã QR                    : " << formatDongShort(p.quetQrHT) << " | Thực tế: " << formatDongShort(p.quetQrThucTe) << " (Khớp 0 đ)\n";
            f << "   - Cà thẻ POS EDC                : " << formatDongShort(p.caThePosHT) << " | Thực tế: " << formatDongShort(p.caThePosThucTe) << " (Khớp 0 đ)\n";
            f << "=================================================================\n";
            f.close();
        }

        // Only lock the report in shift_managements; do NOT overwrite hours, revenue, or actual cash
        sqlite3* dbConn = nullptr;
        if (sqlite3_open("DATA/cinema.db", &dbConn) == SQLITE_OK) {
            std::string updSql = "UPDATE shift_managements SET daKhoaBaoCao='true' WHERE maCa='" + p.maCa + "';";
            sqlite3_exec(dbConn, updSql.c_str(), nullptr, nullptr, nullptr);
            sqlite3_close(dbConn);
        }

        std::string curCa = p.maCa;
        std::string curThuNgan = p.thuNgan;
        long long curTienMat = p.tienMatThucTe;
        reloadShiftReportData();

        ActivityLog::ghiNhan(currentUserId, currentUserRole, "BAN_GIAO_CA", "Thu ngân " + curThuNgan + " lập biên bản bàn giao ca " + curCa + " cho " + nextStaff);
        if (confirmPopup) {
            confirmPopup->show(
                "LẬP BIÊN BẢN BÀN GIAO THÀNH CÔNG",
                "Đã khóa báo cáo ca " + curCa + " trong SQLite (giữ nguyên số liệu gốc) và xuất biên bản bàn giao:\n"
                "• File: DATA/bien_ban_ban_giao_ca.txt\n"
                "• Thu ngân bàn giao: " + curThuNgan + "\n"
                "• Nhân viên nhận ca sau: " + nextStaff + "\n"
                "• Tiền mặt thực tế bàn giao: " + formatDongShort(curTienMat),
                ButtonStyle::Primary,
                "Hoàn tất",
                ""
            );
        }
    });

    btnAdminManagerApprove = std::make_unique<RoundRectButton>(
        "QUẢN LÝ DUYỆT CA",
        sf::Vector2f(rColX + 298.f, 892.f),
        sf::Vector2f(266.f, 40.f),
        ButtonStyle::Primary,
        8.f
    );
    btnAdminManagerApprove->setFontSize(11);
    btnAdminManagerApprove->setCustomColors(
        sf::Color(2, 132, 199),
        sf::Color(3, 105, 161),
        sf::Color(7, 89, 133),
        sf::Color(148, 163, 184)
    );
    btnAdminManagerApprove->setOnClick([this]() {
        std::string pin = inputAdminManagerPin ? inputAdminManagerPin->getText() : "";
        if (pin.empty()) {
            if (confirmPopup) {
                confirmPopup->show(
                    "YÊU CẦU MÃ PIN QUẢN LÝ",
                    "Vui lòng nhập Mã PIN Quản lý trước khi xác nhận duyệt ca!",
                    ButtonStyle::Danger,
                    "Đóng",
                    ""
                );
            }
            return;
        }

        // Authenticate Manager PIN against Admin credentials in DB or standard manager PIN "1234"
        bool pinValid = (pin == "1234");
        if (!pinValid) {
            sqlite3* dbConn = nullptr;
            if (sqlite3_open("DATA/cinema.db", &dbConn) == SQLITE_OK) {
                sqlite3_stmt* stmt = nullptr;
                const char* qPin = "SELECT matKhau FROM users WHERE vaiTro='Admin' AND trangThai!='DaNghi';";
                if (sqlite3_prepare_v2(dbConn, qPin, -1, &stmt, nullptr) == SQLITE_OK) {
                    while (sqlite3_step(stmt) == SQLITE_ROW) {
                        const char* pw = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
                        if (pw && pin == std::string(pw)) {
                            pinValid = true;
                            break;
                        }
                    }
                    sqlite3_finalize(stmt);
                }
                sqlite3_close(dbConn);
            }
        }

        if (!pinValid) {
            if (confirmPopup) {
                confirmPopup->show(
                    "MÃ PIN KHÔNG CHÍNH XÁC",
                    "Mã PIN Quản lý không đúng! Vui lòng kiểm tra lại Mã PIN Quản lý (hoặc mật khẩu Admin) trước khi duyệt ca.",
                    ButtonStyle::Danger,
                    "Đóng",
                    ""
                );
            }
            return;
        }

        const auto& profiles = getAdminShiftProfiles();
        const auto& p = profiles[adminSelectedShiftIdx % profiles.size()];
        if (p.maCa == "CA00") return;

        if (p.trangThai == "DaDuyet") {
            if (confirmPopup) {
                confirmPopup->show(
                    "CA TRỰC ĐÃ ĐƯỢC DUYỆT",
                    "Ca trực " + p.maCa + " (" + p.ngay + ") đã ở trạng thái ĐÃ DUYỆT và khóa báo cáo!",
                    ButtonStyle::Primary,
                    "Đóng",
                    ""
                );
            }
            return;
        }

        std::string nextStaff = adminNextCashierDropdown ? adminNextCashierDropdown->getSelectedValue() : "Phạm Như Anh Thư (U002)";

        // Update only trangThai and daKhoaBaoCao for the selected shift p.maCa; never overwrite revenue/hours or other shift codes
        sqlite3* dbConn = nullptr;
        if (sqlite3_open("DATA/cinema.db", &dbConn) == SQLITE_OK) {
            std::string updSql = "UPDATE shift_managements SET trangThai='DaDuyet', daKhoaBaoCao='true' WHERE maCa='" + p.maCa + "';";
            sqlite3_exec(dbConn, updSql.c_str(), nullptr, nullptr, nullptr);
            sqlite3_close(dbConn);
        }

        if (inputAdminManagerPin) inputAdminManagerPin->setText("");

        std::string approvedCa = p.maCa;
        std::string approvedTenCa = p.tenCa;
        std::string approvedQuay = p.quay;
        long long approvedRev = p.tongDoanhThu;
        long long approvedDiff = p.chenhLech;

        reloadShiftReportData();
        adminShiftApproved = true;

        ActivityLog::ghiNhan(currentUserId, currentUserRole, "DUYET_CA_TRUC", "Quản lý duyệt chốt ca & đối soát doanh thu " + approvedCa + " (" + formatVnd(approvedRev) + ")");

        if (confirmPopup) {
            confirmPopup->show(
                "QUẢN LÝ DUYỆT CA THÀNH CÔNG",
                "Đã xác nhận duyệt kết ca & khóa báo cáo ca " + approvedCa + " trong SQLite!\n"
                "• Ca trực: " + approvedTenCa + " (" + approvedQuay + ") - Mã ca: " + approvedCa + "\n"
                "• Doanh thu hệ thống: " + formatVnd(approvedRev) + " (Chênh lệch: " + formatDongShort(approvedDiff) + ")\n"
                "• Nhân viên nhận bàn giao ca sau: " + nextStaff,
                ButtonStyle::Primary,
                "Hoàn tất",
                ""
            );
        }
    });

    // =========================================================================
    // B. STAFF SHIFT REPORT & AUDIT CONTROLS (media_1795195645330_f67074fa.jpg)
    // =========================================================================
    btnShiftExportReport = std::make_unique<RoundRectButton>(
        "Xuất biên bản ca",
        sf::Vector2f(startX + contentW - 266.f, 84.f),
        sf::Vector2f(152.f, 38.f),
        ButtonStyle::Primary,
        8.f
    );
    btnShiftExportReport->setFontSize(12);
    btnShiftExportReport->setCustomColors(
        sf::Color(14, 165, 233),
        sf::Color(2, 132, 199),
        sf::Color(3, 105, 161),
        sf::Color(148, 163, 184)
    );
    btnShiftExportReport->setOnClick([this]() {
        std::string activeCashier = !currentUserName.empty() ? currentUserName : (currentUserId == "U002" ? "Phạm Như Anh Thư" : (currentUserId == "U001" ? "Lê Minh Tuấn" : "Nguyễn Văn A"));
        for (const auto& u : staffUserList) {
            if (u.maNguoiDung == currentUserId && !u.hoTen.empty()) {
                activeCashier = u.hoTen;
                break;
            }
        }

        std::ofstream f("DATA/bien_ban_chot_ca.txt");
        if (f.is_open()) {
            f << "=========================================================\n";
            f << "           CINESTAR CINEMA & POS SYSTEM                  \n";
            f << "         BIÊN BẢN CHỐT CA & ĐỐI SOÁT DOANH THU           \n";
            f << "=========================================================\n\n";
            f << "Thu ngân phụ trách      : " << activeCashier << " (" << currentUserId << ")\n";
            f << "Quầy giao dịch          : Quầy POS 01\n";
            f << "Ca làm việc             : Ca Chiều (14h00 - 22h00)\n";
            f << "Ngày kết toán           : " << getTodayDateStr() << "\n";
            f << "Thời gian kết xuất      : " << getCurrentTimeStr() << "\n";
            f << "---------------------------------------------------------\n";
            f << "1. Doanh thu hệ thống POS ghi nhận : " << formatVnd(shiftCalculatedSysRev) << "\n";
            f << "2. Tiền mặt kiểm đếm thực tế       : " << formatVnd(shiftCalculatedActualCash) << "\n";
            f << "3. Chênh lệch kiểm quỹ             : " << formatVnd(shiftCalculatedDiff) << "\n";
            f << "4. Trạng thái chốt ca              : " << (shiftIsClosed ? "ĐÃ KHÓA SỔ & CHỐT CA" : "CHỜ DUYỆT BÀN GIAO") << "\n";
            f << "---------------------------------------------------------\n";
            f.close();

            ActivityLog::ghiNhan(currentUserId, currentUserRole, "XUAT_BIEN_BAN_CA", "Xuất biên bản bàn giao doanh thu ca trực ra DATA/bien_ban_chot_ca.txt");
            if (confirmPopup) {
                confirmPopup->show("XUẤT BIÊN BẢN THÀNH CÔNG", "Biên bản chốt ca & đối soát doanh thu đã được lưu tại:\nDATA/bien_ban_chot_ca.txt", ButtonStyle::Primary, "Đóng", "");
            }
        }
    });

    btnShiftRefresh = std::make_unique<RoundRectButton>(
        "↻  Làm mới",
        sf::Vector2f(startX + contentW - 104.f, 84.f),
        sf::Vector2f(104.f, 38.f),
        ButtonStyle::Secondary,
        8.f
    );
    btnShiftRefresh->setFontSize(12);
    btnShiftRefresh->setCustomColors(
        sf::Color::White,
        sf::Color(248, 250, 252),
        sf::Color(241, 245, 249),
        sf::Color(203, 213, 225)
    );
    btnShiftRefresh->setTextColor(sf::Color(30, 41, 59));
    btnShiftRefresh->setOnClick([this]() {
        reloadShiftReportData();
        if (confirmPopup) {
            confirmPopup->show("LÀM MỚI DỮ LIỆU", "Đã đồng bộ doanh thu POS và lịch sử các ca trực mới nhất từ hệ thống SQLite!", ButtonStyle::Primary, "Đóng", "");
        }
    });

    float leftCardW = 436.f;
    float topLeftCardY = 276.f;

    inputShiftActualCash = std::make_unique<UIInputBox>(
        sf::Vector2f(startX + 20.f, topLeftCardY + 112.f),
        sf::Vector2f(leftCardW - 40.f, 44.f),
        "Nhập tổng tiền mặt thực tế...",
        15
    );
    inputShiftActualCash->setText("222000");

    btnShiftCalculate = std::make_unique<RoundRectButton>(
        "ĐỐI SOÁT QUỸ",
        sf::Vector2f(startX + 20.f, topLeftCardY + 170.f),
        sf::Vector2f(190.f, 44.f),
        ButtonStyle::Primary,
        8.f
    );
    btnShiftCalculate->setFontSize(12);
    btnShiftCalculate->setCustomColors(
        sf::Color(14, 165, 233),
        sf::Color(2, 132, 199),
        sf::Color(3, 105, 161),
        sf::Color(148, 163, 184)
    );
    btnShiftCalculate->setOnClick([this]() {
        std::string raw = inputShiftActualCash ? inputShiftActualCash->getText() : "0";
        std::string digits;
        for (char ch : raw) {
            if (ch >= '0' && ch <= '9') digits.push_back(ch);
        }
        try {
            shiftCalculatedActualCash = digits.empty() ? 0 : std::stoll(digits);
        } catch (...) {
            shiftCalculatedActualCash = 0;
        }

        std::string activeCashier = !currentUserName.empty() ? currentUserName : (currentUserId == "U002" ? "Phạm Như Anh Thư" : (currentUserId == "U001" ? "Lê Minh Tuấn" : "Nguyễn Văn A"));
        for (const auto& u : staffUserList) {
            if (u.maNguoiDung == currentUserId && !u.hoTen.empty()) {
                activeCashier = u.hoTen;
                break;
            }
        }

        ShiftReport tempReport("CA_CUR", currentUserId, activeCashier, getTodayDateStr(), "14:00", "22:00", shiftCalculatedSysRev, shiftCalculatedActualCash);
        tempReport.tinhChenhLech();
        shiftCalculatedDiff = tempReport.getChenhLech();
    });

    btnShiftCloseShift = std::make_unique<RoundRectButton>(
        "KHÓA SỔ &\nCHỐT CA",
        sf::Vector2f(startX + 224.f, topLeftCardY + 170.f),
        sf::Vector2f(192.f, 44.f),
        ButtonStyle::Success,
        8.f
    );
    btnShiftCloseShift->setFontSize(11);
    btnShiftCloseShift->setCustomColors(
        sf::Color(5, 150, 105),
        sf::Color(4, 120, 87),
        sf::Color(6, 95, 70),
        sf::Color(148, 163, 184)
    );
    btnShiftCloseShift->setOnClick([this]() {
        std::string raw = inputShiftActualCash ? inputShiftActualCash->getText() : "0";
        std::string digits;
        for (char ch : raw) {
            if (ch >= '0' && ch <= '9') digits.push_back(ch);
        }
        try {
            shiftCalculatedActualCash = digits.empty() ? 0 : std::stoll(digits);
        } catch (...) {
            shiftCalculatedActualCash = 0;
        }

        std::string activeCashier = !currentUserName.empty() ? currentUserName : (currentUserId == "U002" ? "Phạm Như Anh Thư" : (currentUserId == "U001" ? "Lê Minh Tuấn" : "Nguyễn Văn A"));
        for (const auto& u : staffUserList) {
            if (u.maNguoiDung == currentUserId && !u.hoTen.empty()) {
                activeCashier = u.hoTen;
                break;
            }
        }

        ShiftReport newShift("CA_NEW", currentUserId, activeCashier, getTodayDateStr(), "14:00", "22:00", shiftCalculatedSysRev, shiftCalculatedActualCash, "ChoDuyet", true);
        newShift.tinhChenhLech();
        newShift.khoaBaoCao();
        shiftCalculatedDiff = newShift.getChenhLech();

        sqlite3* dbConn = nullptr;
        if (sqlite3_open("DATA/cinema.db", &dbConn) == SQLITE_OK) {
            sqlite3_stmt* stmt = nullptr;
            int maxCa = 8;
            if (sqlite3_prepare_v2(dbConn, "SELECT maCa FROM shift_managements;", -1, &stmt, nullptr) == SQLITE_OK) {
                while (sqlite3_step(stmt) == SQLITE_ROW) {
                    const char* t = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
                    if (t && std::string(t).rfind("CA", 0) == 0) {
                        try {
                            int v = std::stoi(std::string(t).substr(2));
                            if (v > maxCa) maxCa = v;
                        } catch (...) {}
                    }
                }
                sqlite3_finalize(stmt);
            }

            char caId[32];
            std::snprintf(caId, sizeof(caId), "CA%02d", maxCa + 1);

            std::time_t tNow = std::time(nullptr);
            char timeBuf[16];
            std::strftime(timeBuf, sizeof(timeBuf), "%H:%M:%S", std::localtime(&tNow));

            std::string qInsert = "INSERT INTO shift_managements (maCa, maNhanVien, tenNhanVien, ngayLam, gioBatDau, gioKetThuc, doanhThuHeThong, tienThucTe, chenhLech, trangThai, daKhoaBaoCao) VALUES ('"
                + std::string(caId) + "', '"
                + currentUserId + "', '"
                + activeCashier + "', '"
                + getTodayDateStr() + "', '14:00', '"
                + std::string(timeBuf) + "', '"
                + std::to_string(shiftCalculatedSysRev) + "', '"
                + std::to_string(shiftCalculatedActualCash) + "', '"
                + std::to_string(shiftCalculatedDiff) + "', 'ChoDuyet', 'true');";

            sqlite3_exec(dbConn, qInsert.c_str(), nullptr, nullptr, nullptr);
            sqlite3_close(dbConn);

            shiftIsClosed = true;
            ActivityLog::ghiNhan(currentUserId, currentUserRole, "CHOT_CA_TRUC", "Khóa sổ ca trực " + std::string(caId) + " - Doanh thu: " + formatVnd(shiftCalculatedSysRev));

            if (confirmPopup) {
                confirmPopup->show("KHÓA SỔ & CHỐT CA THÀNH CÔNG",
                    "Ca trực " + std::string(caId) + " đã được khóa sổ và lưu vào bảng shift_managements!\n"
                    "• Doanh thu hệ thống POS: " + formatVnd(shiftCalculatedSysRev) + "\n"
                    "• Tiền mặt kiểm đếm thực tế: " + formatVnd(shiftCalculatedActualCash) + "\n"
                    "• Chênh lệch đối soát: " + formatVnd(shiftCalculatedDiff),
                    ButtonStyle::Primary, "Hoàn tất", "");
            }
        }
    });

    float rightPanelX = startX + 456.f;
    float rightPanelW = contentW - 456.f;

    btnShiftPrevPage = std::make_unique<RoundRectButton>(
        "Trước",
        sf::Vector2f(rightPanelX + rightPanelW - 156.f, 650.f),
        sf::Vector2f(52.f, 26.f),
        ButtonStyle::Secondary,
        5.f
    );
    btnShiftPrevPage->setFontSize(10);
    btnShiftPrevPage->setCustomColors(
        sf::Color(248, 250, 252),
        sf::Color(241, 245, 249),
        sf::Color(226, 232, 240),
        sf::Color(248, 250, 252)
    );
    btnShiftPrevPage->setTextColor(sf::Color(148, 163, 184));
    btnShiftPrevPage->setOnClick([this]() {
        if (shiftCurrentPage > 0) shiftCurrentPage--;
    });

    btnShiftNextPage = std::make_unique<RoundRectButton>(
        "Sau",
        sf::Vector2f(rightPanelX + rightPanelW - 66.f, 650.f),
        sf::Vector2f(46.f, 26.f),
        ButtonStyle::Secondary,
        5.f
    );
    btnShiftNextPage->setFontSize(10);
    btnShiftNextPage->setCustomColors(
        sf::Color(248, 250, 252),
        sf::Color(241, 245, 249),
        sf::Color(226, 232, 240),
        sf::Color(248, 250, 252)
    );
    btnShiftNextPage->setTextColor(sf::Color(148, 163, 184));
    btnShiftNextPage->setOnClick([this]() {
        shiftCurrentPage++;
    });

    reloadShiftReportData();
}

void CinemaUIApp::reloadShiftReportData() {
    // 1. Refresh Admin shift profiles from shift_managements + invoices/tickets/F&B in SQLite
    s_cachedAdminProfiles = loadAdminShiftProfilesFromDB();
    if (adminSelectedShiftIdx < 0 || adminSelectedShiftIdx >= static_cast<int>(s_cachedAdminProfiles.size())) {
        adminSelectedShiftIdx = 0;
    }
    if (!s_cachedAdminProfiles.empty()) {
        adminShiftApproved = (s_cachedAdminProfiles[adminSelectedShiftIdx].trangThai == "DaDuyet");
    }

    // Also refresh next cashier dropdown with real staff from staffUserList
    if (adminNextCashierDropdown && !staffUserList.empty()) {
        std::vector<std::string> nextCashierOpts;
        for (const auto& u : staffUserList) {
            if ((u.vaiTro == "Staff" || u.vaiTro == "Admin") && u.trangThaiLamViec != "DaNghi") {
                nextCashierOpts.push_back(u.hoTen + " (" + u.maNguoiDung + ")");
            }
        }
        if (!nextCashierOpts.empty()) {
            int curSel = adminNextCashierDropdown->getSelectedIndex();
            if (curSel < 0 || curSel >= static_cast<int>(nextCashierOpts.size())) curSel = 0;
            adminNextCashierDropdown->setOptions(nextCashierOpts, curSel);
        }
    }

    // 2. Calculate current Staff shift revenue filtered by current cashier, current date, and current shift window
    sqlite3* dbConn = nullptr;
    long long shiftTotalRev = 0;
    long long shiftCashRev = 0;

    std::string activeStaffId = (!currentUserId.empty() && currentUserRole == "Staff") ? currentUserId : "U002";
    std::string todayNorm = DateFormat::normalizeDate(getTodayDateStr());

    std::time_t tNow = std::time(nullptr);
    std::tm tmNow;
#ifdef _WIN32
    localtime_s(&tmNow, &tNow);
#else
    localtime_r(&tNow, &tmNow);
#endif
    int curHour = tmNow.tm_hour;
    int shiftStartSec = (curHour < 14) ? (7 * 3600) : (14 * 3600);
    int shiftEndSec = 24 * 3600 - 1;

    if (sqlite3_open("DATA/cinema.db", &dbConn) == SQLITE_OK) {
        // Check if this staff already closed an earlier shift today; if so, only count invoices after that shift's end time
        sqlite3_stmt* shStmt = nullptr;
        const char* qClosed = "SELECT maNhanVien, ngayLam, gioKetThuc FROM shift_managements;";
        if (sqlite3_prepare_v2(dbConn, qClosed, -1, &shStmt, nullptr) == SQLITE_OK) {
            while (sqlite3_step(shStmt) == SQLITE_ROW) {
                const char* cNv = reinterpret_cast<const char*>(sqlite3_column_text(shStmt, 0));
                const char* cDt = reinterpret_cast<const char*>(sqlite3_column_text(shStmt, 1));
                const char* cKt = reinterpret_cast<const char*>(sqlite3_column_text(shStmt, 2));
                std::string nv = cNv ? cNv : "";
                std::string dt = DateFormat::normalizeDate(cDt ? cDt : "");
                std::string kt = cKt ? cKt : "";
                if (dt == todayNorm && staffIdMatches(nv, activeStaffId)) {
                    int endS = parseTimeSeconds(kt, 0);
                    if (endS > shiftStartSec) shiftStartSec = endS + 1;
                }
            }
            sqlite3_finalize(shStmt);
        }

        // Sum invoices belonging ONLY to this staff member, on today's date, within the current shift's time range
        sqlite3_stmt* stmt = nullptr;
        const char* qInv = "SELECT maNhanVien, thoiGianLap, CAST(tongTien AS INTEGER), hinhThucThanhToan "
                           "FROM invoices WHERE trangThaiThanhToan != 'DaHuy';";
        if (sqlite3_prepare_v2(dbConn, qInv, -1, &stmt, nullptr) == SQLITE_OK) {
            while (sqlite3_step(stmt) == SQLITE_ROW) {
                const char* cNv = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
                const char* cDt = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
                long long amt = sqlite3_column_int64(stmt, 2);
                const char* cMethod = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));

                std::string invNv = cNv ? cNv : "";
                if (!staffIdMatches(invNv, activeStaffId)) continue;

                std::string dtFull = cDt ? cDt : "";
                std::string invDate, invTime = "12:00:00";
                size_t sp = dtFull.find(' ');
                if (sp != std::string::npos) {
                    invDate = DateFormat::normalizeDate(dtFull.substr(0, sp));
                    invTime = dtFull.substr(sp + 1);
                } else {
                    invDate = DateFormat::normalizeDate(dtFull);
                }
                if (invDate != todayNorm) continue;

                int invSec = parseTimeSeconds(invTime, 12 * 3600);
                if (invSec < shiftStartSec || invSec > shiftEndSec) continue;

                shiftTotalRev += amt;
                std::string method = cMethod ? cMethod : "";
                if (method == "TienMat" || method == "CASH") {
                    shiftCashRev += amt;
                }
            }
            sqlite3_finalize(stmt);
        }

        sqlite3_close(dbConn);
    }

    shiftCalculatedSysRev = shiftTotalRev;
    shiftCalculatedActualCash = shiftTotalRev;

    if (inputShiftActualCash) {
        inputShiftActualCash->setText(std::to_string(shiftCalculatedActualCash));
    }

    shiftCalculatedDiff = shiftCalculatedActualCash - shiftCalculatedSysRev;
}

void CinemaUIApp::renderShiftReportView() {
    float contentAreaW = 1728.f - sidebarWidth;
    float contentAreaH = 972.f;

    // Background Canvas (#f1f5f9 / #f8fafc)
    sf::RectangleShape bgRect(sf::Vector2f(contentAreaW, contentAreaH));
    bgRect.setPosition(sidebarWidth, 0.f);
    bgRect.setFillColor(sf::Color(241, 245, 249));
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

    // =========================================================================================
    // MODE A: ADMIN REVENUE RECONCILIATION & SHIFT CLOSING (media_1795197286781_0606a2d9.jpg)
    // =========================================================================================
    if ((currentUserRole == "Admin" || currentUserRole == "Khách") && !adminShowHistorySubView) {
        float startX = sidebarWidth + 18.f; // 268.f
        float contentW = 1728.f - startX - 18.f; // 1442.f

        const auto& profiles = getAdminShiftProfiles();
        const auto& p = profiles[adminSelectedShiftIdx % profiles.size()];
        adminShiftApproved = (p.trangThai == "DaDuyet");

        // -----------------------------------------------------------------
        // 1. TOP POS / SHIFT HEADER CARD (y = 10.f, h = 54.f)
        // -----------------------------------------------------------------
        float topY = 10.f;
        float topH = 54.f;
        RoundedRectRenderer::render(
            window,
            sf::Vector2f(startX, topY),
            sf::Vector2f(contentW, topH),
            10.f,
            sf::Color::White,
            1.f,
            sf::Color(226, 232, 240)
        );

        // Blue POS Monitor Icon Sticker
        RoundedRectRenderer::render(window, sf::Vector2f(startX + 14.f, topY + 9.f), sf::Vector2f(36.f, 36.f), 8.f, sf::Color(2, 132, 199));
        RoundedRectRenderer::render(window, sf::Vector2f(startX + 22.f, topY + 16.f), sf::Vector2f(20.f, 14.f), 2.f, sf::Color::White);
        RoundedRectRenderer::render(window, sf::Vector2f(startX + 24.f, topY + 18.f), sf::Vector2f(16.f, 10.f), 1.f, sf::Color(2, 132, 199));
        sf::RectangleShape monStand(sf::Vector2f(8.f, 3.f));
        monStand.setPosition(startX + 28.f, topY + 31.f);
        monStand.setFillColor(sf::Color::White);
        window.draw(monStand);
        sf::RectangleShape monBase(sf::Vector2f(14.f, 2.f));
        monBase.setPosition(startX + 25.f, topY + 34.f);
        monBase.setFillColor(sf::Color::White);
        window.draw(monBase);

        drawTxt("CINESTAR – POS (QUẦY", startX + 60.f, topY + 9.f, 13, sf::Color(15, 23, 42), true);
        drawTxt("NHÂN VIÊN)", startX + 60.f, topY + 27.f, 13, sf::Color(15, 23, 42), true);

        // Online Status Pill
        float pillOnlineX = startX + 266.f;
        RoundedRectRenderer::render(
            window,
            sf::Vector2f(pillOnlineX, topY + 9.f),
            sf::Vector2f(154.f, 36.f),
            18.f,
            sf::Color(236, 253, 245),
            1.f,
            sf::Color(167, 243, 208)
        );
        sf::CircleShape onlineDot(3.5f);
        onlineDot.setFillColor(sf::Color(16, 185, 129));
        onlineDot.setPosition(pillOnlineX + 12.f, topY + 24.f);
        window.draw(onlineDot);
        drawTxt("HỆ THỐNG TRỰC", pillOnlineX + 26.f, topY + 12.f, 10, sf::Color(5, 150, 105), true);
        drawTxt("TUYẾN", pillOnlineX + 26.f, topY + 26.f, 10, sf::Color(5, 150, 105), true);

        // Calendar Sticker & Date
        float calX = startX + 438.f;
        RoundedRectRenderer::render(window, sf::Vector2f(calX, topY + 20.f), sf::Vector2f(14.f, 14.f), 2.f, sf::Color::White, 1.3f, sf::Color(100, 116, 139));
        sf::RectangleShape calBar(sf::Vector2f(14.f, 4.f));
        calBar.setPosition(calX, topY + 20.f);
        calBar.setFillColor(sf::Color(100, 116, 139));
        window.draw(calBar);

        drawTxt("Hôm nay, Thứ Bảy", calX + 22.f, topY + 11.f, 11, sf::Color(71, 85, 105));
        drawTxt(p.ngay, calX + 22.f, topY + 27.f, 11.5, sf::Color(30, 41, 59), true);

        // Shift Info
        float shInfoX = startX + 610.f;
        drawTxt("Ca: " + p.tenCa.substr(3), shInfoX, topY + 11.f, 11.5, sf::Color(180, 83, 9), true);
        drawTxt("(" + p.quay + ")", shInfoX, topY + 27.f, 11.5, sf::Color(180, 83, 9), true);

        // Cashier Info
        float cashInfoX = startX + 716.f;
        drawTxt("Thu ngân: " + p.thuNgan, cashInfoX, topY + 19.f, 11.5, sf::Color(3, 105, 161), true);

        // Clock Box
        float clkBoxX = startX + 930.f;
        RoundedRectRenderer::render(
            window,
            sf::Vector2f(clkBoxX, topY + 11.f),
            sf::Vector2f(96.f, 32.f),
            6.f,
            sf::Color(241, 245, 249),
            1.f,
            sf::Color(203, 213, 225)
        );
        drawCenteredTxt(getCurrentTimeStr(), clkBoxX + 48.f, topY + 27.f, 12, sf::Color(15, 23, 42), true);

        if (btnAdminShiftRefresh) btnAdminShiftRefresh->draw(window);
        if (btnAdminShiftExit) btnAdminShiftExit->draw(window);

        // -----------------------------------------------------------------
        // 2. SECOND HEADER BANNER CARD (y = 72.f, h = 64.f)
        // -----------------------------------------------------------------
        float banY = 72.f;
        float banH = 64.f;
        RoundedRectRenderer::render(
            window,
            sf::Vector2f(startX, banY),
            sf::Vector2f(contentW, banH),
            10.f,
            sf::Color::White,
            1.f,
            sf::Color(226, 232, 240)
        );

        // Clipboard Check Sticker Badge
        RoundedRectRenderer::render(
            window,
            sf::Vector2f(startX + 18.f, banY + 13.f),
            sf::Vector2f(38.f, 38.f),
            8.f,
            sf::Color(240, 249, 255),
            1.f,
            sf::Color(186, 230, 253)
        );
        RoundedRectRenderer::render(window, sf::Vector2f(startX + 29.f, banY + 21.f), sf::Vector2f(16.f, 22.f), 2.f, sf::Color::White, 1.5f, sf::Color(2, 132, 199));
        RoundedRectRenderer::render(window, sf::Vector2f(startX + 33.f, banY + 19.f), sf::Vector2f(8.f, 4.f), 1.f, sf::Color(2, 132, 199));
        {
            sf::RectangleShape arm1(sf::Vector2f(4.f, 1.8f));
            arm1.setRotation(45.f);
            arm1.setPosition(startX + 33.5f, banY + 31.5f);
            arm1.setFillColor(sf::Color(2, 132, 199));
            sf::RectangleShape arm2(sf::Vector2f(7.5f, 1.8f));
            arm2.setRotation(-50.f);
            arm2.setPosition(startX + 36.3f, banY + 34.3f);
            arm2.setFillColor(sf::Color(2, 132, 199));
            window.draw(arm1);
            window.draw(arm2);
        }

        drawTxt("BÁO CÁO KẾT CA & ĐỐI SOÁT DOANH THU", startX + 70.f, banY + 11.f, 15.5, sf::Color(15, 23, 42), true);

        // Pending / Approved Status Pill next to Title
        float stPillX = startX + 430.f;
        RoundedRectRenderer::render(
            window,
            sf::Vector2f(stPillX, banY + 11.f),
            sf::Vector2f(164.f, 23.f),
            11.5f,
            adminShiftApproved ? sf::Color(220, 252, 231) : sf::Color(254, 243, 199),
            1.f,
            adminShiftApproved ? sf::Color(167, 243, 208) : sf::Color(253, 230, 138)
        );
        sf::CircleShape stPillDot(3.5f);
        stPillDot.setFillColor(adminShiftApproved ? sf::Color(22, 163, 74) : sf::Color(217, 119, 6));
        stPillDot.setPosition(stPillX + 11.f, banY + 19.f);
        window.draw(stPillDot);
        drawTxt(
            adminShiftApproved ? "Đã duyệt chốt đối soát" : "Đang chờ chốt đối soát",
            stPillX + 22.f,
            banY + 15.f,
            10.5,
            adminShiftApproved ? sf::Color(21, 128, 61) : sf::Color(180, 83, 9),
            true
        );

        std::string subLine = "Cinestar Cinema  •  " + p.quay + "  •  " + p.tenCa + "   •   Thu ngân: " + p.thuNgan + " (" + p.maNV + ")   •   Giờ trực: " + p.khungGio + "  •  " + p.ngay;
        drawTxt(subLine, startX + 70.f, banY + 39.f, 11, sf::Color(71, 85, 105));

        if (btnAdminShiftHistory) btnAdminShiftHistory->draw(window);

        // -----------------------------------------------------------------
        // 3. FOUR KPI STAT CARDS ROW (y = 144.f, h = 124.f)
        // -----------------------------------------------------------------
        float kpiY = 144.f;
        float kpiH = 124.f;
        float kpiGap = 14.f;
        float kpiW = (contentW - 3.f * kpiGap) / 4.f; // 350.f

        // --- KPI Card 1: TỔNG DOANH THU CA ---
        {
            float cx = startX;
            RoundedRectRenderer::render(window, sf::Vector2f(cx, kpiY), sf::Vector2f(kpiW, kpiH), 10.f, sf::Color::White, 1.f, sf::Color(226, 232, 240));
            drawTxt("TỔNG DOANH THU CA", cx + 18.f, kpiY + 14.f, 11, sf::Color(100, 116, 139), true);

            // Banknote Sticker Badge
            RoundedRectRenderer::render(window, sf::Vector2f(cx + kpiW - 48.f, kpiY + 12.f), sf::Vector2f(32.f, 32.f), 8.f, sf::Color(240, 249, 255), 1.f, sf::Color(186, 230, 253));
            RoundedRectRenderer::render(window, sf::Vector2f(cx + kpiW - 41.f, kpiY + 22.f), sf::Vector2f(18.f, 12.f), 2.f, sf::Color::White, 1.3f, sf::Color(2, 132, 199));
            sf::CircleShape billCir(2.5f);
            billCir.setFillColor(sf::Color(2, 132, 199));
            billCir.setPosition(cx + kpiW - 34.5f, kpiY + 25.5f);
            window.draw(billCir);

            std::string revStr = formatDotNumber(p.tongDoanhThu);
            drawTxt(revStr, cx + 18.f, kpiY + 38.f, 22, sf::Color(15, 23, 42), true);
            drawTxt("VNĐ", cx + 26.f + static_cast<float>(revStr.size()) * 12.5f, kpiY + 47.f, 10.5, sf::Color(100, 116, 139), true);

            drawTxt(std::to_string(p.tongLuotGD) + " lượt giao dịch", cx + 18.f, kpiY + 80.f, 11, sf::Color(100, 116, 139));

            RoundedRectRenderer::render(window, sf::Vector2f(cx + kpiW - 108.f, kpiY + 77.f), sf::Vector2f(90.f, 22.f), 6.f, sf::Color(220, 252, 231));
            drawCenteredTxt("↗ " + std::to_string(p.phanTramTarget) + "% Target", cx + kpiW - 63.f, kpiY + 88.f, 10, sf::Color(21, 128, 61), true);

            // Progress bar
            RoundedRectRenderer::render(window, sf::Vector2f(cx + 18.f, kpiY + 108.f), sf::Vector2f(kpiW - 36.f, 5.f), 2.5f, sf::Color(226, 232, 240));
            RoundedRectRenderer::render(window, sf::Vector2f(cx + 18.f, kpiY + 108.f), sf::Vector2f((kpiW - 36.f) * 0.82f, 5.f), 2.5f, sf::Color(2, 132, 199));
        }

        // --- KPI Card 2: DOANH THU VÉ XEM PHIM ---
        {
            float cx = startX + (kpiW + kpiGap);
            RoundedRectRenderer::render(window, sf::Vector2f(cx, kpiY), sf::Vector2f(kpiW, kpiH), 10.f, sf::Color::White, 1.f, sf::Color(226, 232, 240));
            drawTxt("DOANH THU VÉ XEM PHIM", cx + 18.f, kpiY + 14.f, 11, sf::Color(100, 116, 139), true);

            // Ticket Sticker Badge
            RoundedRectRenderer::render(window, sf::Vector2f(cx + kpiW - 48.f, kpiY + 12.f), sf::Vector2f(32.f, 32.f), 8.f, sf::Color(240, 249, 255), 1.f, sf::Color(186, 230, 253));
            RoundedRectRenderer::render(window, sf::Vector2f(cx + kpiW - 41.f, kpiY + 22.f), sf::Vector2f(18.f, 12.f), 2.f, sf::Color::White, 1.3f, sf::Color(2, 132, 199));
            sf::RectangleShape tkPerf(sf::Vector2f(1.2f, 8.f));
            tkPerf.setPosition(cx + kpiW - 29.f, kpiY + 24.f);
            tkPerf.setFillColor(sf::Color(2, 132, 199));
            window.draw(tkPerf);

            std::string vStr = formatDotNumber(p.doanhThuVe);
            drawTxt(vStr, cx + 18.f, kpiY + 38.f, 22, sf::Color(2, 132, 199), true);
            drawTxt("VNĐ", cx + 26.f + static_cast<float>(vStr.size()) * 12.5f, kpiY + 47.f, 10.5, sf::Color(100, 116, 139), true);

            drawTxt("Tổng số vé: " + std::to_string(p.tongSoVe) + " vé", cx + 18.f, kpiY + 72.f, 11, sf::Color(71, 85, 105), true);
            drawRightTxt("69.6% tổng ca", cx + kpiW - 18.f, kpiY + 72.f, 11, sf::Color(148, 163, 184));

            // 3 Seat Category Pills cleanly aligned without overlap
            float pY = kpiY + 94.f;
            RoundedRectRenderer::render(window, sf::Vector2f(cx + 18.f, pY), sf::Vector2f(88.f, 22.f), 5.f, sf::Color(254, 249, 195), 1.f, sf::Color(253, 224, 71));
            drawCenteredTxt("VIP: " + std::to_string(p.veVip), cx + 18.f + 44.f, pY + 11.f, 10.5, sf::Color(161, 98, 7), true);

            RoundedRectRenderer::render(window, sf::Vector2f(cx + 116.f, pY), sf::Vector2f(104.f, 22.f), 5.f, sf::Color(241, 245, 249), 1.f, sf::Color(203, 213, 225));
            drawCenteredTxt("Thường: " + std::to_string(p.veThuong), cx + 116.f + 52.f, pY + 11.f, 10.5, sf::Color(51, 65, 85), true);

            RoundedRectRenderer::render(window, sf::Vector2f(cx + 230.f, pY), sf::Vector2f(102.f, 22.f), 5.f, sf::Color(252, 231, 243), 1.f, sf::Color(249, 168, 212));
            drawCenteredTxt("Sweetbox: " + std::to_string(p.veSweetbox), cx + 230.f + 51.f, pY + 11.f, 10.5, sf::Color(190, 24, 93), true);
        }

        // --- KPI Card 3: DOANH THU BẮP NƯỚC (F&B) ---
        {
            float cx = startX + 2.f * (kpiW + kpiGap);
            RoundedRectRenderer::render(window, sf::Vector2f(cx, kpiY), sf::Vector2f(kpiW, kpiH), 10.f, sf::Color::White, 1.f, sf::Color(226, 232, 240));
            drawTxt("DOANH THU BẮP NƯỚC (F&B)", cx + 18.f, kpiY + 14.f, 11, sf::Color(100, 116, 139), true);

            // Popcorn / Cup Sticker Badge
            RoundedRectRenderer::render(window, sf::Vector2f(cx + kpiW - 48.f, kpiY + 12.f), sf::Vector2f(32.f, 32.f), 8.f, sf::Color(254, 243, 199), 1.f, sf::Color(253, 230, 138));
            RoundedRectRenderer::render(window, sf::Vector2f(cx + kpiW - 38.f, kpiY + 21.f), sf::Vector2f(12.f, 14.f), 2.f, sf::Color::White, 1.3f, sf::Color(217, 119, 6));
            sf::RectangleShape straw(sf::Vector2f(6.f, 1.5f));
            straw.setPosition(cx + kpiW - 32.f, kpiY + 18.f);
            straw.setFillColor(sf::Color(217, 119, 6));
            window.draw(straw);

            std::string fStr = formatDotNumber(p.doanhThuFnb);
            drawTxt(fStr, cx + 18.f, kpiY + 38.f, 22, sf::Color(217, 119, 6), true);
            drawTxt("VNĐ", cx + 26.f + static_cast<float>(fStr.size()) * 12.5f, kpiY + 47.f, 10.5, sf::Color(100, 116, 139), true);

            drawTxt(std::to_string(p.soComboMonLe) + " combo / món lẻ", cx + 18.f, kpiY + 80.f, 11, sf::Color(100, 116, 139));
            drawRightTxt("30.4% tổng ca", cx + kpiW - 18.f, kpiY + 80.f, 11, sf::Color(148, 163, 184));

            // Progress bar
            RoundedRectRenderer::render(window, sf::Vector2f(cx + 18.f, kpiY + 108.f), sf::Vector2f(kpiW - 36.f, 5.f), 2.5f, sf::Color(226, 232, 240));
            RoundedRectRenderer::render(window, sf::Vector2f(cx + 18.f, kpiY + 108.f), sf::Vector2f((kpiW - 36.f) * 0.34f, 5.f), 2.5f, sf::Color(245, 158, 11));
        }

        // --- KPI Card 4: VOUCHER / GIẢM GIÁ ---
        {
            float cx = startX + 3.f * (kpiW + kpiGap);
            RoundedRectRenderer::render(window, sf::Vector2f(cx, kpiY), sf::Vector2f(kpiW, kpiH), 10.f, sf::Color::White, 1.f, sf::Color(226, 232, 240));
            drawTxt("VOUCHER / GIẢM GIÁ", cx + 18.f, kpiY + 14.f, 11, sf::Color(100, 116, 139), true);

            // Discount Tag Sticker Badge
            RoundedRectRenderer::render(window, sf::Vector2f(cx + kpiW - 48.f, kpiY + 12.f), sf::Vector2f(32.f, 32.f), 8.f, sf::Color(255, 241, 242), 1.f, sf::Color(254, 205, 211));
            RoundedRectRenderer::render(window, sf::Vector2f(cx + kpiW - 40.f, kpiY + 21.f), sf::Vector2f(16.f, 13.f), 3.f, sf::Color::White, 1.3f, sf::Color(225, 29, 72));
            sf::CircleShape tagHole(1.8f);
            tagHole.setFillColor(sf::Color(225, 29, 72));
            tagHole.setPosition(cx + kpiW - 37.f, kpiY + 25.f);
            window.draw(tagHole);

            std::string vcStr = "-" + formatDotNumber(p.voucherGiamGia);
            drawTxt(vcStr, cx + 18.f, kpiY + 38.f, 22, sf::Color(225, 29, 72), true);
            drawTxt("VNĐ", cx + 26.f + static_cast<float>(vcStr.size()) * 12.2f, kpiY + 47.f, 10.5, sf::Color(100, 116, 139), true);

            drawTxt(std::to_string(p.soLuotVoucher) + " lượt voucher & điểm", cx + 18.f, kpiY + 80.f, 11, sf::Color(100, 116, 139));

            RoundedRectRenderer::render(window, sf::Vector2f(cx + kpiW - 102.f, kpiY + 77.f), sf::Vector2f(84.f, 22.f), 6.f, sf::Color(255, 241, 242));
            drawCenteredTxt("Đã đối soát", cx + kpiW - 60.f, kpiY + 88.f, 10, sf::Color(225, 29, 72), true);

            // Progress bar
            RoundedRectRenderer::render(window, sf::Vector2f(cx + 18.f, kpiY + 108.f), sf::Vector2f(kpiW - 36.f, 5.f), 2.5f, sf::Color(226, 232, 240));
            RoundedRectRenderer::render(window, sf::Vector2f(cx + 18.f, kpiY + 108.f), sf::Vector2f((kpiW - 36.f) * 0.48f, 5.f), 2.5f, sf::Color(244, 63, 94));
        }

        // -----------------------------------------------------------------
        // 4. LEFT COLUMN (x = 268.f, w = 846.f)
        // -----------------------------------------------------------------
        float leftW = 846.f;

        // === 4.1 TOP LEFT CARD: BẢNG ĐỐI SOÁT PHƯƠNG THỨC THANH TOÁN (y = 278.f, h = 340.f) ===
        float payCardY = 278.f;
        float payCardH = 340.f;
        RoundedRectRenderer::render(
            window,
            sf::Vector2f(startX, payCardY),
            sf::Vector2f(leftW, payCardH),
            10.f,
            sf::Color::White,
            1.f,
            sf::Color(226, 232, 240)
        );

        // Wallet Sticker + Header Title
        RoundedRectRenderer::render(window, sf::Vector2f(startX + 18.f, payCardY + 16.f), sf::Vector2f(20.f, 15.f), 3.f, sf::Color(224, 242, 254), 1.4f, sf::Color(2, 132, 199));
        sf::CircleShape wSnap(2.f);
        wSnap.setFillColor(sf::Color(2, 132, 199));
        wSnap.setPosition(startX + 31.f, payCardY + 21.5f);
        window.draw(wSnap);

        drawTxt("BẢNG ĐỐI SOÁT PHƯƠNG THỨC THANH TOÁN", startX + 46.f, payCardY + 14.f, 13.5, sf::Color(15, 23, 42), true);

        bool isShiftBalanced = (p.chenhLech == 0);
        std::string auditStatusStr = isShiftBalanced
            ? "Trạng thái: 100% Khớp số liệu"
            : ("Trạng thái: Lệch " + std::string(p.chenhLech > 0 ? "+" : "") + formatDongShort(p.chenhLech));
        RoundedRectRenderer::render(
            window,
            sf::Vector2f(startX + leftW - 226.f, payCardY + 12.f),
            sf::Vector2f(208.f, 24.f),
            12.f,
            isShiftBalanced ? sf::Color(220, 252, 231) : sf::Color(254, 243, 199),
            1.f,
            isShiftBalanced ? sf::Color(167, 243, 208) : sf::Color(253, 230, 138)
        );
        drawCenteredTxt(auditStatusStr, startX + leftW - 122.f, payCardY + 24.f, 10.5, isShiftBalanced ? sf::Color(21, 128, 61) : sf::Color(180, 83, 9), true);

        // Table Header Bar (y = 326.f, h = 38.f)
        float thY = payCardY + 48.f;
        float thH = 38.f;
        sf::RectangleShape thBg(sf::Vector2f(leftW - 2.f, thH));
        thBg.setPosition(startX + 1.f, thY);
        thBg.setFillColor(sf::Color(248, 250, 252));
        window.draw(thBg);

        sf::RectangleShape thLineTop(sf::Vector2f(leftW - 2.f, 1.f));
        thLineTop.setPosition(startX + 1.f, thY);
        thLineTop.setFillColor(sf::Color(226, 232, 240));
        window.draw(thLineTop);

        sf::RectangleShape thLineBot(sf::Vector2f(leftW - 2.f, 1.f));
        thLineBot.setPosition(startX + 1.f, thY + thH);
        thLineBot.setFillColor(sf::Color(226, 232, 240));
        window.draw(thLineBot);

        float col1X = startX + 18.f;       // PHƯƠNG THỨC
        float col2CX = startX + 310.f;     // DOANH THU HT GHI NHẬN (Centered)
        float col3CX = startX + 472.f;     // THỰC TẾ ĐẾM / SAO KÊ (Centered)
        float col4CX = startX + 602.f;     // CHÊNH LỆCH (Centered)
        float col5X = startX + 668.f;      // CHỨNG TỪ / GHI CHÚ

        drawTxt("PHƯƠNG THỨC", col1X, thY + 11.f, 10.5, sf::Color(100, 116, 139), true);
        drawCenteredTxt("DOANH THU HT GHI NHẬN", col2CX, thY + 19.f, 10.5, sf::Color(100, 116, 139), true);
        drawCenteredTxt("THỰC TẾ ĐẾM / SAO KÊ", col3CX, thY + 19.f, 10.5, sf::Color(100, 116, 139), true);
        drawCenteredTxt("CHÊNH LỆCH", col4CX, thY + 19.f, 10.5, sf::Color(100, 116, 139), true);
        drawTxt("CHỨNG TỪ / GHI CHÚ", col5X, thY + 11.f, 10.5, sf::Color(100, 116, 139), true);

        struct PaymentAuditRow {
            std::string title;
            std::string sub;
            long long sysAmt;
            long long actAmt;
            std::string note;
            int iconType; // 0: Cash, 1: QR, 2: Card
        };

        std::vector<PaymentAuditRow> payRows = {
            {"Tiền mặt (Cash)", "Tiền giấy tại két",       p.tienMatHT,  p.tienMatThucTe,  "Đã đếm két tiền", 0},
            {"Quét mã QR",      "VietQR, MoMo, VNPay",     p.quetQrHT,   p.quetQrThucTe,   "Ngân hàng báo có " + std::to_string(p.soGdQR) + " GD", 1},
            {"Cà thẻ POS EDC",  "Visa, Mastercard, Napas", p.caThePosHT, p.caThePosThucTe, "Bill settlement kết ca EDC " + p.maEdc, 2}
        };

        float rowBaseY = thY + thH;
        float payRowH = 66.f;

        for (size_t i = 0; i < payRows.size(); ++i) {
            float ry = rowBaseY + static_cast<float>(i) * payRowH;
            const auto& pr = payRows[i];

            sf::RectangleShape rLine(sf::Vector2f(leftW - 36.f, 1.f));
            rLine.setPosition(startX + 18.f, ry + payRowH);
            rLine.setFillColor(sf::Color(241, 245, 249));
            window.draw(rLine);

            // Method Icon Sticker
            if (pr.iconType == 0) {
                RoundedRectRenderer::render(window, sf::Vector2f(col1X, ry + 17.f), sf::Vector2f(32.f, 32.f), 8.f, sf::Color(220, 252, 231));
                RoundedRectRenderer::render(window, sf::Vector2f(col1X + 7.f, ry + 27.f), sf::Vector2f(18.f, 12.f), 2.f, sf::Color::White, 1.3f, sf::Color(21, 128, 61));
                sf::CircleShape cDot(2.2f);
                cDot.setFillColor(sf::Color(21, 128, 61));
                cDot.setPosition(col1X + 13.8f, ry + 30.8f);
                window.draw(cDot);
            } else if (pr.iconType == 1) {
                RoundedRectRenderer::render(window, sf::Vector2f(col1X, ry + 17.f), sf::Vector2f(32.f, 32.f), 8.f, sf::Color(224, 242, 254));
                RoundedRectRenderer::render(window, sf::Vector2f(col1X + 8.f, ry + 25.f), sf::Vector2f(6.f, 6.f), 1.f, sf::Color(2, 132, 199));
                RoundedRectRenderer::render(window, sf::Vector2f(col1X + 18.f, ry + 25.f), sf::Vector2f(6.f, 6.f), 1.f, sf::Color(2, 132, 199));
                RoundedRectRenderer::render(window, sf::Vector2f(col1X + 8.f, ry + 35.f), sf::Vector2f(6.f, 6.f), 1.f, sf::Color(2, 132, 199));
                RoundedRectRenderer::render(window, sf::Vector2f(col1X + 18.f, ry + 35.f), sf::Vector2f(6.f, 6.f), 1.f, sf::Color(2, 132, 199));
            } else {
                RoundedRectRenderer::render(window, sf::Vector2f(col1X, ry + 17.f), sf::Vector2f(32.f, 32.f), 8.f, sf::Color(238, 242, 255));
                RoundedRectRenderer::render(window, sf::Vector2f(col1X + 7.f, ry + 26.f), sf::Vector2f(18.f, 13.f), 2.f, sf::Color::White, 1.3f, sf::Color(79, 70, 229));
                sf::RectangleShape cStripe(sf::Vector2f(18.f, 3.f));
                cStripe.setPosition(col1X + 7.f, ry + 29.f);
                cStripe.setFillColor(sf::Color(79, 70, 229));
                window.draw(cStripe);
            }

            drawTxt(pr.title, col1X + 42.f, ry + 15.f, 12.5, sf::Color(15, 23, 42), true);
            drawTxt(pr.sub, col1X + 42.f, ry + 34.f, 10.5, sf::Color(148, 163, 184));

            drawCenteredTxt(formatDongShort(pr.sysAmt), col2CX, ry + 33.f, 12.5, sf::Color(15, 23, 42), true);
            drawCenteredTxt(formatDongShort(pr.actAmt), col3CX, ry + 33.f, 12.5, sf::Color(15, 23, 42), true);

            // Chênh lệch pill
            double rowDiff = pr.actAmt - pr.sysAmt;
            if (std::abs(rowDiff) < 0.5) {
                RoundedRectRenderer::render(
                    window,
                    sf::Vector2f(col4CX - 36.f, ry + 21.f),
                    sf::Vector2f(72.f, 24.f),
                    12.f,
                    sf::Color(220, 252, 231),
                    1.f,
                    sf::Color(167, 243, 208)
                );
                // Crisp vector checkmark sticker before 0 đ
                float iconCX = col4CX - 15.f;
                float iconCY = ry + 32.f;
                sf::RectangleShape ck1(sf::Vector2f(4.2f, 1.8f));
                ck1.setRotation(45.f);
                ck1.setPosition(iconCX - 4.f, iconCY);
                ck1.setFillColor(sf::Color(21, 128, 61));
                sf::RectangleShape ck2(sf::Vector2f(7.5f, 1.8f));
                ck2.setRotation(-50.f);
                ck2.setPosition(iconCX - 1.f, iconCY + 3.f);
                ck2.setFillColor(sf::Color(21, 128, 61));
                window.draw(ck1);
                window.draw(ck2);
                drawTxt("0 đ", iconCX + 7.f, ry + 25.5f, 11, sf::Color(21, 128, 61), true);
            } else {
                bool isPositive = (rowDiff > 0);
                sf::Color bgCol = isPositive ? sf::Color(254, 243, 199) : sf::Color(254, 226, 226);
                sf::Color brCol = isPositive ? sf::Color(253, 230, 138) : sf::Color(254, 202, 202);
                sf::Color txCol = isPositive ? sf::Color(180, 83, 9) : sf::Color(220, 38, 38);
                RoundedRectRenderer::render(
                    window,
                    sf::Vector2f(col4CX - 45.f, ry + 21.f),
                    sf::Vector2f(90.f, 24.f),
                    12.f,
                    bgCol,
                    1.f,
                    brCol
                );
                std::string diffStr = (isPositive ? "+" : "") + formatDongShort(rowDiff);
                drawCenteredTxt(diffStr, col4CX, ry + 33.f, 11, txCol, true);
            }

            drawTxt(pr.note, col5X, ry + 25.f, 11, sf::Color(100, 116, 139));
        }

        // Table Footer Summary Row (y = 562.f, h = 54.f)
        float tfY = rowBaseY + 3.f * payRowH;
        float tfH = payCardH - (tfY - payCardY) - 2.f;
        RoundedRectRenderer::render(
            window,
            sf::Vector2f(startX + 2.f, tfY),
            sf::Vector2f(leftW - 4.f, tfH),
            8.f,
            sf::Color(248, 250, 252)
        );
        drawTxt("TỔNG CỘNG DOANH THU", col1X, tfY + 18.f, 12, sf::Color(15, 23, 42), true);
        drawCenteredTxt(formatDongShort(p.tongDoanhThu), col2CX, tfY + 26.f, 13, sf::Color(2, 132, 199), true);
        drawCenteredTxt(formatDongShort(p.tienThucTe), col3CX, tfY + 26.f, 13, sf::Color(2, 132, 199), true);
        if (std::abs(p.chenhLech) < 0.5) {
            drawCenteredTxt("0 đ (Trùng khớp)", col4CX, tfY + 26.f, 11.5, sf::Color(21, 128, 61), true);
        } else {
            std::string st = formatDongShort(p.chenhLech) + (p.chenhLech > 0 ? " (Dư)" : " (Thiếu)");
            drawCenteredTxt(st, col4CX, tfY + 26.f, 11.5, sf::Color(220, 38, 38), true);
        }
        drawTxt("3 nguồn thanh toán", col5X, tfY + 18.f, 11.5, sf::Color(51, 65, 85), true);

        // -----------------------------------------------------------------
        // 5. RIGHT COLUMN (x = 1128.f, w = 582.f)
        // -----------------------------------------------------------------
        float rightX = startX + leftW + 14.f; // 1128.f
        float rightW = contentW - leftW - 14.f; // 582.f

        // === 5.1 TOP RIGHT CARD: BÁN THEO SUẤT CHIẾU TRONG CA (y = 278.f, h = 414.f) ===
        float stCardY = 278.f;
        float stCardH = 414.f;
        RoundedRectRenderer::render(
            window,
            sf::Vector2f(rightX, stCardY),
            sf::Vector2f(rightW, stCardH),
            10.f,
            sf::Color::White,
            1.f,
            sf::Color(226, 232, 240)
        );

        // Clapperboard Sticker + Title
        RoundedRectRenderer::render(window, sf::Vector2f(rightX + 18.f, stCardY + 15.f), sf::Vector2f(18.f, 15.f), 2.f, sf::Color(224, 242, 254), 1.3f, sf::Color(2, 132, 199));
        sf::RectangleShape clapTop(sf::Vector2f(18.f, 4.f));
        clapTop.setPosition(rightX + 18.f, stCardY + 15.f);
        clapTop.setFillColor(sf::Color(2, 132, 199));
        window.draw(clapTop);

        drawTxt("BÁN THEO SUẤT CHIẾU TRONG CA", rightX + 44.f, stCardY + 14.f, 13.5, sf::Color(15, 23, 42), true);
        std::string movieCountStr = std::to_string(p.mvStats.size()) + " tựa phim";
        drawRightTxt(movieCountStr, rightX + rightW - 18.f, stCardY + 16.f, 11.5, sf::Color(100, 116, 139));

        std::vector<AdminShiftMovieStat> mvStats = p.mvStats;
        if (mvStats.empty()) {
            mvStats = {
                {"Không có giao dịch vé", "0 suất  •  0 vé", "0 đ", "0% vé", false}
            };
        }

        float mvStartY = stCardY + 46.f;
        float mvH = 52.f;
        float mvGap = 8.f;

        for (size_t i = 0; i < mvStats.size(); ++i) {
            float my = mvStartY + static_cast<float>(i) * (mvH + mvGap);
            RoundedRectRenderer::render(
                window,
                sf::Vector2f(rightX + 18.f, my),
                sf::Vector2f(rightW - 36.f, mvH),
                8.f,
                sf::Color(248, 250, 252),
                1.f,
                sf::Color(241, 245, 249)
            );

            drawTxt(mvStats[i].title, rightX + 32.f, my + 8.f, 12.5, sf::Color(15, 23, 42), true);
            drawTxt(mvStats[i].detail, rightX + 32.f, my + 28.f, 11, sf::Color(71, 85, 105));

            drawRightTxt(mvStats[i].revenue, rightX + rightW - 32.f, my + 8.f, 12.5, sf::Color(15, 23, 42), true);
            drawRightTxt(
                mvStats[i].share,
                rightX + rightW - 32.f,
                my + 28.f,
                10.5,
                mvStats[i].highlightShare ? sf::Color(22, 163, 74) : sf::Color(100, 116, 139),
                mvStats[i].highlightShare
            );
        }

        // Best-Selling F&B Sub-Section inside Top Right Card (y = 572.f)
        float fnbSecY = stCardY + 294.f;
        drawTxt("BẮP NƯỚC BÁN CHẠY NHẤT TRONG CA:", rightX + 18.f, fnbSecY, 11, sf::Color(100, 116, 139), true);

        std::vector<AdminTopFnbStat> topFnbs = p.topFnbs;
        if (topFnbs.empty()) {
            topFnbs = {
                {"0 phần", "Không có F&B"},
                {"0 phần", "Không có F&B"},
                {"0 phần", "Không có F&B"}
            };
        }

        float fCardY = fnbSecY + 24.f;
        float fCardH = 60.f;
        float fCardGap = 12.f;
        float fCardW = (rightW - 36.f - 2.f * fCardGap) / 3.f; // 174.f

        for (size_t i = 0; i < topFnbs.size(); ++i) {
            float fx = rightX + 18.f + static_cast<float>(i) * (fCardW + fCardGap);
            RoundedRectRenderer::render(
                window,
                sf::Vector2f(fx, fCardY),
                sf::Vector2f(fCardW, fCardH),
                8.f,
                sf::Color(254, 252, 232),
                1.f,
                sf::Color(254, 240, 138)
            );
            drawCenteredTxt(topFnbs[i].qty, fx + fCardW * 0.5f, fCardY + 20.f, 13, sf::Color(180, 83, 9), true);
            drawCenteredTxt(topFnbs[i].name, fx + fCardW * 0.5f, fCardY + 41.f, 11, sf::Color(100, 116, 139));
        }

        // === 5.2 BOTTOM RIGHT CARD: XÁC NHẬN CHỐT CA & BÀN GIAO (y = 702.f, h = 256.f) ===
        float cfCardY = 702.f;
        float cfCardH = 256.f;
        RoundedRectRenderer::render(
            window,
            sf::Vector2f(rightX, cfCardY),
            sf::Vector2f(rightW, cfCardH),
            10.f,
            sf::Color::White,
            1.f,
            sf::Color(226, 232, 240)
        );

        // Shield Check Sticker + Title
        sf::CircleShape shCir(9.f);
        shCir.setFillColor(sf::Color(220, 252, 231));
        shCir.setOutlineThickness(1.3f);
        shCir.setOutlineColor(sf::Color(22, 163, 74));
        shCir.setPosition(rightX + 18.f, cfCardY + 13.f);
        window.draw(shCir);
        {
            sf::RectangleShape ck1(sf::Vector2f(4.f, 1.8f));
            ck1.setRotation(45.f);
            ck1.setPosition(rightX + 23.5f, cfCardY + 20.5f);
            ck1.setFillColor(sf::Color(21, 128, 61));
            sf::RectangleShape ck2(sf::Vector2f(7.5f, 1.8f));
            ck2.setRotation(-50.f);
            ck2.setPosition(rightX + 26.3f, cfCardY + 23.3f);
            ck2.setFillColor(sf::Color(21, 128, 61));
            window.draw(ck1);
            window.draw(ck2);
        }

        drawTxt("XÁC NHẬN CHỐT CA & BÀN GIAO", rightX + 44.f, cfCardY + 14.f, 13.5, sf::Color(15, 23, 42), true);

        drawTxt("Ghi chú ca làm việc:", rightX + 18.f, cfCardY + 42.f, 11, sf::Color(100, 116, 139));
        drawRightTxt("Tất cả số liệu khớp", rightX + rightW - 18.f, cfCardY + 42.f, 11, sf::Color(22, 163, 74), true);

        // Note Box
        float noteY = cfCardY + 62.f;
        RoundedRectRenderer::render(
            window,
            sf::Vector2f(rightX + 18.f, noteY),
            sf::Vector2f(rightW - 36.f, 46.f),
            6.f,
            sf::Color(248, 250, 252),
            1.f,
            sf::Color(226, 232, 240)
        );
        drawTxt("Ca sáng vận hành ổn định, không có sự cố kỹ thuật, két tiền mặt và biên lai thẻ POS", rightX + 28.f, noteY + 7.f, 11, sf::Color(30, 41, 59));
        drawTxt("trùng khớp 100%.", rightX + 28.f, noteY + 25.f, 11, sf::Color(30, 41, 59));

        // Form Labels
        drawTxt("Nhân viên nhận ca sau:", rightX + 18.f, cfCardY + 116.f, 11, sf::Color(100, 116, 139));
        drawTxt("Mã PIN Quản lý duyệt:", rightX + 342.f, cfCardY + 116.f, 11, sf::Color(100, 116, 139));

        if (adminNextCashierDropdown) adminNextCashierDropdown->draw(window);
        if (inputAdminManagerPin) inputAdminManagerPin->draw(window);

        if (btnAdminCashierHandover) btnAdminCashierHandover->draw(window);
        if (btnAdminManagerApprove) btnAdminManagerApprove->draw(window);

        return;
    }

    // =========================================================================================
    // MODE B: STAFF SHIFT REPORT & AUDIT HISTORY VIEW (media_1795195645330_f67074fa.jpg)
    // =========================================================================================
    float startX = sidebarWidth + 24.f; // 274.f
    float contentW = 1728.f - startX - 24.f; // 1430.f

    // 1. TOP HEADER BAR (White Bar y = 0..60)
    float topH = 60.f;
    sf::RectangleShape topBar(sf::Vector2f(contentAreaW, topH));
    topBar.setPosition(sidebarWidth, 0.f);
    topBar.setFillColor(sf::Color::White);
    window.draw(topBar);

    sf::RectangleShape topBorder(sf::Vector2f(contentAreaW, 1.f));
    topBorder.setPosition(sidebarWidth, topH - 1.f);
    topBorder.setFillColor(sf::Color(226, 232, 240));
    window.draw(topBorder);

    // Left Logo & Title inside Top Bar
    RoundedRectRenderer::render(window, sf::Vector2f(startX, 11.f), sf::Vector2f(38.f, 38.f), 10.f, sf::Color(14, 165, 233));
    RoundedRectRenderer::render(window, sf::Vector2f(startX + 9.f, 19.f), sf::Vector2f(20.f, 22.f), 2.f, sf::Color::White);
    RoundedRectRenderer::render(window, sf::Vector2f(startX + 13.f, 22.f), sf::Vector2f(12.f, 7.f), 1.f, sf::Color(14, 165, 233));
    RoundedRectRenderer::render(window, sf::Vector2f(startX + 13.f, 31.f), sf::Vector2f(12.f, 7.f), 1.f, sf::Color(14, 165, 233));

    drawTxt("CINESTAR", startX + 48.f, 11.f, 15, sf::Color(15, 23, 42), true);
    drawTxt("CINEMA & POS SYSTEM", startX + 48.f, 32.f, 9, sf::Color(14, 165, 233), true);

    sf::RectangleShape bDiv(sf::Vector2f(1.f, 28.f));
    bDiv.setPosition(startX + 188.f, 16.f);
    bDiv.setFillColor(sf::Color(226, 232, 240));
    window.draw(bDiv);

    // Breadcrumb
    drawTxt("Bảng điều khiển   ›   Nghiệp vụ quầy   ›", startX + 206.f, 22.f, 11.5, sf::Color(100, 116, 139));
    drawTxt("Báo cáo doanh thu & Chốt ca trực", startX + 432.f, 22.f, 11.5, sf::Color(2, 132, 199), true);

    // Right Profile Pill
    float profX = 1728.f - 284.f;
    RoundedRectRenderer::render(
        window,
        sf::Vector2f(profX, 8.f),
        sf::Vector2f(260.f, 44.f),
        22.f,
        sf::Color(248, 250, 252),
        1.f,
        sf::Color(226, 232, 240)
    );
    std::string activeCashier = !currentUserName.empty() ? currentUserName : (currentUserId == "U002" ? "Phạm Như Anh Thư" : (currentUserId == "U001" ? "Lê Minh Tuấn" : "Nguyễn Văn A"));
    for (const auto& u : staffUserList) {
        if (u.maNguoiDung == currentUserId && !u.hoTen.empty()) {
            activeCashier = u.hoTen;
            break;
        }
    }
    std::string cashierInitials = "NV";
    if (!activeCashier.empty()) {
        std::stringstream ss(activeCashier);
        std::string w;
        std::vector<std::string> words;
        while (ss >> w) words.push_back(w);
        if (words.size() >= 2) {
            cashierInitials = std::string(1, words.front()[0]) + std::string(1, words.back()[0]);
        } else if (!words.empty()) {
            cashierInitials = words[0].substr(0, std::min<size_t>(2, words[0].size()));
        }
    }

    sf::CircleShape ptAvatar(16.f);
    ptAvatar.setFillColor(sf::Color(2, 132, 199));
    ptAvatar.setPosition(profX + 8.f, 14.f);
    window.draw(ptAvatar);
    drawCenteredTxt(cashierInitials, profX + 24.f, 30.f, 11, sf::Color::White, true);

    drawTxt((currentUserRole == "Admin") ? "Vai trò: Quản trị viên" : "Vai trò: Nhân viên", profX + 50.f, 13.f, 10, sf::Color(2, 132, 199), true);
    drawTxt("Xin chào, ", profX + 50.f, 28.f, 10.5, sf::Color(71, 85, 105));
    drawTxt(activeCashier, profX + 104.f, 28.f, 10.5, sf::Color(15, 23, 42), true);

    // 2. TITLE & ACTION BUTTONS (y = 78.f)
    drawTxt("CHỐT CA & ĐỐI SOÁT DOANH THU", startX, 78.f, 20, sf::Color(15, 23, 42), true);

    RoundedRectRenderer::render(
        window,
        sf::Vector2f(startX + 438.f, 80.f),
        sf::Vector2f(104.f, 26.f),
        6.f,
        sf::Color(224, 242, 254),
        1.f,
        sf::Color(186, 230, 253)
    );
    drawCenteredTxt("(SHIFT AUDIT)", startX + 438.f + 52.f, 93.f, 10.5, sf::Color(2, 132, 199), true);

    drawTxt("Kiểm kê quỹ tiền mặt, đối soát doanh thu hệ thống POS và lập biên bản bàn giao ca trực", startX, 112.f, 12, sf::Color(100, 116, 139));

    if ((currentUserRole == "Admin" || currentUserRole == "Khách") && btnAdminShiftHistory) {
        btnAdminShiftHistory->draw(window);
    }
    if (btnShiftExportReport) btnShiftExportReport->draw(window);
    if (btnShiftRefresh) btnShiftRefresh->draw(window);

    // 3. FOUR KPI SUMMARY CARDS (y = 148.f, h = 108.f)
    float kpiY = 148.f;
    float kpiH = 108.f;
    float kpiGap = 18.f;
    float kpiW = (contentW - 3.f * kpiGap) / 4.f;

    // Card 1: DOANH THU HỆ THỐNG POS
    {
        float cx = startX;
        RoundedRectRenderer::render(window, sf::Vector2f(cx, kpiY), sf::Vector2f(kpiW, kpiH), 12.f, sf::Color::White, 1.f, sf::Color(226, 232, 240));
        sf::RectangleShape bar(sf::Vector2f(3.5f, 44.f));
        bar.setPosition(cx + 16.f, kpiY + 18.f);
        bar.setFillColor(sf::Color(14, 165, 233));
        window.draw(bar);

        drawTxt("DOANH THU HỆ THỐNG POS", cx + 28.f, kpiY + 17.f, 10.5, sf::Color(100, 116, 139), true);
        std::string v1 = formatDotNumber(shiftCalculatedSysRev);
        drawTxt(v1, cx + 28.f, kpiY + 37.f, 20, sf::Color(15, 23, 42), true);
        drawTxt("VNĐ", cx + 34.f + static_cast<float>(v1.size()) * 11.5f, kpiY + 45.f, 11.5, sf::Color(100, 116, 139), true);

        RoundedRectRenderer::render(window, sf::Vector2f(cx + 16.f, kpiY + 73.f), sf::Vector2f(130.f, 22.f), 6.f, sf::Color(240, 249, 255), 1.f, sf::Color(186, 230, 253));
        drawCenteredTxt("Ghi nhận từ vé & F&B", cx + 16.f + 65.f, kpiY + 84.f, 10, sf::Color(2, 132, 199), true);
    }

    // Card 2: TIỀN MẶT KIỂM KÊ (THỰC TẾ)
    {
        float cx = startX + (kpiW + kpiGap);
        RoundedRectRenderer::render(window, sf::Vector2f(cx, kpiY), sf::Vector2f(kpiW, kpiH), 12.f, sf::Color::White, 1.f, sf::Color(226, 232, 240));
        sf::RectangleShape bar(sf::Vector2f(3.5f, 44.f));
        bar.setPosition(cx + 16.f, kpiY + 18.f);
        bar.setFillColor(sf::Color(99, 102, 241));
        window.draw(bar);

        drawTxt("TIỀN MẶT KIỂM KÊ (THỰC TẾ)", cx + 28.f, kpiY + 17.f, 10.5, sf::Color(100, 116, 139), true);
        std::string v2 = formatDotNumber(shiftCalculatedActualCash);
        drawTxt(v2, cx + 28.f, kpiY + 37.f, 20, sf::Color(15, 23, 42), true);
        drawTxt("VNĐ", cx + 34.f + static_cast<float>(v2.size()) * 11.5f, kpiY + 45.f, 11.5, sf::Color(100, 116, 139), true);

        RoundedRectRenderer::render(window, sf::Vector2f(cx + 16.f, kpiY + 73.f), sf::Vector2f(114.f, 22.f), 6.f, sf::Color(238, 242, 255), 1.f, sf::Color(199, 210, 254));
        drawCenteredTxt("Tiền trong két POS", cx + 16.f + 57.f, kpiY + 84.f, 10, sf::Color(79, 70, 229), true);
    }

    // Card 3: CHÊNH LỆCH ĐỐI SOÁT
    {
        float cx = startX + 2.f * (kpiW + kpiGap);
        bool isBalanced = (shiftCalculatedDiff == 0);
        sf::Color diffCol = isBalanced ? sf::Color(5, 150, 105) : sf::Color(220, 38, 38);
        RoundedRectRenderer::render(window, sf::Vector2f(cx, kpiY), sf::Vector2f(kpiW, kpiH), 12.f, sf::Color::White, 1.f, sf::Color(226, 232, 240));
        sf::RectangleShape bar(sf::Vector2f(3.5f, 44.f));
        bar.setPosition(cx + 16.f, kpiY + 18.f);
        bar.setFillColor(isBalanced ? sf::Color(16, 185, 129) : sf::Color(239, 68, 68));
        window.draw(bar);

        drawTxt("CHÊNH LỆCH ĐỐI SOÁT", cx + 28.f, kpiY + 17.f, 10.5, sf::Color(100, 116, 139), true);
        std::string v3 = formatDotNumber(shiftCalculatedDiff);
        drawTxt(v3, cx + 28.f, kpiY + 37.f, 20, diffCol, true);
        drawTxt("VNĐ", cx + 34.f + static_cast<float>(v3.size()) * 12.f, kpiY + 45.f, 11.5, diffCol, true);

        std::string badge3 = isBalanced ? "● Chuẩn quỹ tiền mặt 100%" : (shiftCalculatedDiff > 0 ? "Thừa quỹ +" + formatVnd(shiftCalculatedDiff) : "Lệch quỹ " + formatVnd(shiftCalculatedDiff));
        RoundedRectRenderer::render(
            window,
            sf::Vector2f(cx + 16.f, kpiY + 73.f),
            sf::Vector2f(156.f, 22.f),
            6.f,
            isBalanced ? sf::Color(236, 253, 245) : sf::Color(254, 242, 242),
            1.f,
            isBalanced ? sf::Color(167, 243, 208) : sf::Color(254, 202, 202)
        );
        drawCenteredTxt(badge3, cx + 16.f + 78.f, kpiY + 84.f, 10, diffCol, true);
    }

    // Card 4: TRẠNG THÁI CA TRỰC
    {
        float cx = startX + 3.f * (kpiW + kpiGap);
        RoundedRectRenderer::render(window, sf::Vector2f(cx, kpiY), sf::Vector2f(kpiW, kpiH), 12.f, sf::Color::White, 1.f, sf::Color(226, 232, 240));
        sf::RectangleShape bar(sf::Vector2f(3.5f, 44.f));
        bar.setPosition(cx + 16.f, kpiY + 18.f);
        bar.setFillColor(sf::Color(16, 185, 129));
        window.draw(bar);

        drawTxt("TRẠNG THÁI CA TRỰC", cx + 28.f, kpiY + 17.f, 10.5, sf::Color(100, 116, 139), true);
        sf::CircleShape stDot(4.5f);
        stDot.setFillColor(shiftIsClosed ? sf::Color(245, 158, 11) : sf::Color(16, 185, 129));
        stDot.setPosition(cx + 28.f, kpiY + 46.f);
        window.draw(stDot);

        drawTxt(shiftIsClosed ? "ĐÃ KHÓA SỔ CA" : "ĐANG HOẠT ĐỘNG", cx + 43.f, kpiY + 40.f, 15, shiftIsClosed ? sf::Color(217, 119, 6) : sf::Color(5, 150, 105), true);

        RoundedRectRenderer::render(window, sf::Vector2f(cx + 16.f, kpiY + 73.f), sf::Vector2f(86.f, 22.f), 6.f, sf::Color(240, 253, 244), 1.f, sf::Color(187, 247, 208));
        drawCenteredTxt("Quầy POS 01", cx + 16.f + 43.f, kpiY + 84.f, 10, sf::Color(21, 128, 61), true);
    }

    // 4. LEFT COLUMN: TWO STACKED CARDS (w = 436.f)
    float leftCardW = 436.f;
    float topLeftCardY = 276.f;
    float topLeftCardH = 236.f;

    RoundedRectRenderer::render(
        window,
        sf::Vector2f(startX, topLeftCardY),
        sf::Vector2f(leftCardW, topLeftCardH),
        12.f,
        sf::Color::White,
        1.f,
        sf::Color(226, 232, 240)
    );

    RoundedRectRenderer::render(window, sf::Vector2f(startX + 20.f, topLeftCardY + 20.f), sf::Vector2f(18.f, 14.f), 3.f, sf::Color(224, 242, 254), 1.2f, sf::Color(2, 132, 199));
    drawTxt("KIỂM ĐẾM & BÀN GIAO TIỀN KÉT", startX + 46.f, topLeftCardY + 18.f, 13.5, sf::Color(15, 23, 42), true);
    if (currentUserRole == "Admin" || currentUserRole == "Khách") {
        drawTxt("Chế độ xem lịch sử ca trực (Dành cho Quản lý / Admin)", startX + 20.f, topLeftCardY + 44.f, 11, sf::Color(100, 116, 139));
        drawTxt("Thao tác chốt ca trực chỉ thực hiện tại quầy của Thu ngân.", startX + 20.f, topLeftCardY + 60.f, 11, sf::Color(100, 116, 139));
        RoundedRectRenderer::render(
            window,
            sf::Vector2f(startX + 20.f, topLeftCardY + 95.f),
            sf::Vector2f(leftCardW - 40.f, 85.f),
            8.f,
            sf::Color(248, 250, 252),
            1.f,
            sf::Color(226, 232, 240)
        );
        drawCenteredTxt("Chỉ xem lịch sử đối soát", startX + leftCardW * 0.5f, topLeftCardY + 125.f, 12, sf::Color(100, 116, 139), true);
        drawCenteredTxt("Bấm 'Quay lại đối soát ca' ở trên để trở về", startX + leftCardW * 0.5f, topLeftCardY + 150.f, 11, sf::Color(2, 132, 199));
    } else {
        drawTxt("Nhập tổng tiền mặt kiểm đếm được tại quầy POS để đối soát", startX + 20.f, topLeftCardY + 44.f, 11, sf::Color(100, 116, 139));
        drawTxt("với hệ thống", startX + 20.f, topLeftCardY + 60.f, 11, sf::Color(100, 116, 139));

        drawTxt("TỔNG TIỀN MẶT THỰC TẾ (VNĐ)", startX + 20.f, topLeftCardY + 88.f, 10.5, sf::Color(51, 65, 85), true);

        if (inputShiftActualCash) inputShiftActualCash->draw(window);
        drawRightTxt("VNĐ", startX + leftCardW - 34.f, topLeftCardY + 126.f, 11.5, sf::Color(148, 163, 184), true);

        if (btnShiftCalculate) btnShiftCalculate->draw(window);
        if (btnShiftCloseShift) btnShiftCloseShift->draw(window);
    }

    // 4.2 Left Bottom Card: THÔNG TIN CA TRỰC HIỆN TẠI (y = 530.f, h = 392.f)
    float botLeftCardY = 530.f;
    float botLeftCardH = 392.f;
    RoundedRectRenderer::render(
        window,
        sf::Vector2f(startX, botLeftCardY),
        sf::Vector2f(leftCardW, botLeftCardH),
        12.f,
        sf::Color::White,
        1.f,
        sf::Color(226, 232, 240)
    );

    drawTxt("THÔNG TIN CA TRỰC HIỆN TẠI", startX + 20.f, botLeftCardY + 18.f, 13, sf::Color(15, 23, 42), true);
    sf::CircleShape curShiftDot(4.5f);
    curShiftDot.setFillColor(sf::Color(16, 185, 129));
    curShiftDot.setPosition(startX + leftCardW - 28.f, botLeftCardY + 23.f);
    window.draw(curShiftDot);

    sf::RectangleShape curHeaderLine(sf::Vector2f(leftCardW - 40.f, 1.f));
    curHeaderLine.setPosition(startX + 20.f, botLeftCardY + 48.f);
    curHeaderLine.setFillColor(sf::Color(241, 245, 249));
    window.draw(curHeaderLine);

    struct DetailItem {
        std::string label;
        std::string value;
        sf::Color valColor;
        int badgeType;
    };

    std::vector<DetailItem> details = {
        {"Quầy giao dịch:",      "Quầy POS 01",              sf::Color(15, 23, 42),  0},
        {"Ca làm việc:",         "Ca Chiều (14h00 - 22h00)", sf::Color(2, 132, 199), 0},
        {"Thu ngân phụ trách:",  activeCashier,              sf::Color(15, 23, 42),  0},
        {"Ngày kết toán:",       getTodayDateStr(),          sf::Color(15, 23, 42),  0},
        {"Thời gian hiện tại:",  getCurrentTimeStr(),        sf::Color(30, 41, 59),  1},
        {"Doanh thu POS:",       formatVnd(shiftCalculatedSysRev),     sf::Color(2, 132, 199), 0},
        {"Tiền mặt thực tế:",    formatVnd(shiftCalculatedActualCash), sf::Color(15, 23, 42),  0},
        {"Chênh lệch quỹ:",      formatVnd(shiftCalculatedDiff),       (shiftCalculatedDiff == 0) ? sf::Color(5, 150, 105) : sf::Color(220, 38, 38), 0},
        {"Trạng thái chốt ca:",  shiftIsClosed ? "ĐÃ KHÓA SỔ BÀN GIAO" : "CHỜ DUYỆT BÀN GIAO", sf::Color(180, 83, 9), 2}
    };

    float rowY = botLeftCardY + 60.f;
    float rowStep = 36.f;
    for (size_t i = 0; i < details.size(); ++i) {
        drawTxt(details[i].label, startX + 20.f, rowY + 4.f, 11.5, sf::Color(100, 116, 139));

        if (details[i].badgeType == 1) {
            RoundedRectRenderer::render(
                window,
                sf::Vector2f(startX + leftCardW - 96.f, rowY),
                sf::Vector2f(76.f, 24.f),
                5.f,
                sf::Color(241, 245, 249),
                1.f,
                sf::Color(226, 232, 240)
            );
            drawCenteredTxt(details[i].value, startX + leftCardW - 58.f, rowY + 12.f, 10.5, details[i].valColor, true);
        } else if (details[i].badgeType == 2) {
            RoundedRectRenderer::render(
                window,
                sf::Vector2f(startX + leftCardW - 158.f, rowY - 2.f),
                sf::Vector2f(138.f, 26.f),
                6.f,
                shiftIsClosed ? sf::Color(220, 252, 231) : sf::Color(254, 249, 195),
                1.f,
                shiftIsClosed ? sf::Color(167, 243, 208) : sf::Color(253, 230, 138)
            );
            drawCenteredTxt(details[i].value, startX + leftCardW - 89.f, rowY + 11.f, 10, shiftIsClosed ? sf::Color(5, 150, 105) : sf::Color(180, 83, 9), true);
        } else {
            drawRightTxt(details[i].value, startX + leftCardW - 20.f, rowY + 4.f, 11.5, details[i].valColor, true);
        }

        if (i + 1 < details.size()) {
            sf::RectangleShape dLine(sf::Vector2f(leftCardW - 40.f, 1.f));
            dLine.setPosition(startX + 20.f, rowY + 30.f);
            dLine.setFillColor(sf::Color(248, 250, 252));
            window.draw(dLine);
        }
        rowY += rowStep;
    }

    // 5. RIGHT COLUMN CARD: LỊCH SỬ CÁC CA TRỰC ĐÃ KẾT TOÁN
    float rightPanelX = startX + 456.f;
    float rightPanelW = contentW - 456.f;
    float rightPanelY = 276.f;
    float rightPanelH = 418.f;

    RoundedRectRenderer::render(
        window,
        sf::Vector2f(rightPanelX, rightPanelY),
        sf::Vector2f(rightPanelW, rightPanelH),
        12.f,
        sf::Color::White,
        1.f,
        sf::Color(226, 232, 240)
    );

    sf::CircleShape clkIcon(8.f);
    clkIcon.setFillColor(sf::Color::Transparent);
    clkIcon.setOutlineThickness(1.6f);
    clkIcon.setOutlineColor(sf::Color(2, 132, 199));
    clkIcon.setPosition(rightPanelX + 22.f, rightPanelY + 20.f);
    window.draw(clkIcon);
    drawCenteredTxt("L", rightPanelX + 30.f, rightPanelY + 27.f, 8, sf::Color(2, 132, 199), true);

    drawTxt("LỊCH SỬ CÁC CA TRỰC ĐÃ KẾT TOÁN", rightPanelX + 46.f, rightPanelY + 18.f, 13.5, sf::Color(15, 23, 42), true);
    drawTxt("Danh sách các phiên làm việc và đối soát quỹ tiền mặt từ hệ thống", rightPanelX + 22.f, rightPanelY + 42.f, 11.5, sf::Color(100, 116, 139));

    std::vector<ShiftRecord> records;
    sqlite3* dbConn = nullptr;
    if (sqlite3_open("DATA/cinema.db", &dbConn) == SQLITE_OK) {
        sqlite3_stmt* stmt = nullptr;
        std::string q = "SELECT maCa, maNhanVien, tenNhanVien, ngayLam, gioBatDau, gioKetThuc, doanhThuHeThong, tienThucTe, chenhLech, trangThai, daKhoaBaoCao FROM shift_managements ORDER BY maCa DESC;";
        if (sqlite3_prepare_v2(dbConn, q.c_str(), -1, &stmt, nullptr) == SQLITE_OK) {
            while (sqlite3_step(stmt) == SQLITE_ROW) {
                auto colStr = [&](int c) -> std::string {
                    const char* pStr = reinterpret_cast<const char*>(sqlite3_column_text(stmt, c));
                    return pStr ? std::string(pStr) : "";
                };
                ShiftRecord r;
                r.maCa = colStr(0);
                r.maNV = colStr(1);
                r.tenNV = colStr(2);
                if (r.tenNV == "Pham Nhu Anh Thu" || r.tenNV.empty()) r.tenNV = "Phạm Như Anh Thư";
                r.ngayLam = colStr(3);
                r.gioBatDau = colStr(4);
                r.gioKetThuc = colStr(5);
                try { r.doanhThu = std::stoll(colStr(6)); } catch (...) { r.doanhThu = 0; }
                try { r.tienThucTe = std::stoll(colStr(7)); } catch (...) { r.tienThucTe = 0; }
                try { r.chenhLech = std::stoll(colStr(8)); } catch (...) { r.chenhLech = 0; }
                r.trangThai = colStr(9);
                r.daKhoa = (colStr(10) == "true");
                records.push_back(r);
            }
            sqlite3_finalize(stmt);
        }
        sqlite3_close(dbConn);
    }

    float rHeaderY = rightPanelY + 72.f;
    float rHeaderH = 36.f;
    sf::RectangleShape rthBg(sf::Vector2f(rightPanelW - 2.f, rHeaderH));
    rthBg.setPosition(rightPanelX + 1.f, rHeaderY);
    rthBg.setFillColor(sf::Color(248, 250, 252));
    window.draw(rthBg);

    sf::RectangleShape rthTop(sf::Vector2f(rightPanelW - 2.f, 1.f));
    rthTop.setPosition(rightPanelX + 1.f, rHeaderY);
    rthTop.setFillColor(sf::Color(226, 232, 240));
    window.draw(rthTop);

    sf::RectangleShape rthBot(sf::Vector2f(rightPanelW - 2.f, 1.f));
    rthBot.setPosition(rightPanelX + 1.f, rHeaderY + rHeaderH);
    rthBot.setFillColor(sf::Color(226, 232, 240));
    window.draw(rthBot);

    float c0X = rightPanelX + 24.f;
    float c1X = rightPanelX + 104.f;
    float c2X = rightPanelX + 278.f;
    float c3X = rightPanelX + 392.f;
    float c4Right = rightPanelX + 678.f;
    float c5Right = rightPanelX + 818.f;
    float c6Center = rightPanelX + 902.f;

    drawTxt("MÃ CA", c0X, rHeaderY + 11.f, 10.5, sf::Color(100, 116, 139), true);
    drawTxt("THU NGÂN", c1X, rHeaderY + 11.f, 10.5, sf::Color(100, 116, 139), true);
    drawTxt("NGÀY LÀM", c2X, rHeaderY + 11.f, 10.5, sf::Color(100, 116, 139), true);
    drawTxt("GIỜ TRỰC", c3X, rHeaderY + 11.f, 10.5, sf::Color(100, 116, 139), true);
    drawRightTxt("DOANH THU POS", c4Right, rHeaderY + 11.f, 10.5, sf::Color(100, 116, 139), true);
    drawRightTxt("THỰC TẾ", c5Right, rHeaderY + 11.f, 10.5, sf::Color(100, 116, 139), true);
    drawCenteredTxt("TRẠNG THÁI", c6Center, rHeaderY + 18.f, 10.5, sf::Color(100, 116, 139), true);

    int rowsPerPage = 7;
    int totalPages = std::max(1, static_cast<int>((records.size() + rowsPerPage - 1) / rowsPerPage));
    if (shiftCurrentPage >= totalPages) shiftCurrentPage = totalPages - 1;
    if (shiftCurrentPage < 0) shiftCurrentPage = 0;

    int startIdx = shiftCurrentPage * rowsPerPage;
    int endIdx = std::min(startIdx + rowsPerPage, static_cast<int>(records.size()));

    float rRowStartY = rHeaderY + rHeaderH;
    float rRowH = 36.f;

    for (int i = startIdx; i < endIdx; ++i) {
        const auto& r = records[i];
        float ry = rRowStartY + static_cast<float>(i - startIdx) * rRowH;

        sf::RectangleShape rDiv(sf::Vector2f(rightPanelW - 40.f, 1.f));
        rDiv.setPosition(rightPanelX + 20.f, ry + rRowH);
        rDiv.setFillColor(sf::Color(241, 245, 249));
        window.draw(rDiv);

        drawTxt(r.maCa, c0X, ry + 10.f, 11.5, sf::Color(2, 132, 199), true);
        drawTxt(r.tenNV, c1X, ry + 10.f, 11.5, sf::Color(15, 23, 42), true);
        drawTxt(r.ngayLam, c2X, ry + 10.f, 11.5, sf::Color(71, 85, 105));
        drawTxt(r.gioBatDau + "  -  " + r.gioKetThuc, c3X, ry + 10.f, 11, sf::Color(100, 116, 139));
        drawRightTxt(formatVnd(r.doanhThu), c4Right, ry + 10.f, 11.5, sf::Color(15, 23, 42), true);
        drawRightTxt(formatVnd(r.tienThucTe), c5Right, ry + 10.f, 11.5, sf::Color(51, 65, 85));

        bool isApproved = (r.trangThai == "DaDuyet");
        sf::Color sBg  = isApproved ? sf::Color(220, 252, 231) : sf::Color(254, 249, 195);
        sf::Color sOut = isApproved ? sf::Color(187, 247, 208) : sf::Color(254, 240, 138);
        sf::Color sFg  = isApproved ? sf::Color(5, 150, 105)   : sf::Color(180, 83, 9);
        std::string sText = isApproved ? "Đã duyệt" : "Chờ duyệt";

        RoundedRectRenderer::render(
            window,
            sf::Vector2f(c6Center - 40.f, ry + 6.f),
            sf::Vector2f(80.f, 23.f),
            11.5f,
            sBg,
            1.f,
            sOut
        );
        drawCenteredTxt(sText, c6Center, ry + 17.5f, 10, sFg, true);
    }

    std::string footStr = "Hiển thị " + std::to_string(records.empty() ? 0 : startIdx + 1) + " - " + std::to_string(endIdx) + " trên " + std::to_string(records.size()) + " ca";
    drawTxt(footStr, rightPanelX + 24.f, 655.f, 11.5, sf::Color(71, 85, 105), true);

    if (btnShiftPrevPage) btnShiftPrevPage->draw(window);

    RoundedRectRenderer::render(
        window,
        sf::Vector2f(rightPanelX + rightPanelW - 99.f, 650.f),
        sf::Vector2f(28.f, 26.f),
        5.f,
        sf::Color(224, 242, 254),
        1.f,
        sf::Color(186, 230, 253)
    );
    drawCenteredTxt(std::to_string(shiftCurrentPage + 1), rightPanelX + rightPanelW - 85.f, 663.f, 11, sf::Color(2, 132, 199), true);

    if (btnShiftNextPage) btnShiftNextPage->draw(window);

    drawCenteredTxt("© 2026 CINESTAR Cinema Management System • Hệ Thống Quản Lý Bán Vé POS & Kiểm Kê Doanh Thu", sidebarWidth + contentAreaW / 2.f, 946.f, 10.5, sf::Color(148, 163, 184));
}

} // namespace UI
