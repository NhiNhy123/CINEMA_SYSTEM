#ifndef DROPDOWN_BOX_H
#define DROPDOWN_BOX_H

#include "UITheme.h"
#include "RoundedRectRenderer.h"
#include <vector>
#include <string>
#include <functional>
#include <algorithm>
#include <cmath>

namespace UI {

class DropdownBox {
private:
    sf::Vector2f position;
    sf::Vector2f size;
    std::vector<std::string> options;
    int selectedIndex;
    int hoveredOptionIndex;
    bool isExpanded;
    bool isVisible;
    bool isEnabled;
    const sf::Font* font;
    unsigned int fontSize;
    float itemHeight;
    std::function<void(int, const std::string&)> onSelect;

    bool isLightStyle;
    int maxVisibleItems;
    int scrollOffset;
    bool isDraggingScrollbar;
    std::string leftIcon; // "date", "room", "time", "icon_only_date", "icon_only_time" or ""
    std::string placeholder;
    float popupWidth;
    float popupOffsetX;

    void updateScrollFromMouseY(float mouseY) {
        int totalOpts = static_cast<int>(options.size());
        int maxScroll = totalOpts - maxVisibleItems;
        if (maxScroll <= 0) return;

        float dropY = position.y + size.y + 4.f;
        int visibleCount = std::min(totalOpts, maxVisibleItems);
        float totalHeight = static_cast<float>(visibleCount) * itemHeight;
        float sbTrackY = dropY + 4.f;
        float sbTrackH = totalHeight - 8.f;
        float thumbH = std::max(22.f, sbTrackH * (static_cast<float>(maxVisibleItems) / static_cast<float>(totalOpts)));
        float maxThumbTravel = sbTrackH - thumbH;

        if (maxThumbTravel > 0.f) {
            float relThumbY = mouseY - sbTrackY - thumbH * 0.5f;
            float ratio = std::clamp(relThumbY / maxThumbTravel, 0.f, 1.f);
            scrollOffset = std::clamp(static_cast<int>(std::round(ratio * static_cast<float>(maxScroll))), 0, maxScroll);
        }
    }

public:
    DropdownBox(
        sf::Vector2f pos = sf::Vector2f(0.f, 0.f),
        sf::Vector2f sz = sf::Vector2f(220.f, 40.f),
        const std::vector<std::string>& opts = {},
        int defaultIdx = 0
    ) : position(pos), size(sz), options(opts), selectedIndex(defaultIdx),
        hoveredOptionIndex(-1), isExpanded(false), isVisible(true),
        isEnabled(true), font(&Theme::FontPrimary), fontSize(Theme::SizeBody),
        itemHeight(38.f), onSelect(nullptr),
        isLightStyle(false), maxVisibleItems(6), scrollOffset(0),
        isDraggingScrollbar(false),
        leftIcon(""),
        placeholder("-- Chọn --"),
        popupWidth(-1.f), popupOffsetX(0.f) {}

    void setPosition(sf::Vector2f pos) { position = pos; }
    void setSize(sf::Vector2f sz) { size = sz; }
    void setPlaceholder(const std::string& p) { placeholder = p; }
    std::string getPlaceholder() const { return placeholder; }
    void setOptions(const std::vector<std::string>& opts, int defIdx = 0) {
        options = opts;
        selectedIndex = (defIdx >= 0 && defIdx < static_cast<int>(options.size())) ? defIdx : 0;
        int maxScroll = std::max(0, static_cast<int>(options.size()) - maxVisibleItems);
        scrollOffset = std::clamp(scrollOffset, 0, maxScroll);
        isDraggingScrollbar = false;
    }
    void setSelectedIndex(int idx) {
        if (idx >= 0 && idx < static_cast<int>(options.size())) {
            selectedIndex = idx;
        } else if (idx < 0) {
            selectedIndex = -1;
        }
    }
    int getSelectedIndex() const { return selectedIndex; }
    std::string getSelectedValue() const {
        if (selectedIndex >= 0 && selectedIndex < static_cast<int>(options.size())) {
            return options[selectedIndex];
        }
        return "";
    }
    bool getExpanded() const { return isExpanded; }
    void setExpanded(bool exp) {
        isExpanded = exp;
        if (!exp) {
            scrollOffset = 0;
            isDraggingScrollbar = false;
        }
    }
    void setVisible(bool vis) { isVisible = vis; }
    void setEnabled(bool en) { isEnabled = en; }
    void setOnSelect(std::function<void(int, const std::string&)> cb) { onSelect = cb; }
    void setFontSize(unsigned int sz) { fontSize = sz; }
    void setItemHeight(float h) { itemHeight = h; }
    void setLightStyle(bool ls) { isLightStyle = ls; }
    void setMaxVisibleItems(int m) { maxVisibleItems = m; }
    void setLeftIcon(const std::string& ic) { leftIcon = ic; }
    void setPopupLayout(float width, float offX = 0.f) {
        popupWidth = width;
        popupOffsetX = offX;
    }
    float getEffectivePopupWidth() const {
        return popupWidth > 0.f ? popupWidth : size.x;
    }
    float getEffectivePopupX() const {
        return position.x + popupOffsetX;
    }

