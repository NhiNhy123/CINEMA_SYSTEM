#ifndef PASSWORD_FIELD_H
#define PASSWORD_FIELD_H

#include "UIInputBox.h"
#include "TextButton.h"

namespace UI {

class PasswordField : public UIInputBox {
private:
    bool isPasswordHidden;
    sf::Vector2f toggleBtnPos;
    sf::Vector2f toggleBtnSize;

public:
    PasswordField(
        sf::Vector2f pos = sf::Vector2f(0.f, 0.f),
        sf::Vector2f sz = sf::Vector2f(280.f, 42.f),
        const std::string& holder = "Mật khẩu...",
        unsigned int fSize = Theme::SizeBody
    ) : UIInputBox(pos, sz, holder, fSize),
        isPasswordHidden(true),
        toggleBtnSize(sf::Vector2f(50.f, 30.f))
    {
        updateTogglePos();
    }

    void updateTogglePos() {
        toggleBtnPos = sf::Vector2f(
            position.x + size.x - toggleBtnSize.x - 8.f,
            position.y + (size.y - toggleBtnSize.y) / 2.0f
        );
    }

    bool isHidden() const { return isPasswordHidden; }
    void setHidden(bool hidden) { isPasswordHidden = hidden; }

    bool handleMouseButtonPressed(sf::Mouse::Button button, sf::Vector2f mousePos) override {
        if (!isVisible || !isEnabled) return false;

        // Check if toggle button clicked
        if (button == sf::Mouse::Left) {
            if (mousePos.x >= toggleBtnPos.x && mousePos.x <= toggleBtnPos.x + toggleBtnSize.x &&
                mousePos.y >= toggleBtnPos.y && mousePos.y <= toggleBtnPos.y + toggleBtnSize.y) {
                isPasswordHidden = !isPasswordHidden;
                return true;
            }
        }

        return UIInputBox::handleMouseButtonPressed(button, mousePos);
    }

    void draw(sf::RenderTarget& target) override {
        if (!isVisible) return;
        updateTogglePos();

        // Render base input box
        UIInputBox::draw(target);

        sf::Color iconColor = isPasswordHidden ? Theme::TextSecondary : Theme::Cyan;
        sf::Vector2f center(
            toggleBtnPos.x + toggleBtnSize.x / 2.0f,
            toggleBtnPos.y + toggleBtnSize.y / 2.0f
        );

        sf::CircleShape eye(8.f);
        eye.setScale(1.25f, 0.75f);
        eye.setFillColor(sf::Color::Transparent);
        eye.setOutlineThickness(1.5f);
        eye.setOutlineColor(iconColor);
        eye.setPosition(center.x - 10.f, center.y - 6.f);
        target.draw(eye);

        sf::CircleShape pupil(3.f);
        pupil.setFillColor(iconColor);
        pupil.setPosition(center.x - 3.f, center.y - 3.f);
        target.draw(pupil);
    }

protected:
    sf::String getDisplayText() const override {
        if (!isPasswordHidden) {
            return unicodeText;
        }
        sf::String masked;
        for (std::size_t i = 0; i < unicodeText.getSize(); ++i) {
            masked += '*';
        }
        return masked;
    }
};

} // namespace UI

#endif // PASSWORD_FIELD_H
