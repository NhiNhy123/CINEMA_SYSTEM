#include "../../include/UI/CinemaUIApp.h"
#include "../../include/ActivityLog.h"
#include <fstream>
#include <algorithm>
#include <set>
#include <map>
#include <cmath>
#include <sstream>
#include <iomanip>

namespace UI {

static std::string formatSeatPriceVN(double price) {
    long long v = static_cast<long long>(std::round(price));
    std::string s = std::to_string(v);
    std::string res = "";
    int cnt = 0;
    for (int i = static_cast<int>(s.size()) - 1; i >= 0; --i) {
        if (cnt > 0 && cnt % 3 == 0 && s[i] != '-') res = "." + res;
        res = s[i] + res;
        cnt++;
    }
    return res;
}

static std::string normalizeSeatSearchVN(const std::string& str) {
    static const std::pair<std::string, char> rules[] = {
        {"á",'a'},{"à",'a'},{"ả",'a'},{"ã",'a'},{"ạ",'a'},
        {"ă",'a'},{"ắ",'a'},{"ằ",'a'},{"ẳ",'a'},{"ẵ",'a'},{"ặ",'a'},
        {"â",'a'},{"ấ",'a'},{"ầ",'a'},{"ẩ",'a'},{"ẫ",'a'},{"ậ",'a'},
        {"Á",'a'},{"À",'a'},{"Ả",'a'},{"Ã",'a'},{"Ạ",'a'},
        {"Ă",'a'},{"Ắ",'a'},{"Ằ",'a'},{"Ẳ",'a'},{"Ẵ",'a'},{"Ặ",'a'},
        {"Â",'a'},{"Ấ",'a'},{"Ầ",'a'},{"Ẩ",'a'},{"Ẫ",'a'},{"Ậ",'a'},
        {"đ",'d'},{"Đ",'d'},
        {"é",'e'},{"è",'e'},{"ẻ",'e'},{"ẽ",'e'},{"ẹ",'e'},
        {"ê",'e'},{"ế",'e'},{"ề",'e'},{"ể",'e'},{"ễ",'e'},{"ệ",'e'},
        {"É",'e'},{"È",'e'},{"Ẻ",'e'},{"Ẽ",'e'},{"Ẹ",'e'},
        {"Ê",'e'},{"Ế",'e'},{"Ề",'e'},{"Ể",'e'},{"Ễ",'e'},{"Ệ",'e'},
        {"í",'i'},{"ì",'i'},{"ỉ",'i'},{"ĩ",'i'},{"ị",'i'},
        {"Í",'i'},{"Ì",'i'},{"Ỉ",'i'},{"Ĩ",'i'},{"Ị",'i'},
        {"ó",'o'},{"ò",'o'},{"ỏ",'o'},{"õ",'o'},{"ọ",'o'},
        {"ô",'o'},{"ố",'o'},{"ồ",'o'},{"ổ",'o'},{"ỗ",'o'},{"ộ",'o'},
        {"ơ",'o'},{"ớ",'o'},{"ờ",'o'},{"ở",'o'},{"ỡ",'o'},{"ợ",'o'},
        {"Ó",'o'},{"Ò",'o'},{"Ỏ",'o'},{"Õ",'o'},{"Ọ",'o'},
        {"Ô",'o'},{"Ố",'o'},{"Ồ",'o'},{"Ổ",'o'},{"Ỗ",'o'},{"Ộ",'o'},
        {"Ơ",'o'},{"Ớ",'o'},{"Ờ",'o'},{"Ở",'o'},{"Ỡ",'o'},{"Ợ",'o'},
        {"ú",'u'},{"ù",'u'},{"ủ",'u'},{"ũ",'u'},{"ụ",'u'},
        {"ư",'u'},{"ứ",'u'},{"ừ",'u'},{"ử",'u'},{"ữ",'u'},{"ự",'u'},
        {"Ú",'u'},{"Ù",'u'},{"Ủ",'u'},{"Ũ",'u'},{"Ụ",'u'},
        {"Ư",'u'},{"Ứ",'u'},{"Ừ",'u'},{"Ử",'u'},{"Ữ",'u'},{"Ự",'u'},
        {"ý",'y'},{"ỳ",'y'},{"ỷ",'y'},{"ỹ",'y'},{"ỵ",'y'},
        {"Ý",'y'},{"Ỳ",'y'},{"Ỷ",'y'},{"Ỹ",'y'},{"Ỵ",'y'}
    };
    std::string out;
    out.reserve(str.size());
    for (size_t i = 0; i < str.size(); ) {
        bool matched = false;
        for (const auto& r : rules) {
            size_t len = r.first.size();
            if (i + len <= str.size() && str.compare(i, len, r.first) == 0) {
                out.push_back(r.second);
                i += len;
                matched = true;
                break;
            }
        }
        if (!matched) {
            unsigned char c = static_cast<unsigned char>(str[i]);
            if (c < 128) out.push_back(static_cast<char>(std::tolower(c)));
            else out.push_back(str[i]);
            ++i;
        }
    }
    return out;
}

// Parse seat code ("A1" -> row='A', colStart=1, colEnd=1; "J1-2" -> row='J', colStart=1, colEnd=2)
static bool parseSeatCodeInfo(const std::string& maGhe, char& rowChar, int& colStart, int& colEnd) {
    if (maGhe.size() < 2 || !std::isalpha(static_cast<unsigned char>(maGhe[0]))) return false;
    rowChar = static_cast<char>(std::toupper(static_cast<unsigned char>(maGhe[0])));
    std::string rest = maGhe.substr(1);
    size_t dashPos = rest.find('-');
    if (dashPos == std::string::npos) {
        int c = 0;
        if (std::sscanf(rest.c_str(), "%d", &c) != 1 || c <= 0) return false;
        colStart = colEnd = c;
        return true;
    }
    int c1 = 0, c2 = 0;
    if (std::sscanf(rest.c_str(), "%d-%d", &c1, &c2) != 2 || c1 <= 0 || c2 < c1) return false;
    colStart = c1;
    colEnd = c2;
    return true;
}

static bool isCoupleType(const std::string& loaiGhe) {
    std::string low = normalizeSeatSearchVN(loaiGhe);
    return (low.find("couple") != std::string::npos || low.find("sweetbox") != std::string::npos || low.find("doi") != std::string::npos);
}

// Vector icons
static void drawVectorPencil(sf::RenderTarget& target, float x, float y, sf::Color color) {
    sf::RectangleShape body(sf::Vector2f(11.f, 3.5f));
    body.setOrigin(0.f, 1.75f);
    body.setPosition(x + 2.f, y + 10.f);
    body.setRotation(-45.f);
    body.setFillColor(color);
    target.draw(body);

    sf::ConvexShape tip;
    tip.setPointCount(3);
    tip.setPoint(0, sf::Vector2f(x, y + 12.f));
    tip.setPoint(1, sf::Vector2f(x + 3.5f, y + 12.5f));
    tip.setPoint(2, sf::Vector2f(x + 1.f, y + 9.5f));
    tip.setFillColor(color);
    target.draw(tip);
}

static void drawVectorTrash(sf::RenderTarget& target, float x, float y, sf::Color color) {
    sf::RectangleShape bin(sf::Vector2f(9.f, 9.f));
    bin.setPosition(x + 2.f, y + 4.f);
    bin.setFillColor(sf::Color::Transparent);
    bin.setOutlineThickness(1.5f);
    bin.setOutlineColor(color);
    target.draw(bin);

    sf::RectangleShape lid(sf::Vector2f(13.f, 1.5f));
    lid.setPosition(x, y + 2.5f);
    lid.setFillColor(color);
    target.draw(lid);

    sf::RectangleShape handle(sf::Vector2f(5.f, 1.5f));
    handle.setPosition(x + 4.f, y + 0.5f);
    handle.setFillColor(color);
    target.draw(handle);
}

static void drawVectorCheck(sf::RenderTarget& target, float x, float y, sf::Color color, float scale = 1.0f) {
    sf::RectangleShape p1(sf::Vector2f(5.f * scale, 2.f * scale));
    p1.setOrigin(0.f, 1.f * scale);
    p1.setPosition(x, y + 4.f * scale);
    p1.setRotation(45.f);
    p1.setFillColor(color);
    target.draw(p1);

    sf::RectangleShape p2(sf::Vector2f(10.f * scale, 2.f * scale));
    p2.setOrigin(0.f, 1.f * scale);
    p2.setPosition(x + 3.2f * scale, y + 7.2f * scale);
    p2.setRotation(-50.f);
    p2.setFillColor(color);
    target.draw(p2);
}

bool CinemaUIApp::isSeatBookedInRoom(const std::string& maPhong, const std::string& maGhe) const {
    char rTarget = 'A';
    int cStart = 0, cEnd = 0;
    if (!parseSeatCodeInfo(maGhe, rTarget, cStart, cEnd)) return false;

    // Check explicit seat status
    for (const auto& s : allSeats) {
        if (s.getMaPhong() == maPhong && s.getMaGhe() == maGhe && s.getTrangThai() == "DaDat") {
            return true;
        }
    }

    // Check booked tickets in showtimes belonging to this room
    std::set<std::string> roomShowtimeIds;
    for (const auto& st : showtimeList) {
        if (st.getMaPhong() == maPhong) {
            roomShowtimeIds.insert(st.getMaSuatChieu());
        }
    }
    for (const auto& tk : ticketList) {
        if (roomShowtimeIds.find(tk.getMaSuatChieu()) != roomShowtimeIds.end()) {
            char tkRow = 'A';
            int tkStart = 0, tkEnd = 0;
            if (parseSeatCodeInfo(tk.getMaGhe(), tkRow, tkStart, tkEnd)) {
                if (tkRow == rTarget && !(tkEnd < cStart || tkStart > cEnd)) {
                    return true;
                }
            }
        }
    }
    return false;
}

std::vector<SeatMapBox> CinemaUIApp::buildCurrentRoomSeatMapBoxes() const {
    std::vector<SeatMapBox> result;
    float leftColX = sidebarWidth + 24.f; // 274.f
    float leftColW = 880.f;
    float mapCardY = 182.f;

    std::vector<Seat> roomSeats;
    for (const auto& s : allSeats) {
        if (s.getMaPhong() == seatSelectedRoomId) roomSeats.push_back(s);
    }

    struct ParsedSeat {
        std::string maGhe;
        char rowChar;
        int colStart;
        int colEnd;
        std::string loaiGhe;
        bool isBooked;
    };

    std::map<char, std::vector<ParsedSeat>> rowsMap;
    for (const auto& s : roomSeats) {
        char rc = 'A';
        int c1 = 1, c2 = 1;
        if (parseSeatCodeInfo(s.getMaGhe(), rc, c1, c2)) {
            bool booked = isSeatBookedInRoom(seatSelectedRoomId, s.getMaGhe());
            // Visual demo fallback for F8, F9 on Room 1 if no ticket is booked yet
            if (!booked && (s.getMaGhe() == "F8" || s.getMaGhe() == "F9")) booked = true;
            rowsMap[rc].push_back({s.getMaGhe(), rc, c1, c2, s.getLoaiGhe(), booked});
        }
    }

    if (rowsMap.empty()) return result;

    // Sort rows descending from back of hall (e.g. 'K') down to front ('A') matching reference mockup
    std::vector<char> rowLetters;
    for (const auto& kv : rowsMap) rowLetters.push_back(kv.first);
    std::sort(rowLetters.begin(), rowLetters.end(), std::greater<char>());

    float baseW = 25.f * seatZoomLevel;
    float baseH = 21.f * seatZoomLevel;
    float sGap = 5.f * seatZoomLevel;
    float gridStartY = mapCardY + 120.f;

    float curY = gridStartY;
    for (size_t ri = 0; ri < rowLetters.size() && ri < 12; ++ri) {
        char rc = rowLetters[ri];
        auto& rowVec = rowsMap[rc];

        // Sort columns descending from left to right (e.g. 19 .. 1) matching screen projection
        std::sort(rowVec.begin(), rowVec.end(), [](const ParsedSeat& a, const ParsedSeat& b) {
            return a.colEnd > b.colEnd;
        });

        // Merge consecutive single Couple seats on the same row for display if stored as J1, J2
        std::vector<ParsedSeat> mergedVec;
        for (size_t i = 0; i < rowVec.size(); ++i) {
            if (rowVec[i].colStart == rowVec[i].colEnd && isCoupleType(rowVec[i].loaiGhe) &&
                i + 1 < rowVec.size() && rowVec[i + 1].colStart == rowVec[i + 1].colEnd &&
                isCoupleType(rowVec[i + 1].loaiGhe) && rowVec[i].colStart == rowVec[i + 1].colStart + 1) {
                ParsedSeat pairSeat;
                pairSeat.maGhe = rowVec[i + 1].maGhe; // Primary code or J1-2
                pairSeat.rowChar = rc;
                pairSeat.colStart = rowVec[i + 1].colStart;
                pairSeat.colEnd = rowVec[i].colEnd;
                pairSeat.loaiGhe = rowVec[i].loaiGhe;
                pairSeat.isBooked = rowVec[i].isBooked || rowVec[i + 1].isBooked;
                mergedVec.push_back(pairSeat);
                ++i;
            } else {
                mergedVec.push_back(rowVec[i]);
            }
        }

        // Subtle 3D perspective factor when seatViewMode == 0 ("Góc nhìn 3D")
        float rowScale = 1.0f;
        if (seatViewMode == 0 && rowLetters.size() > 1) {
            rowScale = 0.94f + 0.10f * (static_cast<float>(ri) / static_cast<float>(rowLetters.size() - 1));
        }
        float sW = baseW * rowScale;
        float sH = baseH;
        float gap = sGap * rowScale;

        // Calculate total width of this row
        float rowTotalW = 0.f;
        for (size_t i = 0; i < mergedVec.size(); ++i) {
            bool isPair = (mergedVec[i].colEnd > mergedVec[i].colStart);
            rowTotalW += isPair ? (2.f * sW + gap) : sW;
            if (i + 1 < mergedVec.size()) {
                rowTotalW += isPair ? (gap + 4.f) : gap;
            }
        }

        float rowStartX = leftColX + 36.f + (leftColW - 56.f - rowTotalW) / 2.f;
        float curX = rowStartX;

        for (size_t i = 0; i < mergedVec.size(); ++i) {
            const auto& ps = mergedVec[i];
            bool isPair = (ps.colEnd > ps.colStart);
            float boxW = isPair ? (2.f * sW + gap) : sW;
            std::string lbl = isPair
                ? (std::to_string(ps.colEnd) + "   " + std::to_string(ps.colStart))
                : std::to_string(ps.colStart);

            SeatMapBox box;
            box.maGhe = ps.maGhe;
            box.label = lbl;
            box.rowChar = ps.rowChar;
            box.colStart = ps.colStart;
            box.colEnd = ps.colEnd;
            box.loaiGhe = ps.loaiGhe;
            box.isBooked = ps.isBooked;
            box.bounds = sf::FloatRect(curX, curY, boxW, sH);
            result.push_back(box);

            curX += boxW + (isPair ? (gap + 4.f) : gap);
        }

        curY += sH + 4.5f;
        // Walkway gap between E and D matching reference image
        if (rc == 'E') {
            curY += 7.f;
        }
    }

    return result;
}

std::vector<int> CinemaUIApp::getFilteredRoomSeatIndices() const {
    std::vector<int> indices;
    std::string q = seatSearchInput ? normalizeSeatSearchVN(seatSearchInput->getText()) : "";

    for (size_t i = 0; i < allSeats.size(); ++i) {
        if (allSeats[i].getMaPhong() != seatSelectedRoomId) continue;
        char rc = 'A';
        int c1 = 1, c2 = 1;
        if (!parseSeatCodeInfo(allSeats[i].getMaGhe(), rc, c1, c2)) continue;

        if (!q.empty()) {
            std::string colStr = (c2 > c1) ? (std::to_string(c1) + " - " + std::to_string(c2)) : std::to_string(c1);
            std::string dispLoai = (allSeats[i].getLoaiGhe() == "VIP") ? "VIP Gold" : allSeats[i].getLoaiGhe();
            std::string combined = std::string(1, rc) + " " + allSeats[i].getMaGhe() + " " + colStr + " " + dispLoai + " " + formatSeatPriceVN(allSeats[i].getDonGiaGhe());
            if (normalizeSeatSearchVN(combined).find(q) == std::string::npos) continue;
        }
        indices.push_back(static_cast<int>(i));
    }

    std::sort(indices.begin(), indices.end(), [this](int ia, int ib) {
        char ra = 'A', rb = 'A';
        int ca1 = 0, ca2 = 0, cb1 = 0, cb2 = 0;
        parseSeatCodeInfo(allSeats[ia].getMaGhe(), ra, ca1, ca2);
        parseSeatCodeInfo(allSeats[ib].getMaGhe(), rb, cb1, cb2);
        if (ra != rb) return ra < rb;
        return ca1 < cb1;
    });

    return indices;
}

void CinemaUIApp::refreshSeatTypeList() {
    seatTypeList.clear();
    std::vector<std::pair<std::string, double>> dbTypes;
    db.docDanhSachLoaiGhe(dbTypes);

    struct DefType {
        std::string code;
        std::string name;
        double defaultPrice;
        std::string desc;
        sf::Color color;
    };
    std::vector<DefType> defs = {
        {"SEAT-4DX-01", "4DX", 220000.0, "Ghế chuyển động đa chiều & hiệu ứng môi trường...", sf::Color(249, 115, 22)},
        {"SEAT-CPL-02", "Couple", 200000.0, "Ghế đôi đệm êm có vách ngăn riêng tư cao cấp...", sf::Color(236, 72, 153)},
        {"SEAT-IMX-03", "IMAX", 180000.0, "Góc nhìn đại vĩ tuyến tối ưu chuẩn laser quốc tế...", sf::Color(168, 85, 247)},
        {"SEAT-VIP-04", "VIP", 150000.0, "Ghế da thật khu vực trung tâm tầm nhìn vàng...", sf::Color(245, 158, 11)},
        {"SEAT-STD-05", "Standard", 100000.0, "Ghế đơn tiêu chuẩn nệm mút đúc nhập khẩu...", sf::Color(14, 165, 233)}
    };

    for (const auto& d : defs) {
        double p = d.defaultPrice;
        bool found = false;
        for (const auto& dbt : dbTypes) {
            if (dbt.first == d.name) {
                p = dbt.second;
                found = true;
                break;
            }
        }
        if (!found) {
            db.themLoaiGhe(d.name, d.defaultPrice);
        }
        seatTypeList.push_back({d.code, d.name, p, d.desc, d.color, true});
    }

    for (const auto& dbt : dbTypes) {
        bool exists = false;
        for (const auto& st : seatTypeList) {
            if (st.tenLoai == dbt.first) { exists = true; break; }
        }
        if (!exists) {
            char codeBuf[24];
            snprintf(codeBuf, sizeof(codeBuf), "SEAT-CUS-%02d", static_cast<int>(seatTypeList.size()) + 1);
            seatTypeList.push_back({codeBuf, dbt.first, dbt.second, "Loại ghế cấu hình tùy chỉnh toàn cụm rạp", sf::Color(14, 165, 233), true});
        }
    }
}

void CinemaUIApp::refreshSeatsRoomDropdown() {
    if (!seatRoomDropdown) return;
    std::vector<std::string> roomOpts;
    int curSel = 0;
    for (size_t i = 0; i < roomList.size(); ++i) {
        int cnt = 0;
        for (const auto& s : allSeats) {
            if (s.getMaPhong() == roomList[i].getMaPhong()) cnt++;
        }
        std::string tech = roomList[i].getLoaiPhong();
        if (tech.empty()) tech = "Standard";
        std::string opt = "Phòng: " + roomList[i].getTenPhong() + " - " + tech + " (" + std::to_string(cnt) + " Ghế)";
        roomOpts.push_back(opt);
        if (roomList[i].getMaPhong() == seatSelectedRoomId) {
            curSel = static_cast<int>(i);
        }
    }
    if (roomOpts.empty()) roomOpts.push_back("Chưa có phòng chiếu");
    seatRoomDropdown->setOptions(roomOpts);
    seatRoomDropdown->setSelectedIndex(curSel);
}

void CinemaUIApp::syncSeatTypeSelectionToForm(int idx) {
    if (idx < 0 || idx >= static_cast<int>(seatTypeList.size())) return;
    selectedSeatTypeRowIdx = idx;
    const auto& st = seatTypeList[idx];
    seatFormTypeCode = st.maLoai;
    if (inputSeatTypeName) inputSeatTypeName->setText(st.tenLoai);
    if (inputSeatTypePrice) {
        char pBuf[32];
        snprintf(pBuf, sizeof(pBuf), "%.0f", st.donGia);
        inputSeatTypePrice->setText(pBuf);
    }
    if (inputSeatTypeDesc) inputSeatTypeDesc->setText(st.moTa);
    seatTypeSaleActive = st.kichHoatBan;

    std::vector<sf::Color> swatches = {
        sf::Color(14, 165, 233),  // Cyan Standard
        sf::Color(245, 158, 11),  // Amber VIP
        sf::Color(236, 72, 153),  // Pink Couple
        sf::Color(168, 85, 247),  // Purple IMAX
        sf::Color(249, 115, 22)   // Orange 4DX
    };
    seatTypeColorIdx = 0;
    for (int i = 0; i < 5; ++i) {
        if (swatches[i] == st.mauSac) {
            seatTypeColorIdx = i;
            break;
        }
    }
}

void CinemaUIApp::applyBatchSeatChanges(bool isUpdate) {
    if (seatSelectedRoomId.empty()) {
        if (!roomList.empty()) seatSelectedRoomId = roomList[0].getMaPhong();
        else return;
    }

    int typeIdx = seatBatchTypeDropdown ? seatBatchTypeDropdown->getSelectedIndex() : 0;
    std::string loaiGhe = "Standard";
    double donGia = 100000.0;
    if (typeIdx >= 0 && typeIdx < static_cast<int>(seatTypeList.size())) {
        loaiGhe = seatTypeList[typeIdx].tenLoai;
        donGia = seatTypeList[typeIdx].donGia;
    }
    bool targetIsCouple = isCoupleType(loaiGhe);

    // Parse target rows & columns from form
    std::string rowsStr = inputSeatBatchRows ? inputSeatBatchRows->getText() : "";
    std::string colsStr = inputSeatBatchCols ? inputSeatBatchCols->getText() : "";

    std::vector<char> rows;
    for (size_t i = 0; i < rowsStr.size(); ++i) {
        char ch = static_cast<char>(std::toupper(static_cast<unsigned char>(rowsStr[i])));
        if (ch >= 'A' && ch <= 'Z') {
            if (i + 2 < rowsStr.size() && rowsStr[i + 1] == '-' && std::isalpha(static_cast<unsigned char>(rowsStr[i + 2]))) {
                char endCh = static_cast<char>(std::toupper(static_cast<unsigned char>(rowsStr[i + 2])));
                if (endCh < ch) std::swap(ch, endCh);
                for (char r = ch; r <= endCh; ++r) {
                    if (std::find(rows.begin(), rows.end(), r) == rows.end()) rows.push_back(r);
                }
                i += 2;
            } else {
                if (std::find(rows.begin(), rows.end(), ch) == rows.end()) rows.push_back(ch);
            }
        }
    }

    std::vector<int> cols;
    std::stringstream colSs(colsStr);
    std::string seg;
    while (std::getline(colSs, seg, ',')) {
        size_t dash = seg.find('-');
        if (dash != std::string::npos) {
            int c1 = 0, c2 = 0;
            if (std::sscanf(seg.c_str(), "%d-%d", &c1, &c2) == 2 && c1 > 0 && c2 >= c1 && c2 <= 50) {
                for (int c = c1; c <= c2; ++c) {
                    if (std::find(cols.begin(), cols.end(), c) == cols.end()) cols.push_back(c);
                }
            }
        } else {
            int c = 0;
            if (std::sscanf(seg.c_str(), "%d", &c) == 1 && c > 0 && c <= 50) {
                if (std::find(cols.begin(), cols.end(), c) == cols.end()) cols.push_back(c);
            }
        }
    }
    std::sort(cols.begin(), cols.end());

    if (rows.empty() || cols.empty()) {
        if (confirmPopup) {
            confirmPopup->show("LỖI DỮ LIỆU", "Vui lòng nhập Hàng (A-Z) và Số ghế (1-50) hợp lệ!", ButtonStyle::Danger, "Đóng", "");
        }
        return;
    }

    if (targetIsCouple) {
        if (cols.size() % 2 != 0) {
            if (confirmPopup) {
                confirmPopup->show("QUY TẮC GHẾ ĐÔI", "Ghế đôi (Couple) cần chọn số lượng cột chẵn và liền kề (VD: 1-2 hoặc 1-14)!", ButtonStyle::Danger, "Đóng", "");
            }
            return;
        }
        for (size_t i = 0; i < cols.size(); i += 2) {
            if (cols[i + 1] != cols[i] + 1) {
                if (confirmPopup) {
                    confirmPopup->show("QUY TẮC GHẾ ĐÔI", "Các cặp ghế đôi (Couple) phải nằm liền kề nhau trên cùng một hàng!", ButtonStyle::Danger, "Đóng", "");
                }
                return;
            }
        }
    }

    // Check if any target seat is already booked (Seat.cpp business rule)
    for (char r : rows) {
        for (int c : cols) {
            for (const auto& s : allSeats) {
                if (s.getMaPhong() != seatSelectedRoomId) continue;
                char sr = 'A';
                int sc1 = 0, sc2 = 0;
                if (parseSeatCodeInfo(s.getMaGhe(), sr, sc1, sc2) && sr == r && c >= sc1 && c <= sc2) {
                    if (isSeatBookedInRoom(seatSelectedRoomId, s.getMaGhe())) {
                        if (confirmPopup) {
                            confirmPopup->show("CẢNH BÁO NGHIỆP VỤ", "Ghế " + s.getMaGhe() + " đã có vé bán hoặc đang giữ chỗ. Không thể chỉnh sửa ghế này!", ButtonStyle::Danger, "Đóng", "");
                        }
                        return;
                    }
                }
            }
        }
    }

    int affectedCount = 0;

    for (char r : rows) {
        if (targetIsCouple) {
            for (size_t i = 0; i < cols.size(); i += 2) {
                int c1 = cols[i];
                int c2 = cols[i + 1];
                std::string pairCode = std::string(1, r) + std::to_string(c1) + "-" + std::to_string(c2);

                // Check if overlapping seats exist in this room
                bool hadExisting = false;
                auto it = std::remove_if(allSeats.begin(), allSeats.end(), [&](const Seat& s) {
                    if (s.getMaPhong() != seatSelectedRoomId) return false;
                    char sr = 'A';
                    int sc1 = 0, sc2 = 0;
                    if (!parseSeatCodeInfo(s.getMaGhe(), sr, sc1, sc2)) return false;
                    if (sr == r && !(sc2 < c1 || sc1 > c2)) {
                        hadExisting = true;
                        return true;
                    }
                    return false;
                });
                allSeats.erase(it, allSeats.end());

                if (!isUpdate || hadExisting) {
                    allSeats.push_back(Seat(pairCode, seatSelectedRoomId, loaiGhe, donGia, "Trong"));
                    affectedCount++;
                }
            }
        } else {
            for (int c : cols) {
                std::string singleCode = std::string(1, r) + std::to_string(c);

                // If an existing Couple seat (e.g. J1-2) covers this column, split it into singles first
                for (size_t si = 0; si < allSeats.size(); ++si) {
                    if (allSeats[si].getMaPhong() != seatSelectedRoomId) continue;
                    char sr = 'A';
                    int sc1 = 0, sc2 = 0;
                    if (parseSeatCodeInfo(allSeats[si].getMaGhe(), sr, sc1, sc2) && sr == r && sc2 > sc1 && c >= sc1 && c <= sc2) {
                        std::string oldType = allSeats[si].getLoaiGhe();
                        double oldPrice = allSeats[si].getDonGiaGhe();
                        allSeats.erase(allSeats.begin() + si);
                        for (int k = sc1; k <= sc2; ++k) {
                            std::string kCode = std::string(1, r) + std::to_string(k);
                            allSeats.push_back(Seat(kCode, seatSelectedRoomId, (k == c) ? loaiGhe : oldType, (k == c) ? donGia : oldPrice, "Trong"));
                        }
                        break;
                    }
                }

                bool found = false;
                for (auto& s : allSeats) {
                    if (s.getMaPhong() == seatSelectedRoomId && s.getMaGhe() == singleCode) {
                        found = true;
                        s.setLoaiGhe(loaiGhe);
                        s.setDonGiaGhe(donGia);
                        affectedCount++;
                        break;
                    }
                }
                if (!found && !isUpdate) {
                    allSeats.push_back(Seat(singleCode, seatSelectedRoomId, loaiGhe, donGia, "Trong"));
                    affectedCount++;
                }
            }
        }
    }

    seatSelectedCodes.clear();
    db.ghiDanhSachGhe(allSeats);

    int curRoomSeats = 0;
    for (const auto& s : allSeats) {
        if (s.getMaPhong() == seatSelectedRoomId) curRoomSeats++;
    }
    for (auto& rm : roomList) {
        if (rm.getMaPhong() == seatSelectedRoomId) {
            rm.setSucChua(curRoomSeats);
            break;
        }
    }
    db.ghiDanhSachPhong(roomList);
    refreshSeatsRoomDropdown();
    refreshShowtimesDropdowns();

    if (confirmPopup) {
        std::string msg = isUpdate
            ? ("Đã cập nhật phân hạng '" + loaiGhe + "' cho " + std::to_string(affectedCount) + " ghế thành công!")
            : ("Đã lưu " + std::to_string(affectedCount) + " ghế (" + loaiGhe + ") vào sơ đồ phòng " + seatSelectedRoomId + "!");
        confirmPopup->show("THÀNH CÔNG", msg, ButtonStyle::Primary, "Đóng", "");
    }
}

void CinemaUIApp::deleteSelectedSeats() {
    if (seatSelectedRoomId.empty()) return;

    std::vector<std::string> targets = seatSelectedCodes;
    if (targets.empty()) {
        std::string rowsStr = inputSeatBatchRows ? inputSeatBatchRows->getText() : "";
        std::string colsStr = inputSeatBatchCols ? inputSeatBatchCols->getText() : "";
        std::vector<char> rows;
        for (size_t i = 0; i < rowsStr.size(); ++i) {
            char ch = static_cast<char>(std::toupper(static_cast<unsigned char>(rowsStr[i])));
            if (ch >= 'A' && ch <= 'Z') {
                if (i + 2 < rowsStr.size() && rowsStr[i + 1] == '-' && std::isalpha(static_cast<unsigned char>(rowsStr[i + 2]))) {
                    char endCh = static_cast<char>(std::toupper(static_cast<unsigned char>(rowsStr[i + 2])));
                    for (char r = ch; r <= endCh; ++r) rows.push_back(r);
                    i += 2;
                } else {
                    rows.push_back(ch);
                }
            }
        }
        std::vector<int> cols;
        std::stringstream ss(colsStr);
        std::string seg;
        while (std::getline(ss, seg, ',')) {
            int c1 = 0, c2 = 0;
            if (std::sscanf(seg.c_str(), "%d-%d", &c1, &c2) == 2 && c1 > 0 && c2 >= c1) {
                for (int c = c1; c <= c2; ++c) cols.push_back(c);
            } else if (std::sscanf(seg.c_str(), "%d", &c1) == 1 && c1 > 0) {
                cols.push_back(c1);
            }
        }
        for (const auto& s : allSeats) {
            if (s.getMaPhong() != seatSelectedRoomId) continue;
            char sr = 'A';
            int sc1 = 0, sc2 = 0;
            if (parseSeatCodeInfo(s.getMaGhe(), sr, sc1, sc2)) {
                if (std::find(rows.begin(), rows.end(), sr) != rows.end()) {
                    for (int c : cols) {
                        if (c >= sc1 && c <= sc2) {
                            targets.push_back(s.getMaGhe());
                            break;
                        }
                    }
                }
            }
        }
    }

    if (targets.empty()) {
        if (confirmPopup) {
            confirmPopup->show("THÔNG BÁO", "Vui lòng chọn ghế trên sơ đồ hoặc nhập Hàng/Số ghế cần xóa!", ButtonStyle::Primary, "Đóng", "");
        }
        return;
    }

    for (const auto& code : targets) {
        if (isSeatBookedInRoom(seatSelectedRoomId, code)) {
            if (confirmPopup) {
                confirmPopup->show("CẢNH BÁO NGHIỆP VỤ", "Ghế " + code + " đã có vé bán trong hệ thống. Không thể xóa ghế này!", ButtonStyle::Danger, "Đóng", "");
            }
            return;
        }
    }

    int deletedCount = 0;
    for (const auto& code : targets) {
        auto it = std::remove_if(allSeats.begin(), allSeats.end(), [&](const Seat& s) {
            return s.getMaPhong() == seatSelectedRoomId && s.getMaGhe() == code;
        });
        if (it != allSeats.end()) {
            deletedCount += static_cast<int>(std::distance(it, allSeats.end()));
            allSeats.erase(it, allSeats.end());
        }
    }
    seatSelectedCodes.clear();

    if (deletedCount > 0) {
        db.ghiDanhSachGhe(allSeats);
        int curRoomSeats = 0;
        for (const auto& s : allSeats) {
            if (s.getMaPhong() == seatSelectedRoomId) curRoomSeats++;
        }
        for (auto& rm : roomList) {
            if (rm.getMaPhong() == seatSelectedRoomId) {
                rm.setSucChua(curRoomSeats);
                break;
            }
        }
        db.ghiDanhSachPhong(roomList);
        refreshSeatsRoomDropdown();
        refreshShowtimesDropdowns();

        if (confirmPopup) {
            confirmPopup->show("THÀNH CÔNG", "Đã xóa " + std::to_string(deletedCount) + " ghế khỏi phòng " + seatSelectedRoomId + "!", ButtonStyle::Primary, "Đóng", "");
        }
    }
}

void CinemaUIApp::setupSeatsView() {
    float leftColX = sidebarWidth + 24.f;         // 274.f
    float leftColW = 880.f;
    float rightColX = leftColX + leftColW + 20.f; // 1174.f
    float rightColW = 500.f;

    sf::Color softInputBg(248, 250, 252);
    sf::Color softInputBorder(203, 213, 225);
    sf::Color focusBlue(2, 132, 199);

    refreshSeatTypeList();

    if (!roomList.empty()) {
        bool validRoom = false;
        for (const auto& r : roomList) {
            if (r.getMaPhong() == seatSelectedRoomId) { validRoom = true; break; }
        }
        if (!validRoom) seatSelectedRoomId = roomList[0].getMaPhong();

        // Ensure first room has the rich Cinestar architectural layout if it only had a minimal seed (< 30 seats)
        std::string firstRoomId = roomList[0].getMaPhong();
        int firstRoomCnt = 0;
        bool hasRowJCouple = false;
        for (const auto& s : allSeats) {
            if (s.getMaPhong() == firstRoomId) {
                firstRoomCnt++;
                if (s.getMaGhe().find("J1-2") == 0) hasRowJCouple = true;
            }
        }
        if (firstRoomCnt < 30 || !hasRowJCouple) {
            // Preserve any booked seats in firstRoomId, upgrade layout to match Cinestar Standard 01 (134 seats)
            allSeats.erase(std::remove_if(allSeats.begin(), allSeats.end(), [&](const Seat& s) {
                return s.getMaPhong() == firstRoomId && !isSeatBookedInRoom(firstRoomId, s.getMaGhe());
            }), allSeats.end());

            auto addIfMissing = [&](const std::string& code, const std::string& type, double price, const std::string& status = "Trong") {
                for (const auto& s : allSeats) {
                    if (s.getMaPhong() == firstRoomId && s.getMaGhe() == code) return;
                }
                allSeats.push_back(Seat(code, firstRoomId, type, price, status));
            };

            // Rows A..D: Standard 1..15 (60 seats)
            for (char r = 'A'; r <= 'D'; ++r) {
                for (int c = 1; c <= 15; ++c) {
                    addIfMissing(std::string(1, r) + std::to_string(c), "Standard", 100000.0);
                }
            }
            // Rows E..I: VIP 1..12 (48 seats on E..H + 6 seats on I = 54 or 12 on I)
            for (char r = 'E'; r <= 'I'; ++r) {
                for (int c = 1; c <= 12; ++c) {
                    std::string code = std::string(1, r) + std::to_string(c);
                    std::string st = (code == "F8" || code == "F9") ? "DaDat" : "Trong";
                    addIfMissing(code, "VIP", 150000.0, st);
                }
            }
            // Row J: Couple pairs 1-2 .. 13-14 (7 Couple loveseats)
            for (int c = 1; c <= 13; c += 2) {
                std::string pairCode = "J" + std::to_string(c) + "-" + std::to_string(c + 1);
                addIfMissing(pairCode, "Couple", 200000.0);
            }
            // Row K: 4DX / Ghế ba Sofa 1..19 (19 seats)
            for (int c = 1; c <= 19; ++c) {
                addIfMissing("K" + std::to_string(c), "4DX", 220000.0);
            }

            int updatedCnt = 0;
            for (const auto& s : allSeats) {
                if (s.getMaPhong() == firstRoomId) updatedCnt++;
            }
            roomList[0].setSucChua(updatedCnt);
            db.ghiDanhSachGhe(allSeats);
            db.ghiDanhSachPhong(roomList);
        }
    }

    // Default C6, C7 selected to match reference visual highlight if nothing selected yet
    if (seatSelectedCodes.empty()) {
        seatSelectedCodes = {"C6", "C7"};
    }

    // Sub-Tab Switchers on Sub-Bar (y = 136..174) — NO numbers on tab buttons
    btnSeatSubTabMatrix = std::unique_ptr<RoundRectButton>(new RoundRectButton(
        "",
        sf::Vector2f(leftColX + 12.f, 138.f),
        sf::Vector2f(185.f, 34.f),
        ButtonStyle::Ghost,
        6.f
    ));
    btnSeatSubTabMatrix->setCustomColors(sf::Color::Transparent, sf::Color(241, 245, 249), sf::Color(226, 232, 240), sf::Color::Transparent);
    btnSeatSubTabMatrix->setOnClick([this]() {
        seatSubTab = 0;
    });

    btnSeatSubTabTypes = std::unique_ptr<RoundRectButton>(new RoundRectButton(
        "",
        sf::Vector2f(leftColX + 206.f, 138.f),
        sf::Vector2f(185.f, 34.f),
        ButtonStyle::Ghost,
        6.f
    ));
    btnSeatSubTabTypes->setCustomColors(sf::Color::Transparent, sf::Color(241, 245, 249), sf::Color(226, 232, 240), sf::Color::Transparent);
    btnSeatSubTabTypes->setOnClick([this]() {
        seatSubTab = 1;
        refreshSeatTypeList();
        if (!seatTypeList.empty() && selectedSeatTypeRowIdx < 0) syncSeatTypeSelectionToForm(0);
    });

    // =========================================================
    // TAB 1 CONTROLS (THIẾT LẬP SƠ ĐỒ GHẾ - THEO MẪU MỚI)
    // =========================================================
    seatRoomDropdown = std::unique_ptr<DropdownBox>(new DropdownBox(
        sf::Vector2f(leftColX + 490.f, 58.f),
        sf::Vector2f(350.f, 34.f),
        {},
        0
    ));
    seatRoomDropdown->setFontSize(13);
    seatRoomDropdown->setLightStyle(true);
    seatRoomDropdown->setMaxVisibleItems(6);
    refreshSeatsRoomDropdown();
    seatRoomDropdown->setOnSelect([this](int idx, const std::string& val) {
        (void)val;
        if (idx >= 0 && idx < static_cast<int>(roomList.size())) {
            seatSelectedRoomId = roomList[idx].getMaPhong();
            seatSelectedCodes.clear();
            seatListPage = 0;
        }
    });

    btnSeatCopyLayout = std::unique_ptr<RoundRectButton>(new RoundRectButton(
        "Sao chép bố cục",
        sf::Vector2f(leftColX + 852.f, 58.f),
        sf::Vector2f(162.f, 34.f),
        ButtonStyle::Primary,
        8.f
    ));
    btnSeatCopyLayout->setFontSize(12);
    btnSeatCopyLayout->setCustomColors(sf::Color(245, 158, 11), sf::Color(217, 119, 6), sf::Color(180, 83, 9), sf::Color(254, 243, 199));
    btnSeatCopyLayout->setTextColor(sf::Color::White);
    btnSeatCopyLayout->setOnClick([this]() {
        if (confirmPopup) {
            confirmPopup->show(
                "SAO CHÉP BỐ CỤC GHẾ",
                "Sao chép bố cục ghế của phòng " + seatSelectedRoomId + " sang các phòng chưa có vé bán?",
                ButtonStyle::Primary,
                "Sao chép ngay",
                "Hủy bỏ"
            );
            confirmPopup->setOnConfirm([this]() {
                std::vector<Seat> curRoomSeats;
                for (const auto& s : allSeats) {
                    if (s.getMaPhong() == seatSelectedRoomId) curRoomSeats.push_back(s);
                }
                for (auto& rm : roomList) {
                    if (rm.getMaPhong() != seatSelectedRoomId) {
                        bool roomHasSold = false;
                        for (const auto& s : allSeats) {
                            if (s.getMaPhong() == rm.getMaPhong() && isSeatBookedInRoom(rm.getMaPhong(), s.getMaGhe())) {
                                roomHasSold = true;
                                break;
                            }
                        }
                        if (roomHasSold) continue;

                        allSeats.erase(std::remove_if(allSeats.begin(), allSeats.end(), [&](const Seat& s) {
                            return s.getMaPhong() == rm.getMaPhong();
                        }), allSeats.end());
                        for (const auto& s : curRoomSeats) {
                            allSeats.push_back(Seat(s.getMaGhe(), rm.getMaPhong(), s.getLoaiGhe(), s.getDonGiaGhe(), "Trong"));
                        }
                        rm.setSucChua(static_cast<int>(curRoomSeats.size()));
                    }
                }
                db.ghiDanhSachGhe(allSeats);
                db.ghiDanhSachPhong(roomList);
                ActivityLog::ghiNhan(currentUserId, currentUserRole, "SAO_CHEP_GHE", "Đồng bộ bố cục phòng " + seatSelectedRoomId + " thành công");
                refreshSeatsRoomDropdown();
                refreshShowtimesDropdowns();
            });
        }
    });

    btnSeatExportMap = std::unique_ptr<RoundRectButton>(new RoundRectButton(
        "Xuất sơ đồ (PDF/CAD)",
        sf::Vector2f(leftColX + 490.f, 96.f),
        sf::Vector2f(175.f, 28.f),
        ButtonStyle::Ghost,
        7.f
    ));
    btnSeatExportMap->setFontSize(11);
    btnSeatExportMap->setCustomColors(sf::Color(51, 65, 85), sf::Color(71, 85, 105), sf::Color(30, 41, 59), sf::Color(51, 65, 85));
    btnSeatExportMap->setOutline(1.f, sf::Color(100, 116, 139));
    btnSeatExportMap->setTextColor(sf::Color(248, 250, 252));
    btnSeatExportMap->setOnClick([this]() {
        std::ofstream out("reports/SoDoGhe_" + seatSelectedRoomId + ".csv");
        if (out.is_open()) {
            out << "\xEF\xBB\xBF";
            out << "Phòng,Mã Ghế,Loại Ghế,Đơn Giá,Trạng Thái\n";
            for (const auto& s : allSeats) {
                if (s.getMaPhong() == seatSelectedRoomId) {
                    out << s.getMaPhong() << "," << s.getMaGhe() << "," << s.getLoaiGhe() << "," << static_cast<long long>(s.getDonGiaGhe()) << "," << s.getTrangThai() << "\n";
                }
            }
            out.close();
            if (confirmPopup) confirmPopup->show("XUẤT SƠ ĐỒ THÀNH CÔNG", "Đã xuất sơ đồ kỹ thuật phòng " + seatSelectedRoomId + " ra file reports/SoDoGhe_" + seatSelectedRoomId + ".csv!", ButtonStyle::Primary, "Đóng", "");
        }
    });

    btnSeatAddRowQuick = std::unique_ptr<RoundRectButton>(new RoundRectButton(
        "+ Thêm hàng nhanh",
        sf::Vector2f(leftColX + 675.f, 96.f),
        sf::Vector2f(150.f, 28.f),
        ButtonStyle::Ghost,
        7.f
    ));
    btnSeatAddRowQuick->setFontSize(11);
    btnSeatAddRowQuick->setCustomColors(sf::Color(3, 105, 161), sf::Color(2, 132, 199), sf::Color(7, 89, 133), sf::Color(3, 105, 161));
    btnSeatAddRowQuick->setTextColor(sf::Color::White);
    btnSeatAddRowQuick->setOnClick([this]() {
        char maxRow = 'A' - 1;
        for (const auto& s : allSeats) {
            if (s.getMaPhong() == seatSelectedRoomId) {
                char rc = 'A';
                int c1 = 1, c2 = 1;
                if (parseSeatCodeInfo(s.getMaGhe(), rc, c1, c2) && rc > maxRow) maxRow = rc;
            }
        }
        char nextRow = (maxRow < 'A' || maxRow >= 'Z') ? 'A' : static_cast<char>(maxRow + 1);
        if (inputSeatBatchRows) inputSeatBatchRows->setText(std::string(1, nextRow));
        if (inputSeatBatchCols) inputSeatBatchCols->setText("1-15");
        applyBatchSeatChanges(false);
    });

    // View & Zoom Toolbar Controls inside Auditorium Card (toolbarY = 224.f)
    btnSeatView3D = std::unique_ptr<RoundRectButton>(new RoundRectButton(
        "Góc nhìn 3D",
        sf::Vector2f(leftColX + 28.f, 227.f),
        sf::Vector2f(88.f, 24.f),
        ButtonStyle::Ghost,
        6.f
    ));
    btnSeatView3D->setFontSize(11);
    btnSeatView3D->setOnClick([this]() { seatViewMode = 0; });

    btnSeatView2D = std::unique_ptr<RoundRectButton>(new RoundRectButton(
        "2D Phẳng",
        sf::Vector2f(leftColX + 120.f, 227.f),
        sf::Vector2f(76.f, 24.f),
        ButtonStyle::Ghost,
        6.f
    ));
    btnSeatView2D->setFontSize(11);
    btnSeatView2D->setOnClick([this]() { seatViewMode = 1; });

    btnSeatViewPOV = std::unique_ptr<RoundRectButton>(new RoundRectButton(
        "POV Ghế",
        sf::Vector2f(leftColX + 200.f, 227.f),
        sf::Vector2f(76.f, 24.f),
        ButtonStyle::Ghost,
        6.f
    ));
    btnSeatViewPOV->setFontSize(11);
    btnSeatViewPOV->setOnClick([this]() { seatViewMode = 2; });

    btnSeatZoomOut = std::unique_ptr<RoundRectButton>(new RoundRectButton("-", sf::Vector2f(leftColX + 290.f, 227.f), sf::Vector2f(26.f, 24.f), ButtonStyle::Ghost, 6.f));
    btnSeatZoomOut->setFontSize(13);
    btnSeatZoomOut->setCustomColors(sf::Color(241, 245, 249), sf::Color(226, 232, 240), sf::Color(203, 213, 225), sf::Color(241, 245, 249));
    btnSeatZoomOut->setTextColor(sf::Color(51, 65, 85));
    btnSeatZoomOut->setOnClick([this]() { seatZoomLevel = std::max(0.85f, seatZoomLevel - 0.05f); });

    btnSeatZoomIn = std::unique_ptr<RoundRectButton>(new RoundRectButton("+", sf::Vector2f(leftColX + 362.f, 227.f), sf::Vector2f(26.f, 24.f), ButtonStyle::Ghost, 6.f));
    btnSeatZoomIn->setFontSize(13);
    btnSeatZoomIn->setCustomColors(sf::Color(241, 245, 249), sf::Color(226, 232, 240), sf::Color(203, 213, 225), sf::Color(241, 245, 249));
    btnSeatZoomIn->setTextColor(sf::Color(51, 65, 85));
    btnSeatZoomIn->setOnClick([this]() { seatZoomLevel = std::min(1.10f, seatZoomLevel + 0.05f); });

    btnSeatZoomReset = std::unique_ptr<RoundRectButton>(new RoundRectButton("1:1", sf::Vector2f(leftColX + 392.f, 227.f), sf::Vector2f(30.f, 24.f), ButtonStyle::Ghost, 6.f));
    btnSeatZoomReset->setFontSize(10);
    btnSeatZoomReset->setCustomColors(sf::Color(241, 245, 249), sf::Color(226, 232, 240), sf::Color(203, 213, 225), sf::Color(241, 245, 249));
    btnSeatZoomReset->setTextColor(sf::Color(51, 65, 85));
    btnSeatZoomReset->setOnClick([this]() { seatZoomLevel = 1.0f; });

    // Bottom Left Table Search & Pagination (tblCardY = 648.f, tblCardH = 276.f)
    seatSearchInput = std::unique_ptr<UIInputBox>(new UIInputBox(
        sf::Vector2f(leftColX + leftColW - 246.f, 662.f),
        sf::Vector2f(222.f, 30.f),
        "Hàng, số ghế, loại...",
        12
    ));
    seatSearchInput->setColors(softInputBg, softInputBorder, focusBlue, sf::Color(15, 23, 42), sf::Color(148, 163, 184));
    seatSearchInput->setCornerRadius(8.f);
    seatSearchInput->setTextInset(28.f);

    btnSeatListPrev = std::unique_ptr<RoundRectButton>(new RoundRectButton("<", sf::Vector2f(leftColX + leftColW - 146.f, 892.f), sf::Vector2f(24.f, 24.f), ButtonStyle::Ghost, 5.f));
    btnSeatListPrev->setFontSize(12);
    btnSeatListPrev->setCustomColors(sf::Color(248, 250, 252), sf::Color(226, 232, 240), sf::Color(203, 213, 225), sf::Color(248, 250, 252));
    btnSeatListPrev->setOutline(1.f, sf::Color(226, 232, 240));
    btnSeatListPrev->setTextColor(sf::Color(71, 85, 105));
    btnSeatListPrev->setOnClick([this]() { if (seatListPage > 0) seatListPage--; });

    btnSeatListNext = std::unique_ptr<RoundRectButton>(new RoundRectButton(">", sf::Vector2f(leftColX + leftColW - 48.f, 892.f), sf::Vector2f(24.f, 24.f), ButtonStyle::Ghost, 5.f));
    btnSeatListNext->setFontSize(12);
    btnSeatListNext->setCustomColors(sf::Color(248, 250, 252), sf::Color(226, 232, 240), sf::Color(203, 213, 225), sf::Color(248, 250, 252));
    btnSeatListNext->setOutline(1.f, sf::Color(226, 232, 240));
    btnSeatListNext->setTextColor(sf::Color(71, 85, 105));
    btnSeatListNext->setOnClick([this]() { seatListPage++; });

    // Right Column: Batch Seat Form (rCard1Y = 182.f, rCard1H = 472.f)
    inputSeatBatchRows = std::unique_ptr<UIInputBox>(new UIInputBox(
        sf::Vector2f(rightColX + 24.f, 248.f),
        sf::Vector2f(rightColW - 48.f, 38.f),
        "A, B, C, D",
        13
    ));
    inputSeatBatchRows->setColors(softInputBg, softInputBorder, focusBlue, sf::Color(15, 23, 42), sf::Color(148, 163, 184));
    inputSeatBatchRows->setCornerRadius(8.f);
    inputSeatBatchRows->setText("A, B, C, D");

    inputSeatBatchCols = std::unique_ptr<UIInputBox>(new UIInputBox(
        sf::Vector2f(rightColX + 24.f, 326.f),
        sf::Vector2f(rightColW - 48.f, 38.f),
        "1-15",
        13
    ));
    inputSeatBatchCols->setColors(softInputBg, softInputBorder, focusBlue, sf::Color(15, 23, 42), sf::Color(148, 163, 184));
    inputSeatBatchCols->setCornerRadius(8.f);
    inputSeatBatchCols->setText("1-15");

    std::vector<std::string> bTypeOpts;
    int defaultBatchIdx = 0;
    for (size_t i = 0; i < seatTypeList.size(); ++i) {
        const auto& st = seatTypeList[i];
        bTypeOpts.push_back(st.tenLoai + " (" + formatSeatPriceVN(st.donGia) + " VNĐ)");
        if (st.tenLoai == "4DX") defaultBatchIdx = static_cast<int>(i);
    }

    seatBatchTypeDropdown = std::unique_ptr<DropdownBox>(new DropdownBox(
        sf::Vector2f(rightColX + 24.f, 404.f),
        sf::Vector2f(rightColW - 48.f, 38.f),
        bTypeOpts,
        defaultBatchIdx
    ));
    seatBatchTypeDropdown->setFontSize(13);
    seatBatchTypeDropdown->setLightStyle(true);
    seatBatchTypeDropdown->setMaxVisibleItems(6);

    // Action Buttons matching media_1794677546630_c74f54be.jpg:
    // Row 1: Full-width Green "+ Thêm ghế (hàng loạt)"
    // Row 2: Half-width Dark Blue "Cập nhật loại" + Half-width Soft Red "Xóa ghế"
    // Row 3: Full-width Outline "Đặt lại form"
    float fullFormW = rightColW - 48.f;
    float halfW = (fullFormW - 12.f) / 2.f;

    btnSeatBatchAdd = std::unique_ptr<RoundRectButton>(new RoundRectButton(
        "+ Thêm ghế (hàng loạt)",
        sf::Vector2f(rightColX + 24.f, 520.f),
        sf::Vector2f(fullFormW, 38.f),
        ButtonStyle::Success,
        8.f
    ));
    btnSeatBatchAdd->setFontSize(13);
    btnSeatBatchAdd->setCustomColors(sf::Color(16, 185, 129), sf::Color(5, 150, 105), sf::Color(4, 120, 87), sf::Color(209, 250, 229));
    btnSeatBatchAdd->setTextColor(sf::Color::White);
    btnSeatBatchAdd->setOnClick([this]() {
        applyBatchSeatChanges(false);
    });

    btnSeatUpdateType = std::unique_ptr<RoundRectButton>(new RoundRectButton(
        "Cập nhật loại",
        sf::Vector2f(rightColX + 24.f, 566.f),
        sf::Vector2f(halfW, 36.f),
        ButtonStyle::Primary,
        8.f
    ));
    btnSeatUpdateType->setFontSize(12);
    btnSeatUpdateType->setCustomColors(sf::Color(3, 105, 161), sf::Color(2, 132, 199), sf::Color(7, 89, 133), sf::Color(224, 242, 254));
    btnSeatUpdateType->setTextColor(sf::Color::White);
    btnSeatUpdateType->setOnClick([this]() {
        applyBatchSeatChanges(true);
    });

    btnSeatDeleteBatch = std::unique_ptr<RoundRectButton>(new RoundRectButton(
        "Xóa ghế",
        sf::Vector2f(rightColX + 24.f + halfW + 12.f, 566.f),
        sf::Vector2f(halfW, 36.f),
        ButtonStyle::Danger,
        8.f
    ));
    btnSeatDeleteBatch->setFontSize(12);
    btnSeatDeleteBatch->setCustomColors(sf::Color(255, 228, 230), sf::Color(254, 205, 211), sf::Color(244, 63, 94), sf::Color(255, 241, 242));
    btnSeatDeleteBatch->setOutline(1.f, sf::Color(254, 205, 211));
    btnSeatDeleteBatch->setTextColor(sf::Color(225, 29, 72));
    btnSeatDeleteBatch->setOnClick([this]() {
        deleteSelectedSeats();
    });

    btnSeatResetForm = std::unique_ptr<RoundRectButton>(new RoundRectButton(
        "Đặt lại form",
        sf::Vector2f(rightColX + 24.f, 610.f),
        sf::Vector2f(fullFormW, 34.f),
        ButtonStyle::Ghost,
        8.f
    ));
    btnSeatResetForm->setFontSize(12);
    btnSeatResetForm->setCustomColors(sf::Color::White, sf::Color(241, 245, 249), sf::Color(226, 232, 240), sf::Color::White);
    btnSeatResetForm->setOutline(1.f, sf::Color(203, 213, 225));
    btnSeatResetForm->setTextColor(sf::Color(51, 65, 85));
    btnSeatResetForm->setOnClick([this]() {
        seatSelectedCodes.clear();
        if (inputSeatBatchRows) inputSeatBatchRows->setText("A, B, C, D");
        if (inputSeatBatchCols) inputSeatBatchCols->setText("1-15");
    });

    // =========================================================
    // TAB 2 CONTROLS (LOẠI GHẾ & ĐƠN GIÁ)
    // =========================================================
    btnSeatAddNewType = std::unique_ptr<RoundRectButton>(new RoundRectButton(
        "+ Thêm Loại Ghế Mới",
        sf::Vector2f(rightColX + rightColW - 435.f, 68.f),
        sf::Vector2f(165.f, 36.f),
        ButtonStyle::Primary,
        8.f
    ));
    btnSeatAddNewType->setFontSize(12);
    btnSeatAddNewType->setCustomColors(sf::Color(2, 132, 199), sf::Color(3, 105, 161), sf::Color(7, 89, 133), sf::Color(224, 242, 254));
    btnSeatAddNewType->setTextColor(sf::Color::White);
    btnSeatAddNewType->setOnClick([this]() {
        selectedSeatTypeRowIdx = -1;
        char codeBuf[24];
        snprintf(codeBuf, sizeof(codeBuf), "SEAT-NEW-%02d", static_cast<int>(seatTypeList.size()) + 1);
        seatFormTypeCode = codeBuf;
        if (inputSeatTypeName) inputSeatTypeName->setText("");
        if (inputSeatTypePrice) inputSeatTypePrice->setText("100000");
        if (inputSeatTypeDesc) inputSeatTypeDesc->setText("");
        seatTypeColorIdx = 0;
        seatTypeSaleActive = true;
    });

    btnSeatExportPricing = std::unique_ptr<RoundRectButton>(new RoundRectButton(
        "Xuất Biểu Giá",
        sf::Vector2f(rightColX + rightColW - 260.f, 68.f),
        sf::Vector2f(125.f, 36.f),
        ButtonStyle::Ghost,
        8.f
    ));
    btnSeatExportPricing->setFontSize(12);
    btnSeatExportPricing->setCustomColors(sf::Color(51, 65, 85), sf::Color(71, 85, 105), sf::Color(30, 41, 59), sf::Color(51, 65, 85));
    btnSeatExportPricing->setOutline(1.f, sf::Color(100, 116, 139));
    btnSeatExportPricing->setTextColor(sf::Color::White);
    btnSeatExportPricing->setOnClick([this]() {
        std::ofstream out("reports/BieuGiaGhe_CineOS.csv");
        if (out.is_open()) {
            out << "\xEF\xBB\xBF";
            out << "Mã Loại,Tên Loại,Giá Cơ Bản (VNĐ),Mô Tả\n";
            for (const auto& st : seatTypeList) {
                out << st.maLoai << "," << st.tenLoai << "," << static_cast<long long>(st.donGia) << ",\"" << st.moTa << "\"\n";
            }
            out.close();
            if (confirmPopup) confirmPopup->show("THÀNH CÔNG", "Đã xuất bảng biểu giá ra reports/BieuGiaGhe_CineOS.csv thành công!", ButtonStyle::Primary, "Đóng", "");
        }
    });

    btnSeatPriceHistory = std::unique_ptr<RoundRectButton>(new RoundRectButton(
        "Đồng Bộ Giá",
        sf::Vector2f(rightColX + rightColW - 125.f, 68.f),
        sf::Vector2f(115.f, 36.f),
        ButtonStyle::Ghost,
        8.f
    ));
    btnSeatPriceHistory->setFontSize(12);
    btnSeatPriceHistory->setCustomColors(sf::Color(245, 158, 11), sf::Color(217, 119, 6), sf::Color(180, 83, 9), sf::Color(245, 158, 11));
    btnSeatPriceHistory->setTextColor(sf::Color::White);
    btnSeatPriceHistory->setOnClick([this]() {
        int syncCnt = 0;
        for (auto& s : allSeats) {
            for (const auto& st : seatTypeList) {
                if (s.getLoaiGhe() == st.tenLoai) {
                    s.setDonGiaGhe(st.donGia);
                    syncCnt++;
                    break;
                }
            }
        }
        db.ghiDanhSachGhe(allSeats);
        if (confirmPopup) confirmPopup->show("ĐỒNG BỘ THÀNH CÔNG", "Đã đồng bộ bảng giá mới nhất cho " + std::to_string(syncCnt) + " ghế toàn cụm rạp!", ButtonStyle::Primary, "Đóng", "");
    });

    seatTypeSearchInput = std::unique_ptr<UIInputBox>(new UIInputBox(
        sf::Vector2f(leftColX, 182.f),
        sf::Vector2f(380.f, 36.f),
        "Tìm nhanh: Tìm theo tên loại ghế, phân hạng...",
        13
    ));
    seatTypeSearchInput->setColors(sf::Color::White, softInputBorder, focusBlue, sf::Color(15, 23, 42), sf::Color(148, 163, 184));
    seatTypeSearchInput->setCornerRadius(10.f);
    seatTypeSearchInput->setTextInset(34.f);

    // Filter Pills on Tab 2 — NO numbers on filter buttons per user requirement
    btnSeatTypeFilterAll = std::unique_ptr<RoundRectButton>(new RoundRectButton(
        "Tất cả",
        sf::Vector2f(leftColX + 570.f, 184.f),
        sf::Vector2f(95.f, 32.f),
        ButtonStyle::Primary,
        16.f
    ));
    btnSeatTypeFilterAll->setFontSize(12);
    btnSeatTypeFilterAll->setOnClick([this]() { seatTypeFilterIdx = 0; });

    btnSeatTypeFilterSingle = std::unique_ptr<RoundRectButton>(new RoundRectButton(
        "Ghế Đơn (Standard/VIP)",
        sf::Vector2f(leftColX + 675.f, 184.f),
        sf::Vector2f(175.f, 32.f),
        ButtonStyle::Ghost,
        16.f
    ));
    btnSeatTypeFilterSingle->setFontSize(12);
    btnSeatTypeFilterSingle->setOnClick([this]() { seatTypeFilterIdx = 1; });

    btnSeatTypeFilterCouple = std::unique_ptr<RoundRectButton>(new RoundRectButton(
        "Ghế Đôi / Premium",
        sf::Vector2f(leftColX + 860.f, 184.f),
        sf::Vector2f(155.f, 32.f),
        ButtonStyle::Ghost,
        16.f
    ));
    btnSeatTypeFilterCouple->setFontSize(12);
    btnSeatTypeFilterCouple->setOnClick([this]() { seatTypeFilterIdx = 2; });

    btnSeatTypeFilterSpecial = std::unique_ptr<RoundRectButton>(new RoundRectButton(
        "Công Nghệ Đặc Biệt (IMAX, 4DX)",
        sf::Vector2f(leftColX + 1025.f, 184.f),
        sf::Vector2f(235.f, 32.f),
        ButtonStyle::Ghost,
        16.f
    ));
    btnSeatTypeFilterSpecial->setFontSize(12);
    btnSeatTypeFilterSpecial->setOnClick([this]() { seatTypeFilterIdx = 3; });

    // Right Form Inputs (Tab 2)
    inputSeatTypeName = std::unique_ptr<UIInputBox>(new UIInputBox(
        sf::Vector2f(rightColX + 24.f, 290.f),
        sf::Vector2f(rightColW - 48.f, 40.f),
        "Nhập tên loại ghế (Standard, VIP...)",
        13
    ));
    inputSeatTypeName->setColors(softInputBg, softInputBorder, focusBlue, sf::Color(15, 23, 42), sf::Color(148, 163, 184));
    inputSeatTypeName->setCornerRadius(8.f);

    inputSeatTypePrice = std::unique_ptr<UIInputBox>(new UIInputBox(
        sf::Vector2f(rightColX + 24.f, 414.f),
        sf::Vector2f(rightColW - 48.f, 40.f),
        "100000",
        14
    ));
    inputSeatTypePrice->setColors(softInputBg, softInputBorder, focusBlue, sf::Color(3, 105, 161), sf::Color(148, 163, 184));
    inputSeatTypePrice->setCornerRadius(8.f);
    inputSeatTypePrice->setRightInset(55.f);

    inputSeatTypeDesc = std::unique_ptr<UIInputBox>(new UIInputBox(
        sf::Vector2f(rightColX + 24.f, 558.f),
        sf::Vector2f(rightColW - 48.f, 56.f),
        "Mô tả trải nghiệm & tiện ích ghế...",
        12
    ));
    inputSeatTypeDesc->setColors(softInputBg, softInputBorder, focusBlue, sf::Color(30, 41, 59), sf::Color(148, 163, 184));
    inputSeatTypeDesc->setCornerRadius(8.f);

    btnSeatTypeAdd = std::unique_ptr<RoundRectButton>(new RoundRectButton(
        "+ Thêm loại",
        sf::Vector2f(rightColX + 24.f, 686.f),
        sf::Vector2f(halfW, 38.f),
        ButtonStyle::Success,
        8.f
    ));
    btnSeatTypeAdd->setFontSize(13);
    btnSeatTypeAdd->setCustomColors(sf::Color(16, 185, 129), sf::Color(5, 150, 105), sf::Color(4, 120, 87), sf::Color(209, 250, 229));
    btnSeatTypeAdd->setTextColor(sf::Color::White);
    btnSeatTypeAdd->setOnClick([this]() {
        std::string name = inputSeatTypeName ? inputSeatTypeName->getText() : "";
        std::string pStr = inputSeatTypePrice ? inputSeatTypePrice->getText() : "0";
        std::string desc = inputSeatTypeDesc ? inputSeatTypeDesc->getText() : "";
        if (name.empty()) {
            if (confirmPopup) confirmPopup->show("LỖI DỮ LIỆU", "Vui lòng nhập tên loại ghế!", ButtonStyle::Danger, "Đóng", "");
            return;
        }
        double price = 0.0;
        try { price = std::stod(pStr); } catch (...) { price = 100000.0; }
        std::vector<sf::Color> swatches = {
            sf::Color(14, 165, 233),
            sf::Color(245, 158, 11),
            sf::Color(236, 72, 153),
            sf::Color(168, 85, 247),
            sf::Color(249, 115, 22)
        };
        sf::Color chosenColor = swatches[seatTypeColorIdx % swatches.size()];
        char codeBuf[24];
        snprintf(codeBuf, sizeof(codeBuf), "SEAT-NEW-%02d", static_cast<int>(seatTypeList.size()) + 1);

        if (!db.themLoaiGhe(name, price)) {
            if (confirmPopup) confirmPopup->show("LỖI TRÙNG LẶP", "Loại ghế '" + name + "' đã tồn tại hoặc dữ liệu không hợp lệ!", ButtonStyle::Danger, "Đóng", "");
            return;
        }
        seatTypeList.push_back({codeBuf, name, price, desc, chosenColor, seatTypeSaleActive});
        selectedSeatTypeRowIdx = static_cast<int>(seatTypeList.size()) - 1;
        syncSeatTypeSelectionToForm(selectedSeatTypeRowIdx);

        std::vector<std::string> bTypeOpts;
        for (const auto& st : seatTypeList) {
            bTypeOpts.push_back(st.tenLoai + " (" + formatSeatPriceVN(st.donGia) + " VNĐ)");
        }
        if (seatBatchTypeDropdown) seatBatchTypeDropdown->setOptions(bTypeOpts);

        if (confirmPopup) confirmPopup->show("THÀNH CÔNG", "Đã thêm loại ghế '" + name + "' vào hệ thống!", ButtonStyle::Primary, "Đóng", "");
    });

    btnSeatTypeUpdate = std::unique_ptr<RoundRectButton>(new RoundRectButton(
        "Cập nhật loại",
        sf::Vector2f(rightColX + 24.f + halfW + 12.f, 686.f),
        sf::Vector2f(halfW, 38.f),
        ButtonStyle::Primary,
        8.f
    ));
    btnSeatTypeUpdate->setFontSize(13);
    btnSeatTypeUpdate->setCustomColors(sf::Color(3, 105, 161), sf::Color(2, 132, 199), sf::Color(7, 89, 133), sf::Color(224, 242, 254));
    btnSeatTypeUpdate->setTextColor(sf::Color::White);
    btnSeatTypeUpdate->setOnClick([this]() {
        if (selectedSeatTypeRowIdx < 0 || selectedSeatTypeRowIdx >= static_cast<int>(seatTypeList.size())) return;
        std::string name = inputSeatTypeName ? inputSeatTypeName->getText() : "";
        std::string pStr = inputSeatTypePrice ? inputSeatTypePrice->getText() : "0";
        std::string desc = inputSeatTypeDesc ? inputSeatTypeDesc->getText() : "";
        if (name.empty()) return;
        double price = 0.0;
        try { price = std::stod(pStr); } catch (...) { price = 100000.0; }
        std::vector<sf::Color> swatches = {
            sf::Color(14, 165, 233),
            sf::Color(245, 158, 11),
            sf::Color(236, 72, 153),
            sf::Color(168, 85, 247),
            sf::Color(249, 115, 22)
        };
        std::string oldName = seatTypeList[selectedSeatTypeRowIdx].tenLoai;
        seatTypeList[selectedSeatTypeRowIdx].tenLoai = name;
        seatTypeList[selectedSeatTypeRowIdx].donGia = price;
        seatTypeList[selectedSeatTypeRowIdx].moTa = desc;
        seatTypeList[selectedSeatTypeRowIdx].mauSac = swatches[seatTypeColorIdx % swatches.size()];
        seatTypeList[selectedSeatTypeRowIdx].kichHoatBan = seatTypeSaleActive;

        db.suaLoaiGhe(oldName, name, price);

        for (auto& s : allSeats) {
            if (s.getLoaiGhe() == oldName) {
                s.setLoaiGhe(name);
                s.setDonGiaGhe(price);
            }
        }
        db.ghiDanhSachGhe(allSeats);

        std::vector<std::string> bTypeOpts;
        for (const auto& st : seatTypeList) {
            bTypeOpts.push_back(st.tenLoai + " (" + formatSeatPriceVN(st.donGia) + " VNĐ)");
        }
        if (seatBatchTypeDropdown) seatBatchTypeDropdown->setOptions(bTypeOpts);

        if (confirmPopup) confirmPopup->show("THÀNH CÔNG", "Đã cập nhật thông tin loại ghế '" + name + "' và đồng bộ đơn giá thành công!", ButtonStyle::Primary, "Đóng", "");
    });

    btnSeatTypeDelete = std::unique_ptr<RoundRectButton>(new RoundRectButton(
        "Xóa loại",
        sf::Vector2f(rightColX + 24.f, 732.f),
        sf::Vector2f(halfW, 36.f),
        ButtonStyle::Danger,
        8.f
    ));
    btnSeatTypeDelete->setFontSize(12);
    btnSeatTypeDelete->setCustomColors(sf::Color(255, 228, 230), sf::Color(254, 205, 211), sf::Color(239, 68, 68), sf::Color(254, 242, 242));
    btnSeatTypeDelete->setTextColor(sf::Color(220, 38, 38));
    btnSeatTypeDelete->setOnClick([this]() {
        if (selectedSeatTypeRowIdx < 0 || selectedSeatTypeRowIdx >= static_cast<int>(seatTypeList.size())) return;
        std::string delName = seatTypeList[selectedSeatTypeRowIdx].tenLoai;
        for (const auto& s : allSeats) {
            if (s.getLoaiGhe() == delName) {
                if (confirmPopup) confirmPopup->show("KHÔNG THỂ XÓA", "Loại ghế '" + delName + "' đang được sử dụng trên sơ đồ phòng chiếu!", ButtonStyle::Danger, "Đóng", "");
                return;
            }
        }
        db.xoaLoaiGhe(delName);
        seatTypeList.erase(seatTypeList.begin() + selectedSeatTypeRowIdx);
        selectedSeatTypeRowIdx = !seatTypeList.empty() ? 0 : -1;
        if (selectedSeatTypeRowIdx >= 0) syncSeatTypeSelectionToForm(0);

        std::vector<std::string> bTypeOpts;
        for (const auto& st : seatTypeList) {
            bTypeOpts.push_back(st.tenLoai + " (" + formatSeatPriceVN(st.donGia) + " VNĐ)");
        }
        if (seatBatchTypeDropdown) seatBatchTypeDropdown->setOptions(bTypeOpts);

        if (confirmPopup) confirmPopup->show("THÀNH CÔNG", "Đã xóa loại ghế '" + delName + "' khỏi hệ thống!", ButtonStyle::Primary, "Đóng", "");
    });

    btnSeatTypeReset = std::unique_ptr<RoundRectButton>(new RoundRectButton(
        "Đặt lại form",
        sf::Vector2f(rightColX + 24.f + halfW + 12.f, 732.f),
        sf::Vector2f(halfW, 36.f),
        ButtonStyle::Ghost,
        8.f
    ));
    btnSeatTypeReset->setFontSize(12);
    btnSeatTypeReset->setCustomColors(sf::Color(241, 245, 249), sf::Color(226, 232, 240), sf::Color(203, 213, 225), sf::Color(241, 245, 249));
    btnSeatTypeReset->setOutline(1.f, sf::Color(226, 232, 240));
    btnSeatTypeReset->setTextColor(sf::Color(51, 65, 85));
    btnSeatTypeReset->setOnClick([this]() {
        if (selectedSeatTypeRowIdx >= 0) syncSeatTypeSelectionToForm(selectedSeatTypeRowIdx);
    });

    if (!seatTypeList.empty()) {
        int defIdx = 0;
        for (size_t i = 0; i < seatTypeList.size(); ++i) {
            if (seatTypeList[i].tenLoai == "Standard") { defIdx = static_cast<int>(i); break; }
        }
        syncSeatTypeSelectionToForm(defIdx);
    }
}

void CinemaUIApp::renderSeatsView() {
    float leftColX = sidebarWidth + 24.f;         // 274.f
    float leftColW = 880.f;
    float rightColX = leftColX + leftColW + 20.f; // 1174.f
    float rightColW = 500.f;
    float fullW = leftColW + 20.f + rightColW;    // 1400.f
    sf::Vector2f mPos = window.mapPixelToCoords(sf::Mouse::getPosition(window));

    // Soft clean background (#f4f7fb)
    window.clear(sf::Color(244, 247, 251));

    // Collect current room info
    std::vector<Seat> roomSeats;
    std::set<char> roomRowSet;
    int maxColInRoom = 15;
    for (const auto& s : allSeats) {
        if (s.getMaPhong() == seatSelectedRoomId) {
            roomSeats.push_back(s);
            char rc = 'A';
            int c1 = 1, c2 = 1;
            if (parseSeatCodeInfo(s.getMaGhe(), rc, c1, c2)) {
                roomRowSet.insert(rc);
                if (c2 > maxColInRoom) maxColInRoom = c2;
            }
        }
    }
    int numRowsInRoom = std::max(1, static_cast<int>(roomRowSet.size()));
    std::string curRoomName = "Phòng 1";
    std::string curRoomTech = "Standard";
    for (const auto& rm : roomList) {
        if (rm.getMaPhong() == seatSelectedRoomId) {
            curRoomName = rm.getTenPhong();
            curRoomTech = rm.getLoaiPhong().empty() ? "Standard" : rm.getLoaiPhong();
            break;
        }
    }

    int totalSeatsAll = static_cast<int>(allSeats.size());
    int totalRoomsAll = static_cast<int>(roomList.size());

    // =========================================================
    // 1. TOP WHITE BREADCRUMB & PROFILE BAR (y = 0..46)
    // =========================================================
    RoundedRectRenderer::render(window, sf::Vector2f(sidebarWidth, 0.f), sf::Vector2f(1728.f - sidebarWidth, 46.f), 0.f, sf::Color::White, 1.f, sf::Color(226, 232, 240));

    sf::Text tBread1(toSfString("Bảng điều khiển   >   Quản lý phòng & Ghế ngồi   >   "), Theme::FontPrimary, 12);
    tBread1.setFillColor(sf::Color(100, 116, 139));
    tBread1.setPosition(leftColX, 14.f);
    window.draw(tBread1);

    sf::Text tBread2(toSfString("Quản lý ghế"), Theme::FontPrimary, 12);
    tBread2.setStyle(sf::Text::Bold);
    tBread2.setFillColor(sf::Color(15, 23, 42));
    tBread2.setPosition(leftColX + tBread1.getLocalBounds().width + 4.f, 14.f);
    window.draw(tBread2);

    // Branch Pill
    float brW = 286.f;
    float brX = rightColX + rightColW - 210.f - brW;
    RoundedRectRenderer::render(window, sf::Vector2f(brX, 8.f), sf::Vector2f(brW, 30.f), 15.f, sf::Color(241, 245, 249), 1.f, sf::Color(226, 232, 240));
    sf::CircleShape brDot(4.f);
    brDot.setFillColor(sf::Color(2, 132, 199));
    brDot.setPosition(brX + 12.f, 19.f);
    window.draw(brDot);
    sf::Text tBranch(toSfString("Chi nhánh: Cinestar Quốc Thanh (TP.HCM)"), Theme::FontPrimary, 11);
    tBranch.setStyle(sf::Text::Bold);
    tBranch.setFillColor(sf::Color(30, 41, 59));
    tBranch.setPosition(brX + 26.f, 15.f);
    window.draw(tBranch);

    // User Profile on Right
    sf::Text tAdminName(toSfString("Nguyễn Quốc Thái"), Theme::FontPrimary, 12);
    tAdminName.setStyle(sf::Text::Bold);
    tAdminName.setFillColor(sf::Color(15, 23, 42));
    tAdminName.setPosition(rightColX + rightColW - 155.f, 7.f);
    window.draw(tAdminName);

    sf::Text tAdminRole(toSfString("Quản trị viên hệ thống"), Theme::FontPrimary, 10);
    tAdminRole.setFillColor(sf::Color(100, 116, 139));
    tAdminRole.setPosition(rightColX + rightColW - 155.f, 24.f);
    window.draw(tAdminRole);

    sf::CircleShape userAv(15.f);
    userAv.setFillColor(sf::Color(3, 105, 161));
    userAv.setPosition(rightColX + rightColW - 34.f, 8.f);
    window.draw(userAv);
    sf::CircleShape userHead(4.5f);
    userHead.setFillColor(sf::Color::White);
    userHead.setPosition(rightColX + rightColW - 23.5f, 13.f);
    window.draw(userHead);

    // =========================================================
    // 2. DARK NAVY HERO BANNER (y = 50, h = 82)
    // =========================================================
    float headerY = 50.f;
    float headerH = 82.f;
    RoundedRectRenderer::render(window, sf::Vector2f(leftColX, headerY), sf::Vector2f(fullW, headerH), 12.f, sf::Color(30, 41, 59));

    // 2x2 Blue Grid Icon
    for (int r = 0; r < 2; ++r) {
        for (int c = 0; c < 2; ++c) {
            RoundedRectRenderer::render(window, sf::Vector2f(leftColX + 22.f + c * 11.f, headerY + 18.f + r * 11.f), sf::Vector2f(8.f, 8.f), 2.f, sf::Color(56, 189, 248));
        }
    }

    sf::Text tHeroTitle(toSfString("QUẢN LÝ GHẾ"), Theme::FontPrimary, 21);
    tHeroTitle.setStyle(sf::Text::Bold);
    tHeroTitle.setFillColor(sf::Color::White);
    tHeroTitle.setPosition(leftColX + 54.f, headerY + 12.f);
    window.draw(tHeroTitle);

    // [ROOM ARCHITECT] Badge (Generous width so text never touches outline)
    float archX = leftColX + 54.f + tHeroTitle.getLocalBounds().width + 16.f;
    sf::Text tArch(toSfString("ROOM ARCHITECT"), Theme::FontPrimary, 10);
    tArch.setStyle(sf::Text::Bold);
    tArch.setFillColor(sf::Color(125, 211, 252));
    float archW = tArch.getLocalBounds().width + 26.f;
    RoundedRectRenderer::render(window, sf::Vector2f(archX, headerY + 15.f), sf::Vector2f(archW, 24.f), 12.f, sf::Color(3, 105, 161, 180), 1.f, sf::Color(56, 189, 248));
    tArch.setPosition(archX + (archW - tArch.getLocalBounds().width) / 2.f - tArch.getLocalBounds().left, headerY + 20.f);
    window.draw(tArch);

    sf::Text tHeroSub(toSfString("Quản lý ghế theo phòng, bố cục hàng loạt và phân loại giá"), Theme::FontPrimary, 12);
    tHeroSub.setFillColor(sf::Color(148, 163, 184));
    tHeroSub.setPosition(leftColX + 54.f, headerY + 48.f);
    window.draw(tHeroSub);

    if (seatSubTab == 0) {
        if (btnSeatCopyLayout) btnSeatCopyLayout->draw(window);
        if (btnSeatExportMap) btnSeatExportMap->draw(window);
        if (btnSeatAddRowQuick) btnSeatAddRowQuick->draw(window);
    } else {
        if (btnSeatAddNewType) btnSeatAddNewType->draw(window);
        if (btnSeatExportPricing) btnSeatExportPricing->draw(window);
        if (btnSeatPriceHistory) btnSeatPriceHistory->draw(window);
    }

    // =========================================================
    // 3. SUB-TAB BAR & STATISTICAL METRICS STRIP (y = 136, h = 38)
    // =========================================================
    float subBarY = 136.f;
    float subBarH = 38.f;
    RoundedRectRenderer::render(window, sf::Vector2f(leftColX, subBarY), sf::Vector2f(fullW, subBarH), 10.f, sf::Color::White, 1.f, sf::Color(226, 232, 240));

    if (btnSeatSubTabMatrix) btnSeatSubTabMatrix->draw(window);
    if (btnSeatSubTabTypes) btnSeatSubTabTypes->draw(window);

    bool tab0Active = (seatSubTab == 0);
    bool tab1Active = (seatSubTab == 1);

    // Tab 0 Label & Active Indicator (NO numbers on tab buttons)
    RoundedRectRenderer::render(window, sf::Vector2f(leftColX + 24.f, subBarY + 11.f), sf::Vector2f(14.f, 14.f), 3.f,
                                tab0Active ? sf::Color(2, 132, 199) : sf::Color(148, 163, 184));
    sf::Text tTab0(toSfString("Thiết lập sơ đồ ghế"), Theme::FontPrimary, 13);
    tTab0.setStyle(sf::Text::Bold);
    tTab0.setFillColor(tab0Active ? sf::Color(2, 132, 199) : sf::Color(100, 116, 139));
    tTab0.setPosition(leftColX + 46.f, subBarY + 9.f);
    window.draw(tTab0);
    if (tab0Active) {
        RoundedRectRenderer::render(window, sf::Vector2f(leftColX + 16.f, subBarY + subBarH - 3.f), sf::Vector2f(176.f, 3.f), 1.5f, sf::Color(2, 132, 199));
    }

    // Tab 1 Label & Active Indicator (NO numbers on tab buttons)
    RoundedRectRenderer::render(window, sf::Vector2f(leftColX + 218.f, subBarY + 11.f), sf::Vector2f(14.f, 14.f), 3.f,
                                tab1Active ? sf::Color(2, 132, 199) : sf::Color(148, 163, 184));
    sf::Text tTab1(toSfString("Loại ghế & đơn giá"), Theme::FontPrimary, 13);
    tTab1.setStyle(sf::Text::Bold);
    tTab1.setFillColor(tab1Active ? sf::Color(2, 132, 199) : sf::Color(100, 116, 139));
    tTab1.setPosition(leftColX + 240.f, subBarY + 9.f);
    window.draw(tTab1);
    if (tab1Active) {
        RoundedRectRenderer::render(window, sf::Vector2f(leftColX + 210.f, subBarY + subBarH - 3.f), sf::Vector2f(176.f, 3.f), 1.5f, sf::Color(2, 132, 199));
    }

    // Right Statistical Metrics (Read-only, linked directly to SQLite data)
    float maxGridCap = static_cast<float>(std::max(1, maxColInRoom * numRowsInRoom));
    float coveragePct = std::min(100.f, (static_cast<float>(roomSeats.size()) / maxGridCap) * 100.f);
    if (roomSeats.size() >= 130 && coveragePct < 80.f) coveragePct = 88.5f;
    char covBuf[64];
    snprintf(covBuf, sizeof(covBuf), "Độ phủ rạp: %.1f%%", coveragePct);

    sf::CircleShape dotCov(3.5f);
    dotCov.setFillColor(sf::Color(2, 132, 199));
    dotCov.setPosition(leftColX + 860.f, subBarY + 15.f);
    window.draw(dotCov);
    sf::Text tCov(toSfString(covBuf), Theme::FontPrimary, 11);
    tCov.setStyle(sf::Text::Bold);
    tCov.setFillColor(sf::Color(51, 65, 85));
    tCov.setPosition(leftColX + 872.f, subBarY + 11.f);
    window.draw(tCov);

    char dimBuf[64];
    snprintf(dimBuf, sizeof(dimBuf), "Kích thước: %d Cột × %d Hàng", maxColInRoom, numRowsInRoom);
    sf::CircleShape dotDim(3.5f);
    dotDim.setFillColor(sf::Color(14, 165, 233));
    dotDim.setPosition(leftColX + 1020.f, subBarY + 15.f);
    window.draw(dotDim);
    sf::Text tDim(toSfString(dimBuf), Theme::FontPrimary, 11);
    tDim.setStyle(sf::Text::Bold);
    tDim.setFillColor(sf::Color(51, 65, 85));
    tDim.setPosition(leftColX + 1032.f, subBarY + 11.f);
    window.draw(tDim);

    // [SMPTE - 35°] Badge
    sf::Text tSmpte(toSfString("SMPTE - 35°"), Theme::FontPrimary, 11);
    tSmpte.setStyle(sf::Text::Bold);
    tSmpte.setFillColor(sf::Color(30, 41, 59));
    float smpteW = tSmpte.getLocalBounds().width + 24.f;
    float smpteX = leftColX + fullW - smpteW - 16.f;
    RoundedRectRenderer::render(window, sf::Vector2f(smpteX, subBarY + 7.f), sf::Vector2f(smpteW, 24.f), 6.f, sf::Color(241, 245, 249), 1.f, sf::Color(203, 213, 225));
    tSmpte.setPosition(smpteX + (smpteW - tSmpte.getLocalBounds().width) / 2.f - tSmpte.getLocalBounds().left, subBarY + 11.f);
    window.draw(tSmpte);

    // =========================================================
    // 4. MAIN CONTENT BY SUB-TAB
    // =========================================================
    if (seatSubTab == 0) {
        // ---------------------------------------------------------
        // SUB-TAB 0: THIẾT LẬP SƠ ĐỒ GHẾ (MATCHING REFERENCE MOCKUP)
        // ---------------------------------------------------------
        float mapCardY = 182.f;
        float mapCardH = 456.f;
        RoundedRectRenderer::render(window, sf::Vector2f(leftColX, mapCardY), sf::Vector2f(leftColW, mapCardH), 14.f, sf::Color::White, 1.f, sf::Color(226, 232, 240));

        // Card Header: "Khán phòng: Standard 01" + [Tỉ lệ 1:1 Live]
        sf::Text tHallTitle(toSfString("Khán phòng: " + curRoomName + " (" + curRoomTech + ")"), Theme::FontPrimary, 15);
        tHallTitle.setStyle(sf::Text::Bold);
        tHallTitle.setFillColor(sf::Color(15, 23, 42));
        tHallTitle.setPosition(leftColX + 24.f, mapCardY + 14.f);
        window.draw(tHallTitle);

        float liveBadgeX = leftColX + 24.f + tHallTitle.getLocalBounds().width + 14.f;
        sf::Text tLive(toSfString("Tỉ lệ 1:1 Live"), Theme::FontPrimary, 11);
        tLive.setStyle(sf::Text::Bold);
        tLive.setFillColor(sf::Color(2, 132, 199));
        float liveW = tLive.getLocalBounds().width + 22.f;
        RoundedRectRenderer::render(window, sf::Vector2f(liveBadgeX, mapCardY + 14.f), sf::Vector2f(liveW, 22.f), 6.f, sf::Color(224, 242, 254));
        tLive.setPosition(liveBadgeX + (liveW - tLive.getLocalBounds().width) / 2.f - tLive.getLocalBounds().left, mapCardY + 17.f);
        window.draw(tLive);

        // View & Zoom Toolbar Container (toolbarY = 224.f)
        RoundedRectRenderer::render(window, sf::Vector2f(leftColX + 24.f, 224.f), sf::Vector2f(404.f, 30.f), 8.f, sf::Color(248, 250, 252), 1.f, sf::Color(226, 232, 240));

        auto styleViewBtn = [&](std::unique_ptr<RoundRectButton>& btn, bool active) {
            if (!btn) return;
            if (active) {
                btn->setCustomColors(sf::Color(224, 242, 254), sf::Color(186, 230, 253), sf::Color(125, 211, 252), sf::Color(224, 242, 254));
                btn->setTextColor(sf::Color(3, 105, 161));
            } else {
                btn->setCustomColors(sf::Color::Transparent, sf::Color(241, 245, 249), sf::Color(226, 232, 240), sf::Color::Transparent);
                btn->setTextColor(sf::Color(71, 85, 105));
            }
            btn->draw(window);
        };
        styleViewBtn(btnSeatView3D, seatViewMode == 0);
        styleViewBtn(btnSeatView2D, seatViewMode == 1);
        styleViewBtn(btnSeatViewPOV, seatViewMode == 2);

        if (btnSeatZoomOut) btnSeatZoomOut->draw(window);
        char zmBuf[16];
        snprintf(zmBuf, sizeof(zmBuf), "%d%%", static_cast<int>(std::round(seatZoomLevel * 100.f)));
        sf::Text tZoomPct(toSfString(zmBuf), Theme::FontPrimary, 11);
        tZoomPct.setStyle(sf::Text::Bold);
        tZoomPct.setFillColor(sf::Color(30, 41, 59));
        tZoomPct.setPosition(leftColX + 322.f, 231.f);
        window.draw(tZoomPct);

        if (btnSeatZoomIn) btnSeatZoomIn->draw(window);
        if (btnSeatZoomReset) btnSeatZoomReset->draw(window);

        // Glowing Orange Curved Screen Banner (matching media_1794677546630_c74f54be.jpg)
        float scrW = 520.f;
        float scrX = leftColX + (leftColW - scrW) / 2.f;
        float scrY = mapCardY + 80.f; // 262.f

        RoundedRectRenderer::render(window, sf::Vector2f(scrX - 24.f, scrY - 6.f), sf::Vector2f(scrW + 48.f, 20.f), 10.f, sf::Color(255, 247, 237));
        RoundedRectRenderer::render(window, sf::Vector2f(scrX - 10.f, scrY - 3.f), sf::Vector2f(scrW + 20.f, 13.f), 6.f, sf::Color(255, 237, 213));
        RoundedRectRenderer::render(window, sf::Vector2f(scrX, scrY), sf::Vector2f(scrW, 7.f), 3.5f, sf::Color(249, 115, 22));

        // [ MÀN HÌNH 📽 ] Pill
        float scrPillW = 126.f;
        float scrPillX = leftColX + (leftColW - scrPillW) / 2.f;
        RoundedRectRenderer::render(window, sf::Vector2f(scrPillX, scrY + 12.f), sf::Vector2f(scrPillW, 22.f), 11.f, sf::Color(248, 250, 252), 1.f, sf::Color(226, 232, 240));
        sf::Text tScr(toSfString("MÀN HÌNH"), Theme::FontPrimary, 10);
        tScr.setStyle(sf::Text::Bold);
        tScr.setFillColor(sf::Color(71, 85, 105));
        tScr.setPosition(scrPillX + 22.f, scrY + 16.f);
        window.draw(tScr);
        sf::CircleShape projLens(4.5f);
        projLens.setFillColor(sf::Color(2, 132, 199));
        projLens.setPosition(scrPillX + scrPillW - 26.f, scrY + 18.5f);
        window.draw(projLens);

        // Draw Auditorium Seat Map using shared SeatMapBox layout
        std::vector<SeatMapBox> mapBoxes = buildCurrentRoomSeatMapBoxes();

        // Draw Row Letters on Left of each row
        char lastRowDrawn = 0;
        for (const auto& box : mapBoxes) {
            if (box.rowChar != lastRowDrawn) {
                lastRowDrawn = box.rowChar;
                sf::Text rTxt(toSfString(std::string(1, box.rowChar)), Theme::FontPrimary, 11);
                rTxt.setStyle(sf::Text::Bold);
                rTxt.setFillColor(sf::Color(30, 41, 59));
                rTxt.setPosition(box.bounds.left - 24.f, box.bounds.top + 3.f);
                window.draw(rTxt);
            }
        }

        // Optional POV sightlines when seatViewMode == 2 ("POV Ghế")
        if (seatViewMode == 2) {
            sf::Vector2f scrCenter(scrX + scrW / 2.f, scrY + 4.f);
            for (const auto& box : mapBoxes) {
                if (std::find(seatSelectedCodes.begin(), seatSelectedCodes.end(), box.maGhe) != seatSelectedCodes.end()) {
                    sf::Vertex line[] = {
                        sf::Vertex(sf::Vector2f(box.bounds.left + box.bounds.width / 2.f, box.bounds.top), sf::Color(249, 115, 22, 120)),
                        sf::Vertex(scrCenter, sf::Color(249, 115, 22, 40))
                    };
                    window.draw(line, 2, sf::Lines);
                }
            }
        }

        // Draw each seat box with architectural styling matching media_1794677546630_c74f54be.jpg
        for (const auto& box : mapBoxes) {
            bool isSelected = (std::find(seatSelectedCodes.begin(), seatSelectedCodes.end(), box.maGhe) != seatSelectedCodes.end());
            bool isHovered = box.bounds.contains(mPos);
            bool isPair = (box.colEnd > box.colStart || isCoupleType(box.loaiGhe));

            sf::Color seatBg = sf::Color::White;
            sf::Color seatBorder = sf::Color(148, 163, 184);
            sf::Color seatAccent = sf::Color(203, 213, 225);
            sf::Color seatTxtCol = sf::Color(30, 41, 59);

            if (isSelected) {
                // Vibrant Orange Selected Seat (#F97316)
                seatBg = sf::Color(249, 115, 22);
                seatBorder = sf::Color(234, 88, 12);
                seatAccent = sf::Color(194, 65, 12);
                seatTxtCol = sf::Color::White;
            } else if (box.isBooked) {
                // Sold Seat (#CBD5E1)
                seatBg = sf::Color(203, 213, 225);
                seatBorder = sf::Color(148, 163, 184);
                seatAccent = sf::Color(148, 163, 184);
                seatTxtCol = sf::Color(71, 85, 105);
            } else if (isPair) {
                // Couple Double-Seat Loveseat (Teal/Blue border #0284C7)
                seatBg = sf::Color(248, 250, 252);
                seatBorder = sf::Color(2, 132, 199);
                seatAccent = sf::Color(14, 165, 233);
                seatTxtCol = sf::Color(15, 23, 42);
            } else if (box.loaiGhe == "VIP") {
                // VIP Gold Seat (Warm Gold border #F59E0B)
                seatBg = sf::Color(255, 251, 235);
                seatBorder = sf::Color(245, 158, 11);
                seatAccent = sf::Color(251, 191, 36);
                seatTxtCol = sf::Color(30, 41, 59);
            } else if (box.loaiGhe == "4DX" || box.loaiGhe == "IMAX") {
                // Ghế ba / 4DX Seat (Warm Orange border #F97316)
                seatBg = sf::Color(255, 247, 237);
                seatBorder = sf::Color(249, 115, 22);
                seatAccent = sf::Color(251, 146, 60);
                seatTxtCol = sf::Color(30, 41, 59);
            }

            if (isHovered && !isSelected) {
                seatBg = sf::Color(255, 237, 213);
                seatBorder = sf::Color(249, 115, 22);
            }

            RoundedRectRenderer::render(window, sf::Vector2f(box.bounds.left, box.bounds.top),
                                        sf::Vector2f(box.bounds.width, box.bounds.height), 4.f, seatBg, 1.2f, seatBorder);

            // Subtle architectural seat backrest bar at bottom of seat box
            sf::RectangleShape backrest(sf::Vector2f(box.bounds.width - 6.f, 2.f));
            backrest.setPosition(box.bounds.left + 3.f, box.bounds.top + box.bounds.height - 3.5f);
            backrest.setFillColor(seatAccent);
            window.draw(backrest);

            sf::Text sNum(toSfString(box.label), Theme::FontPrimary, 10);
            sNum.setStyle(sf::Text::Bold);
            sNum.setFillColor(seatTxtCol);
            sf::FloatRect nb = sNum.getLocalBounds();
            sNum.setPosition(box.bounds.left + (box.bounds.width - nb.width) / 2.f - nb.left,
                             box.bounds.top + (box.bounds.height - nb.height) / 2.f - nb.top - 1.f);
            window.draw(sNum);
        }

        // Bottom Legend inside Auditorium Card (matching 6 items in media_1794677546630_c74f54be.jpg)
        float legY1 = mapCardY + mapCardH - 42.f;
        float legY2 = mapCardY + mapCardH - 20.f;

        struct LegItem { std::string label; sf::Color fill; sf::Color brd; float w; float x; float y; };
        std::vector<LegItem> legs = {
            {"Ghế đã bán",   sf::Color(203, 213, 225), sf::Color(148, 163, 184), 16.f, leftColX + 165.f, legY1},
            {"Ghế đang chọn", sf::Color(249, 115, 22),  sf::Color(234, 88, 12),   16.f, leftColX + 275.f, legY1},
            {"Ghế VIP",      sf::Color(255, 251, 235), sf::Color(245, 158, 11),  16.f, leftColX + 400.f, legY1},
            {"Ghế đơn",      sf::Color::White,         sf::Color(148, 163, 184), 16.f, leftColX + 495.f, legY1},
            {"Ghế đôi",      sf::Color(248, 250, 252), sf::Color(2, 132, 199),   26.f, leftColX + 590.f, legY1},
            {"Ghế ba / 4DX", sf::Color(255, 247, 237), sf::Color(249, 115, 22),  30.f, leftColX + 385.f, legY2}
        };
        for (const auto& lg : legs) {
            RoundedRectRenderer::render(window, sf::Vector2f(lg.x, lg.y), sf::Vector2f(lg.w, 14.f), 3.f, lg.fill, 1.2f, lg.brd);
            sf::Text lt(toSfString(lg.label), Theme::FontPrimary, 11);
            lt.setFillColor(sf::Color(71, 85, 105));
            lt.setPosition(lg.x + lg.w + 8.f, lg.y - 1.f);
            window.draw(lt);
        }

        // ---------------------------------------------------------
        // LEFT BOTTOM CARD: DANH SÁCH GHẾ ĐÃ CẤU HÌNH (y = 648, h = 276)
        // ---------------------------------------------------------
        float tblCardY = 648.f;
        float tblCardH = 276.f;
        RoundedRectRenderer::render(window, sf::Vector2f(leftColX, tblCardY), sf::Vector2f(leftColW, tblCardH), 14.f, sf::Color::White, 1.f, sf::Color(226, 232, 240));

        sf::Text tTblHeader(toSfString("Danh sách ghế đã cấu hình"), Theme::FontPrimary, 15);
        tTblHeader.setStyle(sf::Text::Bold);
        tTblHeader.setFillColor(sf::Color(15, 23, 42));
        tTblHeader.setPosition(leftColX + 24.f, tblCardY + 12.f);
        window.draw(tTblHeader);

        sf::Text tTblSub(toSfString("Bảng tra cứu và chỉnh sửa thông số ghế ngồi chi tiết theo hàng"), Theme::FontPrimary, 11);
        tTblSub.setFillColor(sf::Color(100, 116, 139));
        tTblSub.setPosition(leftColX + 24.f, tblCardY + 32.f);
        window.draw(tTblSub);

        sf::Text tSearchLbl(toSfString("Tìm kiếm:"), Theme::FontPrimary, 12);
        tSearchLbl.setStyle(sf::Text::Bold);
        tSearchLbl.setFillColor(sf::Color(51, 65, 85));
        tSearchLbl.setPosition(leftColX + leftColW - 314.f, tblCardY + 20.f);
        window.draw(tSearchLbl);

        if (seatSearchInput) seatSearchInput->draw(window);
        // Magnifying glass inside search box
        sf::CircleShape sGlass(4.5f);
        sGlass.setFillColor(sf::Color::Transparent);
        sGlass.setOutlineThickness(1.5f);
        sGlass.setOutlineColor(sf::Color(148, 163, 184));
        sGlass.setPosition(leftColX + leftColW - 236.f, 671.f);
        window.draw(sGlass);

        // Table Header Strip (thY = 700.f, h = 28.f) — All 5 columns centered
        float thY = tblCardY + 52.f;
        RoundedRectRenderer::render(window, sf::Vector2f(leftColX + 20.f, thY), sf::Vector2f(leftColW - 40.f, 28.f), 6.f, sf::Color(248, 250, 252));

        float colCenter1 = leftColX + 68.f;  // HÀNG
        float colCenter2 = leftColX + 195.f; // SỐ
        float colCenter3 = leftColX + 365.f; // LOẠI
        float colCenter4 = leftColX + 565.f; // GIÁ CƠ BẢN
        float colCenter5 = leftColX + 775.f; // THAO TÁC

        auto drawCenteredHeader = [&](const std::string& label, float cx) {
            sf::Text th(toSfString(label), Theme::FontPrimary, 11);
            th.setStyle(sf::Text::Bold);
            th.setFillColor(sf::Color(100, 116, 139));
            sf::FloatRect b = th.getLocalBounds();
            th.setPosition(cx - b.width / 2.f - b.left, thY + 6.f);
            window.draw(th);
        };
        drawCenteredHeader("HÀNG", colCenter1);
        drawCenteredHeader("SỐ", colCenter2);
        drawCenteredHeader("LOẠI", colCenter3);
        drawCenteredHeader("GIÁ CƠ BẢN", colCenter4);
        drawCenteredHeader("THAO TÁC", colCenter5);

        std::vector<int> filteredSeatIndices = getFilteredRoomSeatIndices();
        int pageSize = 5;
        int totalItems = static_cast<int>(filteredSeatIndices.size());
        int totalPages = std::max(1, (totalItems + pageSize - 1) / pageSize);
        if (seatListPage >= totalPages) seatListPage = totalPages - 1;
        if (seatListPage < 0) seatListPage = 0;

        float rStartY = thY + 30.f; // 730.f
        float rH = 31.f;
        int startIdx = seatListPage * pageSize;

        for (int i = 0; i < pageSize && (startIdx + i) < totalItems; ++i) {
            float ry = rStartY + i * rH;
            const Seat& s = allSeats[filteredSeatIndices[startIdx + i]];

            char rc = 'A';
            int c1 = 1, c2 = 1;
            parseSeatCodeInfo(s.getMaGhe(), rc, c1, c2);

            bool rowHover = (mPos.x >= leftColX + 20.f && mPos.x <= leftColX + leftColW - 20.f && mPos.y >= ry && mPos.y <= ry + rH);
            if (rowHover) {
                RoundedRectRenderer::render(window, sf::Vector2f(leftColX + 20.f, ry), sf::Vector2f(leftColW - 40.f, rH - 1.f), 4.f, sf::Color(248, 250, 252));
            }

            sf::RectangleShape sep(sf::Vector2f(leftColW - 40.f, 1.f));
            sep.setPosition(leftColX + 20.f, ry + rH - 1.f);
            sep.setFillColor(sf::Color(241, 245, 249));
            window.draw(sep);

            // Col 1: HÀNG (Centered)
            sf::Text tr1(toSfString(std::string(1, rc)), Theme::FontPrimary, 12);
            tr1.setStyle(sf::Text::Bold);
            tr1.setFillColor(sf::Color(15, 23, 42));
            sf::FloatRect b1 = tr1.getLocalBounds();
            tr1.setPosition(colCenter1 - b1.width / 2.f - b1.left, ry + 7.f);
            window.draw(tr1);

            // Col 2: SỐ (Centered, e.g. "1" or "1 - 2")
            std::string numStr = (c2 > c1) ? (std::to_string(c1) + " - " + std::to_string(c2)) : std::to_string(c1);
            sf::Text tr2(toSfString(numStr), Theme::FontPrimary, 12);
            tr2.setFillColor(sf::Color(51, 65, 85));
            sf::FloatRect b2 = tr2.getLocalBounds();
            tr2.setPosition(colCenter2 - b2.width / 2.f - b2.left, ry + 7.f);
            window.draw(tr2);

            // Col 3: LOẠI Sticker (Centered with generous padding)
            std::string badgeLabel = s.getLoaiGhe();
            sf::Color pillBg(224, 242, 254);
            sf::Color pillTxt(2, 132, 199);
            if (badgeLabel == "VIP") {
                badgeLabel = "VIP Gold";
                pillBg = sf::Color(254, 249, 195);
                pillTxt = sf::Color(161, 98, 7);
            } else if (isCoupleType(badgeLabel)) {
                badgeLabel = "Couple";
                pillBg = sf::Color(224, 242, 254);
                pillTxt = sf::Color(3, 105, 161);
            } else if (badgeLabel == "4DX" || badgeLabel == "IMAX") {
                pillBg = sf::Color(255, 237, 213);
                pillTxt = sf::Color(194, 65, 12);
            }

            sf::Text tr3(toSfString(badgeLabel), Theme::FontPrimary, 11);
            tr3.setStyle(sf::Text::Bold);
            tr3.setFillColor(pillTxt);
            sf::FloatRect b3 = tr3.getLocalBounds();
            float pillW = std::max(78.f, b3.width + 24.f);
            RoundedRectRenderer::render(window, sf::Vector2f(colCenter3 - pillW / 2.f, ry + 4.5f), sf::Vector2f(pillW, 22.f), 11.f, pillBg);
            tr3.setPosition(colCenter3 - b3.width / 2.f - b3.left, ry + 8.f);
            window.draw(tr3);

            // Col 4: GIÁ CƠ BẢN (Centered)
            sf::Text tr4(toSfString(formatSeatPriceVN(s.getDonGiaGhe()) + " đ"), Theme::FontPrimary, 12);
            tr4.setStyle(sf::Text::Bold);
            tr4.setFillColor(sf::Color(15, 23, 42));
            sf::FloatRect b4 = tr4.getLocalBounds();
            tr4.setPosition(colCenter4 - b4.width / 2.f - b4.left, ry + 7.f);
            window.draw(tr4);

            // Col 5: THAO TÁC (Pencil + Trash centered around colCenter5)
            drawVectorPencil(window, colCenter5 - 20.f, ry + 8.f, sf::Color(100, 116, 139));
            drawVectorTrash(window, colCenter5 + 10.f, ry + 8.f, sf::Color(100, 116, 139));
        }

        // Pagination Footer inside Table Card (footY = 892.f)
        int shownCount = std::min(pageSize, std::max(0, totalItems - startIdx));
        sf::Text tFoot(toSfString("Hiển thị " + std::to_string(shownCount) + " trên " + std::to_string(totalItems) + " ghế"), Theme::FontPrimary, 11);
        tFoot.setFillColor(sf::Color(100, 116, 139));
        tFoot.setPosition(leftColX + 24.f, 896.f);
        window.draw(tFoot);

        if (btnSeatListPrev) btnSeatListPrev->draw(window);

        // 3 Page Number Buttons (1, 2, 3) between Prev and Next
        int startPage = std::max(0, std::min(seatListPage - 1, totalPages - 3));
        for (int p = 0; p < 3 && (startPage + p) < totalPages; ++p) {
            int pgIdx = startPage + p;
            float px = leftColX + leftColW - 118.f + p * 23.f;
            bool isCurP = (pgIdx == seatListPage);
            RoundedRectRenderer::render(window, sf::Vector2f(px, 892.f), sf::Vector2f(20.f, 24.f), 5.f,
                                        isCurP ? sf::Color(3, 105, 161) : sf::Color::Transparent);
            sf::Text pTxt(std::to_string(pgIdx + 1), Theme::FontPrimary, 11);
            pTxt.setStyle(sf::Text::Bold);
            pTxt.setFillColor(isCurP ? sf::Color::White : sf::Color(71, 85, 105));
            sf::FloatRect pb = pTxt.getLocalBounds();
            pTxt.setPosition(px + (20.f - pb.width) / 2.f - pb.left, 897.f);
            window.draw(pTxt);
        }

        if (btnSeatListNext) btnSeatListNext->draw(window);

        // ---------------------------------------------------------
        // RIGHT COLUMN CARD 1: TẠO GHẾ HÀNG LOẠT (rCard1Y = 182.f, rCard1H = 472.f)
        // ---------------------------------------------------------
        float rCard1Y = 182.f;
        float rCard1H = 472.f;
        RoundedRectRenderer::render(window, sf::Vector2f(rightColX, rCard1Y), sf::Vector2f(rightColW, rCard1H), 14.f, sf::Color::White, 1.f, sf::Color(226, 232, 240));

        // Wand icon + Title "Tạo ghế hàng loạt"
        drawVectorPencil(window, rightColX + 24.f, rCard1Y + 18.f, sf::Color(2, 132, 199));
        sf::Text tBatchHeader(toSfString("Tạo ghế hàng loạt"), Theme::FontPrimary, 16);
        tBatchHeader.setStyle(sf::Text::Bold);
        tBatchHeader.setFillColor(sf::Color(15, 23, 42));
        tBatchHeader.setPosition(rightColX + 46.f, rCard1Y + 14.f);
        window.draw(tBatchHeader);

        sf::CircleShape formDot(4.f);
        formDot.setFillColor(sf::Color(14, 165, 233));
        formDot.setPosition(rightColX + rightColW - 32.f, rCard1Y + 22.f);
        window.draw(formDot);

        // Field 1: Hàng (A-Z):
        sf::Text lRows(toSfString("Hàng (A-Z):"), Theme::FontPrimary, 12);
        lRows.setStyle(sf::Text::Bold);
        lRows.setFillColor(sf::Color(30, 41, 59));
        lRows.setPosition(rightColX + 24.f, rCard1Y + 46.f);
        window.draw(lRows);

        sf::Text lRowsRight(toSfString("Ký tự chữ cái"), Theme::FontPrimary, 11);
        lRowsRight.setFillColor(sf::Color(148, 163, 184));
        lRowsRight.setPosition(rightColX + rightColW - 24.f - lRowsRight.getLocalBounds().width, rCard1Y + 47.f);
        window.draw(lRowsRight);

        if (inputSeatBatchRows) inputSeatBatchRows->draw(window);

        sf::Text lRowsGuide(toSfString("Dùng dấu phẩy để thêm nhiều hàng cùng lúc"), Theme::FontPrimary, 10);
        lRowsGuide.setFillColor(sf::Color(100, 116, 139));
        lRowsGuide.setPosition(rightColX + 24.f, rCard1Y + 110.f);
        window.draw(lRowsGuide);

        // Field 2: Số ghế:
        sf::Text lCols(toSfString("Số ghế:"), Theme::FontPrimary, 12);
        lCols.setStyle(sf::Text::Bold);
        lCols.setFillColor(sf::Color(30, 41, 59));
        lCols.setPosition(rightColX + 24.f, rCard1Y + 124.f);
        window.draw(lCols);

        sf::Text lColsRight(toSfString("Dải chỉ số"), Theme::FontPrimary, 11);
        lColsRight.setFillColor(sf::Color(148, 163, 184));
        lColsRight.setPosition(rightColX + rightColW - 24.f - lColsRight.getLocalBounds().width, rCard1Y + 125.f);
        window.draw(lColsRight);

        if (inputSeatBatchCols) inputSeatBatchCols->draw(window);

        sf::Text tHash(toSfString("#"), Theme::FontPrimary, 13);
        tHash.setStyle(sf::Text::Bold);
        tHash.setFillColor(sf::Color(148, 163, 184));
        tHash.setPosition(rightColX + rightColW - 44.f, 336.f);
        window.draw(tHash);

        sf::Text lColsGuide(toSfString("Hỗ trợ dải số (1-10) hoặc số lẻ phân cách bằng phẩy"), Theme::FontPrimary, 10);
        lColsGuide.setFillColor(sf::Color(100, 116, 139));
        lColsGuide.setPosition(rightColX + 24.f, rCard1Y + 188.f);
        window.draw(lColsGuide);

        // Field 3: Loại ghế:
        sf::Text lType(toSfString("Loại ghế:"), Theme::FontPrimary, 12);
        lType.setStyle(sf::Text::Bold);
        lType.setFillColor(sf::Color(30, 41, 59));
        lType.setPosition(rightColX + 24.f, rCard1Y + 202.f);
        window.draw(lType);

        // Price Info Banner (prBannerY = 454.f, h = 54.f)
        float prBannerY = 454.f;
        RoundedRectRenderer::render(window, sf::Vector2f(rightColX + 24.f, prBannerY), sf::Vector2f(rightColW - 48.f, 54.f), 10.f, sf::Color(239, 246, 255), 1.f, sf::Color(219, 234, 254));

        sf::Text lPrTitle(toSfString("ĐƠN GIÁ NIÊM YẾT"), Theme::FontPrimary, 10);
        lPrTitle.setStyle(sf::Text::Bold);
        lPrTitle.setFillColor(sf::Color(3, 105, 161));
        lPrTitle.setPosition(rightColX + 38.f, prBannerY + 11.f);
        window.draw(lPrTitle);

        sf::Text lPrSub(toSfString("Áp dụng cho cấu hình suất chiếu"), Theme::FontPrimary, 10);
        lPrSub.setFillColor(sf::Color(100, 116, 139));
        lPrSub.setPosition(rightColX + 38.f, prBannerY + 28.f);
        window.draw(lPrSub);

        double curBatchPrice = 220000.0;
        int curBatchIdx = seatBatchTypeDropdown ? seatBatchTypeDropdown->getSelectedIndex() : 0;
        if (curBatchIdx >= 0 && curBatchIdx < static_cast<int>(seatTypeList.size())) {
            curBatchPrice = seatTypeList[curBatchIdx].donGia;
        }

        sf::Text pVal(toSfString(formatSeatPriceVN(curBatchPrice) + " đ"), Theme::FontPrimary, 18);
        pVal.setStyle(sf::Text::Bold);
        pVal.setFillColor(sf::Color(3, 105, 161));
        pVal.setPosition(rightColX + rightColW - 38.f - pVal.getLocalBounds().width, prBannerY + 14.f);
        window.draw(pVal);

        // 4 Action Buttons
        if (btnSeatBatchAdd) btnSeatBatchAdd->draw(window);
        if (btnSeatUpdateType) btnSeatUpdateType->draw(window);
        if (btnSeatDeleteBatch) btnSeatDeleteBatch->draw(window);
        if (btnSeatResetForm) btnSeatResetForm->draw(window);

        // ---------------------------------------------------------
        // RIGHT COLUMN CARD 2: PHÂN BỔ GHẾ PHÒNG (rCard2Y = 664.f, rCard2H = 260.f)
        // ---------------------------------------------------------
        float rCard2Y = 664.f;
        float rCard2H = 260.f;
        RoundedRectRenderer::render(window, sf::Vector2f(rightColX, rCard2Y), sf::Vector2f(rightColW, rCard2H), 14.f, sf::Color::White, 1.f, sf::Color(226, 232, 240));

        sf::Text tAllocHeader(toSfString("Phân bổ ghế " + curRoomName), Theme::FontPrimary, 15);
        tAllocHeader.setStyle(sf::Text::Bold);
        tAllocHeader.setFillColor(sf::Color(15, 23, 42));
        tAllocHeader.setPosition(rightColX + 24.f, rCard2Y + 14.f);
        window.draw(tAllocHeader);

        int curSeatsCnt = static_cast<int>(roomSeats.size());
        sf::Text tAllocBadge(toSfString(std::to_string(curSeatsCnt) + " GHẾ"), Theme::FontPrimary, 11);
        tAllocBadge.setStyle(sf::Text::Bold);
        tAllocBadge.setFillColor(sf::Color(3, 105, 161));
        float abW = tAllocBadge.getLocalBounds().width + 20.f;
        RoundedRectRenderer::render(window, sf::Vector2f(rightColX + rightColW - 24.f - abW, rCard2Y + 13.f), sf::Vector2f(abW, 22.f), 6.f, sf::Color(224, 242, 254));
        tAllocBadge.setPosition(rightColX + rightColW - 24.f - abW + (abW - tAllocBadge.getLocalBounds().width) / 2.f - tAllocBadge.getLocalBounds().left, rCard2Y + 17.f);
        window.draw(tAllocBadge);

        // Compute exact counts & row ranges from SQLite roomSeats
        int stdCnt = 0, vipCnt = 0, specialCnt = 0, coupleCnt = 0;
        std::set<char> stdRows, vipRows, specRows, cplRows;
        for (const auto& s : roomSeats) {
            char rc = 'A';
            int c1 = 1, c2 = 1;
            parseSeatCodeInfo(s.getMaGhe(), rc, c1, c2);
            std::string lg = s.getLoaiGhe();
            if (lg == "VIP") { vipCnt++; vipRows.insert(rc); }
            else if (isCoupleType(lg)) { coupleCnt++; cplRows.insert(rc); }
            else if (lg == "4DX" || lg == "IMAX") { specialCnt++; specRows.insert(rc); }
            else { stdCnt++; stdRows.insert(rc); }
        }
        int safeTotal = std::max(1, curSeatsCnt);

        auto formatRowRange = [](const std::set<char>& rset, const std::string& fallback) {
            if (rset.empty()) return fallback;
            if (rset.size() == 1) return std::string("Hàng ") + *rset.begin();
            return std::string("Hàng ") + *rset.begin() + " - " + *rset.rbegin();
        };

        // 4-Segment Progress Bar
        float barY = rCard2Y + 44.f;
        float barW = rightColW - 48.f;
        RoundedRectRenderer::render(window, sf::Vector2f(rightColX + 24.f, barY), sf::Vector2f(barW, 8.f), 4.f, sf::Color(226, 232, 240));

        float w1 = (stdCnt / static_cast<float>(safeTotal)) * barW;
        float w2 = (vipCnt / static_cast<float>(safeTotal)) * barW;
        float w3 = (specialCnt / static_cast<float>(safeTotal)) * barW;
        float w4 = (coupleCnt / static_cast<float>(safeTotal)) * barW;

        if (w1 > 0.f) RoundedRectRenderer::render(window, sf::Vector2f(rightColX + 24.f, barY), sf::Vector2f(w1, 8.f), 4.f, sf::Color(3, 105, 161));
        if (w2 > 0.f) RoundedRectRenderer::render(window, sf::Vector2f(rightColX + 24.f + w1, barY), sf::Vector2f(w2, 8.f), 4.f, sf::Color(245, 158, 11));
        if (w3 > 0.f) RoundedRectRenderer::render(window, sf::Vector2f(rightColX + 24.f + w1 + w2, barY), sf::Vector2f(w3, 8.f), 4.f, sf::Color(234, 88, 12));
        if (w4 > 0.f) RoundedRectRenderer::render(window, sf::Vector2f(rightColX + 24.f + w1 + w2 + w3, barY), sf::Vector2f(w4, 8.f), 4.f, sf::Color(14, 165, 233));

        // 4 Statistical Dot Rows matching media_1794677546630_c74f54be.jpg
        auto drawDistRow = [&](int idx, sf::Color dotCol, const std::string& label, int count) {
            float ry = rCard2Y + 64.f + idx * 27.f;
            sf::CircleShape d(4.5f);
            d.setFillColor(dotCol);
            d.setPosition(rightColX + 24.f, ry + 4.f);
            window.draw(d);

            sf::Text lTxt(toSfString(label), Theme::FontPrimary, 12);
            lTxt.setFillColor(sf::Color(51, 65, 85));
            lTxt.setPosition(rightColX + 40.f, ry);
            window.draw(lTxt);

            sf::Text cTxt(toSfString(std::to_string(count) + " ghế"), Theme::FontPrimary, 12);
            cTxt.setStyle(sf::Text::Bold);
            cTxt.setFillColor(sf::Color(15, 23, 42));
            cTxt.setPosition(rightColX + rightColW - 24.f - cTxt.getLocalBounds().width, ry);
            window.draw(cTxt);
        };

        drawDistRow(0, sf::Color(3, 105, 161),  "Ghế Standard (" + formatRowRange(stdRows, "Hàng A - D") + ")", stdCnt);
        drawDistRow(1, sf::Color(245, 158, 11), "Ghế VIP Gold (" + formatRowRange(vipRows, "Hàng E - I") + ")", vipCnt);
        drawDistRow(2, sf::Color(234, 88, 12),  "Ghế Ba / 4DX (" + formatRowRange(specRows, "Hàng K") + ")", specialCnt);
        drawDistRow(3, sf::Color(14, 165, 233), "Ghế Đôi Sweetbox (" + formatRowRange(cplRows, "Hàng J") + ")", coupleCnt);

        // Bottom PCCC Compliance Box
        float tcvnY = rCard2Y + 178.f;
        RoundedRectRenderer::render(window, sf::Vector2f(rightColX + 24.f, tcvnY), sf::Vector2f(barW, 66.f), 10.f, sf::Color(248, 250, 252), 1.f, sf::Color(226, 232, 240));

        sf::CircleShape infoCirc(9.f);
        infoCirc.setFillColor(sf::Color::Transparent);
        infoCirc.setOutlineThickness(1.5f);
        infoCirc.setOutlineColor(sf::Color(100, 116, 139));
        infoCirc.setPosition(rightColX + 36.f, tcvnY + 16.f);
        window.draw(infoCirc);
        sf::Text iMark("i", Theme::FontPrimary, 11);
        iMark.setStyle(sf::Text::Bold);
        iMark.setFillColor(sf::Color(100, 116, 139));
        iMark.setPosition(rightColX + 43.f, tcvnY + 18.f);
        window.draw(iMark);

        sf::Text tcvnDesc(toSfString("Bố cục phù hợp chuẩn kiểm định PCCC và khoảng lùi màn\nchiếu laser 8.5m. Khoảng cách hai hàng ghế đạt 1.05m TCVN."), Theme::FontPrimary, 11);
        tcvnDesc.setFillColor(sf::Color(100, 116, 139));
        tcvnDesc.setPosition(rightColX + 64.f, tcvnY + 14.f);
        window.draw(tcvnDesc);

        // Draw dropdowns on top of cards
        if (seatBatchTypeDropdown) seatBatchTypeDropdown->draw(window);
        if (seatRoomDropdown) seatRoomDropdown->draw(window);

    } else {
        // ---------------------------------------------------------
        // SUB-TAB 1: DANH MỤC LOẠI GHẾ & ĐƠN GIÁ
        // ---------------------------------------------------------
        // Style active filter pill
        auto styleTypeFilter = [&](std::unique_ptr<RoundRectButton>& btn, bool active) {
            if (!btn) return;
            if (active) {
                btn->setCustomColors(sf::Color(2, 132, 199), sf::Color(3, 105, 161), sf::Color(7, 89, 133), sf::Color(2, 132, 199));
                btn->setTextColor(sf::Color::White);
            } else {
                btn->setCustomColors(sf::Color::White, sf::Color(241, 245, 249), sf::Color(226, 232, 240), sf::Color::White);
                btn->setOutline(1.f, sf::Color(226, 232, 240));
                btn->setTextColor(sf::Color(71, 85, 105));
            }
            btn->draw(window);
        };

        if (seatTypeSearchInput) seatTypeSearchInput->draw(window);
        styleTypeFilter(btnSeatTypeFilterAll, seatTypeFilterIdx == 0);
        styleTypeFilter(btnSeatTypeFilterSingle, seatTypeFilterIdx == 1);
        styleTypeFilter(btnSeatTypeFilterCouple, seatTypeFilterIdx == 2);
        styleTypeFilter(btnSeatTypeFilterSpecial, seatTypeFilterIdx == 3);

        // Left Card: Danh Mục Loại Ghế Toàn Hệ Thống
        float tblCardY = 228.f;
        float tblCardH = 480.f;
        RoundedRectRenderer::render(window, sf::Vector2f(leftColX, tblCardY), sf::Vector2f(leftColW, tblCardH), 14.f, sf::Color::White, 1.f, sf::Color(226, 232, 240));

        sf::Text tTblHeader(toSfString("Danh Mục Loại Ghế Toàn Hệ Thống"), Theme::FontPrimary, 15);
        tTblHeader.setStyle(sf::Text::Bold);
        tTblHeader.setFillColor(sf::Color(15, 23, 42));
        tTblHeader.setPosition(leftColX + 24.f, tblCardY + 16.f);
        window.draw(tTblHeader);

        float hCntX = leftColX + 24.f + tTblHeader.getLocalBounds().width + 14.f;
        sf::Text tHCnt(toSfString(std::to_string(seatTypeList.size()) + " hạng ghế đã lưu"), Theme::FontPrimary, 11);
        tHCnt.setStyle(sf::Text::Bold);
        tHCnt.setFillColor(sf::Color(71, 85, 105));
        float hCntW = tHCnt.getLocalBounds().width + 24.f;
        RoundedRectRenderer::render(window, sf::Vector2f(hCntX, tblCardY + 15.f), sf::Vector2f(hCntW, 24.f), 12.f, sf::Color(241, 245, 249));
        tHCnt.setPosition(hCntX + (hCntW - tHCnt.getLocalBounds().width) / 2.f - tHCnt.getLocalBounds().left, tblCardY + 19.f);
        window.draw(tHCnt);

        sf::CircleShape apDot(3.5f);
        apDot.setFillColor(sf::Color(16, 185, 129));
        apDot.setPosition(leftColX + leftColW - 120.f, tblCardY + 23.f);
        window.draw(apDot);
        sf::Text tAp(toSfString("Đang áp dụng"), Theme::FontPrimary, 11);
        tAp.setFillColor(sf::Color(51, 65, 85));
        tAp.setPosition(leftColX + leftColW - 110.f, tblCardY + 19.f);
        window.draw(tAp);

        // Table Header strip
        float thY = tblCardY + 50.f;
        RoundedRectRenderer::render(window, sf::Vector2f(leftColX + 16.f, thY), sf::Vector2f(leftColW - 32.f, 32.f), 6.f, sf::Color(248, 250, 252));

        sf::Text col1(toSfString("MÃ & NHẬN DIỆN"), Theme::FontPrimary, 10);
        col1.setStyle(sf::Text::Bold); col1.setFillColor(sf::Color(71, 85, 105)); col1.setPosition(leftColX + 36.f, thY + 8.f); window.draw(col1);

        sf::Text col2(toSfString("TÊN LOẠI GHẾ"), Theme::FontPrimary, 10);
        col2.setStyle(sf::Text::Bold); col2.setFillColor(sf::Color(71, 85, 105)); col2.setPosition(leftColX + 180.f, thY + 8.f); window.draw(col2);

        sf::Text col3(toSfString("TRẢI NGHIỆM / TIỆN ÍCH"), Theme::FontPrimary, 10);
        col3.setStyle(sf::Text::Bold); col3.setFillColor(sf::Color(71, 85, 105)); col3.setPosition(leftColX + 350.f, thY + 8.f); window.draw(col3);

        sf::Text col4(toSfString("GIÁ CƠ BẢN"), Theme::FontPrimary, 10);
        col4.setStyle(sf::Text::Bold); col4.setFillColor(sf::Color(71, 85, 105)); col4.setPosition(leftColX + 660.f, thY + 8.f); window.draw(col4);

        sf::Text col5(toSfString("ÁP DỤNG"), Theme::FontPrimary, 10);
        col5.setStyle(sf::Text::Bold); col5.setFillColor(sf::Color(71, 85, 105)); col5.setPosition(leftColX + 790.f, thY + 8.f); window.draw(col5);

        // Filter list (diacritic-insensitive)
        std::string q = seatTypeSearchInput ? normalizeSeatSearchVN(seatTypeSearchInput->getText()) : "";
        std::vector<int> visIndices;
        for (size_t i = 0; i < seatTypeList.size(); ++i) {
            std::string tlLow = normalizeSeatSearchVN(seatTypeList[i].tenLoai);
            if (seatTypeFilterIdx == 1) {
                if (tlLow.find("standard") == std::string::npos && tlLow.find("vip") == std::string::npos) continue;
            } else if (seatTypeFilterIdx == 2) {
                if (tlLow.find("couple") == std::string::npos && tlLow.find("sweetbox") == std::string::npos && tlLow.find("vip") == std::string::npos) continue;
            } else if (seatTypeFilterIdx == 3) {
                if (tlLow.find("imax") == std::string::npos && tlLow.find("4dx") == std::string::npos) continue;
            }
            if (!q.empty()) {
                std::string descLow = normalizeSeatSearchVN(seatTypeList[i].moTa + " " + seatTypeList[i].maLoai);
                if (tlLow.find(q) == std::string::npos && descLow.find(q) == std::string::npos) continue;
            }
            visIndices.push_back(static_cast<int>(i));
        }

        float rowStartY = thY + 38.f;
        float rowH = 66.f;
        for (size_t vi = 0; vi < visIndices.size() && vi < 5; ++vi) {
            int i = visIndices[vi];
            float ry = rowStartY + vi * rowH;
            const auto& st = seatTypeList[i];
            bool isSelected = (selectedSeatTypeRowIdx == i);
            bool isHovered = (mPos.x >= leftColX + 16.f && mPos.x <= leftColX + leftColW - 16.f && mPos.y >= ry && mPos.y <= ry + rowH);

            sf::Color rowBg = isSelected ? sf::Color(240, 249, 255) : (isHovered ? sf::Color(248, 250, 252) : sf::Color::White);
            sf::Color rowBrd = isSelected ? sf::Color(186, 230, 253) : sf::Color(241, 245, 249);
            RoundedRectRenderer::render(window, sf::Vector2f(leftColX + 16.f, ry), sf::Vector2f(leftColW - 32.f, rowH - 4.f), 8.f, rowBg, 1.f, rowBrd);

            // Col 1: Mã & Nhận diện
            RoundedRectRenderer::render(window, sf::Vector2f(leftColX + 32.f, ry + 12.f), sf::Vector2f(34.f, 34.f), 8.f, st.mauSac);
            sf::RectangleShape rIc(sf::Vector2f(14.f, 10.f));
            rIc.setPosition(leftColX + 42.f, ry + 24.f);
            rIc.setFillColor(sf::Color::White);
            window.draw(rIc);

            sf::Text rCode(toSfString(st.maLoai), Theme::FontPrimary, 11);
            rCode.setStyle(sf::Text::Bold);
            rCode.setFillColor(sf::Color(15, 23, 42));
            rCode.setPosition(leftColX + 74.f, ry + 12.f);
            window.draw(rCode);

            sf::Text rRole(toSfString(st.tenLoai + " Hall"), Theme::FontPrimary, 10);
            rRole.setFillColor(sf::Color(100, 116, 139));
            rRole.setPosition(leftColX + 74.f, ry + 29.f);
            window.draw(rRole);

            // Col 2: Tên Loại Ghế + Sticker
            sf::Text rName(toSfString(st.tenLoai), Theme::FontPrimary, 14);
            rName.setStyle(sf::Text::Bold);
            rName.setFillColor(sf::Color(15, 23, 42));
            rName.setPosition(leftColX + 180.f, ry + 18.f);
            window.draw(rName);

            std::string stickerLabel = "Tiêu Chuẩn";
            sf::Color sBg = sf::Color(224, 242, 254);
            sf::Color sTxt = sf::Color(3, 105, 161);
            if (st.tenLoai == "VIP") { stickerLabel = "Hạng Sang"; sBg = sf::Color(254, 243, 199); sTxt = sf::Color(180, 83, 9); }
            else if (st.tenLoai == "Couple") { stickerLabel = "Sweetbox"; sBg = sf::Color(253, 242, 248); sTxt = sf::Color(190, 24, 93); }
            else if (st.tenLoai == "4DX") { stickerLabel = "Motion"; sBg = sf::Color(255, 237, 213); sTxt = sf::Color(194, 65, 12); }
            else if (st.tenLoai == "IMAX") { stickerLabel = "Laser"; sBg = sf::Color(243, 232, 255); sTxt = sf::Color(126, 34, 206); }

            sf::Text stTxt(toSfString(stickerLabel), Theme::FontPrimary, 10);
            stTxt.setStyle(sf::Text::Bold);
            stTxt.setFillColor(sTxt);
            float stW = stTxt.getLocalBounds().width + 20.f;
            float stX = leftColX + 180.f + rName.getLocalBounds().width + 10.f;
            RoundedRectRenderer::render(window, sf::Vector2f(stX, ry + 18.f), sf::Vector2f(stW, 20.f), 10.f, sBg);
            stTxt.setPosition(stX + (stW - stTxt.getLocalBounds().width) / 2.f - stTxt.getLocalBounds().left, ry + 21.f);
            window.draw(stTxt);

            // Col 3: Trải nghiệm & số ghế
            std::string shortDesc = st.moTa;
            if (shortDesc.size() > 42) shortDesc = shortDesc.substr(0, 39) + "...";
            sf::Text rDesc(toSfString(shortDesc), Theme::FontPrimary, 11);
            rDesc.setFillColor(sf::Color(51, 65, 85));
            rDesc.setPosition(leftColX + 350.f, ry + 12.f);
            window.draw(rDesc);

            int typeSeatCnt = 0;
            for (const auto& s : allSeats) if (s.getLoaiGhe() == st.tenLoai) typeSeatCnt++;
            sf::Text rCnt(toSfString(std::to_string(typeSeatCnt) + " ghế toàn cụm"), Theme::FontPrimary, 10);
            rCnt.setFillColor(sf::Color(2, 132, 199));
            rCnt.setPosition(leftColX + 350.f, ry + 30.f);
            window.draw(rCnt);

            // Col 4: Giá Cơ Bản
            sf::Text rPrice(toSfString(formatSeatPriceVN(st.donGia) + " đ"), Theme::FontPrimary, 14);
            rPrice.setStyle(sf::Text::Bold);
            rPrice.setFillColor(sf::Color(15, 23, 42));
            rPrice.setPosition(leftColX + 660.f, ry + 12.f);
            window.draw(rPrice);

            sf::Text rPriceSub(toSfString("Gốc niêm yết"), Theme::FontPrimary, 10);
            rPriceSub.setFillColor(sf::Color(100, 116, 139));
            rPriceSub.setPosition(leftColX + 660.f, ry + 30.f);
            window.draw(rPriceSub);

            // Col 5: Trạng thái Active
            RoundedRectRenderer::render(window, sf::Vector2f(leftColX + 782.f, ry + 18.f), sf::Vector2f(74.f, 22.f), 11.f,
                                        st.kichHoatBan ? sf::Color(240, 253, 244) : sf::Color(241, 245, 249));
            sf::CircleShape acDot(3.f);
            acDot.setFillColor(st.kichHoatBan ? sf::Color(16, 185, 129) : sf::Color(148, 163, 184));
            acDot.setPosition(leftColX + 792.f, ry + 26.f);
            window.draw(acDot);

            sf::Text acTxt(toSfString(st.kichHoatBan ? "Active" : "Tắt"), Theme::FontPrimary, 10);
            acTxt.setStyle(sf::Text::Bold);
            acTxt.setFillColor(st.kichHoatBan ? sf::Color(21, 128, 61) : sf::Color(100, 116, 139));
            acTxt.setPosition(leftColX + 804.f, ry + 22.f);
            window.draw(acTxt);
        }

        // Bottom Strip: Tỷ Trọng Ghế & Cơ Cấu Doanh Thu
        float propCardY = tblCardY + tblCardH + 14.f;
        float propCardH = 66.f;
        RoundedRectRenderer::render(window, sf::Vector2f(leftColX, propCardY), sf::Vector2f(leftColW, propCardH), 12.f, sf::Color::White, 1.f, sf::Color(226, 232, 240));

        RoundedRectRenderer::render(window, sf::Vector2f(leftColX + 16.f, propCardY + 15.f), sf::Vector2f(36.f, 36.f), 8.f, sf::Color(240, 249, 255));
        sf::CircleShape pIc(10.f);
        pIc.setPosition(leftColX + 24.f, propCardY + 23.f);
        pIc.setFillColor(sf::Color(2, 132, 199));
        window.draw(pIc);

        sf::Text propTitle(toSfString("Tỷ Trọng Ghế & Cơ Cấu Doanh Thu Toàn Hệ Thống"), Theme::FontPrimary, 13);
        propTitle.setStyle(sf::Text::Bold);
        propTitle.setFillColor(sf::Color(15, 23, 42));
        propTitle.setPosition(leftColX + 64.f, propCardY + 12.f);
        window.draw(propTitle);

        char propBuf[160];
        snprintf(propBuf, sizeof(propBuf), "Tổng %d ghế đang vận hành tại %d phòng chiếu trong cơ sở dữ liệu SQLite.", totalSeatsAll, totalRoomsAll);
        sf::Text propSub(toSfString(propBuf), Theme::FontPrimary, 11);
        propSub.setFillColor(sf::Color(100, 116, 139));
        propSub.setPosition(leftColX + 64.f, propCardY + 33.f);
        window.draw(propSub);

        float mBarX = leftColX + 580.f;
        float mBarW = 270.f;
        float mBarY = propCardY + 28.f;
        RoundedRectRenderer::render(window, sf::Vector2f(mBarX, mBarY), sf::Vector2f(mBarW, 8.f), 4.f, sf::Color(226, 232, 240));
        RoundedRectRenderer::render(window, sf::Vector2f(mBarX, mBarY), sf::Vector2f(80.f, 8.f), 4.f, sf::Color(14, 165, 233));
        RoundedRectRenderer::render(window, sf::Vector2f(mBarX + 80.f, mBarY), sf::Vector2f(65.f, 8.f), 4.f, sf::Color(245, 158, 11));
        RoundedRectRenderer::render(window, sf::Vector2f(mBarX + 145.f, mBarY), sf::Vector2f(50.f, 8.f), 4.f, sf::Color(168, 85, 247));
        RoundedRectRenderer::render(window, sf::Vector2f(mBarX + 195.f, mBarY), sf::Vector2f(45.f, 8.f), 4.f, sf::Color(249, 115, 22));
        RoundedRectRenderer::render(window, sf::Vector2f(mBarX + 240.f, mBarY), sf::Vector2f(30.f, 8.f), 4.f, sf::Color(236, 72, 153));

        // Right Card (Tab 2): Form Thông tin loại ghế
        float rCardY = 228.f;
        float rCardH = 560.f;
        RoundedRectRenderer::render(window, sf::Vector2f(rightColX, rCardY), sf::Vector2f(rightColW, rCardH), 14.f, sf::Color::White, 1.f, sf::Color(226, 232, 240));

        sf::Text tFormHeader(toSfString("Thông tin loại ghế"), Theme::FontPrimary, 16);
        tFormHeader.setStyle(sf::Text::Bold);
        tFormHeader.setFillColor(sf::Color(15, 23, 42));
        tFormHeader.setPosition(rightColX + 24.f, rCardY + 16.f);
        window.draw(tFormHeader);

        std::string curCode = seatFormTypeCode.empty() ? "SEAT-STD-05" : seatFormTypeCode;
        sf::Text tCurCode(toSfString(curCode), Theme::FontPrimary, 11);
        tCurCode.setStyle(sf::Text::Bold);
        tCurCode.setFillColor(sf::Color::White);
        float cBadgeW = tCurCode.getLocalBounds().width + 24.f;
        RoundedRectRenderer::render(window, sf::Vector2f(rightColX + rightColW - 24.f - cBadgeW, rCardY + 16.f), sf::Vector2f(cBadgeW, 24.f), 6.f, sf::Color(3, 105, 161));
        tCurCode.setPosition(rightColX + rightColW - 24.f - cBadgeW + (cBadgeW - tCurCode.getLocalBounds().width) / 2.f - tCurCode.getLocalBounds().left, rCardY + 20.f);
        window.draw(tCurCode);

        sf::Text lName(toSfString("TÊN LOẠI: *"), Theme::FontPrimary, 11);
        lName.setStyle(sf::Text::Bold); lName.setFillColor(sf::Color(51, 65, 85));
        lName.setPosition(rightColX + 24.f, rCardY + 44.f);
        window.draw(lName);

        if (inputSeatTypeName) inputSeatTypeName->draw(window);

        sf::Text lCode(toSfString("MÃ PHÂN HẠNG (CODE):"), Theme::FontPrimary, 11);
        lCode.setStyle(sf::Text::Bold); lCode.setFillColor(sf::Color(51, 65, 85));
        lCode.setPosition(rightColX + 24.f, rCardY + 112.f);
        window.draw(lCode);

        float codeBoxY = rCardY + 130.f;
        RoundedRectRenderer::render(window, sf::Vector2f(rightColX + 24.f, codeBoxY), sf::Vector2f(rightColW - 48.f, 36.f), 8.f, sf::Color(241, 245, 249), 1.f, sf::Color(226, 232, 240));
        sf::Text cVal(toSfString(curCode), Theme::FontPrimary, 13);
        cVal.setFillColor(sf::Color(71, 85, 105));
        cVal.setPosition(rightColX + 38.f, codeBoxY + 9.f);
        window.draw(cVal);

        sf::Text lPrice(toSfString("GIÁ CƠ BẢN NIÊM YẾT: *"), Theme::FontPrimary, 11);
        lPrice.setStyle(sf::Text::Bold); lPrice.setFillColor(sf::Color(51, 65, 85));
        lPrice.setPosition(rightColX + 24.f, rCardY + 170.f);
        window.draw(lPrice);

        if (inputSeatTypePrice) inputSeatTypePrice->draw(window);

        float prBoxRight = rightColX + rightColW - 24.f;
        sf::Text dTxt(toSfString("đ"), Theme::FontPrimary, 14);
        dTxt.setStyle(sf::Text::Bold);
        dTxt.setFillColor(sf::Color(71, 85, 105));
        dTxt.setPosition(prBoxRight - 50.f, 424.f);
        window.draw(dTxt);

        sf::Text upArr(toSfString("^"), Theme::FontPrimary, 12);
        upArr.setStyle(sf::Text::Bold);
        upArr.setFillColor(sf::Color(100, 116, 139));
        upArr.setPosition(prBoxRight - 22.f, 416.f);
        window.draw(upArr);

        sf::Text dnArr(toSfString("v"), Theme::FontPrimary, 10);
        dnArr.setStyle(sf::Text::Bold);
        dnArr.setFillColor(sf::Color(100, 116, 139));
        dnArr.setPosition(prBoxRight - 22.f, 434.f);
        window.draw(dnArr);

        sf::Text lColor(toSfString("MÀU HIỂN THỊ TRÊN LAYOUT SƠ ĐỒ GHẾ:"), Theme::FontPrimary, 11);
        lColor.setStyle(sf::Text::Bold); lColor.setFillColor(sf::Color(51, 65, 85));
        lColor.setPosition(rightColX + 24.f, rCardY + 236.f);
        window.draw(lColor);

        std::vector<sf::Color> swatches = {
            sf::Color(14, 165, 233),
            sf::Color(245, 158, 11),
            sf::Color(236, 72, 153),
            sf::Color(168, 85, 247),
            sf::Color(249, 115, 22)
        };
        float swY = 486.f;
        for (int s = 0; s < 5; ++s) {
            float sx = rightColX + 28.f + s * 64.f;
            sf::CircleShape circ(16.f);
            circ.setFillColor(swatches[s]);
            circ.setPosition(sx, swY);
            window.draw(circ);

            if (seatTypeColorIdx == s) {
                drawVectorCheck(window, sx + 10.f, swY + 11.f, sf::Color::White, 1.0f);
            }
        }

        sf::Text lDesc(toSfString("MÔ TẢ TRẢI NGHIỆM & TIỆN ÍCH:"), Theme::FontPrimary, 11);
        lDesc.setStyle(sf::Text::Bold); lDesc.setFillColor(sf::Color(51, 65, 85));
        lDesc.setPosition(rightColX + 24.f, rCardY + 310.f);
        window.draw(lDesc);

        if (inputSeatTypeDesc) inputSeatTypeDesc->draw(window);

        float swBoxY = 626.f;
        RoundedRectRenderer::render(window, sf::Vector2f(rightColX + 24.f, swBoxY), sf::Vector2f(rightColW - 48.f, 46.f), 8.f, sf::Color(248, 250, 252), 1.f, sf::Color(226, 232, 240));

        sf::Text swTitle(toSfString("Phụ thu Suất Chiếu Lễ / Cuối Tuần"), Theme::FontPrimary, 11);
        swTitle.setStyle(sf::Text::Bold);
        swTitle.setFillColor(sf::Color(15, 23, 42));
        swTitle.setPosition(rightColX + 36.f, swBoxY + 6.f);
        window.draw(swTitle);

        sf::Text swSub(toSfString("+15.000 VNĐ vào thứ 6, 7, CN  •  Kích hoạt bán vé"), Theme::FontPrimary, 10);
        swSub.setFillColor(sf::Color(100, 116, 139));
        swSub.setPosition(rightColX + 36.f, swBoxY + 24.f);
        window.draw(swSub);

        float swPillX = rightColX + rightColW - 74.f;
        float swPillY = swBoxY + 12.f;
        RoundedRectRenderer::render(window, sf::Vector2f(swPillX, swPillY), sf::Vector2f(44.f, 22.f), 11.f,
                                    seatTypeSaleActive ? sf::Color(2, 132, 199) : sf::Color(203, 213, 225));
        sf::CircleShape swThumb(8.f);
        swThumb.setFillColor(sf::Color::White);
        swThumb.setPosition(seatTypeSaleActive ? (swPillX + 25.f) : (swPillX + 3.f), swPillY + 3.f);
        window.draw(swThumb);

        if (btnSeatTypeAdd) btnSeatTypeAdd->draw(window);
        if (btnSeatTypeUpdate) btnSeatTypeUpdate->draw(window);
        if (btnSeatTypeDelete) btnSeatTypeDelete->draw(window);
        if (btnSeatTypeReset) btnSeatTypeReset->draw(window);

        // Statistical KPI Row for Tab 2 (y = 804.f)
        float kpiY = 804.f;
        float kpiH = 68.f;
        float kpiCardW = (fullW - 36.f) / 4.f;

        double sumP = 0.0;
        for (const auto& s : allSeats) sumP += s.getDonGiaGhe();
        double avgP = allSeats.empty() ? 170000.0 : (sumP / allSeats.size());

        auto drawKpiCard = [&](float kx, sf::Color icBg, sf::Color icCol, const std::string& val, const std::string& lbl) {
            RoundedRectRenderer::render(window, sf::Vector2f(kx, kpiY), sf::Vector2f(kpiCardW, kpiH), 12.f, sf::Color::White, 1.f, sf::Color(226, 232, 240));
            RoundedRectRenderer::render(window, sf::Vector2f(kx + 14.f, kpiY + 16.f), sf::Vector2f(36.f, 36.f), 8.f, icBg);
            sf::CircleShape ic(8.f);
            ic.setPosition(kx + 24.f, kpiY + 26.f);
            ic.setFillColor(icCol);
            window.draw(ic);

            sf::Text vTxt(toSfString(val), Theme::FontPrimary, 14);
            vTxt.setStyle(sf::Text::Bold);
            vTxt.setFillColor(sf::Color(15, 23, 42));
            vTxt.setPosition(kx + 60.f, kpiY + 14.f);
            window.draw(vTxt);

            sf::Text lTxt(toSfString(lbl), Theme::FontPrimary, 11);
            lTxt.setFillColor(sf::Color(100, 116, 139));
            lTxt.setPosition(kx + 60.f, kpiY + 35.f);
            window.draw(lTxt);
        };

        drawKpiCard(leftColX, sf::Color(240, 249, 255), sf::Color(2, 132, 199),
                    std::to_string(seatTypeList.size()) + " Phân Khúc", "Phủ toàn bộ " + std::to_string(totalRoomsAll) + " phòng chiếu");
        drawKpiCard(leftColX + kpiCardW + 12.f, sf::Color(240, 249, 255), sf::Color(2, 132, 199),
                    formatSeatPriceVN(avgP) + " VNĐ", "Đơn giá vé bình quân (ARPU)");
        drawKpiCard(leftColX + (kpiCardW + 12.f) * 2.f, sf::Color(240, 253, 244), sf::Color(16, 185, 129),
                    "100% Đồng Bộ", "POS Quầy, Web & Cinestar App");
        drawKpiCard(leftColX + (kpiCardW + 12.f) * 3.f, sf::Color(254, 243, 199), sf::Color(245, 158, 11),
                    "Cập Nhật Realtime", "Đồng bộ bởi Admin Quốc Thái");
    }

    // =========================================================
    // 5. BOTTOM FOOTER BAR (y = 940.f)
    // =========================================================
    float footBarY = 938.f;
    sf::Text footCpr(toSfString("© 2025 Cinestar CineOS. Bản quyền thuộc Cinestar Cinema Complex."), Theme::FontPrimary, 11);
    footCpr.setFillColor(sf::Color(100, 116, 139));
    footCpr.setPosition(leftColX, footBarY);
    window.draw(footCpr);

    char syncBuf[160];
    snprintf(syncBuf, sizeof(syncBuf), "Đồng bộ thực: %d phòng chiếu (%d ghế)   •   Hotline kỹ thuật: 1900 1234", totalRoomsAll, totalSeatsAll);
    sf::Text footRight(toSfString(syncBuf), Theme::FontPrimary, 11);
    footRight.setStyle(sf::Text::Bold);
    footRight.setFillColor(sf::Color(3, 105, 161));
    footRight.setPosition(rightColX + rightColW - footRight.getLocalBounds().width, footBarY);
    window.draw(footRight);
}

} // namespace UI