    bool containsMain(sf::Vector2f pt) const {
        return pt.x >= position.x && pt.x <= position.x + size.x &&
               pt.y >= position.y && pt.y <= position.y + size.y;
    }

    bool containsDropdown(sf::Vector2f pt) const {
        if (!isExpanded || options.empty()) return false;
        int visibleCount = std::min(static_cast<int>(options.size()), maxVisibleItems);
        float dropHeight = static_cast<float>(visibleCount) * itemHeight;
        float px = getEffectivePopupX();
        float pw = getEffectivePopupWidth();
        return pt.x >= px - 2.f && pt.x <= px + pw + 2.f &&
               pt.y >= position.y + size.y && pt.y <= position.y + size.y + 6.f + dropHeight;
    }

    bool isScrollbarHit(sf::Vector2f pt) const {
        if (!isExpanded || static_cast<int>(options.size()) <= maxVisibleItems) return false;
        float px = getEffectivePopupX();
        float pw = getEffectivePopupWidth();
        float dropY = position.y + size.y + 4.f;
        int visibleCount = std::min(static_cast<int>(options.size()), maxVisibleItems);
        float totalHeight = static_cast<float>(visibleCount) * itemHeight;
        return pt.x >= px + pw - 22.f && pt.x <= px + pw + 4.f &&
               pt.y >= dropY && pt.y <= dropY + totalHeight;
    }

    bool handleMouseMove(sf::Vector2f mousePos) {
        if (!isVisible || !isEnabled) return false;
        if (isExpanded) {
            float dropY = position.y + size.y + 4.f;
            float px = getEffectivePopupX();
            float pw = getEffectivePopupWidth();
            int maxScroll = static_cast<int>(options.size()) - maxVisibleItems;

            if (isDraggingScrollbar && maxScroll > 0) {
                updateScrollFromMouseY(mousePos.y);
                return true;
            }

            hoveredOptionIndex = -1;
            int visibleCount = std::min(static_cast<int>(options.size()) - scrollOffset, maxVisibleItems);
            for (int i = 0; i < visibleCount; ++i) {
                int optIdx = scrollOffset + i;
                float iy = dropY + static_cast<float>(i) * itemHeight;
                if (mousePos.x >= px && mousePos.x <= px + pw &&
                    mousePos.y >= iy && mousePos.y < iy + itemHeight) {
                    hoveredOptionIndex = optIdx;
                    return true;
                }
            }
        }
        return containsMain(mousePos);
    }

    bool handleMouseWheelScrolled(float delta, sf::Vector2f mousePos) {
        if (!isVisible || !isEnabled || !isExpanded) return false;
        if (containsDropdown(mousePos) || containsMain(mousePos)) {
            int maxScroll = std::max(0, static_cast<int>(options.size()) - maxVisibleItems);
            if (maxScroll > 0) {
                int step = (std::abs(delta) > 1.5f) ? 2 : 1;
                if (delta > 0.f && scrollOffset > 0) {
                    scrollOffset = std::max(0, scrollOffset - step);
                } else if (delta < 0.f && scrollOffset < maxScroll) {
                    scrollOffset = std::min(maxScroll, scrollOffset + step);
                }
            }
            return true;
        }
        return false;
    }

