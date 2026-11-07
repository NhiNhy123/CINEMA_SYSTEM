#ifndef DROPDOWN_BOX_H
#define DROPDOWN_BOX_H

#include "UITheme.h"
#include "RoundedRectRenderer.h"
#include <vector>
#include <string>
#include <functional>

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

public:
    DropdownBox(
        sf::Vector2f pos = sf::Vector2f(0.f, 0.f),
        sf::Vector2f sz = sf::Vector2f(220.f, 40.f),
        const std::vector<std::string>& opts = {},
        int defaultIdx = 0
    ) : position(pos), size(sz), options(opts), selectedIndex(defaultIdx),
        hoveredOptionIndex(-1), isExpanded(false), isVisible(true),
        isEnabled(true), font(&Theme::FontPrimary), fontSize(Theme::SizeBody),
        itemHeight(38.f), onSelect(nullptr) {}

    void setPosition(sf::Vector2f pos) { position = pos; }
    void setSize(sf::Vector2f sz) { size = sz; }
    void setOptions(const std::vector<std::string>& opts, int defIdx = 0) {
        options = opts;
        selectedIndex = (defIdx >= 0 && defIdx < static_cast<int>(options.size())) ? defIdx : 0;
    }
    void setSelectedIndex(int idx) {
        if (idx >= 0 && idx < static_cast<int>(options.size())) {
            selectedIndex = idx;
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
    void setExpanded(bool exp) { isExpanded = exp; }
    void setVisible(bool vis) { isVisible = vis; }
    void setEnabled(bool en) { isEnabled = en; }
    void setOnSelect(std::function<void(int, const std::string&)> cb) { onSelect = cb; }

    bool containsMain(sf::Vector2f pt) const {
        return pt.x >= position.x && pt.x <= position.x + size.x &&
               pt.y >= position.y && pt.y <= position.y + size.y;
    }

    bool containsDropdown(sf::Vector2f pt) const {
        if (!isExpanded || options.empty()) return false;
        float dropHeight = static_cast<float>(options.size()) * itemHeight;
        return pt.x >= position.x && pt.x <= position.x + size.x &&
               pt.y >= position.y + size.y + 4.f && pt.y <= position.y + size.y + 4.f + dropHeight;
    }

    bool handleMouseMove(sf::Vector2f mousePos) {
        if (!isVisible || !isEnabled) return false;
        if (isExpanded) {
            float dropY = position.y + size.y + 4.f;
            hoveredOptionIndex = -1;
            for (size_t i = 0; i < options.size(); ++i) {
                float iy = dropY + static_cast<float>(i) * itemHeight;
                if (mousePos.x >= position.x && mousePos.x <= position.x + size.x &&
                    mousePos.y >= iy && mousePos.y < iy + itemHeight) {
                    hoveredOptionIndex = static_cast<int>(i);
                    return true;
                }
            }
        }
        return containsMain(mousePos);
    }

    bool handleMouseButtonPressed(sf::Mouse::Button button, sf::Vector2f mousePos) {
        if (!isVisible || !isEnabled) return false;
        if (button == sf::Mouse::Left) {
            if (containsMain(mousePos)) {
                isExpanded = !isExpanded;
                return true;
            } else if (isExpanded && containsDropdown(mousePos)) {
                float dropY = position.y + size.y + 4.f;
                int clickedIdx = static_cast<int>((mousePos.y - dropY) / itemHeight);
                if (clickedIdx >= 0 && clickedIdx < static_cast<int>(options.size())) {
                    selectedIndex = clickedIdx;
                    isExpanded = false;
                    if (onSelect) onSelect(selectedIndex, options[selectedIndex]);
                    return true;
                }
            } else if (isExpanded) {
                isExpanded = false;
                return true;
            }
        }
        return false;
    }

    void draw(sf::RenderTarget& target) {
        if (!isVisible) return;

        // Draw Main Box
        sf::Color bg = Theme::InputBg;
        sf::Color border = isExpanded ? Theme::Primary : Theme::InputBorder;
        RoundedRectRenderer::render(target, position, size, Theme::RadiusMedium, bg, 1.5f, border);

        if (font) {
            // Draw Selected Item Text
            sf::Text text;
            text.setFont(*font);
            text.setCharacterSize(fontSize);
            text.setFillColor(Theme::TextPrimary);

            std::string disp = getSelectedValue();
            if (disp.empty()) disp = "-- Chọn --";
            text.setString(toSfString(disp));

            sf::FloatRect tb = text.getLocalBounds();
            float tx = position.x + 14.f;
            float ty = position.y + (size.y - fontSize) / 2.0f - 2.f;
            text.setPosition(tx, ty);
            target.draw(text);

            // Draw arrow (▼ or ▲)
            sf::Text arrow;
            arrow.setFont(*font);
            arrow.setCharacterSize(fontSize - 2);
            arrow.setFillColor(Theme::TextSecondary);
            arrow.setString(isExpanded ? "^" : "v");
            arrow.setPosition(position.x + size.x - 22.f, ty);
            target.draw(arrow);
        }
    }

    // Call drawPopup AFTER other UI elements so dropdown renders on top
    void drawPopup(sf::RenderTarget& target) {
        if (!isVisible || !isExpanded || options.empty() || !font) return;

        float dropY = position.y + size.y + 4.f;
        float totalHeight = static_cast<float>(options.size()) * itemHeight;

        // Dropdown menu container
        RoundedRectRenderer::render(
            target,
            sf::Vector2f(position.x, dropY),
            sf::Vector2f(size.x, totalHeight),
            Theme::RadiusMedium,
            Theme::CardBg,
            1.5f,
            Theme::CardBorder
        );

        // Draw Option Items
        for (size_t i = 0; i < options.size(); ++i) {
            float iy = dropY + static_cast<float>(i) * itemHeight;
            bool isHovered = (hoveredOptionIndex == static_cast<int>(i));
            bool isSelected = (selectedIndex == static_cast<int>(i));

            if (isHovered || isSelected) {
                sf::Color itemBg = isSelected ? sf::Color(99, 102, 241, 60) : Theme::CardBgHover;
                sf::RectangleShape highlight(sf::Vector2f(size.x - 4.f, itemHeight - 2.f));
                highlight.setPosition(position.x + 2.f, iy + 1.f);
                highlight.setFillColor(itemBg);
                target.draw(highlight);
            }

            sf::Text optText;
            optText.setFont(*font);
            optText.setCharacterSize(fontSize);
            optText.setFillColor(isSelected ? Theme::Cyan : (isHovered ? Theme::TextPrimary : Theme::TextSecondary));
            optText.setString(toSfString(options[i]));

            float tx = position.x + 14.f;
            float ty = iy + (itemHeight - fontSize) / 2.0f - 2.f;
            optText.setPosition(tx, ty);
            target.draw(optText);
        }
    }
};

} // namespace UI

#endif // DROPDOWN_BOX_H

