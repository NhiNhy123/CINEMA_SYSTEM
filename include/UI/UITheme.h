#ifndef UI_THEME_H
#define UI_THEME_H

#include <SFML/Graphics.hpp>
#include <SFML/Config.hpp>
#include <string>
#include <vector>
#include <iostream>

#if (defined(SFML_VERSION_MAJOR) && SFML_VERSION_MAJOR >= 3)
    #define SFML_LOAD_FONT(fontObj, pathStr) (fontObj).openFromFile(pathStr)
#else
    #define SFML_LOAD_FONT(fontObj, pathStr) (fontObj).loadFromFile(pathStr)
#endif

namespace UI {

inline sf::String toSfString(const std::string& utf8Str) {
    return sf::String::fromUtf8(utf8Str.begin(), utf8Str.end());
}

inline std::vector<sf::String> wrapSfTextLines(
    const sf::Font& font,
    const sf::String& input,
    unsigned int charSize,
    float maxWidth,
    sf::Uint32 style = sf::Text::Regular
) {
    std::vector<sf::String> lines;
    if (input.isEmpty()) {
        lines.push_back(sf::String());
        return lines;
    }

    sf::Text measureTxt;
    measureTxt.setFont(font);
    measureTxt.setCharacterSize(charSize);
    measureTxt.setStyle(style);

    auto measureWidth = [&](const sf::String& s) -> float {
        if (s.isEmpty()) return 0.f;
        measureTxt.setString(s);
        sf::FloatRect b = measureTxt.getLocalBounds();
        return b.left + b.width;
    };

    sf::String currentLine;
    for (std::size_t i = 0; i < input.getSize(); ++i) {
        sf::Uint32 ch = input[i];
        if (ch == '\r') continue;
        if (ch == '\n') {
            lines.push_back(currentLine);
            currentLine.clear();
            continue;
        }

        sf::String candidate = currentLine + ch;
        if (!currentLine.isEmpty() && measureWidth(candidate) > maxWidth) {
            // Try breaking at the last space, slash, or hyphen
            int breakIdx = -1;
            for (int k = static_cast<int>(currentLine.getSize()) - 1; k > 0; --k) {
                sf::Uint32 c = currentLine[static_cast<std::size_t>(k)];
                if (c == ' ' || c == '/' || c == '-') {
                    breakIdx = k;
                    break;
                }
            }

            if (ch == ' ') {
                lines.push_back(currentLine);
                currentLine.clear();
            } else if (breakIdx > 0) {
                sf::Uint32 brkChar = currentLine[static_cast<std::size_t>(breakIdx)];
                std::size_t headLen = (brkChar == ' ')
                    ? static_cast<std::size_t>(breakIdx)
                    : static_cast<std::size_t>(breakIdx + 1);
                lines.push_back(currentLine.substring(0, headLen));
                currentLine = currentLine.substring(static_cast<std::size_t>(breakIdx + 1)) + ch;
            } else {
                lines.push_back(currentLine);
                currentLine = sf::String(ch);
            }
        } else {
            currentLine = candidate;
        }
    }
    lines.push_back(currentLine);
    return lines;
}

inline sf::String wrapSfTextString(
    const sf::Font& font,
    const sf::String& input,
    unsigned int charSize,
    float maxWidth,
    std::size_t maxLines = 2,
    sf::Uint32 style = sf::Text::Regular,
    std::size_t* outLineCount = nullptr
) {
    std::vector<sf::String> lines = wrapSfTextLines(font, input, charSize, maxWidth, style);
    if (maxLines > 0 && lines.size() > maxLines) {
        lines.resize(maxLines);
    }
    if (outLineCount) *outLineCount = lines.size();
    sf::String joined;
    for (std::size_t i = 0; i < lines.size(); ++i) {
        if (i > 0) joined += "\n";
        joined += lines[i];
    }
    return joined;
}

class Theme {
public:
    // Main Palette (Modern Cinema Dark Navy & Vibrant Accents)
    static const sf::Color BgDark;           // #0B1120 Deep Dark Navy
    static const sf::Color BgMain;           // #0F172A Slate Dark
    static const sf::Color SidebarBg;        // #0F172A Sidebar
    static const sf::Color CardBg;           // #1E293B Slate Card
    static const sf::Color CardBgHover;      // #273549 Card Hover
    static const sf::Color CardBorder;       // #334155 Muted Border
    static const sf::Color InputBg;          // #141E33 Input Background
    static const sf::Color InputBorder;      // #475569 Input Border
    static const sf::Color InputFocus;       // #6366F1 Focused Input Border

    // Accent Colors
    static const sf::Color Primary;          // #6366F1 Indigo Accent
    static const sf::Color PrimaryHover;     // #4F46E5 Indigo Hover
    static const sf::Color PrimaryActive;    // #4338CA Indigo Pressed
    static const sf::Color Cyan;             // #06B6D4 Modern Cyan
    static const sf::Color CyanHover;        // #0891B2 Cyan Hover
    static const sf::Color Success;          // #10B981 Emerald Green
    static const sf::Color Warning;          // #F59E0B Amber Warning
    static const sf::Color Danger;           // #EF4444 Rose / Red Warning
    static const sf::Color DangerHover;      // #DC2626 Red Hover

    // Text Colors
    static const sf::Color TextPrimary;      // #F8FAFC Pure Crisp White
    static const sf::Color TextSecondary;    // #94A3B8 Slate Gray
    static const sf::Color TextMuted;        // #64748B Darker Muted Text
    static const sf::Color TextPlaceholder;  // #475569 Input Placeholder

    // Seat Type Colors
    static const sf::Color SeatStandard;     // #3B82F6 Standard Seat (Blue)
    static const sf::Color SeatVIP;          // #F59E0B VIP Seat (Gold)
    static const sf::Color SeatSweetbox;     // #EC4899 Sweetbox/Couple (Pink)
    static const sf::Color SeatSelected;     // #10B981 Selected (Emerald)
    static const sf::Color SeatBooked;       // #EF4444 Booked (Red)
    static const sf::Color SeatMaintenance;  // #475569 Maintenance (Dark Gray)

    // Typography Fonts
    static sf::Font FontPrimary;     // Segoe UI (Main UI, Vietnamese Unicode)
    static sf::Font FontSecondary;   // Quicksand Medium (Sub details, modern rounded)
    static sf::Font FontTitle;       // Segoe UI Bold (Titles, Headings with full Vietnamese Unicode)
    static sf::Font FontCinema;      // Bebas Neue (Cinema Latin title: CINESTAR CINEMA)
    static bool FontsLoaded;

    // Font Sizes
    static const unsigned int SizeDisplay     = 32;
    static const unsigned int SizeTitle       = 24;
    static const unsigned int SizeHeader      = 19;
    static const unsigned int SizeSubheader   = 16;
    static const unsigned int SizeBody        = 14;
    static const unsigned int SizeSmall       = 12;

    // Border Radii
    static constexpr float RadiusSmall   = 6.f;
    static constexpr float RadiusMedium  = 10.f;
    static constexpr float RadiusLarge   = 16.f;
    static constexpr float RadiusPill    = 999.f;

    static bool loadFonts(const std::string& fontDir = "assets/fonts");
};

} // namespace UI

#endif // UI_THEME_H