    bool handleMouseButtonPressed(sf::Mouse::Button button, sf::Vector2f mousePos) {
        if (!isVisible || !isEnabled) return false;
        if (button == sf::Mouse::Left) {
            if (containsMain(mousePos)) {
                isExpanded = !isExpanded;
                if (!isExpanded) isDraggingScrollbar = false;
                return true;
            } else if (isExpanded && containsDropdown(mousePos)) {
                float px = getEffectivePopupX();
                float pw = getEffectivePopupWidth();
                float dropY = position.y + size.y + 4.f;
                int maxScroll = static_cast<int>(options.size()) - maxVisibleItems;

                // Priority 1: Check scrollbar hit first (22px right margin)
                if (maxScroll > 0 && isScrollbarHit(mousePos)) {
                    isDraggingScrollbar = true;
                    updateScrollFromMouseY(mousePos.y);
                    return true;
                }

                // Priority 2: Option selection
                int clickedRow = static_cast<int>((mousePos.y - dropY) / itemHeight);
                int clickedIdx = scrollOffset + clickedRow;
                if (clickedIdx >= 0 && clickedIdx < static_cast<int>(options.size())) {
                    selectedIndex = clickedIdx;
                    isExpanded = false;
                    isDraggingScrollbar = false;
                    if (onSelect) onSelect(selectedIndex, options[selectedIndex]);
                    return true;
                }
            } else if (isExpanded) {
                isExpanded = false;
                isDraggingScrollbar = false;
                return true;
            }
        }
        return false;
    }

    bool handleMouseButtonReleased(sf::Mouse::Button button, sf::Vector2f mousePos) {
        if (button == sf::Mouse::Left) {
            if (isDraggingScrollbar) {
                isDraggingScrollbar = false;
                return true;
            }
        }
        return false;
    }

