#ifndef UI_CHECKBOX_ITEM_H
#define UI_CHECKBOX_ITEM_H

#include "UITheme.h"
#include "RoundedRectRenderer.h"
#include <functional>
#include <string>

namespace UI {

class UICheckboxItem {
private:
    sf::Vector2f position;
    std::string label;
    bool isChecked;
    bool isHovered;
    bool isVisible;
    bool isEnabled;
    float boxSize;
    const sf::Font* font;
    unsigned int fontSize;
    std::function<void(bool)> onToggle;

    bool customColors;
    sf::Color customBoxBg;
    sf::Color customBoxBorder;
    sf::Color customCheckColor;
    sf::Color customLabelColor;

public:
    UICheckboxItem(
        const std::string& text = "",
        sf::Vector2f pos = sf::Vector2f(0.f, 0.f),
        bool checked = false,
        unsigned int fSize = Theme::SizeBody
    ) : position(pos), label(text), isChecked(checked), isHovered(false),
        isVisible(true), isEnabled(true), boxSize(18.f),
        font(&Theme::FontPrimary), fontSize(fSize), onToggle(nullptr),
        customColors(false) {}

    void setColors(sf::Color bg, sf::Color border, sf::Color check, sf::Color labelCol) {
        customColors = true;
        customBoxBg = bg;
        customBoxBorder = border;
        customCheckColor = check;
        customLabelColor = labelCol;
    }

    void setPosition(sf::Vector2f pos) { position = pos; }
    void setLabel(const std::string& text) { label = text; }
    void setChecked(bool checked) { isChecked = checked; }
    bool getChecked() const { return isChecked; }
    void setVisible(bool vis) { isVisible = vis; }
    void setEnabled(bool en) { isEnabled = en; }
    void setOnToggle(std::function<void(bool)> cb) { onToggle = cb; }

    sf::FloatRect getBounds() const {
        float totalWidth = boxSize + 10.f + static_cast<float>(label.length() * fontSize * 0.7f);
        return sf::FloatRect(position.x, position.y, totalWidth, boxSize);
    }

    bool handleMouseMove(sf::Vector2f mousePos) {
        if (!isVisible || !isEnabled) return false;
        isHovered = getBounds().contains(mousePos);
        return isHovered;
    }

    bool handleMouseButtonPressed(sf::Mouse::Button button, sf::Vector2f mousePos) {
        if (!isVisible || !isEnabled) return false;
        if (button == sf::Mouse::Left && getBounds().contains(mousePos)) {
            isChecked = !isChecked;
            if (onToggle) onToggle(isChecked);
            return true;
        }
        return false;
    }

    void draw(sf::RenderTarget& target) {
        if (!isVisible) return;

        // Checkbox square
        sf::Color boxBg;
        sf::Color boxBorder;
        sf::Color checkColor;
        sf::Color labelColor;

        if (customColors) {
            boxBg = customBoxBg;
            boxBorder = isHovered ? Theme::Primary : customBoxBorder;
            checkColor = customCheckColor;
            labelColor = isHovered ? sf::Color(15, 23, 42) : customLabelColor;
        } else {
            boxBg = isChecked ? Theme::Primary : (isHovered ? Theme::CardBgHover : Theme::InputBg);
            boxBorder = isChecked ? Theme::Primary : (isHovered ? Theme::Cyan : Theme::InputBorder);
            checkColor = Theme::TextPrimary;
            labelColor = isEnabled ? (isHovered ? Theme::TextPrimary : Theme::TextSecondary) : Theme::TextMuted;
        }

        RoundedRectRenderer::render(
            target,
            position,
            sf::Vector2f(boxSize, boxSize),
            4.f,
            boxBg,
            1.5f,
            boxBorder
        );

        // Checkmark (simple tick using 2 lines)
        if (isChecked) {
            sf::VertexArray tick(sf::LinesStrip, 3);
            tick[0].position = sf::Vector2f(position.x + 3.f, position.y + 9.f);
            tick[1].position = sf::Vector2f(position.x + 7.f, position.y + 13.f);
            tick[2].position = sf::Vector2f(position.x + 14.f, position.y + 4.f);
            tick[0].color = checkColor;
            tick[1].color = checkColor;
            tick[2].color = checkColor;
            target.draw(tick);
        }

        // Label text
        if (!label.empty() && font) {
            sf::Text text;
            text.setFont(*font);
            text.setString(toSfString(label));
            text.setCharacterSize(fontSize);
            text.setFillColor(labelColor);

            sf::FloatRect tb = text.getLocalBounds();
            float tx = position.x + boxSize + 8.f;
            float ty = position.y + (boxSize - tb.height) / 2.0f - tb.top;
            text.setPosition(tx, ty);
            target.draw(text);
        }
    }
};

} // namespace UI

#endif // UI_CHECKBOX_ITEM_H

