#ifndef CINEMA_UI_APP_SHOWTIME_HELPERS_H
#define CINEMA_UI_APP_SHOWTIME_HELPERS_H

#include "UITheme.h"
#include "../../include/DateFormat.h"
#include <string>
#include <ctime>
#include <cmath>
#include <cctype>

namespace UI {

inline const float ST_HEADER_Y = 38.f;   // banner header card
inline const float ST_TAB_Y    = 118.f;  // sub-tab row (below page heading)
inline const float ST_FILTER_Y = 166.f;  // search / filter row
inline const float ST_CARD_Y   = 214.f;  // top of the two main cards
inline const float ST_CARD_H   = 616.f;  // height of the two main cards
inline const float ST_ROW_H    = 76.f;   // table row height (fits 6 rows neatly)
inline const float ST_KPI_Y    = 842.f;  // footer KPI cards row
inline const float ST_FOOTER_Y = 924.f;  // bottom copyright & hotline bar

// Layout coordinates for right form card (consistent across setup & render)
inline const float ST_FORM_MOVIE_LBL_Y  = 276.f;
inline const float ST_FORM_MOVIE_Y      = 298.f;
inline const float ST_FORM_MOVIE_INFO_Y = 342.f;
inline const float ST_FORM_ROOM_LBL_Y   = 374.f;
inline const float ST_FORM_ROOM_Y       = 396.f;
inline const float ST_FORM_ROOM_INFO_Y  = 440.f;
inline const float ST_FORM_DATE_LBL_Y   = 470.f;
inline const float ST_FORM_DATE_Y       = 492.f;
inline const float ST_FORM_EST_Y        = 546.f;
inline const float ST_FORM_EST_H        = 92.f;
inline const float ST_FORM_BTN_ROW_Y    = 778.f;

inline bool stParseTimeToMinutes(const std::string& raw, int& outMinutes) {
    if (raw.empty()) return false;
    std::string s;
    for (char c : raw) {
        if (!::isspace(static_cast<unsigned char>(c))) {
            s += static_cast<char>(::toupper(static_cast<unsigned char>(c)));
        }
    }
    if (s.empty()) return false;

    bool isPM = (s.find("PM") != std::string::npos || s.find("CH") != std::string::npos || s.find("TOI") != std::string::npos);
    bool isAM = (s.find("AM") != std::string::npos || s.find("SA") != std::string::npos);

    std::string numPart;
    for (char c : s) {
        if (c >= '0' && c <= '9') {
            numPart += c;
        } else if (c == ':' || c == 'H' || c == '.') {
            if (numPart.empty() || numPart.back() != ':') {
                numPart += ':';
            }
        }
    }

    if (numPart.empty()) return false;

    int h = -1, m = 0;
    size_t colon = numPart.find(':');
    if (colon != std::string::npos) {
        try {
            std::string hStr = numPart.substr(0, colon);
            if (!hStr.empty()) h = std::stoi(hStr);
            else return false;

            std::string rem = numPart.substr(colon + 1);
            if (!rem.empty()) {
                size_t colon2 = rem.find(':');
                if (colon2 != std::string::npos) rem = rem.substr(0, colon2);
                if (!rem.empty()) m = std::stoi(rem);
            }
        } catch (...) {
            return false;
        }
    } else {
        try {
            int val = std::stoi(numPart);
            if (val >= 100) {
                h = val / 100;
                m = val % 100;
            } else {
                h = val;
                m = 0;
            }
        } catch (...) {
            return false;
        }
    }

    if (h < 0 || h > 23 || m < 0 || m > 59) return false;
    if (isPM && h < 12) h += 12;
    if (isAM && h == 12) h = 0;

    outMinutes = h * 60 + m;
    return true;
}

inline std::string stFormatHHMM(int minutes) {
    minutes = ((minutes % 1440) + 1440) % 1440;
    char buf[8];
    snprintf(buf, sizeof(buf), "%02d:%02d", minutes / 60, minutes % 60);
    return buf;
}

inline bool stDateToTm(const std::string& date, std::tm& out) {
    int d = 0, mo = 0, y = 0;
    if (!DateFormat::parseDate(date, d, mo, y)) return false;
    out = std::tm();
    out.tm_mday = d;
    out.tm_mon = mo - 1;
    out.tm_year = y - 1900;
    out.tm_hour = 12;
    std::mktime(&out);
    return true;
}

inline std::string stAddDays(const std::string& date, int days) {
    std::tm t;
    if (!stDateToTm(date, t)) return date;
    t.tm_mday += days;
    std::mktime(&t);
    char buf[16];
    snprintf(buf, sizeof(buf), "%02d/%02d/%04d", t.tm_mday, t.tm_mon + 1, t.tm_year + 1900);
    return buf;
}

inline std::string stWeekdayVi(const std::string& date) {
    std::tm t;
    if (!stDateToTm(date, t)) return "";
    static const char* names[] = {"Chủ Nhật", "Thứ Hai", "Thứ Ba", "Thứ Tư", "Thứ Năm", "Thứ Sáu", "Thứ Bảy"};
    return names[t.tm_wday % 7];
}

inline std::string stTodayStr() {
    std::time_t now = std::time(nullptr);
    std::tm* lt = std::localtime(&now);
    char buf[16];
    snprintf(buf, sizeof(buf), "%02d/%02d/%04d", lt->tm_mday, lt->tm_mon + 1, lt->tm_year + 1900);
    return buf;
}

inline std::string stPeriodVi(int minutes) {
    int h = (((minutes % 1440) + 1440) % 1440) / 60;
    if (h < 11) return "Sáng";
    if (h < 13) return "Trưa";
    if (h < 18) return "Chiều";
    return "Tối";
}

inline sf::String stFitText(const std::string& s, unsigned int size, float maxW, bool bold) {
    sf::Text t(toSfString(s), Theme::FontPrimary, size);
    if (bold) t.setStyle(sf::Text::Bold);
    if (t.getLocalBounds().width <= maxW) return t.getString();
    sf::String full = t.getString();
    std::size_t n = full.getSize();
    while (n > 1) {
        --n;
        sf::String cut = full.substring(0, n) + sf::String("...");
        t.setString(cut);
        if (t.getLocalBounds().width <= maxW) return cut;
    }
    return full;
}

inline std::string removeVietnameseDiacritics(const std::string& input) {
    sf::String u32Str = toSfString(input);
    std::string result;
    result.reserve(input.size());
    for (std::size_t i = 0; i < u32Str.getSize(); ++i) {
        sf::Uint32 cp = u32Str[i];
        if (cp >= 'A' && cp <= 'Z') {
            result.push_back(static_cast<char>(cp + ('a' - 'A')));
            continue;
        }
        if (cp < 128) {
            result.push_back(static_cast<char>(cp));
            continue;
        }
        switch (cp) {
            // A, a
            case 0x00C0: case 0x00C1: case 0x00C2: case 0x00C3:
            case 0x00E0: case 0x00E1: case 0x00E2: case 0x00E3:
            case 0x0102: case 0x0103:
            case 0x1EA0: case 0x1EA1: case 0x1EA2: case 0x1EA3:
            case 0x1EA4: case 0x1EA5: case 0x1EA6: case 0x1EA7:
            case 0x1EA8: case 0x1EA9: case 0x1EAA: case 0x1EAB:
            case 0x1EAC: case 0x1EAD: case 0x1EAE: case 0x1EAF:
            case 0x1EB0: case 0x1EB1: case 0x1EB2: case 0x1EB3:
            case 0x1EB4: case 0x1EB5: case 0x1EB6: case 0x1EB7:
                result.push_back('a');
                break;

            // E, e
            case 0x00C8: case 0x00C9: case 0x00CA:
            case 0x00E8: case 0x00E9: case 0x00EA:
            case 0x1EB8: case 0x1EB9: case 0x1EBA: case 0x1EBB:
            case 0x1EBC: case 0x1EBD: case 0x1EBE: case 0x1EBF:
            case 0x1EC0: case 0x1EC1: case 0x1EC2: case 0x1EC3:
            case 0x1EC4: case 0x1EC5: case 0x1EC6: case 0x1EC7:
                result.push_back('e');
                break;

            // I, i
            case 0x00CC: case 0x00CD:
            case 0x00EC: case 0x00ED:
            case 0x0128: case 0x0129:
            case 0x1EC8: case 0x1EC9: case 0x1ECA: case 0x1ECB:
                result.push_back('i');
                break;

            // O, o
            case 0x00D2: case 0x00D3: case 0x00D4: case 0x00D5:
            case 0x00F2: case 0x00F3: case 0x00F4: case 0x00F5:
            case 0x01A0: case 0x01A1:
            case 0x1ECC: case 0x1ECD: case 0x1ECE: case 0x1ECF:
            case 0x1ED0: case 0x1ED1: case 0x1ED2: case 0x1ED3:
            case 0x1ED4: case 0x1ED5: case 0x1ED6: case 0x1ED7:
            case 0x1ED8: case 0x1ED9: case 0x1EDA: case 0x1EDB:
            case 0x1EDC: case 0x1EDD: case 0x1EDE: case 0x1EDF:
            case 0x1EE0: case 0x1EE1: case 0x1EE2: case 0x1EE3:
                result.push_back('o');
                break;

            // U, u
            case 0x00D9: case 0x00DA:
            case 0x00F9: case 0x00FA:
            case 0x0168: case 0x0169:
            case 0x01AF: case 0x01B0:
            case 0x1EE4: case 0x1EE5: case 0x1EE6: case 0x1EE7:
            case 0x1EE8: case 0x1EE9: case 0x1EEA: case 0x1EEB:
            case 0x1EEC: case 0x1EED: case 0x1EEE: case 0x1EEF:
            case 0x1EF0: case 0x1EF1:
                result.push_back('u');
                break;

            // Y, y
            case 0x00DD: case 0x00FD:
            case 0x1EF2: case 0x1EF3: case 0x1EF4: case 0x1EF5:
            case 0x1EF6: case 0x1EF7: case 0x1EF8: case 0x1EF9:
                result.push_back('y');
                break;

            // D, d
            case 0x0110: case 0x0111:
                result.push_back('d');
                break;

            default:
                if (cp < 256) result.push_back(static_cast<char>(cp));
                break;
        }
    }
    return result;
}

} // namespace UI

#endif // CINEMA_UI_APP_SHOWTIME_HELPERS_H