    void draw(sf::RenderTarget& target) {
        if (!isVisible) return;

        // Draw Main Box
        sf::Color bg = isLightStyle ? sf::Color::White : Theme::InputBg;
        sf::Color border = isExpanded
            ? (isLightStyle ? sf::Color(2, 132, 199) : Theme::Primary)
            : (isLightStyle ? sf::Color(226, 232, 240) : Theme::InputBorder);
        bool iconOnly = (leftIcon == "icon_only_date" || leftIcon == "icon_only_time");
        if (iconOnly) {
            // Icon-only trigger is embedded inside an input box: draw a subtle
            // separator instead of a full box so the icon does not look "stuck on".
            sf::RectangleShape sep(sf::Vector2f(1.f, size.y - 16.f));
            sep.setPosition(position.x, position.y + 8.f);
            sep.setFillColor(sf::Color(226, 232, 240));
            target.draw(sep);
            if (isExpanded) {
                RoundedRectRenderer::render(target, sf::Vector2f(position.x + 4.f, position.y + 5.f),
                                            sf::Vector2f(size.x - 8.f, size.y - 10.f), 6.f, sf::Color(224, 242, 254));
            }
        } else {
            RoundedRectRenderer::render(target, position, size, isLightStyle ? 8.f : Theme::RadiusMedium, bg, 1.f, border);
        }

        float textLeftPad = 14.f;

        // Left Icon or Icon Only
        if (leftIcon == "icon_only_date") {
            float icX = position.x + (size.x - 16.f) / 2.f;
            float icY = position.y + (size.y - 16.f) / 2.f;
            RoundedRectRenderer::render(target, sf::Vector2f(icX, icY), sf::Vector2f(16.f, 16.f), 3.f, sf::Color(240, 249, 255), 1.f, sf::Color(2, 132, 199));
            sf::RectangleShape cLine(sf::Vector2f(12.f, 2.f));
            cLine.setPosition(icX + 2.f, icY + 4.f);
            cLine.setFillColor(sf::Color(2, 132, 199));
            target.draw(cLine);
            return;
        } else if (leftIcon == "icon_only_time") {
            float icX = position.x + (size.x - 16.f) / 2.f;
            float icY = position.y + (size.y - 16.f) / 2.f;
            sf::CircleShape clk(7.5f);
            clk.setPosition(icX, icY);
            clk.setFillColor(sf::Color::Transparent);
            clk.setOutlineThickness(1.5f);
            clk.setOutlineColor(sf::Color(2, 132, 199));
            target.draw(clk);
            sf::RectangleShape hHand(sf::Vector2f(4.f, 1.5f));
            hHand.setPosition(icX + 7.5f, icY + 7.5f);
            hHand.setFillColor(sf::Color(2, 132, 199));
            target.draw(hHand);
            sf::RectangleShape mHand(sf::Vector2f(1.5f, 5.5f));
            mHand.setPosition(icX + 7.5f, icY + 3.f);
            mHand.setFillColor(sf::Color(2, 132, 199));
            target.draw(mHand);
            return;
        } else if (leftIcon == "date") {
            // Calendar icon
            float icX = position.x + 12.f;
            float icY = position.y + (size.y - 14.f) / 2.f;
            RoundedRectRenderer::render(target, sf::Vector2f(icX, icY), sf::Vector2f(14.f, 14.f), 2.f, sf::Color(241, 245, 249), 1.f, sf::Color(100, 116, 139));
            sf::RectangleShape cLine(sf::Vector2f(10.f, 1.5f));
            cLine.setPosition(icX + 2.f, icY + 4.f);
            cLine.setFillColor(sf::Color(100, 116, 139));
            target.draw(cLine);
            textLeftPad = 34.f;
        } else if (leftIcon == "room") {
            // Projector/screen icon
            float icX = position.x + 12.f;
            float icY = position.y + (size.y - 12.f) / 2.f;
            sf::RectangleShape rCam(sf::Vector2f(11.f, 9.f));
            rCam.setPosition(icX, icY + 1.f);
            rCam.setFillColor(sf::Color(224, 242, 254));
            rCam.setOutlineThickness(1.2f);
            rCam.setOutlineColor(sf::Color(2, 132, 199));
            target.draw(rCam);

            sf::ConvexShape rLens(3);
            rLens.setPoint(0, sf::Vector2f(icX + 11.f, icY + 3.f));
            rLens.setPoint(1, sf::Vector2f(icX + 14.f, icY + 1.f));
            rLens.setPoint(2, sf::Vector2f(icX + 14.f, icY + 7.f));
            rLens.setFillColor(sf::Color(2, 132, 199));
            target.draw(rLens);
            textLeftPad = 34.f;
        }

        if (font) {
            // Draw Selected Item Text
            sf::Text text;
            text.setFont(*font);
            text.setCharacterSize(fontSize);
            std::string disp = getSelectedValue();
            bool isPh = disp.empty();
            if (isPh) disp = placeholder;
            text.setFillColor(isPh ? sf::Color(148, 163, 184) : (isLightStyle ? sf::Color(30, 41, 59) : Theme::TextPrimary));
            // Truncate (by measured pixel width, UTF-8 safe) to leave room for chevron
            float maxTextW = size.x - textLeftPad - 30.f;
            text.setString(toSfString(disp));
            if (text.getLocalBounds().width > maxTextW) {
                sf::String full = toSfString(disp);
                std::size_t n = full.getSize();
                while (n > 1) {
                    --n;
                    sf::String cut = full.substring(0, n) + sf::String("...");
                    text.setString(cut);
                    if (text.getLocalBounds().width <= maxTextW) break;
                }
            }

            float tx = position.x + textLeftPad;
            float ty = position.y + (size.y - fontSize) / 2.0f - 2.f;
            text.setPosition(tx, ty);
            target.draw(text);

            // Draw Chevron (▼ or ▲)
            sf::Text arrow;
            arrow.setFont(*font);
            arrow.setCharacterSize(fontSize - 1);
            arrow.setFillColor(isLightStyle ? sf::Color(100, 116, 139) : Theme::TextSecondary);
            arrow.setString(isExpanded ? "^" : "v");
            arrow.setPosition(position.x + size.x - 20.f, ty);
            target.draw(arrow);
        }
    }

