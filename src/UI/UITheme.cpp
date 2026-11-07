#include "../../include/UI/UITheme.h"

namespace UI {

const sf::Color Theme::BgDark          = sf::Color(11, 17, 32);      // #0B1120
const sf::Color Theme::BgMain          = sf::Color(15, 23, 42);      // #0F172A
const sf::Color Theme::SidebarBg       = sf::Color(15, 23, 42);      // #0F172A
const sf::Color Theme::CardBg          = sf::Color(30, 41, 59);      // #1E293B
const sf::Color Theme::CardBgHover     = sf::Color(39, 53, 73);      // #273549
const sf::Color Theme::CardBorder      = sf::Color(51, 65, 85);      // #334155
const sf::Color Theme::InputBg         = sf::Color(20, 30, 51);      // #141E33
const sf::Color Theme::InputBorder     = sf::Color(71, 85, 105);     // #475569
const sf::Color Theme::InputFocus      = sf::Color(99, 102, 241);    // #6366F1

const sf::Color Theme::Primary         = sf::Color(99, 102, 241);    // #6366F1 Indigo
const sf::Color Theme::PrimaryHover    = sf::Color(79, 70, 229);     // #4F46E5
const sf::Color Theme::PrimaryActive   = sf::Color(67, 56, 202);     // #4338CA
const sf::Color Theme::Cyan            = sf::Color(6, 182, 212);     // #06B6D4
const sf::Color Theme::CyanHover       = sf::Color(8, 145, 178);     // #0891B2
const sf::Color Theme::Success         = sf::Color(16, 185, 129);    // #10B981
const sf::Color Theme::Warning         = sf::Color(245, 158, 11);    // #F59E0B
const sf::Color Theme::Danger          = sf::Color(239, 68, 68);     // #EF4444
const sf::Color Theme::DangerHover     = sf::Color(220, 38, 38);     // #DC2626

const sf::Color Theme::TextPrimary     = sf::Color(248, 250, 252);   // #F8FAFC
const sf::Color Theme::TextSecondary   = sf::Color(148, 163, 184);   // #94A3B8
const sf::Color Theme::TextMuted       = sf::Color(100, 116, 139);   // #64748B
const sf::Color Theme::TextPlaceholder = sf::Color(71, 85, 105);     // #475569

const sf::Color Theme::SeatStandard    = sf::Color(59, 130, 246);    // #3B82F6
const sf::Color Theme::SeatVIP         = sf::Color(245, 158, 11);    // #F59E0B
const sf::Color Theme::SeatSweetbox    = sf::Color(236, 72, 153);    // #EC4899
const sf::Color Theme::SeatSelected    = sf::Color(16, 185, 129);    // #10B981
const sf::Color Theme::SeatBooked      = sf::Color(239, 68, 68);     // #EF4444
const sf::Color Theme::SeatMaintenance = sf::Color(71, 85, 105);     // #475569

sf::Font Theme::FontPrimary;
sf::Font Theme::FontSecondary;
sf::Font Theme::FontTitle;
sf::Font Theme::FontCinema;
bool Theme::FontsLoaded = false;

bool Theme::loadFonts(const std::string& fontDir) {
    if (FontsLoaded) return true;

    bool ok = false;
    std::vector<std::string> segoeCandidates = {
        fontDir + "/segoeui.ttf",
        "C:/Windows/Fonts/segoeui.ttf",
        fontDir + "/arial.ttf",
        "C:/Windows/Fonts/arial.ttf"
    };

    for (size_t i = 0; i < segoeCandidates.size(); ++i) {
        if (SFML_LOAD_FONT(FontPrimary, segoeCandidates[i])) {
            ok = true;
            break;
        }
    }

    // Load FontTitle with Segoe UI Bold to support 100% Vietnamese Unicode characters
    std::vector<std::string> segoeBoldCandidates = {
        fontDir + "/segoeuib.ttf",
        "C:/Windows/Fonts/segoeuib.ttf",
        fontDir + "/arialbd.ttf",
        "C:/Windows/Fonts/arialbd.ttf"
    };
    bool loadedBold = false;
    for (size_t i = 0; i < segoeBoldCandidates.size(); ++i) {
        if (SFML_LOAD_FONT(FontTitle, segoeBoldCandidates[i])) {
            loadedBold = true;
            break;
        }
    }
    if (!loadedBold) {
        FontTitle = FontPrimary;
    }

    std::vector<std::string> quicksandCandidates = {
        fontDir + "/quicksand_medium.ttf",
        fontDir + "/Quicksand-Medium.ttf",
        fontDir + "/quicksand.ttf"
    };
    bool loadedQuick = false;
    for (size_t i = 0; i < quicksandCandidates.size(); ++i) {
        if (SFML_LOAD_FONT(FontSecondary, quicksandCandidates[i])) {
            loadedQuick = true;
            break;
        }
    }
    if (!loadedQuick) {
        FontSecondary = FontPrimary;
    }

    // FontCinema: Bebas Neue for English brand display
    std::vector<std::string> bebasCandidates = {
        fontDir + "/BEBAS_NEUE_ZSMALL.ttf",
        fontDir + "/BebasNeue-Regular.ttf",
        fontDir + "/bebasneue.ttf"
    };
    bool loadedBebas = false;
    for (size_t i = 0; i < bebasCandidates.size(); ++i) {
        if (SFML_LOAD_FONT(FontCinema, bebasCandidates[i])) {
            loadedBebas = true;
            break;
        }
    }
    if (!loadedBebas) {
        FontCinema = FontTitle;
    }

    FontsLoaded = ok;
    return ok;
}

} // namespace UI

