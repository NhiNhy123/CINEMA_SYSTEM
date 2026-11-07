#ifndef GLOBAL_SEARCH_BAR_H
#define GLOBAL_SEARCH_BAR_H

#include "UIInputBox.h"

namespace UI {

class GlobalSearchBar : public UIInputBox {
private:
    std::function<void(const std::string&)> onSearchCallback;
    sf::Vector2f clearBtnPos;
    float clearBtnRadius;

public:
    GlobalSearchBar(
        sf::Vector2f pos = sf::Vector2f(0.f, 0.f),
        sf::Vector2f sz = sf::Vector2f(340.f, 40.f),
        const std::string& holder = "Tìm kiếm phim, lịch chiếu, khách hàng..."
    ) : UIInputBox(pos, sz, holder, Theme::SizeBody),
        clearBtnRadius(10.f)
    {
        setCornerRadius(Theme::RadiusPill);
        updateClearPos();
        setOnTextChanged([this](const std::string& q) {
            if (onSearchCallback) onSearchCallback(q);
        });
    }

    void updateClearPos() {
        clearBtnPos = sf::Vector2f(
            position.x + size.x - 26.f,
            position.y + size.y / 2.0f
        );
    }

    void setOnSearch(std::function<void(const std::string&)> cb) {
        onSearchCallback = cb;
    }

    bool handleMouseButtonPressed(sf::Mouse::Button button, sf::Vector2f mousePos) override {
        if (!isVisible || !isEnabled) return false;

        // Check if clear button clicked
        if (button == sf::Mouse::Left && !unicodeText.isEmpty()) {
            float dx = mousePos.x - clearBtnPos.x;
            float dy = mousePos.y - clearBtnPos.y;
            if (dx * dx + dy * dy <= clearBtnRadius * clearBtnRadius * 2.f) {
                unicodeText.clear();
                if (onSearchCallback) onSearchCallback("");
                return true;
            }
        }

        return UIInputBox::handleMouseButtonPressed(button, mousePos);
    }

    void draw(sf::RenderTarget& target) override {
        if (!isVisible) return;
        updateClearPos();

        // Base Input
        UIInputBox::draw(target);

        // Draw Clear button (X) when text is not empty
        if (!unicodeText.isEmpty() && font) {
            sf::CircleShape clearBg(clearBtnRadius);
            clearBg.setOrigin(clearBtnRadius, clearBtnRadius);
            clearBg.setPosition(clearBtnPos);
            clearBg.setFillColor(Theme::CardBorder);
            target.draw(clearBg);

            sf::Text xText;
            xText.setFont(*font);
            xText.setCharacterSize(12);
            xText.setFillColor(Theme::TextSecondary);
            xText.setString("X");
            sf::FloatRect xb = xText.getLocalBounds();
            xText.setPosition(clearBtnPos.x - xb.width / 2.f - xb.left, clearBtnPos.y - xb.height / 2.f - xb.top);
            target.draw(xText);
        }
    }
};

} // namespace UI

#endif // GLOBAL_SEARCH_BAR_H