    // Call drawPopup AFTER other UI elements so dropdown renders on top
    void drawPopup(sf::RenderTarget& target) {
        if (!isVisible || !isExpanded || options.empty() || !font) return;

        int visibleCount = std::min(static_cast<int>(options.size()), maxVisibleItems);
        float dropY = position.y + size.y + 4.f;
        float totalHeight = static_cast<float>(visibleCount) * itemHeight;
        float px = getEffectivePopupX();
        float pw = getEffectivePopupWidth();

        // Dropdown menu container
        sf::Color contBg = isLightStyle ? sf::Color::White : Theme::CardBg;
        sf::Color contBorder = isLightStyle ? sf::Color(203, 213, 225) : Theme::CardBorder;
        RoundedRectRenderer::render(
            target,
            sf::Vector2f(px, dropY),
            sf::Vector2f(pw, totalHeight),
            isLightStyle ? 8.f : Theme::RadiusMedium,
            contBg,
            1.f,
            contBorder
        );

        // Draw visible options
        for (int i = 0; i < visibleCount; ++i) {
            int optIdx = scrollOffset + i;
            if (optIdx >= static_cast<int>(options.size())) break;

            float iy = dropY + static_cast<float>(i) * itemHeight;
            bool isHovered = (hoveredOptionIndex == optIdx);
            bool isSelected = (selectedIndex == optIdx);

            if (isHovered || isSelected) {
                sf::Color itemBg;
                if (isLightStyle) {
                    itemBg = isSelected ? sf::Color(224, 242, 254) : sf::Color(241, 245, 249);
                } else {
                    itemBg = isSelected ? sf::Color(99, 102, 241, 60) : Theme::CardBgHover;
                }
                sf::RectangleShape highlight(sf::Vector2f(pw - 4.f, itemHeight - 2.f));
                highlight.setPosition(px + 2.f, iy + 1.f);
                highlight.setFillColor(itemBg);
                target.draw(highlight);
            }

            sf::Text optText;
            optText.setFont(*font);
            optText.setCharacterSize(fontSize);
            if (isLightStyle) {
                optText.setFillColor(isSelected ? sf::Color(2, 132, 199) : (isHovered ? sf::Color(15, 23, 42) : sf::Color(71, 85, 105)));
                if (isSelected) optText.setStyle(sf::Text::Bold);
            } else {
                optText.setFillColor(isSelected ? Theme::Cyan : (isHovered ? Theme::TextPrimary : Theme::TextSecondary));
            }

            std::string dispOpt = options[optIdx];
            int maxOptChars = std::max(22, static_cast<int>((pw - 28.f) / 8.5f));
            if (static_cast<int>(dispOpt.length()) > maxOptChars) dispOpt = dispOpt.substr(0, maxOptChars - 2) + "...";
            optText.setString(toSfString(dispOpt));

            float tx = px + 14.f;
            float ty = iy + (itemHeight - fontSize) / 2.0f - 2.f;
            optText.setPosition(tx, ty);
            target.draw(optText);
        }

        // Draw Scrollbar if options exceed maxVisibleItems
        if (static_cast<int>(options.size()) > maxVisibleItems) {
            int maxScroll = static_cast<int>(options.size()) - maxVisibleItems;
            float sbTrackX = px + pw - 9.f;
            float sbTrackY = dropY + 4.f;
            float sbTrackH = totalHeight - 8.f;

            // Draw subtle track
            RoundedRectRenderer::render(target, sf::Vector2f(sbTrackX - 1.f, sbTrackY), sf::Vector2f(7.f, sbTrackH), 3.5f,
                                        isLightStyle ? sf::Color(241, 245, 249) : sf::Color(30, 41, 59));

            float thumbH = std::max(22.f, sbTrackH * (static_cast<float>(maxVisibleItems) / static_cast<float>(options.size())));
            float thumbY = sbTrackY + (sbTrackH - thumbH) * (static_cast<float>(scrollOffset) / static_cast<float>(maxScroll));

            sf::Color thumbColor = isDraggingScrollbar
                ? (isLightStyle ? sf::Color(2, 132, 199) : Theme::Cyan)
                : (isLightStyle ? sf::Color(148, 163, 184) : sf::Color(100, 116, 139));

            RoundedRectRenderer::render(target, sf::Vector2f(sbTrackX, thumbY), sf::Vector2f(5.f, thumbH), 2.5f,
                                        thumbColor);
        }
    }
};

} // namespace UI

#endif // DROPDOWN_BOX_H
